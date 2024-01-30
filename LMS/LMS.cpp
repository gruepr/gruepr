#include "LMS.h"
#include <QDesktopServices>
#include <QGridLayout>

LMS::LMS(QObject *parent) :
    QObject(parent),
    redirectUri(QString(REDIRECT_URI)),
    port(static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT))) {
}

void LMS::initOAuth2() {
    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setTransferTimeout(TIMEOUT_TIME);

    OAuthFlow = new QOAuth2AuthorizationCodeFlow(manager, this);
    OAuthFlow->setScope(getScopes());
    OAuthFlow->setAuthorizationUrl(getClientAuthorizationUrl());
    OAuthFlow->setAccessTokenUrl(getClientAccessTokenUrl());
    OAuthFlow->setClientIdentifier(getClientID());
    OAuthFlow->setClientIdentifierSharedKey(getClientSecret());
    OAuthFlow->setModifyParametersFunction(getModifyParametersFunction());
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    replyHandler = new grueprOAuthHttpServerReplyHandler(port, this);
    OAuthFlow->setReplyHandler(replyHandler);
    replyHandler->setCallbackText("<font size=\"+2\" face=\"arial\" color=\"" DEEPWATERHEX "\">" +
                                  tr("Authorization complete. You may close this page and return to gruepr.") + "</font>");
    // connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, this, [](const QString &error) {
    //     qDebug() << "replyHandler error: ";
    //     qDebug() << error;
    //     qDebug() << "************";
    // });
}

bool LMS::authenticate() {
    if(!OAuthFlow->refreshToken().isEmpty()) {
        OAuthFlow->refreshAccessToken();
    }
    else {
        OAuthFlow->grant();
    }
    return true;
}

bool LMS::authenticated() {
    return OAuthFlow->status() == QAbstractOAuth::Status::Granted;
}

QByteArray LMS::httpRequest(const Method method, const QUrl &url, const QByteArray &data) {
    switch(method) {
    case Method::get: {
        QEventLoop loop;
        auto *reply = OAuthFlow->get(url);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        int attempt = 1;
        while((reply->error() != QNetworkReply::NoError) && (attempt < 5)) {
            //qDebug() << reply->errorString();
            //qDebug() << "attempt " << attempt << " of " << url << " failed. Retrying.";
            delete reply;
            emit retrying(++attempt);
            reply = OAuthFlow->get(url);
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
        }

        if((reply->error() != QNetworkReply::NoError) || (reply->bytesAvailable() == 0)) {
            //qDebug() << "**** failed or empty reply";
            reply->deleteLater();
            return {};
        }

        auto replyBody = reply->readAll();
        reply->deleteLater();
        return replyBody;
        break;
    }
    case Method::post: {
        QEventLoop loop;
        auto *reply = OAuthFlow->post(url, data);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        int attempt = 1;
        while((reply->error() != QNetworkReply::NoError) && (attempt < 5)) {
            //qDebug() << reply->errorString();
            //qDebug() << "attempt " << attempt << " of " << url << " failed. Retrying.";
            delete reply;
            emit retrying(++attempt);
            reply = OAuthFlow->post(url, data);
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
        }

        if((reply->error() != QNetworkReply::NoError) || (reply->bytesAvailable() == 0)) {
            //qDebug() << "**** failed or empty reply";
            reply->deleteLater();
            return {};
        }

        auto replyBody = reply->readAll();
        reply->deleteLater();
        return replyBody;
        break;
    }
    default: {
        return {};
    }
    }
}

QDialog* LMS::actionDialog(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
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
    return dialog;
}

void LMS::actionComplete(QDialog *busyDialog) {
    QApplication::restoreOverrideCursor();
    if(busyDialog != nullptr) {
        busyDialog->accept();
        disconnect(busyDialog);
        busyDialog->deleteLater();
    }
}


void grueprOAuthHttpServerReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        emit error(QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(reply->error()));
    }
    QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
};
