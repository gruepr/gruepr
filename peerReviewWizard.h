#ifndef PEERREVIEWWIZARD_H
#define PEERREVIEWWIZARD_H

#include "peerReview.h"
#include "studentRecord.h"
#include "surveyMakerWizard.h"
#include "dialogs/peerReviewSampleQuestionsDialog.h"
#include "widgets/peerReviewQuestion.h"
#include <QButtonGroup>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QWizard>

class CanvasHandler;


/**
 * @brief The PeerReviewWizard class is responsible for creating a new peer review: getting the teams, building the questions,
 * and creating the Google Form that the students fill out.
 */
class PeerReviewWizard : public QWizard
{
    Q_OBJECT

public:
    /**
     * @brief PeerReviewWizard
     * @param earlierPeerReview If not null, the new peer review starts as a copy of this one (a new round with the same teams and questions).
     * @param parent The parent window.
     */
    explicit PeerReviewWizard(const PeerReview *earlierPeerReview = nullptr, QWidget *parent = nullptr);
    ~PeerReviewWizard() override;
    PeerReviewWizard(const PeerReviewWizard&) = delete;
    PeerReviewWizard operator= (const PeerReviewWizard&) = delete;
    PeerReviewWizard(PeerReviewWizard&&) = delete;
    PeerReviewWizard& operator= (PeerReviewWizard&&) = delete;

    enum Page{teams, rated, written, previewcreate};
    static inline const QStringList pageNames = {QObject::tr("Name & Teams"), QObject::tr("Rating & Choice Questions"),
                                                 QObject::tr("Written Questions"), QObject::tr("Preview & Create")};

    PeerReview peerReview;                  // the peer review being built
    const bool isNewRound;                  // started as a copy of an earlier peer review
    QString earlierPeerReviewFile;          // for a new round, the earlier peer review's file
    QFileInfo saveFileLocation;
    bool previewPageVisited = false;
    bool peerReviewHasBeenCreated = false;
    long long canvasTeamsCourseID = -1;     // the Canvas course the teams came from (or, in a new round, the earlier round's course), if any

    /**
     * @brief connectToCanvas The wizard's connection to Canvas, kept so the instructor signs in only once: the one already made,
     * or else a new one, once the instructor signs in.
     * @return The connection, or nullptr if the instructor didn't sign in.
     */
    CanvasHandler *connectToCanvas();

    /**
     * @brief dropCanvasConnection Lets go of the connection to Canvas, so the next connectToCanvas() asks the instructor to sign in again
     * (used whenever something asked of Canvas fails, since the URL or token may be wrong).
     */
    void dropCanvasConnection();

    /**
     * @brief setQuestions Puts the questions onto the two question pages (the written ones on the written questions page).
     */
    void setQuestions(const QList<PeerReviewQuestion> &questions);

    /**
     * @brief currentQuestions The questions on the two question pages, the rating and choice questions first.
     */
    QList<PeerReviewQuestion> currentQuestions() const;

    /**
     * @brief startFromQuestions Puts a new set of questions onto the two question pages, as the questions to start from.
     */
    void startFromQuestions(const QList<PeerReviewQuestion> &questions);

    /**
     * @brief questionsWereEdited Whether the questions on the two question pages have been changed since they were started from.
     */
    bool questionsWereEdited() const;

    /**
     * @brief loadTeams Loads the teams of the first page. Since reading a large saved gruepr session takes a moment,
     * this is called once the wizard is showing, rather than as it is built.
     */
    void loadTeams();

    /**
     * @brief previewNames The names of the members of the first team, used in the previews of the questions.
     */
    QStringList previewNames() const;

    static QString nextRoundTitle(const QString &earlierTitle);

public slots:
    void reject() override;

signals:
    void teamsChanged();

private:
    static QJsonArray questionsAsJson(const QList<PeerReviewQuestion> &questions);
    QJsonArray startingQuestions;          // the questions most recently started from, as the question pages hold them
    CanvasHandler *canvas = nullptr;        // the connection to Canvas, once the instructor has signed in (a child of the wizard)
};


/**
 * @brief The PeerReviewTeamsPage class is the page of the peer review wizard that names the peer review and gets its teams.
 */
class PeerReviewTeamsPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PeerReviewTeamsPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;

    /**
     * @brief loadTeams Loads the teams from the source chosen when the page is first shown.
     */
    void loadTeams();

