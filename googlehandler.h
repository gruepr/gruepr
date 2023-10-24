#ifndef GOOGLEHANDLER_H
#define GOOGLEHANDLER_H

#include "googlesecrets.h"
#include "gruepr_globals.h"
#include "survey.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QtNetwork>


struct GoogleForm
{
    QString name;
    QString ID;
    QString createdTime;
    QUrl responderURL;
};

struct GoogleFormQuestion
{
    QString ID;
    QString text;
    enum class Type{notSchedule, schedule} type = Type::notSchedule;
};

class GoogleHandler : public QObject
{
    Q_OBJECT

public:
    GoogleHandler();
    ~GoogleHandler() override;
    GoogleHandler(const GoogleHandler&) = delete;
    GoogleHandler operator= (const GoogleHandler&) = delete;
    GoogleHandler(GoogleHandler&&) = delete;
    GoogleHandler& operator= (GoogleHandler&&) = delete;

    void authenticate();
    bool authenticated = false;
    bool refreshTokenExists = false;

    QDialog* busy(QWidget *parent = nullptr);
    QLabel* busyBoxIcon = nullptr;
    QLabel* busyBoxLabel = nullptr;
    QDialogButtonBox* busyBoxButtons = nullptr;
    void notBusy(QDialog *busyDialog);

    GoogleForm createSurvey(const Survey *const survey);
    QStringList getSurveyList();
    QString downloadSurveyResult(const QString &surveyName);

signals:
    void granted();
    void denied();

private:
    void postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                     const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);

    QOAuth2AuthorizationCodeFlow *google = nullptr;
    QNetworkAccessManager *manager = nullptr;
    QList<GoogleForm> formsList;

    inline static const QSize GOOGLEICONSIZE{MSGBOX_ICON_SIZE,MSGBOX_ICON_SIZE};
    inline static const int RELOAD_DELAY_TIME = 4000;   //msec
    inline static const int TIMEOUT_TIME = 20000;   //msec
    inline static const char AUTHENTICATEURL[]{"https://accounts.google.com/o/oauth2/auth"};
    inline static const char ACCESSTOKENURL[]{"https://oauth2.googleapis.com/token"};
    //inline static const char CLIENT_ID[]{** Hidden in googlesecrets.h **};
    //inline static const char CLIENT_SECRET[]{** Hidden in googlesecrets.h **};
    inline static const int REDIRECT_URI_PORT = 6174;   //Kaprekar's number
    inline static const QString REDIRECT_URI{"https://127.0.0.1:" + QString::number(REDIRECT_URI_PORT)};
};


// helper class below is used in order to expose & emit the errorString
class grueprOAuthHttpServerReplyHandler : public QOAuthHttpServerReplyHandler
{
    Q_OBJECT

public:
    grueprOAuthHttpServerReplyHandler(quint16 port, QObject *parent = nullptr);

signals:
    void error(const QString &errorString);

private:
    void networkReplyFinished(QNetworkReply *reply) override;
};

#endif // GOOGLEHANDLER_H
