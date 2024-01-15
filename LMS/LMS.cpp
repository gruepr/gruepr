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
    auto *replyHandler = new grueprOAuthHttpServerReplyHandler(port, this);
    OAuthFlow->setReplyHandler(replyHandler);
    OAuthFlow->setScope(getScopes());
    OAuthFlow->setClientIdentifier(getClientID());
    OAuthFlow->setClientIdentifierSharedKey(getClientSecret());
    OAuthFlow->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            // The only way to get refresh_token from Google Cloud
            parameters->insert("access_type", "offline");
            parameters->insert("prompt", "consent");
        }
        else if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            // Percent-decode the "code" parameter so Google can match it
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->replace("code", QUrl::fromPercentEncoding(code));
        }
    });
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::granted, this, [this](){
        authenticated = true;
        emit granted();
    });
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, this, [this](/*const QString &error*/){
        authenticated = false;
        OAuthFlow->setRefreshToken("");
        refreshTokenExists = false;
        emit denied();
    });
}

bool LMS::authenticate() {
    if(refreshTokenExists) {
        OAuthFlow->refreshAccessToken();
    }
    else {
        OAuthFlow->grant();
    }
    return true;
}


QDialog* LMS::actionDialog(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *busyDialog = new QDialog(parent);
    busyDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
    actionDialogIcon = new QLabel(busyDialog);
    actionDialogIcon->setPixmap(QPixmap(getActionDialogIcon()).scaled(ICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionDialogLabel = new QLabel(busyDialog);
    actionDialogLabel->setStyleSheet(LABEL10PTSTYLE);
    actionDialogLabel->setText(getActionDialogLabel());
    actionDialogButtons = new QDialogButtonBox(busyDialog);
    actionDialogButtons->setStyleSheet(SMALLBUTTONSTYLE);
    actionDialogButtons->setStandardButtons(QDialogButtonBox::NoButton);
    connect(actionDialogButtons, &QDialogButtonBox::accepted, busyDialog, &QDialog::accept);
    auto *theGrid = new QGridLayout;
    busyDialog->setLayout(theGrid);
    theGrid->addWidget(actionDialogIcon, 0, 0);
    theGrid->addWidget(actionDialogLabel, 0, 1);
    theGrid->addWidget(actionDialogButtons, 1, 0, 1, -1);
    busyDialog->setModal(true);
    busyDialog->show();
    busyDialog->adjustSize();
    return busyDialog;
}

void LMS::actionComplete(QDialog *busyDialog) {
    QApplication::restoreOverrideCursor();
    busyDialog->accept();
    busyDialog->deleteLater();
}


void grueprOAuthHttpServerReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    }

    QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
};
