#include "LMS.h"
#include <algorithm>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QPropertyAnimation>
#include <QTimer>

void LMS::initOAuth2()
{
    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setTransferTimeout(TIMEOUT_TIME);

    OAuthFlow = new QOAuth2AuthorizationCodeFlow(manager, this);
    OAuthFlow->setRequestedScopeTokens(getScopes());
    OAuthFlow->setAuthorizationUrl(getClientAuthorizationUrl());
    OAuthFlow->setTokenUrl(getClientAccessTokenUrl());
    OAuthFlow->setClientIdentifier(getClientID());
    OAuthFlow->setClientIdentifierSharedKey(getClientSecret());
    OAuthFlow->setModifyParametersFunction(getModifyParametersFunction());
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    replyHandler = new grueprOAuthHttpServerReplyHandler(port, this);
    OAuthFlow->setReplyHandler(replyHandler);
    replyHandler->setCallbackText("<font size=\"+2\" face=\"arial\" color=\"" DEEPWATERHEX "\">" +
                                  tr("Authorization complete! You may close this page and return to gruepr.") + "</font>");
    //connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, this, [](const QString &error) {
        //qDebug() << "replyHandler error: ";
        //qDebug() << error;
        //qDebug() << "************";
    //});
}

bool LMS::authenticate()
{
    if(!OAuthFlow->refreshToken().isEmpty()) {
        OAuthFlow->refreshTokens();
    }
    else {
        OAuthFlow->grant();
    }
    return true;
}

bool LMS::authenticated()
{
    return OAuthFlow->status() == QAbstractOAuth::Status::Granted;
}

