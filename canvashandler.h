#ifndef CANVASHANDLER_H
#define CANVASHANDLER_H

#include "studentRecord.h"
#include "survey.h"
#include <QMessageBox>
#include <QOAuth2AuthorizationCodeFlow>
#include <QtNetwork>


struct CanvasQuiz
{
    QString name;
    int ID = 0;
};

struct CanvasCourse
{
    QString name;
    int ID = 0;
    int numStudents = 0;
};

class CanvasHandler : QObject
{
    Q_OBJECT

public:
    CanvasHandler(const QString &authenticateURL = "", const QString &accessTokenURL = "", const QString &baseAPIURL = "");
    ~CanvasHandler();

    void authenticate();
    QStringList askUserForManualToken(const QString &currentURL = "", const QString &currentToken = "", QWidget *parent = nullptr);
    void setBaseURL(const QString &baseAPIURL);
    void authenticate(const QString &token);
    bool authenticated = false;

    QMessageBox* busy(QWidget *parent = nullptr);
    void notBusy(QMessageBox *busyDialog);

    QStringList getCourses();
    int getStudentCount(const QString &courseName);
    QVector<StudentRecord> getStudentRoster(const QString &courseName);
    bool createTeams(const QString &courseName, const QString &setName, const QStringList &teamNames, const QVector<QVector<StudentRecord>> &teams);
    bool createSurvey(const QString &courseName, const Survey *const survey);
    QStringList getQuizList(const QString &courseName);
    bool downloadQuizResult(const QString &courseName, const QString &quizName);

private:
    int getCourseID(const QString &courseName);
    int getQuizID(const QString &quizName);
    QUrl getQuizResultsURL(const int courseID, const int quizID);

    void getPaginatedCanvasResults(const QString &initialURL, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                              const QStringList &intParams, QList<QList<int>*> &intVals,
                                                              const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);
    void postToCanvasGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                     const QStringList &intParams, QList<QList<int>*> &intVals,
                                                                                     const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);
    bool downloadFile(const QUrl &URL, const QString &filepath);

    QOAuth2AuthorizationCodeFlow *canvas = nullptr;
    QString baseURL;

    QNetworkAccessManager *manager = nullptr;
    QVector<CanvasCourse> canvasCourses;
    QVector<StudentRecord> roster;
    QVector<CanvasQuiz> quizList;

    inline static const QSize CANVASICONSIZE{40,40};
    inline static const int RELOAD_DELAY_TIME = 2000;   //msec
    inline static const int TIMEOUT_TIME = 20000;   //msec
    inline static const QString SCHEDULEQUESTIONINTRO1{QObject::tr("The following ")};
    inline static const QString SCHEDULEQUESTIONINTRO2{QObject::tr(" questions ask about your schedule on ")};
    inline static const int NUM_PAGES_TO_LOAD = 20;
    inline static const QString CLIENT_ID{""};
    inline static const QString CLIENT_SECRET{""};
    inline static const int REDIRECT_URI_PORT = 1234;
    inline static const QString REDIRECT_URI{"https://127.0.0.1:" + QString::number(REDIRECT_URI_PORT) + "/"};
};

#endif // CANVASHANDLER_H
