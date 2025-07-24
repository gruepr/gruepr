#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include "csvfile.h"
#include "dataOptions.h"
#include "criteria/genderCriterion.h"
#include "criteria/gradeBalanceCriterion.h"
#include "criteria/multipleChoiceCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/singleURMIdentityCriterion.h"
#include "criteria/teammatesCriterion.h"
#include "dialogs/progressDialog.h"
#include "dialogs/attributeRulesDialog.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
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
    explicit gruepr(DataOptions &dataOptions, QList<StudentRecord> &students);
    ~gruepr() override;
    gruepr(const gruepr&) = delete;
    gruepr operator= (const gruepr&) = delete;
    gruepr(gruepr&&) = delete;
    gruepr& operator= (gruepr&&) = delete;

    static void calcTeamScores(const QList<StudentRecord> &_students, const long long _numStudents,
                               TeamSet &_teams, const TeamingOptions *const _teamingOptions);

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
    void responsesRulesButton_clicked(int attribute, AttributeRulesDialog::TypeOfRules typeOfRules);
    void changeIdealTeamSize();
    void chooseTeamSizes(int index);
    void startOptimization();
    void updateOptimizationProgress(const float *const allScores, const int *const orderedIndex,
                                    const int generation, const float scoreStability, const bool unpenalizedGenomePresent);
    void optimizationComplete();
    void dataDisplayTabClose(int closingTabIndex);
    void editDataDisplayTabName(int tabIndex);
    void saveState();

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
    bool loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails);   // returns false if file is invalid; checks names and emails against roster
    void refreshStudentDisplay();
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column

        // team set optimization
    QPushButton *letsDoItButton = nullptr;
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
                                         const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, MultipleChoiceCriterion *criterion,
                                         float *_criterionScore, const int attribute, std::multiset<int> &attributeLevelsInTeam,
                                         std::multiset<float> &timezoneLevelsInTeam, int *_penaltyPoints);
    inline static void getScheduleScores(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions,
                                         float *_schedScore, bool **_availabilityChart, int *_penaltyPoints);
    inline static void getGenderPenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                          const TeamingOptions *const _teamingOptions, int *_penaltyPoints);
    inline static void getURMPenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                       int *_penaltyPoints);
    inline static void getTeammatePenalties(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                            const TeamingOptions *const _teamingOptions, int *_penaltyPoints);
    inline static void getGenderScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                      const TeamingOptions *const _teamingOptions, GenderCriterion *criterion,
                                      float *_criterionScore, int *_penaltyPoints);
    inline static void getSingleURMScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                         const TeamingOptions *const _teamingOptions, SingleURMIdentityCriterion *criterion,
                                         float *_criterionScore, int *_penaltyPoints);
    inline static void getPreventedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                  const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                                  float *_criterionScore, int *_penaltyPoints);
    inline static void getRequiredTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                 const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                                 float *_criterionScore, int *_penaltyPoints);
    inline static void getRequestedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                 const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                                  float *_criterionScore, int *_penaltyPoints);
    inline static void getGradeBalanceScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                  const TeamingOptions *const _teamingOptions, GradeBalanceCriterion *criterion,
                                                  float *_criterionScore, int *_penaltyPoints);
    inline static void getScheduleScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                                  const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, ScheduleCriterion *criterion,
                                                  float *_criterionScore, bool **_availabilityChart, int *_penaltyPoints);
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


        //Criteria Cards
    GroupingCriteriaCard *teamsizeCriteriaCard = nullptr;
        QComboBox *teamSizeBox = nullptr;
        QSpinBox *idealTeamSizeBox = nullptr;
    GroupingCriteriaCard *sectionCriteriaCard = nullptr;
        QComboBox *sectionSelectionBox = nullptr;
    GroupingCriteriaCard *genderIdentityCriteriaCard = nullptr;
    GroupingCriteriaCard *meetingScheduleCriteriaCard = nullptr;
    QList<Criterion::CriteriaType> teammateRulesExistence;
    QList<GroupingCriteriaCard*> initializedAttributeCriteriaCards;
    QList<AttributeWidget*> attributeWidgets;
        QList<int> addedAttributeNumbersList;
    QList<GroupingCriteriaCard*> criteriaCardsList;
    QPushButton *addNewCriteriaCardButton = nullptr;
    QMenu *addNewCriteriaMenu = nullptr;
    void addCriteriaCard(Criterion::CriteriaType criteriaType);
    void addCriteriaCard(Criterion::CriteriaType criteriaType, int attribute);
    void initializeCriteriaCardPriorities();
    void refreshCriteriaLayout();

        //Single: URM Identity Card
    GroupingCriteriaCard *urmIdentityCard = nullptr;

        //Single: Grade Balance Criteria Card
    GroupingCriteriaCard *gradeBalanceCriteriaCard = nullptr;
    QDoubleSpinBox *minimumMeanGradeSpinBox = nullptr;
    QDoubleSpinBox *maximumMeanGradeSpinBox = nullptr;
};

#endif // GRUEPR_H
