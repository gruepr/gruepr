#ifndef CANVASHANDLER_H
#define CANVASHANDLER_H

#include "LMS.h"
#include "peerReview.h"
#include "studentRecord.h"
#include "survey.h"
#include <functional>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>


class CanvasHandler : public LMS
{
    Q_OBJECT

public:

    struct CanvasQuiz {QString name; long long ID = 0;};
    struct CanvasCourse {QString name; long long ID = 0; int numStudents = 0; QDateTime creationDate = QDateTime::currentDateTime();};
    struct CanvasGroupSet {QString name; long long ID = 0;};
    struct CanvasGroup {QString name; long long ID = 0; QList<long long> memberIDs;};

    CanvasHandler(QWidget *parent = nullptr);
    ~CanvasHandler() override;
    CanvasHandler(const CanvasHandler&) = delete;
    CanvasHandler operator= (const CanvasHandler&) = delete;
    CanvasHandler(CanvasHandler&&) = delete;
    CanvasHandler& operator= (CanvasHandler&&) = delete;

    bool authenticate() override;
    void setBaseURL(const QString &baseAPIURL);

    enum class ReportStage {preparing, downloading};        // the stages of downloading a survey's results

    QList<CanvasCourse> getCourses();
    int getStudentCount(long long courseID);
    // progress, if given, is called after each request to Canvas, with the number done and the number to do (0 until that's known)
    QList<StudentRecord> getStudentRoster(long long courseID, const std::function<void(int stepsDone, int numSteps)> &progress = {});
    // progress, if given, is called as each question is started (questionNum is -1 for the survey itself) and after each request to Canvas
    bool createSurvey(long long courseID, const Survey *const survey,
                      const std::function<void(int questionNum, int stepsDone, int numSteps)> &progress = {});
    QList<CanvasQuiz> getQuizList(long long courseID);
    // returns the filepath of the downloaded file (empty string if error); progress, if given, is called every few seconds while waiting,
    // with how much of the results Canvas says it has prepared (-1 if it doesn't say) and how long it's been
    QString downloadQuizResult(long long courseID, long long quizID, const QString &quizName,
                               const std::function<void(ReportStage stage, int percentDone, int secondsWaited)> &progress = {});
    // progress, if given, is called as each team is started and after each request to Canvas
    bool createTeams(long long courseID, const QString &setName, const QStringList &teamNames, const QList<QList<StudentRecord>> &teams,
                     const std::function<void(int teamNum, int stepsDone, int numSteps)> &progress = {});

    /**
     * @brief getGroupSets Gets a course's group sets.
     * @param ok Set to true if they were all gotten.
     */
    QList<CanvasGroupSet> getGroupSets(const long long courseID, bool *ok);

    /**
     * @brief getGroups Gets the groups in a group set, with the Canvas IDs of each group's members.
     * @param ok Set to true if they were all gotten.
     * @param progress If given, called as each group's members are about to be gotten, with the group's number and the number of groups.
     */
    QList<CanvasGroup> getGroups(const long long groupSetID, bool *ok, const std::function<void(int groupNum, int numGroups)> &progress = {});

    /**
     * @brief createPeerReviewQuizzes Creates a peer review's quizzes in a Canvas course: for each team a Classic graded survey, worth 1 point
     * for completing it and graded complete/incomplete, that only the team's students can see. All of a course's peer review quizzes go in
     * its "Peer Reviews" assignment group, which is made the first time.
     * @param courseID The Canvas course.
     * @param peerReview The peer review, whose students already have their Canvas IDs. It gets the quizzes' details and the record of
     * which question asks what about whom. A team with none of its students in the course gets no quiz.
     * @param progress If given, called as each team's quiz is started and after each request to Canvas, with the team's number,
     * the number of requests done, and the number to do in all.
     * @return True if the quizzes were made (if not, any already made are deleted, and lastErrorMessage says why).
     */
    bool createPeerReviewQuizzes(const long long courseID, PeerReview *const peerReview,
                                 const std::function<void(int teamNum, int stepsDone, int numSteps)> &progress = {});

    /**
     * @brief downloadPeerReviewSubmissions Downloads every submission, with its history, to a peer review's team quizzes.
     * @param ok Set to true if all of them were downloaded.
     * @param progress If given, called as each team quiz's submissions are about to be downloaded, with the number of quizzes done and in all.
     * @return The submissions, as given by the Canvas API.
     */
    QJsonArray downloadPeerReviewSubmissions(const PeerReview &peerReview, bool *ok, const std::function<void(int quizzesDone, int numQuizzes)> &progress = {});

    /**
     * @brief setPeerReviewQuizzesLocked Locks (or unlocks) a peer review's team quizzes, so students can't take them (or can again).
     * @param progress If given, called as each quiz is about to be changed, with the number of quizzes done and in all; undoing is true
     * while the quizzes already changed are being changed back, after one couldn't be.
     * @return True if every quiz was changed (if not, the ones already changed are changed back, and lastErrorMessage says why).
     */
    bool setPeerReviewQuizzesLocked(const PeerReview &peerReview, const bool locked,
                                    const std::function<void(int quizzesDone, int numQuizzes, bool undoing)> &progress = {});

