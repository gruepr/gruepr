#ifndef PEERREVIEW_H
#define PEERREVIEW_H

// the questions, copy of the teams, Google Form, responses, scores, and feedback reports of one peer review

#include "gruepr_globals.h"
#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QStringList>
#include <limits>

class PeerReview;

/**
 * @brief The PeerReviewQuestion class is one question asked in a peer review, along with who it is about and who sees its answers.
 */
class PeerReviewQuestion
{
public:
    enum class Kind {likert, numberScale, choice, numberEntry, contribution, written};
    enum class About {eachTeammate, eachTeammateAndSelf, self, once};
    enum class ShareWith {instructor, personItsAbout, team};
    enum class ContributionStyle {dropdown, points, grid};

    Kind kind = Kind::likert;
    QString text;
    QStringList options;                                    // likert and choice: the response options (likert options are numbered, e.g. "1. Always")
    bool multi = false;                                     // choice: students may select more than one option
    bool bestFirst = true;                                  // likert: the first option is the best one (as in all of gruepr's built-in scales)
    int scaleLow = 1;                                       // number scale: the lowest and highest numbers, with optional labels at each end
    int scaleHigh = 5;
    QString lowLabel;
    QString highLabel;
    ContributionStyle contributionStyle = ContributionStyle::grid;
    About about = About::eachTeammateAndSelf;
    ShareWith shareWith = ShareWith::personItsAbout;
    bool namesShown = false;                                // shared answers are shown with their writer's name (always so for "about yourself")
    bool required = true;

    QJsonObject toJson() const;
    static PeerReviewQuestion fromJson(const QJsonObject &json);

    /**
     * @brief showsWriterNames Whether students who are shown the answers also see who wrote each one.
     */
    bool showsWriterNames() const;

    /**
     * @brief isScored Whether the answers are scores, with higher scores better (likert, number scale, and contribution questions).
     */
    bool isScored() const;

    /**
     * @brief isAboutTeammates Whether the question is asked separately about each member of the team.
     */
    bool isAboutTeammates() const;

    /**
     * @brief requiredIsPossible Whether the form can require an answer. It can't for questions about each teammate only, since
     * every member of a team answers on the same page of the form and each one skips the line with their own name.
     */
    bool requiredIsPossible() const;

    /**
     * @brief isRequiredInForm Whether the question is required in the form, which is only when it's both wanted and possible.
     */
    bool isRequiredInForm() const {return required && requiredIsPossible();}

    /**
     * @brief formLabels The options shown in the form for each answer: the likert or choice options (without their numbers),
     * the numbers of a number scale (with the end labels), or the contribution levels.
     */
    QStringList formLabels() const;

    /**
     * @brief usesGrid Whether the question about each teammate is shown in the form as a grid with one row per team member,
     * rather than as a separate question per team member.
     */
    bool usesGrid() const;

    /**
     * @brief scoreOf The score of an answer to a scored question.
     * @param answer The answer, as given in the form.
     * @param ok Set to true if the answer has a score (false if it is blank, "Not sure", or not understood).
     * @return The score, with higher always better (contribution scores are percent of an equal share, so 100 = equal share).
     */
    double scoreOf(const QString &answer, bool *ok) const;

    /**
     * @brief normalizedScore A score scaled to the range (0, 1], so that the scores of questions with different scales can be combined.
     */
    double normalizedScore(double score) const;

    /**
     * @brief scoreDescription Describes a score in the words of the question, e.g., "4.3 (between Usually and Always)" or "equal share - 10%".
     */
    QString scoreDescription(double score) const;

    /**
     * @brief visibilityNote The note added to the question in the form telling students who will see their answers.
     */
    QString visibilityNote() const;

    /**
     * @brief formDescription The description shown under the question in the form: its notes, such as who will see the answers.
     */
    QString formDescription() const;

    /**
     * @brief sharingChoices The choices of who will see the answers, and whether they will see the writers' names, that suit who the question is about.
     */
    static QList<QPair<ShareWith, bool>> sharingChoices(About about);
    static QString aboutText(About about);
    static QString shareWithText(ShareWith shareWith, About about, bool namesShown);
    static QString kindText(Kind kind);

    /**
     * @brief contributionLevels The contribution dropdown options, from no contribution up to twice an equal share in steps of 10%,
     * with the matching percent of an equal share for each (and -1 for "Not sure").
     */
    static const QList<QPair<QString, int>> &contributionLevels();

    /**
     * @brief contributionGridLevels The columns of the contribution grid, from -100% to +100% of an equal share in steps that are closer
     * together near the middle (-100%, -50%, -25%, -10%, "Equal share", +10%, +25%, +50%, +100%), and the matching percent of an equal share
     * for each (and -1 for "Not sure").
     */
    static const QList<QPair<QString, int>> &contributionGridLevels();

