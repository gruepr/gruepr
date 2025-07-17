#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include "criteria/gradeBalanceCriterion.h"
#include "criteria/mixedGenderCriterion.h"
#include "criteria/multipleChoiceStyleCriterion.h"
#include "criteria/preventedTeammatesCriterion.h"
#include "criteria/requestedTeammatesCriterion.h"
#include "criteria/requiredTeammatesCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/singleGenderCriterion.h"
#include "criteria/singleURMIdentityCriterion.h"
#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/progressDialog.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "widgets/attributeWidget.h"
#include "widgets/boxwhiskerplot.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QPrinter>


namespace Ui {class gruepr;}

/**
 * @brief Responsible for main Gruepr functionality
 */
class gruepr : public QMainWindow
{
    Q_OBJECT

public:
    explicit gruepr(DataOptions &dataOptions, QList<StudentRecord> &students, QWidget *parent = nullptr);
    ~gruepr() override;
    gruepr(const gruepr&) = delete;
    gruepr operator= (const gruepr&) = delete;
    gruepr(gruepr&&) = delete;
    gruepr& operator= (gruepr&&) = delete;

    static void calcTeamScores(const QList<StudentRecord> &_students, const long long _numStudents,
                               TeamSet &_teams, const TeamingOptions *const _teamingOptions);

    bool restartRequested = false;