    /**
     * @brief postSubmissionComment Adds a comment, which the student can see, to a student's submission to an assignment (e.g., their team quiz),
     * optionally with a file attached.
     * @param userID The student's Canvas ID.
     * @param fileName, fileContent, fileContentType The file to attach (none if fileContent is empty).
     * @return The new comment's ID (-1 if it couldn't be posted, and lastErrorMessage says why; 0 if it was posted, but Canvas didn't show which comment it is).
     */
    long long postSubmissionComment(const long long courseID, const long long assignmentID, const long long userID, const QString &text,
                                    const QString &fileName = {}, const QByteArray &fileContent = {}, const QString &fileContentType = {});

    /**
     * @brief deleteSubmissionComment Deletes a comment from a student's submission to an assignment.
     * @return True if it was deleted.
     */
    bool deleteSubmissionComment(const long long courseID, const long long assignmentID, const long long userID, const long long commentID);

    /**
     * @brief sendMessage Sends a student a private message in Canvas (to their Inbox), as a new conversation in the course.
     * @param userID The student's Canvas ID.
     * @return The new conversation's ID (-1 if it couldn't be sent, and lastErrorMessage says why).
     */
    long long sendMessage(const long long courseID, const long long userID, const QString &subject, const QString &body);

    /**
     * @brief getQuizSubmitterIDs Gets the Canvas IDs of the students who have submitted a quiz (or survey), finished or waiting to be graded.
     * @param ok Set to true if they were all gotten.
     */
    QList<long long> getQuizSubmitterIDs(const long long courseID, const long long quizID, bool *ok);

    struct QuizAssignees {bool everyone = true; QSet<long long> studentIDs;};
    /**
     * @brief getQuizAssignees Gets who a quiz (or survey) is assigned to: everyone in the course, or only some students
     * (the Canvas IDs of those it's assigned to, whether by name or through their section or group).
     * @param ok Set to true if Canvas said (false if it didn't, or if the quiz is only for some students but Canvas didn't say which).
     */
    QuizAssignees getQuizAssignees(const long long courseID, const long long quizID, bool *ok);

    QString getBaseURL() const {return baseURL;}

    static QPixmap icon();

    bool downloadCanceled = false;      // set by the user to stop waiting for downloadQuizResult()

    inline static const QString SCHEDULEQUESTIONINTRO1{QObject::tr("The following ")};
    inline static const QString SCHEDULEQUESTIONINTRO2{QObject::tr(" questions ask about your schedule on ")};
    inline static const QString SCHEDULEQUESTIONINTRO3{QObject::tr("You may leave a question blank as appropriate.")};
    inline static const QString PEERREVIEWASSIGNMENTGROUP{QObject::tr("Peer Reviews")};     // the assignment group for every peer review quiz in a course

private:
    void authenticateWithManualToken(const QString &token);
    QStringList askUserForManualURLandToken(const QString &currentAccountName = "", const QString &currentURL = "", const QString &currentToken = "");

    // waiting, if given, is called every few seconds while Canvas prepares the results, with the URL of the report's progress record (if known)
    QUrl getQuizResultsURL(const long long courseID, const long long quizID, const QElapsedTimer &waitTimer,
                           const std::function<void(const QString &progressURL)> &waiting = {});
    int reportPercentDone(const QString &progressURL);      // how much of a report Canvas says it has prepared (-1 if it doesn't say)
    // Canvas's three-step upload of a file to attach to a comment on a submission (given by its API path); returns the file's ID (-1 if not uploaded)
    long long uploadSubmissionCommentFile(const QString &submissionPath, const QString &fileName, const QByteArray &content, const QString &contentType);