QByteArray LMS::httpRequest(const Method method, const QUrl &url, const QByteArray &data, const QByteArray &contentType, const bool retryForbidden)
{
    lastErrorMessage.clear();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + OAuthFlow->token().toUtf8());
    if(!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }
    if((method == Method::post) || (method == Method::put)) {
        request.setTransferTimeout(POST_TIMEOUT_TIME);
    }
    auto sendRequest = [this, method, &request, &data]() {
        switch(method) {
        case Method::get:
            return manager->get(request);
        case Method::post:
            return manager->post(request, data);
        case Method::put:
            return manager->put(request, data);
        case Method::del:
            break;
        }
        return manager->deleteResource(request);
    };

    // a failure is retryable only if it's transient: HTTP 403/408/429/5xx, or a network-level failure with no HTTP status at all;
    // (403 is included because Google/Canvas both also use it for transient rate-limiting, not only permanent permission errors)
    // but a POST or PUT that timed out is NOT retried, since the server may have already committed a non-idempotent request
    // (e.g., a PUT that adds a comment to a submission adds another comment each time it's sent)
    auto isRetryable = [method, retryForbidden](QNetworkReply *reply) {
        if(method == Method::del) {
            return false;   // best-effort cleanup only (currently just deleteOrphanedForm); no user-visible
                             // benefit to retrying, since a failed cleanup never changes what's shown to the user
        }
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if((httpStatus == 403) && !retryForbidden) {
            // a refusal (e.g., sharing a file outside the school's domain) won't change on retrying, but rate-limiting will
            // (Google's reply says "rateLimitExceeded" or "userRateLimitExceeded"; Canvas's says "Rate Limit Exceeded")
            const QByteArray replyText = reply->peek(reply->bytesAvailable()).toLower();
            return replyText.contains("ratelimit") || replyText.contains("rate limit");
        }
        if((httpStatus == 403) || (httpStatus == 408) || (httpStatus == 429) || ((httpStatus >= 500) && (httpStatus < 600))) {
            return true;
        }
        if(httpStatus != 0) {
            return false;
        }
        const bool isTimeout = (reply->error() == QNetworkReply::TimeoutError) || (reply->error() == QNetworkReply::OperationCanceledError);
        if(((method == Method::post) || (method == Method::put)) && isTimeout) {
            return false;
        }
        return true;
    };

    QElapsedTimer deadline;
    deadline.start();

    auto *reply = sendRequest();
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    //connect(reply, &QNetworkReply::requestSent, this, [](){qDebug() << "requestSent";});
    //connect(reply, &QNetworkReply::uploadProgress, this, [](qint64 bytesSent, qint64 bytesTotal){qDebug() << "upload " << bytesSent <<" / " << bytesTotal;});
    //connect(reply, &QNetworkReply::downloadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal){qDebug() << "download " << bytesReceived <<" / " << bytesTotal;});
    loop.exec();
    /*qDebug() << "url: " << url
             << "\nStatus code " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
             << ", " << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray()
             << "\nFrom cache? " << reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool()
             << "\nhttp2 used? " << reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool();*/

    int attempt = 1;
    while((reply->error() != QNetworkReply::NoError) && (attempt < NUM_RETRIES_BEFORE_ABORT) && isRetryable(reply)) {
        //qDebug() << reply->errorString();
        //qDebug() << "attempt " << attempt << " of " << url << " failed. Retrying.";
        if(deadline.elapsed() >= OVERALL_TIMEOUT) {
            lastErrorMessage = tr("The connection has timed out.");
            reply->abort();
            reply->deleteLater();
            emit connectionTimedOut();
            return {};
        }

        reply->deleteLater();
        emit retrying(++attempt);

        const int retryDelay = RETRY_DELAY_TIME << (attempt - 2);   // 500ms, 1s, 2s, 4s, ...
        QEventLoop delayLoop;
        QTimer::singleShot(retryDelay, &delayLoop, &QEventLoop::quit);
        delayLoop.exec();

        reply = sendRequest();
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        //connect(reply, &QNetworkReply::requestSent, this, [](){qDebug() << "requestSent";});
        //connect(reply, &QNetworkReply::uploadProgress, this, [](qint64 bytesSent, qint64 bytesTotal){qDebug() << "upload " << bytesSent <<" / " << bytesTotal;});
        //connect(reply, &QNetworkReply::downloadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal){qDebug() << "download " << bytesReceived <<" / " << bytesTotal;});
        loop.exec();
        /*qDebug() << "*** Attempt " << attempt
                 << "\nStatus code " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << ", " << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray()
                 << "\nFrom cache? " << reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool()
                 << "\nhttp2 used? " << reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool();*/
    }

    // only a request that ultimately failed counts as timed out; a successful reply is kept no matter how long the retries took
    if((reply->error() != QNetworkReply::NoError) && (deadline.elapsed() >= OVERALL_TIMEOUT)) {
        lastErrorMessage = tr("The connection has timed out.");
        reply->abort();
        reply->deleteLater();
        emit connectionTimedOut();
        return {};
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool isSuccessStatus = (httpStatus >= 200) && (httpStatus < 300);
    if((reply->error() != QNetworkReply::NoError) || ((reply->bytesAvailable() == 0) && !isSuccessStatus)) {
        lastErrorMessage = reply->errorString();
        lastErrorMessage.replace(" - ", "\n");
        if(httpStatus != 0) {
            const QString httpReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            lastErrorMessage += " (" + tr("HTTP status") + " " + QString::number(httpStatus) + (httpReason.isEmpty() ? "" : (" " + httpReason)) + ")";
        }
        const QByteArray errorBody = reply->readAll();
        // best-effort extraction of a Google-style {"error":{"message":...}} body; harmless no-op for other
        // LMS subclasses (e.g. Canvas) whose error JSON isn't shaped this way, since this class is their shared base
        const QJsonObject errorObject = QJsonDocument::fromJson(errorBody).object();
        QString jsonErrorMessage = errorObject["error"].toObject()["message"].toString();
        if(jsonErrorMessage.isEmpty() && errorObject.contains("errors")) {
            // Canvas-style {"errors": [{"message": ...}]} or {"errors": {"field": [{"message": ...}]}}
            const QJsonValue errors = errorObject["errors"];
            jsonErrorMessage = errors.isArray()? errors.toArray().at(0).toObject()["message"].toString() :
                                                 QString::fromUtf8(QJsonDocument(errors.toObject()).toJson(QJsonDocument::Compact));
        }
        if(!jsonErrorMessage.isEmpty()) {
            lastErrorMessage += "\n" + jsonErrorMessage;
        }
        //qDebug() << "**** failed or empty reply";
        emit requestFailed(reply->error(), url);
        reply->deleteLater();
        return {};
    }

    auto replyBody = reply->readAll();
    reply->deleteLater();
    return replyBody;
}

