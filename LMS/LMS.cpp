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
    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::error, this, [](const QString &error, const QString &errorDescription, const QUrl &uri) {
        qDebug() << "OAuthFlow error: ";
        qDebug() << error;
        qDebug() << errorDescription;
        qDebug() << uri;
        qDebug() << "************";
    });

    auto *replyHandler = new grueprOAuthHttpServerReplyHandler(port, this);
    OAuthFlow->setReplyHandler(replyHandler);
    replyHandler->setCallbackText(tr("Authorization complete. You may close this page and return to gruepr."));
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::replyDataReceived, this, &LMS::serverReplyReceived);
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, this, [](QString &error, const QString &errorDescription, const QUrl &uri) {
        qDebug() << "replyHandler error: ";
        qDebug() << error;
        qDebug() << errorDescription;
        qDebug() << uri;
        qDebug() << "************";
    });
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
    busyDialog->accept();
    disconnect(busyDialog);
    busyDialog->deleteLater();
}


void grueprOAuthHttpServerReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    }

    QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
};
