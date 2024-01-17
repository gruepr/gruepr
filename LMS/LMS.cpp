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
    OAuthFlow->setClientIdentifier(getClientID());
    OAuthFlow->setClientIdentifierSharedKey(getClientSecret());
    OAuthFlow->setModifyParametersFunction(getModifyParametersFunction());

    auto *replyHandler = new grueprOAuthHttpServerReplyHandler(port, OAuthFlow);
    OAuthFlow->setReplyHandler(replyHandler);

    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::replyDataReceived, this, [this](const QString &reply){
        emit replyReceived(reply);
    });
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::granted, this, [this](){
        authenticated = true;
        if(!OAuthFlow->refreshToken().isEmpty()) {
            refreshTokenExists = true;
        }
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
    auto *dialog = new QDialog(parent);
    dialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
    actionDialogIcon = new QLabel(dialog);
    actionDialogIcon->setPixmap(QPixmap(getActionDialogIcon()).scaled(ICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionDialogLabel = new QLabel(dialog);
    actionDialogLabel->setStyleSheet(LABEL12PTSTYLE);
    actionDialogLabel->setText(getActionDialogLabel());
    actionDialogButtons = new QDialogButtonBox(dialog);
    actionDialogButtons->setStyleSheet(SMALLBUTTONSTYLE);
    actionDialogButtons->setStandardButtons(QDialogButtonBox::NoButton);
    connect(actionDialogButtons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    auto *theGrid = new QGridLayout;
    dialog->setLayout(theGrid);
    theGrid->addWidget(actionDialogIcon, 0, 0);
    theGrid->addWidget(actionDialogLabel, 0, 1);
    theGrid->addWidget(actionDialogButtons, 1, 0, 1, -1);
    dialog->setModal(true);
    dialog->show();
    dialog->adjustSize();
    return dialog;
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