    void getPaginatedCanvasResults(const QString &initialURL, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                              const QStringList &integerParams, QList<QList<long long>*> &integerVals,
                                                              const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals,
                                                              const QStringList &integerInSubArrayParams, QList<QList<long long>*> &integerInSubArrayVals);
    void postToCanvasGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                     const QStringList &integerParams, QList<QList<long long>*> &integerVals,
                                                                                     const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);
    bool downloadFile(const QUrl &URL, const QString &filepath);
    QJsonValue jsonRequest(const Method method, const QString &URL, const QJsonObject &body = {});   // the parsed reply (null if the request failed)
    // every item of a list that comes in pages (each page is the list, or else, if arrayKey is given, an object with the list under that key)
    QJsonArray getAllPages(const QString &initialURL, bool *ok, const QString &arrayKey = {});

    QString baseURL;
    long long ownUserID = -1;               // the Canvas ID of whoever is signed in, once it's needed (to tell which comments are theirs)
    QList<CanvasCourse> canvasCourses;
    QList<StudentRecord> roster;

    QWidget *parent = nullptr;

    QSet<QByteArray> getScopes() const override;
    QString getClientID() const override;
    QString getClientSecret() const override;
    QString getClientAuthorizationUrl() const override;
    QString getClientAccessTokenUrl() const override;
    QString getActionDialogIcon() const override;
    QString getActionDialogLabel() const override;
    std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> getModifyParametersFunction() const override;

    inline static const int NUM_PAGES_TO_LOAD = 20;
    inline static const int MAX_REPORT_WAIT_TIME = 240000;   //msec
    inline static const char SCOPES[]{"url:GET|/api/v1/courses "                                             // get list of user's courses
                                      "url:GET|/api/v1/courses/:course_id/users "                            // get roster of students in a course
                                      "url:GET|/api/v1/courses/:course_id/sections "                         // get the sections in a course
                                      "url:GET|/api/v1/courses/:course_id/sections/:id "                     // get the students in a section
                                      "url:GET|/api/v1/courses/:course_id/group_categories "                 // get the group sets in a course
                                      "url:GET|/api/v1/group_categories/:group_category_id/groups "          // get the groups in a group set
                                      "url:GET|/api/v1/groups/:group_id/users "                              // get the members of a group
                                      "url:POST|/api/v1/courses/:course_id/quizzes "                         // create a quiz (i.e., survey)
                                      "url:POST|/api/v1/courses/:course_id/quizzes/:quiz_id/questions "      // create a quiz question
                                      "url:GET|/api/v1/courses/:course_id/quizzes "                          // get list of quizzes in a course
                                      "url:POST|/api/v1/courses/:course_id/quizzes/:quiz_id/reports "        // create a quiz report (i.e., csv file of responses)
                                      "url:GET|/api/v1/courses/:course_id/quizzes/:quiz_id/reports "         // obtain the URL of the quiz report
                                      "url:GET|/api/v1/progress/:id "                                        // see how much of the quiz report is prepared
                                      "url:POST|/api/v1/courses/:course_id/group_categories "                // create a group set
                                      "url:POST|/api/v1/group_categories/:group_category_id/groups "         // create a group in a group set
                                      "url:POST|/api/v1/groups/:group_id/memberships "                       // put students on a group
                                      "url:GET|/api/v1/courses/:course_id/assignment_groups "                // find the course's peer review assignment group
                                      "url:POST|/api/v1/courses/:course_id/assignment_groups "               // create it, if it isn't there
                                      "url:DELETE|/api/v1/courses/:course_id/assignment_groups/:assignment_group_id "  // delete one, if creating fails
                                      "url:GET|/api/v1/courses/:course_id/quizzes/:id "                      // get a quiz (its assignment)
                                      "url:PUT|/api/v1/courses/:course_id/quizzes/:id "                      // publish a quiz
                                      "url:DELETE|/api/v1/courses/:course_id/quizzes/:id "                   // delete a quiz, if creating fails
                                      "url:PUT|/api/v1/courses/:course_id/assignments/:id "                  // set a quiz's grading (complete/incomplete)
                                      "url:POST|/api/v1/courses/:course_id/assignments/:assignment_id/overrides "      // show a quiz only to a team
                                      "url:PUT|/api/v1/courses/:course_id/assignments/:assignment_id/overrides/:id "   // lock or unlock a team's quiz
                                      "url:GET|/api/v1/courses/:course_id/assignments/:assignment_id/submissions "     // get each student's answers
                                      "url:GET|/api/v1/users/self "                                          // who is signed in (to find the comments they post)
                                      "url:POST|/api/v1/conversations "                                      // send a student a reminder
                                      "url:GET|/api/v1/courses/:course_id/quizzes/:quiz_id/submissions "     // who has submitted a survey
                                      "url:GET|/api/v1/courses/:course_id/quizzes/:quiz_id/date_details "    // who a survey is assigned to
                                      "url:GET|/api/v1/courses/:course_id/assignments/:assignment_id/submissions/:user_id "  // a student's submission, with its comments
                                      "url:PUT|/api/v1/courses/:course_id/assignments/:assignment_id/submissions/:user_id "  // post a comment on it
                                      "url:POST|/api/v1/courses/:course_id/assignments/:assignment_id/submissions/:user_id/comments/files "  // upload a file for a comment
                                      "url:DELETE|/api/v1/courses/:course_id/assignments/:assignment_id/submissions/:user_id/comments/:id"}; // delete a comment, when replacing it
    inline static const char ICON[]{":/icons_new/canvas.png"};
    //***********************************************
    //To be updated after out of beta
    inline static const QString CLIENT_ID{""};
    inline static const QString CLIENT_SECRET{""};
    inline static const QString authenticateURL = "";
    inline static const QString accessTokenURL = "";
    inline static const QString baseAPIURL = "";
    //***********************************************
};

#endif // CANVASHANDLER_H