    static QString withoutOrderNumbers(const QString &option);   // "1. Always" -> "Always"
};


/**
 * @brief The PeerReviewStudent struct is one student in a peer review's copy of the teams.
 */
struct PeerReviewStudent
{
    long long ID = -1;                  // unique within the peer review
    long long LMSID = -1;               // student ID number according to the learning management system, if known
    QString firstname;
    QString lastname;
    QString email;
    QString section;

    QString fullName() const {return (firstname + " " + lastname).simplified();}
    QJsonObject toJson() const;
    static PeerReviewStudent fromJson(const QJsonObject &json);
};


/**
 * @brief The PeerReviewTeam struct is one team in a peer review's copy of the teams.
 */
struct PeerReviewTeam
{
    QString name;
    QList<PeerReviewStudent> members;

    QJsonObject toJson() const;
    static PeerReviewTeam fromJson(const QJsonObject &json);
};


/**
 * @brief The PeerReviewFormItem struct records which question of the Google Form, or of a team's Canvas quiz, asks which peer review question about whom.
 */
struct PeerReviewFormItem
{
    QString itemID;                     // the Google Form's question ID, or for a Canvas quiz "<question ID>" or "<question ID>/<blank ID>"
    int questionNum = -1;               // index into PeerReview::questions
    int teamNum = -1;                   // index into PeerReview::teams
    long long aboutID = ABOUTNOONE;     // the student the answer is about, or one of the two values below

    inline static const long long ABOUTGIVER = -2;     // "about yourself": the answer is about the student who gave it
    inline static const long long ABOUTNOONE = -3;     // asked once: the answer isn't about one particular student
};


/**
 * @brief The PeerReviewResponse struct is one student's submitted form (or one attempt at their team's Canvas quiz).
 */
struct PeerReviewResponse
{
    QString responseID;
    QDateTime submittedTime;
    QString respondentEmail;                    // only collected if the form requires Google sign-in
    QString nameChosen;                         // (for a Canvas quiz, the name of the student who took it)
    long long giverID = -1;                     // -1 if the name chosen doesn't match a student
    bool superseded = false;                    // the same name submitted again later, and only the latest response is used
    bool enteredByInstructor = false;           // submitted while signed in with the instructor's own Google account
    QHash<QString, QStringList> answers;        // the form item ID -> the answer(s)
};


/**
 * @brief The PeerReviewAnswer struct is one answer by one student about one student (or about the team, or themself).
 */
struct PeerReviewAnswer
{
    int questionNum = -1;
    int teamNum = -1;
    long long giverID = -1;
    long long aboutID = PeerReviewFormItem::ABOUTNOONE;     // the student it is about (the giver themself if it's an "about yourself" question)
    QStringList values;
    bool hasScore = false;
    double score = 0;
};


/**
 * @brief The PeerReviewThresholds struct holds the settings used to calculate the adjustment factor and to flag students.
 */
struct PeerReviewThresholds
{
    double lowContributor = 0.85;       // adjustment factor at or below this is flagged as a low contributor
    double highContributor = 1.15;      // adjustment factor at or above this is flagged as a high contributor
    int selfGapPercent = 20;            // rating oneself this much higher or lower (as a percent of the scale) than teammates do is flagged
    int disagreementPercent = 40;       // teammates' ratings of a student that spread this much (as a percent of the scale) are flagged
    bool capFactor = false;             // whether the adjustment factor is limited to factorCap
    double factorCap = 1.05;
    bool factorFromAllQuestions = false;    // with a contribution question: whether the rating questions are averaged in too, instead of contribution alone

    QJsonObject toJson() const;
    static PeerReviewThresholds fromJson(const QJsonObject &json);
};


/**
 * @brief The PeerReviewRelease struct holds the instructor's choices about what goes into each student's feedback report,
 * and a record of which reports were delivered how and when.
 */
struct PeerReviewRelease
{
    struct LogEntry {QDateTime time; long long studentID = -1; QString method; QString detail;};

    QString introText = PEERREVIEWREPORTINTRO;
    int minimumResponders = 2;                          // with fewer teammates responding, the ratings and comments shared without names are withheld
    bool includeTeamAverage = false;
    bool includeAdjustmentFactor = false;
    QSet<long long> heldBack;                           // students whose reports are not delivered
    QSet<long long> minimumOverridden;                  // students whose reports include everything despite too few responders
    QHash<long long, QString> personalNotes;            // student ID -> a note from the instructor in their report
    QHash<QString, QString> editedComments;             // comment key -> the comment text as edited by the instructor
    QSet<QString> hiddenComments;                       // comment keys of the comments left out of the reports
    QList<LogEntry> log;

