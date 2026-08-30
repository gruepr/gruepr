#ifndef GRUEPR_H
#define GRUEPR_H

#include "dataOptions.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "dialogs/progressDialog.h"
#include "widgets/attributeWidget.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include "widgets/scoreLineGraph.h"
#include "widgets/styledComboBox.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QProgressDialog>
#include <QSpinBox>
#include <chrono>
#include <compare>
#include <vector>


namespace Ui {class gruepr;}

// A genome's score paired with the harmonic mean of its positive teams, which breaks ties among
// broken teamsets whose score alone (the average deficit over non-positive teams) is blind to how
// good the healthy teams are.
struct GenomeScore {
    float score = 0;
    float positiveTeamsHarmonicMean = 0;

    friend std::partial_ordering operator<=>(const GenomeScore &a, const GenomeScore &b) {
        if(const auto comparison = a.score <=> b.score; comparison != 0) {
            return comparison;
        }
        return a.positiveTeamsHarmonicMean <=> b.positiveTeamsHarmonicMean;
    }
    friend bool operator==(const GenomeScore &a, const GenomeScore &b) = default;
};

/**
 * @brief Responsible for main Gruepr functionality: UI, scoring genomes and running the optimization,
 * additions to the generic genetic algorithm
 */
class gruepr : public QMainWindow
{
    Q_OBJECT

public:
    explicit gruepr(DataOptions &dataOptions, QList<StudentRecord> &students, QProgressDialog *progressDialog = nullptr);
    ~gruepr() override;
    gruepr(const gruepr&) = delete;
    gruepr operator= (const gruepr&) = delete;
    gruepr(gruepr&&) = delete;
    gruepr& operator= (gruepr&&) = delete;

    static void calcTeamScores(const QList<StudentRecord> &_students, const long long _numStudents,
                               TeamSet &_teams, const TeamingOptions *const _teamingOptions);

    QList<StudentRecord> students;
    DataOptions *dataOptions = nullptr;

