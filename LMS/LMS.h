#ifndef LMS_H
#define LMS_H

#include "gruepr_globals.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QNetworkReply>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QProgressBar>


class grueprOAuthHttpServerReplyHandler : public QOAuthHttpServerReplyHandler
{
    Q_OBJECT
    friend class LMS;

signals:
    void error(const QString &errorString);

private:
    grueprOAuthHttpServerReplyHandler(quint16 port, QObject *parent = nullptr) : QOAuthHttpServerReplyHandler(port, parent) {}
    void networkReplyFinished(QNetworkReply *reply) override;
};


class LMS : public QObject
{
    Q_OBJECT

public:
    LMS(QObject *parent) :
        QObject(parent), redirectUri(QString(REDIRECT_URI)), port(static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT))) { }

    //"Please wait, still communicating" dialog
    QDialog* actionDialog(QWidget *parent = nullptr);
    QLabel *actionDialogIcon = nullptr;
    QLabel *actionDialogLabel = nullptr;
    QProgressBar *actionDialogProgressBar = nullptr;     // hidden unless showActionProgress() is used
    QDialogButtonBox *actionDialogButtons = nullptr;

    /**
     * @brief showActionProgress Shows how far along the action is in the "Please wait" dialog, as a line of text and a progress bar.
     * The text only ever widens the dialog, so no text is cut off and the dialog doesn't change size with every new text.
     * @param text What is being done.
     * @param done How many steps are done.
     * @param total How many steps there are in all (0 if that isn't known, for a bar that keeps moving to show that something is happening).
     */
    void showActionProgress(const QString &text, const int done, const int total);

    /**
     * @brief hideActionProgress Hides the progress bar, e.g., to show the action's result in the dialog.
     */
    void hideActionProgress();

    void actionComplete(QDialog *busyDialog);
    void startBusyAnimation();
    void stopBusyAnimation();
    QString lastErrorMessage;
    inline static const int NUM_RETRIES_BEFORE_ABORT = 5;   //number of times we will retry a GET or POST before fully giving up

signals:
    void retrying(int attemptNum);
    void requestFailed(QNetworkReply::NetworkError error, const QUrl &url);
    void connectionTimedOut();

protected:
    void initOAuth2();
    virtual bool authenticate();
    bool authenticated();
    enum class Method{get, post, put, del};
    // retryForbidden: whether a 403 reply is retried as if it were temporary rate-limiting (false retries it only if the reply says it was rate-limiting)
    QByteArray httpRequest(const Method method, const QUrl &url, const QByteArray &data = "", const QByteArray &contentType = "",
                           const bool retryForbidden = true);

    QOAuth2AuthorizationCodeFlow *OAuthFlow = nullptr;
    QNetworkAccessManager *manager = nullptr;
    grueprOAuthHttpServerReplyHandler *replyHandler = nullptr;
    QUrl redirectUri;
    quint16 port;
    QTimer *busyAnimationTimer = nullptr;

    virtual QSet<QByteArray> getScopes() const = 0;
    virtual QString getClientID() const = 0;
    virtual QString getClientSecret() const = 0;
    virtual QString getClientAuthorizationUrl() const = 0;
    virtual QString getClientAccessTokenUrl() const = 0;
    virtual QString getActionDialogIcon() const = 0;
    virtual QString getActionDialogLabel() const = 0;
    virtual std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> getModifyParametersFunction() const = 0;

    inline static const QSize ICONSIZE{MSGBOX_ICON_SIZE,MSGBOX_ICON_SIZE};
    inline static const int RELOAD_DELAY_TIME = 2000;   //msec
    inline static const int TIMEOUT_TIME = 5000;   //msec
    inline static const int POST_TIMEOUT_TIME = 15000;   //msec, transfer timeout for POSTs (larger requests, e.g. Google Forms batchUpdate, need more than TIMEOUT_TIME)
    inline static const int RETRY_DELAY_TIME = 500;  //msec, base delay before retrying a GET or POST following an error returned; doubles with each subsequent retry
    inline static const int OVERALL_TIMEOUT = 30000;   //msec, total time for all retries of a single httpRequest before giving up
    inline static const int REDIRECT_URI_PORT = 6174;   //Kaprekar's number
    inline static const QString REDIRECT_URI{"https://127.0.0.1:" + QString::number(REDIRECT_URI_PORT) + "/"};
};

#endif // LMS_H