    QJsonObject toJson() const;
    static PeerReviewRelease fromJson(const QJsonObject &json);
};


/**
 * @brief The PeerReviewResults class holds what is calculated from a peer review's responses: who responded, each
 * student's scores, the contribution results, and the flags.
 */
class PeerReviewResults
{
public:
    PeerReviewResults(const PeerReview &peerReview, const QList<PeerReviewResponse> &allResponses);

    inline static const double NOVALUE = std::numeric_limits<double>::quiet_NaN();

    struct ScoreSummary {int count = 0; double average = NOVALUE; double low = NOVALUE; double high = NOVALUE; double self = NOVALUE;};

    struct StudentResult
    {
        long long ID = -1;
        int teamNum = -1;
        int numResponses = 0;                           // forms submitted under this student's name (more than 1 means duplicates)
        bool enteredByInstructor = false;
        QString mismatchedEmail;                        // the sign-in email, if it doesn't match this student's email
        int numRaters = 0;                              // teammates (not counting themself) who answered a question about this student
        QList<ScoreSummary> scores;                     // by question number: what teammates gave this student, and their self rating
        QList<QList<QPair<QString, int>>> choiceCounts; // by question number: how many teammates chose each option for this student
        double peerOverall = NOVALUE;                   // average of teammates' scaled ratings, over all rating questions
        double selfOverall = NOVALUE;                   // the same for this student's ratings of themself
        double givenToTeammates = NOVALUE;              // average of this student's scaled ratings of their teammates
        double spread = NOVALUE;                        // largest spread of teammates' scaled ratings of this student on one question
        double claimed = NOVALUE;                       // contribution: this student's own estimate (100 = equal share)
        double perceived = NOVALUE;                     // contribution: teammates' view, with self-ratings left out (100 = equal share)
        double factor = NOVALUE;                        // adjustment factor
        QStringList flags;
    };

    struct TeamContribution
    {
        QList<QList<double>> ratings;       // [giver][recipient], scaled so each giver's ratings of their teammates average 100
        bool disagreement = false;
    };

    QList<PeerReviewResponse> responses;    // all responses, in the order submitted
    QList<PeerReviewAnswer> answers;        // every answer from the responses that are used
    QList<StudentResult> students;          // in the order of the teams
    QList<TeamContribution> contributions;  // by team, from the first contribution question (if any)
    QStringList unmatchedNames;             // names chosen in the form that aren't in the peer review's teams
    int contributionQuestionNum = -1;

    const StudentResult *result(long long studentID) const;
    int numResponded() const;

private:
    QHash<long long, int> indexOfStudent;
    void calculateContribution(const PeerReview &peerReview, const int teamNum);
};


/**
 * @brief The PeerReview class is one peer review: its questions, its own copy of the teams, its Google Form or Canvas quizzes, and the
 * choices made in releasing feedback to the students. Each peer review is saved as its own file in gruepr's app data folder.
 */
class PeerReview
{
public:
    QString title;
    QDateTime createdTime;
    QString teamsSource;                            // where the teams came from (shown to the instructor)
    QList<PeerReviewTeam> teams;
    QList<PeerReviewQuestion> questions;
    QString instructions = PEERREVIEWINSTRUCTIONS;  // shown at the top of the form
    bool requireSignIn = false;

    enum class Platform {googleForm, canvasQuizzes};
    Platform platform = Platform::googleForm;

    QString formID;
    QString responderURL;
    QString accountName;                            // the Google account that owns the form
    bool formPublished = false;
    bool acceptingResponses = true;
    QString nameQuestionID;
    QList<PeerReviewFormItem> formItems;

    /**
     * @brief The CanvasTeamQuiz struct is one team's quiz in Canvas (a graded survey, visible only to the team's students).
     */
    struct CanvasTeamQuiz {long long quizID = -1; long long assignmentID = -1; long long overrideID = -1; QString url;};
    long long canvasCourseID = -1;
    QString canvasCourseName;
    QString canvasCourseURL;
    long long canvasAssignmentGroupID = -1;         // the assignment group that holds the team quizzes
    bool countTowardFinalGrade = false;             // whether completing a team quiz counts toward the final grade
    QList<CanvasTeamQuiz> canvasQuizzes;            // by team number (a quizID of -1 if the team has no quiz)
    QHash<QString, QString> canvasAnswerLabels;     // "<question ID>:<option ID>" (the IDs Canvas gave them) -> the option
                                                    // (Canvas's option IDs are unique only within a question)
    bool hasForm() const;                           // whether the Google Form or the Canvas quizzes were created

    QJsonArray lastResponses;                       // the most recently downloaded responses, so results can be shown offline
    QDateTime lastResponsesTime;