private:
    enum Source {earlierReview, savedSession, spreadsheet, canvasGroupSet};
    void sourceChange();
    void savedSessionChange();
    void teamSetChange();
    void loadSpreadsheet();

    /**
     * @brief loadCanvasGroupSet Asks the instructor for a Canvas course and one of its group sets, then makes a team of each group
     * that has any of the course's students in it. Each student keeps their Canvas ID, for creating the team quizzes in Canvas.
     */
    void loadCanvasGroupSet();
    void setTeams(const QList<PeerReviewTeam> &newTeams, const QString &source);
    void updatePreview();
    void questionsChange(int index);

    QLineEdit *titleLineEdit = nullptr;
    QButtonGroup *sourceButtons = nullptr;
    QRadioButton *earlierReviewRadioButton = nullptr;
    QRadioButton *savedSessionRadioButton = nullptr;
    QRadioButton *spreadsheetRadioButton = nullptr;
    QRadioButton *canvasRadioButton = nullptr;
    StyledComboBox *savedSessionComboBox = nullptr;
    StyledComboBox *teamSetComboBox = nullptr;
    QPushButton *spreadsheetButton = nullptr;
    QLabel *spreadsheetLabel = nullptr;
    QPushButton *canvasButton = nullptr;
    QLabel *canvasLabel = nullptr;
    StyledComboBox *questionsComboBox = nullptr;
    int questionsComboBoxIndex = 0;                 // the questions most recently started from, to go back to if a choice is cancelled
    QLabel *previewTitle = nullptr;
    QLabel *teamsPreviewLabel = nullptr;
    QHash<QString, QList<PeerReview::SavedSessionTeamSet>> savedSessionTeamSets;     // saved session file name -> its sets of teams, once read
    QList<PeerReview::SavedSessionTeamSet> teamSetsInSession;
    QList<PeerReviewTeam> earlierTeams;
    QString earlierTeamsSource;
    QList<PeerReviewTeam> spreadsheetTeams;
    QString spreadsheetSource;
    QList<PeerReviewTeam> canvasTeams;
    QString canvasSource;
    bool initialized = false;
    bool teamsLoaded = false;
};


/**
 * @brief The PeerReviewQuestionsPage class is a page of the peer review wizard that builds questions: either the rating and
 * choice questions, or the written questions.
 */
class PeerReviewQuestionsPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PeerReviewQuestionsPage(const bool writtenQuestions, QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;

    void setQuestions(const QList<PeerReviewQuestion> &newQuestions);
    QList<PeerReviewQuestion> getQuestions() const;

    inline static const int MAXQUESTIONS = 15;

private:
    void createQuestion(const int questionNum);     // questions are built on demand, in order, from addQuestion()
    void addQuestion();
    void addSampleQuestion(const PeerReviewQuestion &question);
    void deleteAQuestion(int questionNum);
    void updatePreviewNames();

    PeerReviewQuestion questionAt(const int questionNum) const;
    void setQuestionAt(const int questionNum, const PeerReviewQuestion &question);
    void clearQuestionAt(const int questionNum);
    QFrame *questionWidgetAt(const int questionNum) const;

    const bool written;
    int firstQuestionItemIndex = 0;                 // where in questionLayout the first question's spacer goes
    int numQuestions = 0;
    PeerReviewSampleQuestionsDialog *sampleQuestionsDialog = nullptr;
    QList<PeerReviewRatedQuestion *> ratedQuestions;
    QList<PeerReviewWrittenQuestion *> writtenQuestions;
    QList<QSpacerItem *> spacers;
    QList<QFrame *> previewSeparators;
    QPushButton *addQuestionButton = nullptr;
};


/**
 * @brief The PeerReviewPreviewPage class is the last page of the peer review wizard, which previews the form and creates it
 * (a Google Form, or a quiz in Canvas for each team).
 */
class PeerReviewPreviewPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PeerReviewPreviewPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

private:
    void saveQuestionSet();
    void showDestinationSettings();     // the settings that go with the chosen destination (Google Form or Canvas quizzes)
    void createPeerReview();
    void createGoogleForm();
    void createCanvasQuizzes();

    /**
     * @brief matchToCanvasRoster Gives each of the peer review's students their Canvas ID, matching them to a course's roster:
     * by the Canvas ID they already have, then by email, then by their full name (if only one student on the roster has it).
     * Students who would be matched to the same Canvas student are left unmatched, since there's no telling which is which.
     * @return The names of the students who couldn't be matched.
     */
    QStringList matchToCanvasRoster(const QList<StudentRecord> &roster);

    QLabel *summaryLabel = nullptr;
    QWidget *formPreview = nullptr;
    QVBoxLayout *formPreviewLayout = nullptr;
    QLabel *instructionsLabel = nullptr;
    QPlainTextEdit *instructionsTextEdit = nullptr;
    QCheckBox *requireSignInCheckBox = nullptr;
    QCheckBox *countGradeCheckBox = nullptr;
    QRadioButton *destinationGoogle = nullptr;
    QRadioButton *destinationCanvas = nullptr;
    QPushButton *createButton = nullptr;
};


#endif // PEERREVIEWWIZARD_H
