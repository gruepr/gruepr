#include "LMS.h"
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
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

QByteArray LMS::httpRequest(const Method method, const QUrl &url, const QByteArray &data, const QByteArray &contentType)
{
    lastErrorMessage.clear();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + OAuthFlow->token().toUtf8());
    if(!contentType.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }
    if(method == Method::post) {
        request.setTransferTimeout(POST_TIMEOUT_TIME);
    }

    // a failure is retryable only if it's transient: HTTP 403/408/429/5xx, or a network-level failure with no HTTP status at all;
    // (403 is included because Google/Canvas both also use it for transient rate-limiting, not only permanent permission errors)
    // but a POST that timed out is NOT retried, since the server may have already committed a non-idempotent request
    auto isRetryable = [method](QNetworkReply *reply) {
        if(method == Method::del) {
            return false;   // best-effort cleanup only (currently just deleteOrphanedForm); no user-visible
                             // benefit to retrying, since a failed cleanup never changes what's shown to the user
        }
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if((httpStatus == 403) || (httpStatus == 408) || (httpStatus == 429) || ((httpStatus >= 500) && (httpStatus < 600))) {
            return true;
        }
        if(httpStatus != 0) {
            return false;
        }
        const bool isTimeout = (reply->error() == QNetworkReply::TimeoutError) || (reply->error() == QNetworkReply::OperationCanceledError);
        if((method == Method::post) && isTimeout) {
            return false;
        }
        return true;
    };

    QElapsedTimer deadline;
    deadline.start();

    auto *reply = (method == Method::get) ? manager->get(request) :
                  (method == Method::post) ? manager->post(request, data) :
                                              manager->deleteResource(request);
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

        reply = (method == Method::get) ? manager->get(request) :
                (method == Method::post) ? manager->post(request, data) :
                                            manager->deleteResource(request);
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

    if(deadline.elapsed() >= OVERALL_TIMEOUT) {
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
        const QString jsonErrorMessage = QJsonDocument::fromJson(errorBody).object()["error"].toObject()["message"].toString();
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
    actionDialogButtons = new QDialogButtonBox(dialog);
    actionDialogButtons->setStyleSheet(SMALLBUTTONSTYLE);
    actionDialogButtons->setStandardButtons(QDialogButtonBox::NoButton);
    connect(actionDialogButtons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);

    auto *theGrid = new QGridLayout;
    dialog->setLayout(theGrid);
    theGrid->addWidget(actionDialogIcon, 0, 0, Qt::AlignLeft | Qt::AlignTop);
    theGrid->addWidget(actionDialogLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    theGrid->addWidget(actionDialogButtons, 1, 0, 1, -1);
    theGrid->setColumnStretch(1, 1);
    dialog->setModal(true);
    dialog->show();
    dialog->setMinimumSize(XS_DLG_SIZE, ICONSIZE.height());
    dialog->adjustSize();

    startBusyAnimation();

    return dialog;
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