    struct Reminder {QDateTime time; long long studentID = -1;};
    QList<Reminder> reminders;                      // the reminders sent to students who hadn't responded, as Canvas messages

    PeerReviewThresholds thresholds;
    PeerReviewRelease release;
    QString fileName;                               // where this peer review is saved

    QJsonObject toJson() const;
    static PeerReview fromJson(const QJsonObject &json);

    /**
     * @brief save Saves this peer review to its file (giving it a new file if it doesn't have one yet) and lists it in the saved settings.
     * @return True if the file was written.
     */
    bool save();
    static bool load(const QString &fileName, PeerReview &peerReview);

    /**
     * @brief The StoredPeerReview struct is one entry in the list of saved peer reviews.
     */
    struct StoredPeerReview {QString name; QString fileName; QDateTime createdTime; int numStudents = 0; bool hasForm = false; bool onCanvas = false;};
    static QList<StoredPeerReview> storedPeerReviews();         // newest first
    static bool deleteStored(const QString &fileName);

    /**
     * @brief The SavedSessionTeamSet struct is one set of teams found in a saved gruepr session.
     */
    struct SavedSessionTeamSet {QString name; QList<PeerReviewTeam> teams;};
    static QList<QPair<QString, QString>> savedSessions();      // display name and file name of each saved gruepr session, newest first
    static QList<SavedSessionTeamSet> teamSetsInSavedSession(const QString &fileName);
    static QList<PeerReviewTeam> teamsFromSpreadsheetRows(const QList<QStringList> &rows);

    static QList<PeerReviewQuestion> defaultQuestions();
    static QList<PeerReviewQuestion> contributionBasedQuestions();
    static QList<PeerReviewQuestion> sampleQuestions();
    static bool saveQuestionSet(const QString &fileName, const QList<PeerReviewQuestion> &questionSet);
    static bool loadQuestionSet(const QString &fileName, QList<PeerReviewQuestion> &questionSet);

    int numStudents() const;
    const PeerReviewStudent *student(const long long studentID) const;
    int teamOf(const long long studentID) const;

    /**
     * @brief nameOptions The names offered in the form's name question, in the order shown, with the student ID of each.
     * Names shared by more than one student get their team name added to tell them apart, and a number too if they are on the same team.
     */
    QList<QPair<QString, long long>> nameOptions() const;

    /**
     * @brief memberLabel How a team member is named on their team's page of the form: their name, with a number added
     * if a teammate has the same name (e.g., "Alex Kim (2)", numbered in the order the team lists them).
     */
    QString memberLabel(const int teamNum, const int memberNum) const;

    /**
     * @brief teamsWithSameNameMembers For each team with two or more members of the same name, the team and those members' labels.
     */
    QStringList teamsWithSameNameMembers() const;

    /**
     * @brief parseResponses Reads the responses downloaded from the Google Form, or the submissions downloaded from the Canvas quizzes.
     * @param responsesJson The "responses" downloaded from the Forms API, or the submissions (with their history) downloaded from Canvas.
     * @return The responses, in the order submitted, with the superseded and instructor-entered ones marked.
     */
    QList<PeerReviewResponse> parseResponses(const QJsonArray &responsesJson) const;

    static QString commentKey(const PeerReviewAnswer &answer);
    QString commentText(const PeerReviewAnswer &answer) const;      // the answer's text, as edited by the instructor
    bool commentIsHidden(const PeerReviewAnswer &answer) const;

    /**
     * @brief feedbackIsWithheld Whether the ratings and comments shared without names are left out of a student's report because too few teammates responded.
     */
    bool feedbackIsWithheld(const PeerReviewResults &results, const long long studentID) const;

    /**
     * @brief reportHtml The feedback report for one student, as shown to them.
     */
    QString reportHtml(const PeerReviewResults &results, const long long studentID) const;
    QString reportPlainText(const PeerReviewResults &results, const long long studentID) const;

    /**
     * @brief reportFileName A file name for a student's report, e.g., "Lastname_Firstname_feedback", unique within the peer review.
     */
    QString reportFileName(const long long studentID) const;

    inline static const char QUESTIONSETFILEFILTER[] = "gruepr peer review questions (*.gpq)";

private:
    QList<PeerReviewResponse> parseCanvasSubmissions(const QJsonArray &submissionsJson) const;
    int sameNameNumberOnTeam(const int teamNum, const int memberNum) const;     // 1, 2, ... among teammates with the same name; 0 if none share it
    QString describeScores(const PeerReviewResults &results, const PeerReviewResults::StudentResult &studentResult, const int questionNum) const;
    QStringList sharedCommentsFor(const PeerReviewResults &results, const long long studentID, const int questionNum) const;
};

#endif // PEERREVIEW_H
