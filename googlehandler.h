#ifndef GOOGLEHANDLER_H
#define GOOGLEHANDLER_H

#include "googlesecrets.h"
#include "survey.h"
#include <QMessageBox>
#include <QOAuth2AuthorizationCodeFlow>
#include <QtNetwork>


struct GoogleForm
{
    QString name;
    QString ID;
    QString createdTime;
    QString downloadURL;
};

class GoogleHandler : public QObject
{
    Q_OBJECT

public:
    GoogleHandler();
    ~GoogleHandler();

    enum class Permissions{write, readonly};
    void authenticate(Permissions permissions = Permissions::write);
    bool authenticated = false;

    QMessageBox* busy(QWidget *parent = nullptr);
    void notBusy(QMessageBox *busyDialog);

    GoogleForm createSurvey(const Survey *const survey);
    QStringList sendSurveyToFinalizeScript(const GoogleForm &form); // sends back the: (0) editURL, (1) responseURL, (2) csvURL
    QStringList getSurveyList();
    QString downloadSurveyResult(const QString &surveyName);

signals:
    void granted();

private:
    void postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                     const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);

    QOAuth2AuthorizationCodeFlow *google = nullptr;
    QNetworkAccessManager *manager = nullptr;
    QVector<GoogleForm> formsList;

    inline static const QSize GOOGLEICONSIZE{MSGBOX_ICON_SIZE,MSGBOX_ICON_SIZE};
    inline static const int RELOAD_DELAY_TIME = 4000;   //msec
    inline static const int TIMEOUT_TIME = 20000;   //msec
    inline static const char AUTHENTICATEURL[]{"https://accounts.google.com/o/oauth2/auth"};
    inline static const char ACCESSTOKENURL[]{"https://oauth2.googleapis.com/token"};
    //inline static const char FINALIZESCRIPTURL[]{** Hidden in googlesecrets.h **};
    //inline static const char CLIENT_ID[]{** Hidden in googlesecrets.h **};
    //inline static const char CLIENT_SECRET[]{** Hidden in googlesecrets.h **};
    inline static const int REDIRECT_URI_PORT = 1234;
    inline static const QString REDIRECT_URI{"https://127.0.0.1:" + QString::number(REDIRECT_URI_PORT)};
};

#endif // GOOGLEHANDLER_H