QDialog* LMS::actionDialog(QWidget *parent)
{
    auto *dialog = new QDialog(parent);
    dialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
    actionDialogIcon = new QLabel(dialog);
    actionDialogIcon->setPixmap(QPixmap(getActionDialogIcon()).scaled(ICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionDialogLabel = new QLabel(dialog);
    actionDialogLabel->setStyleSheet(LABEL12PTSTYLE);
    actionDialogLabel->setTextFormat(Qt::RichText);
    actionDialogLabel->setWordWrap(false);
    actionDialogLabel->setText(getActionDialogLabel());
    actionDialogProgressBar = new QProgressBar(dialog);
    actionDialogProgressBar->setStyleSheet(PROGRESSBARSTYLE);
    actionDialogProgressBar->setTextVisible(false);
    actionDialogProgressBar->hide();
    actionDialogButtons = new QDialogButtonBox(dialog);
    actionDialogButtons->setStyleSheet(SMALLBUTTONSTYLE);
    actionDialogButtons->setStandardButtons(QDialogButtonBox::NoButton);
    connect(actionDialogButtons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);

    auto *theGrid = new QGridLayout;
    dialog->setLayout(theGrid);
    theGrid->addWidget(actionDialogIcon, 0, 0, Qt::AlignLeft | Qt::AlignTop);
    theGrid->addWidget(actionDialogLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    theGrid->addWidget(actionDialogProgressBar, 1, 0, 1, -1);
    theGrid->addWidget(actionDialogButtons, 2, 0, 1, -1);
    theGrid->setColumnStretch(1, 1);
    dialog->setModal(true);
    dialog->show();
    dialog->setMinimumSize(XS_DLG_SIZE, ICONSIZE.height());
    dialog->adjustSize();

    startBusyAnimation();

    return dialog;
}

void LMS::showActionProgress(const QString &text, const int done, const int total)
{
    if((actionDialogLabel == nullptr) || (actionDialogProgressBar == nullptr)) {
        return;
    }
    actionDialogLabel->setText(text);
    // the label only ever widens, so no text is cut off and the dialog doesn't change size with every new text
    actionDialogLabel->setMinimumWidth(std::max(actionDialogLabel->minimumWidth(), actionDialogLabel->sizeHint().width()));
    actionDialogProgressBar->setRange(0, std::max(0, total));      // (a range of 0 to 0 makes a bar that keeps moving)
    actionDialogProgressBar->setValue(std::clamp(done, 0, std::max(0, total)));
    actionDialogProgressBar->show();
    actionDialogLabel->window()->adjustSize();
    QCoreApplication::processEvents();
}

void LMS::hideActionProgress()
{
    if((actionDialogLabel == nullptr) || (actionDialogProgressBar == nullptr)) {
        return;
    }
    actionDialogProgressBar->hide();
    actionDialogLabel->setMinimumWidth(0);
    actionDialogLabel->window()->adjustSize();
}

void LMS::actionComplete(QDialog *busyDialog)
{
    stopBusyAnimation();
    if(busyDialog != nullptr) {
        busyDialog->accept();
        disconnect(busyDialog);
        busyDialog->deleteLater();
    }
}

void LMS::startBusyAnimation()
{
    stopBusyAnimation();
    auto *effect = new QGraphicsOpacityEffect(actionDialogIcon);
    actionDialogIcon->setGraphicsEffect(effect);
    busyAnimationTimer = new QTimer(this);
    busyAnimationTimer->setInterval(50);
    auto opacity = 1.0f;
    bool dimming = true;
    connect(busyAnimationTimer, &QTimer::timeout, this, [effect, opacity, dimming]() mutable {
        opacity += (dimming ? -0.03f : 0.03f);
        if(opacity <= 0.3f) {
            dimming = false;
        }
        else if(opacity >= 1.0f) {
            dimming = true;
        }
        effect->setOpacity(opacity);
    });
    busyAnimationTimer->start();
}

void LMS::stopBusyAnimation()
{
    if(busyAnimationTimer != nullptr) {
        busyAnimationTimer->stop();
        busyAnimationTimer->deleteLater();
        busyAnimationTimer = nullptr;
    }
    if(actionDialogIcon != nullptr) {
        actionDialogIcon->setGraphicsEffect(nullptr);
    }
}


void grueprOAuthHttpServerReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        emit error(QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error()));
    }
    QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
};