    inline static const int MAINWINDOWPADDING = 20;            // pixels of padding in buttons and above status message
    inline static const int MAINWINDOWFONT = 8;                // increase in font size for main window text
    inline static const int MAINWINDOWBUTTONFONT = 4;          // increase in font size for main window button text

signals:
    void closed();
    void generationComplete(const float *const allScores, const int *const orderedIndex,
                            const int generation, const float scoreStability, const bool unpenalizedGenomePresent);
    void sectionOptimizationFullyComplete();
    void turnOffBusyCursor();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void restartWithNewData();
    void changeSection(int index);
    void editSectionNames();
    void editAStudent();
    void removeAStudent(const QString &name);
    void removeAStudent(const long long ID, const bool delayVisualUpdate = false);
    void addAStudent();
    void compareStudentsToRoster();
    void rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    void simpleUIItemUpdate(QObject *sender = nullptr);
    void selectURMResponses();
    void responsesRulesButton_clicked(int attribute, int tabIndex);
    void changeIdealTeamSize();
    void chooseTeamSizes(int index);
    void makeTeammatesRules();
    void startOptimization();
    void updateOptimizationProgress(const float *const allScores, const int *const orderedIndex,
                                    const int generation, const float scoreStability, const bool unpenalizedGenomePresent);
    void optimizationComplete();
    void dataDisplayTabClose(int closingTabIndex);
    void editDataDisplayTabName(int tabIndex);
    void saveState();
private slots:
    void swapCriteriaCards(int draggedIndex, int targetIndex);
    void deleteCriteriaCard(int deletedIndex);
    void doAutoScroll(QPoint point);
private:
        // setup
    Ui::gruepr *ui;
    void loadDefaultSettings();
    void loadUI();
    DataOptions *dataOptions = nullptr;
    TeamingOptions *teamingOptions = nullptr;
    int numTeams = 1;
    inline void setTeamSizes(const QList<int> &teamSizes);
    inline void setTeamSizes(const int singleSize);
    inline QString writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB);

        // reading survey data
    long long numActiveStudents = MAX_STUDENTS;
    QList<StudentRecord> students;
    inline StudentRecord* findStudentFromID(const long long ID);
    bool loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails);   // returns false if file is invalid; checks survey names and emails against roster
    void refreshStudentDisplay();
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column
    QList<QPushButton *> attributeSelectorButtons;
    QList<AttributeWidget *> attributeWidgets = {};
    QList<GroupingCriteriaCard *> initializedAttributeCriteriaCards = {};
    QList<CriteriaType> teammateRulesExistence;

        // team set optimization
    QList<int> studentIndexes;                                    // the indexes of students to be placed on teams
    QList<int> optimizeTeams(const QList<int> studentIndexes);    // return value is a single permutation-of-indexes
    QFuture< QList<int> > future;                                 // needed so that optimization can happen in a separate thread
    QFutureWatcher< QList<int> > futureWatcher;                   // used for signaling of optimization completion
    BoxWhiskerPlot *progressChart = nullptr;
    progressDialog *progressWindow = nullptr;
    GA ga;                                                        // class for genetic algorithm optimization
    static float getGenomeScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions,
                                float _teamScores[], float **_criteriaScores, bool **_availabilityChart, int *_penaltyPoints);
    inline static void getAttributeScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, MultipleChoiceStyleCriterion *criterion, float *_criterionScore,
                                         const int attribute, std::multiset<int> &attributeLevelsInTeam, std::multiset<float> &timezoneLevelsInTeam,
                                         int *_penaltyPoints);
    inline static void getScheduleScores(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions,
                                         float *_schedScore, bool **_availabilityChart, int *_penaltyPoints);
    inline static void getGenderPenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                          const TeamingOptions *const _teamingOptions, int *_penaltyPoints);
    inline static void getURMPenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                       int *_penaltyPoints);
    inline static void getTeammatePenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                            const TeamingOptions *const _teamingOptions, int *_penaltyPoints);
    inline static void getMixedGenderScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                           const TeamingOptions *const _teamingOptions, MixedGenderCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getSingleGenderScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                            const TeamingOptions *const _teamingOptions, SingleGenderCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getSingleURMScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, SingleURMIdentityCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getPreventedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, PreventedTeammatesCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getRequiredTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                 const TeamingOptions *const _teamingOptions, RequiredTeammatesCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getRequestedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                 const TeamingOptions *const _teamingOptions, RequestedTeammatesCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getGradeBalanceScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                  const TeamingOptions *const _teamingOptions, GradeBalanceCriterion *criterion, float *_criterionScore, int *_penaltyPoints);
    inline static void getScheduleScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                  const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, ScheduleCriterion *criterion, float *_criterionScore, bool **_availabilityChart, int *_penaltyPoints);

    float teamSetScore = 0;
    int finalGeneration = 1;
    QMutex optimizationStoppedmutex;
    bool multipleSectionsInProgress = false;
    bool optimizationStopped = false;
    bool keepOptimizing = false;

        // reporting results
    TeamSet teams;
    QList<int> bestTeamSet;
    TeamSet finalTeams;

    //Extra refactoring
    QPushButton *letsDoItButton = nullptr;
    QPushButton *addGroupingCriteriaButton = nullptr;
    void initializeCriteriaCardPriorities();
    QPushButton *createAddNewCriteriaButton(bool hoverToSee);
    QPushButton *addNewCriteriaCardButton = nullptr;
    void updateIdentityCriteriaCard(GroupingCriteriaCard *identityCard, QString identity, bool addNewCriteria);
    void refreshCriteriaLayout();

    //make an enum criteria type.
    void addCriteriaCard(CriteriaType criteriaType);
    void addCriteriaCard(CriteriaType criteriaType, Gender gender, bool requireMixed = false);
    void addCriteriaCard(CriteriaType criteriaType, QString urmResponse);
    void addCriteriaCard(CriteriaType criteriaType, int attribute);

    QMenu *mainMenu;

    //Criteria Cards
    QList<GroupingCriteriaCard*> criteriaCardsList;

    //Single: Team Size Criteria Card
    GroupingCriteriaCard *teamSizeCriteriaCard = nullptr;
    QHBoxLayout *teamSizeContentAreaLayout = nullptr;
    QComboBox *teamSizeBox = nullptr;
    QSpinBox *idealTeamSizeBox = nullptr;

    //Single: Section Criteria Card
    GroupingCriteriaCard *sectionCriteriaCard = nullptr;
    QHBoxLayout *sectionContentLayout = nullptr;
    QPushButton *editSectionNameButton = nullptr;
    QComboBox *sectionSelectionBox = nullptr;

    //MCQ Criteria Card //likert scale or categorical?
    QList<int> addedAttributeNumbersList;

    //Identity Options Card (these are all objects that can be created)
    QList<GroupingCriteriaCard*> identityOptionsCardList;
    QMap<QString, bool> uiCheckBoxMap;

    //Single: Meeting Schedule Criteria Card
    GroupingCriteriaCard *meetingScheduleCriteriaCard = nullptr;
    GroupingCriteriaCard *gradeBalanceCriteriaCard = nullptr;
    QDoubleSpinBox *minimumMeanGradeSpinBox = nullptr;
    QDoubleSpinBox *maximumMeanGradeSpinBox = nullptr;
    QSpinBox *minMeetingTimes = nullptr;
    QSpinBox *desiredMeetingTimes = nullptr;
    QDoubleSpinBox *meetingLengthSpinBox = nullptr;
};

#endif // GRUEPR_H
