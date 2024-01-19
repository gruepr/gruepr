#ifndef LMS_H
#define LMS_H

#include "gruepr_globals.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QtNetwork>


class LMS : public QObject
{
    Q_OBJECT

public:
    LMS(QObject *parent = nullptr);
    ~LMS() override = default;

    //"Please wait, still communicating" dialog
    QDialog* actionDialog(QWidget *parent = nullptr);
    QLabel *actionDialogIcon = nullptr;
    QLabel *actionDialogLabel = nullptr;
    QDialogButtonBox *actionDialogButtons = nullptr;
    void actionComplete(QDialog *busyDialog);

signals:
    void replyReceived(const QString &reply);
    void granted();
    void denied();

protected:
    void initOAuth2();
    virtual bool authenticate();
    bool authenticated = false;
    bool refreshTokenExists = false;

    QOAuth2AuthorizationCodeFlow *OAuthFlow = nullptr;
    QNetworkAccessManager *manager = nullptr;
    QUrl redirectUri;
    quint16 port;

    virtual QString getScopes() const = 0;
    virtual QString getClientID() const = 0;
    virtual QString getClientSecret() const = 0;
    virtual QString getClientAuthorizationUrl() const = 0;
    virtual QString getClientAccessTokenUrl() const = 0;
    virtual QString getActionDialogIcon() const = 0;
    virtual QString getActionDialogLabel() const = 0;
    virtual std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> getModifyParametersFunction() const = 0;

    inline static const QSize ICONSIZE{MSGBOX_ICON_SIZE,MSGBOX_ICON_SIZE};
    inline static const int RELOAD_DELAY_TIME = 2000;   //msec
    inline static const int TIMEOUT_TIME = 20000;   //msec
    inline static const int REDIRECT_URI_PORT = 6174;   //Kaprekar's number
    inline static const QString REDIRECT_URI{"https://127.0.0.1:" + QString::number(REDIRECT_URI_PORT) + "/"};
};


// helper class below is used in order to expose & emit the errorString
class grueprOAuthHttpServerReplyHandler : public QOAuthHttpServerReplyHandler
{
    Q_OBJECT

public:
    grueprOAuthHttpServerReplyHandler(quint16 port, QObject *parent = nullptr) : QOAuthHttpServerReplyHandler(port, parent) {}

signals:
    void error(const QString &errorString);

private:
    void networkReplyFinished(QNetworkReply *reply) override;
};

#endif // LMS_H