    void addSavedTeamsTabs();
    QStringList getTeamTabNames() const;
    QList<QList<long long>> getTeamSetData(const QString &tabName) const;

signals:
    void closed();
    void generationComplete(float maxScore, int generation, float scoreStability);
    void sectionOptimizationFullyComplete();
    void turnOffBusyCursor();

public slots:
    void moveCriteriaCard(int draggedIndex, int targetIndex);
    void showDropIndicator(int targetIndex);
    void showBottomDropZone();
    void hideDropIndicator();
    void deleteCriteriaCard(int deletedIndex);
    void doAutoScroll(QPoint point);
    void refreshCriteriaLayout();
    void saveState();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void changeSection(int index);
    void editSectionNames();
    void editAStudent();
    void removeAStudent(const QString &name);
    void removeAStudent(const long long ID, const bool delayVisualUpdate = false);
    void addAStudent();
    void compareStudentsToRoster();
    void rebuildDuplicatesTeamsizeURMIdentityAndSectionDataAndRefreshStudentTable();
    void changeIdealTeamSize();
    void chooseTeamSizes(int index);
    void startOptimization();
    void updateOptimizationProgress(float maxScore, int generation, float scoreStability);
    void optimizationComplete();
    void dataDisplayTabClose(int closingTabIndex);
    void editDataDisplayTabName(int tabIndex);

private:
        // setup
    Ui::gruepr *ui;
    void loadUI(QProgressDialog *progressDialog);
    TeamingOptions *teamingOptions = nullptr;
    int numTeams = 1;
    inline void setTeamSizes(const QList<int> &teamSizes);
    inline void setTeamSizes(const int singleSize);
    inline QString writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB);

        // reading survey data
    long long numActiveStudents = MAX_STUDENTS;
    inline StudentRecord* findStudentFromID(const long long ID);
    void refreshStudentDisplay(QProgressDialog *progressDialog = nullptr, int progressStart = 0, int progressEnd = 0);
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column

        // team set optimization
    QPushButton *letsDoItButton = nullptr;
    QList<int> studentIndexes;                                    // the indexes of students to be placed on teams
    QList<int> optimizeTeams(const QList<int> studentIndexes);    // return value is a single permutation-of-indexes
    QFuture< QList<int> > future;                                 // needed so that optimization can happen in a separate thread
    QFutureWatcher< QList<int> > futureWatcher;                   // used for signaling of optimization completion
    ScoreLineGraph *progressChart = nullptr;
    ProgressDialog *progressWindow = nullptr;
    static GenomeScore getGenomeScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                      const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, float _teamScores[],
                                      QList<QList<float> > &_criteriaScores, QList<float> &_penaltyPoints);
    static GenomeScore aggregateTeamScores(const float _teamScores[], const int _teamSizes[], const int _numTeams);

    // Scores an arbitrary subset of teams in isolation from the rest of the genome. Used
    // to cheaply evaluate a candidate swap's effect on just the handful of teams it
    // touches, without rescoring the whole genome.
    static GenomeScore getSubGenomeScore(const StudentRecord *const _students, const int _teammates[], const int _teamStartPositions[],
                                         const std::vector<int> &_teamIndices, const TeamingOptions *const _teamingOptions,
                                         const DataOptions *const _dataOptions, QList<float> &_subTeamScores);
    // Full-genome rescore, needed whenever a move is being kept (some criteria, e.g.
    // AssignmentPreferenceCriterion, can't be accurately evaluated from just a subset of teams).
    GenomeScore rescoreGenome(const int teammates[], const int teamSizes[], QList<float> &teamScores);

    // Select which genomes will receive the repair/exhaustive-search process below
    static constexpr int NUM_GENOMES_TO_REPAIR = 500;
    static std::vector<int> chooseIndexesToRepair(const int populationSize, const int numSamples);

    // Given exactly two teams, exhaustively tries every position swap between them, choosing the
    // acceptance rule from the given teams' own current scores: if either is negative, accept a swap
    // that reduces the negative-team count among just these teams; if both are positive, accept via
    // weak-Pareto (neither drops, at least one strictly rises). Stops and returns true at the first
    // acceptance, leaving teammates improved and reporting the accepted positions in
    // acceptedPosA/acceptedPosB so the caller can undo the move if a full-genome rescore later reveals
    // it was actually a net regression; returns false if nothing found (teammates unchanged -- every
    // rejected trial is reverted).
    bool attempt2TeamSwap(int teammates[], const int teamStartPositions[], const int teamA, const int teamB,
                          int &acceptedPosA, int &acceptedPosB);

    // Every generation, a log-spaced sample of genomes (denser near rank 0) gets one of these two:
    // non-elites get repairBrokenGene() (stop at the first improving 2-team swap targeting their current
    // worst team). Elites get exhaustiveSearch() (up to two full passes of 2-team swaps, every team
    // as anchor against every other team via attempt2TeamSwap()).
    GenomeScore repairBrokenGene(int teammates[], const int teamSizes[], const int teamStartPositions[],
                                const int worstTeam, const GenomeScore &currentScore);
    GenomeScore exhaustiveSearch(int teammates[], const int teamSizes[], const int teamStartPositions[]);

    // Once the main optimization loop stops, spend up to this many more milliseconds on just the single
    // best genome: repeated full passes of 2-team swaps (attempt2TeamSwap(), every team as anchor against
    // every other team), until the deadline runs out or a pass finds nothing more.
    static constexpr int FINAL_LOCAL_SEARCH_TIME = 10000;
    GenomeScore finalGenomeHillClimb(int teammates[], const int teamSizes[], const int teamStartPositions[],
                                     const std::chrono::steady_clock::time_point deadline);

    float teamSetScore = 0;
    int finalGeneration = 1;
    QMutex optimizationStoppedmutex;
    bool multipleSectionsInProgress = false;
    // Set only by a genuine user stop request (button click, or window-close/Escape) -- means abort
    // everything immediately (both the main generation loop and the finalizing search) and build
    // results from whatever the current best genome is. Never set internally by the phase transition
    // from normal GA to finalizing, which is decided purely by generation/stability/checkbox state.
    bool optimizationStopped = false;
    // Thread-safe mirror of progressDialog's "Continue optimizing until I press end" checkbox, refreshed
    // once per generation by updateOptimizationProgress() (which runs on the main thread), since
    // optimizeTeams()'s own loop (worker thread) can't safely call a QCheckBox method directly.
    bool continueIndefinitely = false;
    inline const static float MINIMUM_PENALTY = 1.01f;            // ensures that even the smallest penalty to a team makes that team have negative score

        // reporting results
    TeamSet teams;
    QList<int> bestTeamSet;
    TeamSet finalTeams;


        //Criteria Cards
    GroupingCriteriaCard *teamsizeCriteriaCard = nullptr;
        StyledComboBox *teamSizeBox = nullptr;
        QSpinBox *idealTeamSizeBox = nullptr;
    GroupingCriteriaCard *sectionCriteriaCard = nullptr;
        StyledComboBox *sectionSelectionBox = nullptr;
    GroupingCriteriaCard *genderIdentityCriteriaCard = nullptr;
    GroupingCriteriaCard *urmIdentityCard = nullptr;
    GroupingCriteriaCard *assignmentPreferenceCriteriaCard = nullptr;
    GroupingCriteriaCard *meetingScheduleCriteriaCard = nullptr;
    QList<Criterion::CriteriaType> teammateRulesExistence;
    QList<GroupingCriteriaCard*> initializedAttributeCriteriaCards;
    QList<AttributeWidget*> attributeWidgets;
        QList<int> addedAttributeNumbersList;
    QList<GroupingCriteriaCard*> criteriaCardsList;
    QPushButton *addNewCriteriaCardButton = nullptr;
    QMenu *addNewCriteriaMenu = nullptr;
    QAction *genderMenuAction = nullptr;
    QAction *urmMenuAction = nullptr;
    QAction *assignmentPreferenceMenuAction = nullptr;
    QAction *scheduleMenuAction = nullptr;
    QAction *groupTogetherMenuAction = nullptr;
    QAction *splitApartMenuAction = nullptr;
    QList<QAction*> attributeMenuActions;
    void addCriteriaCard(Criterion::CriteriaType criteriaType);
    void addCriteriaCard(Criterion::CriteriaType criteriaType, int attribute);
    void initializeCriteriaCardPriorities();
    void populateCriterionTypes();
    QFrame *dropIndicator = nullptr;
    QWidget *bottomDropZone = nullptr;
};

#endif // GRUEPR_H
