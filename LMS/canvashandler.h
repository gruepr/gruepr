#ifndef CANVASHANDLER_H
#define CANVASHANDLER_H

#include "LMS.h"
#include "studentRecord.h"
#include "survey.h"

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

class CanvasHandler : public LMS
{
    Q_OBJECT

public:
    CanvasHandler(QObject *parent = nullptr);
    ~CanvasHandler() override;
    CanvasHandler(const CanvasHandler&) = delete;
    CanvasHandler operator= (const CanvasHandler&) = delete;
    CanvasHandler(CanvasHandler&&) = delete;
    CanvasHandler& operator= (CanvasHandler&&) = delete;

    bool authenticate() override;
    void setBaseURL(const QString &baseAPIURL);

    QStringList getCourses();
    int getStudentCount(const QString &courseName);
    QList<StudentRecord> getStudentRoster(const QString &courseName);
    bool createSurvey(const QString &courseName, const Survey *const survey);
    QStringList getQuizList(const QString &courseName);
    QString downloadQuizResult(const QString &courseName, const QString &quizName); //returns the filepath of the downloaded file (empty string if error)
    bool createTeams(const QString &courseName, const QString &setName, const QStringList &teamNames, const QList<QList<StudentRecord>> &teams);

    static QPixmap icon();

    inline static const QString SCHEDULEQUESTIONINTRO1{QObject::tr("The following ")};
    inline static const QString SCHEDULEQUESTIONINTRO2{QObject::tr(" questions ask about your schedule on ")};

private:
    void authenticateWithManualToken(const QString &token);
    QStringList askUserForManualURLandToken(const QString &currentURL = "", const QString &currentToken = "");

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

    QString baseURL;
    QList<CanvasCourse> canvasCourses;
    QList<StudentRecord> roster;
    QList<CanvasQuiz> quizList;

    QString getScopes() const override;
    QString getClientID() const override;
    QString getClientSecret() const override;
    QString getActionDialogIcon() const override;
    QString getActionDialogLabel() const override;

    inline static const int NUM_PAGES_TO_LOAD = 20;
    inline static const QString CLIENT_ID{""};
    inline static const QString CLIENT_SECRET{""};
};

#endif // CANVASHANDLER_H
