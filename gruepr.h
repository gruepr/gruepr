#ifndef GRUEPR_H
#define GRUEPR_H

#include "csvfile.h"
#include "dataOptions.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "criteria/attributeCriterion.h"
#include "dialogs/progressDialog.h"
#include "widgets/boxwhiskerplot.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QComboBox>
#include <QFuture>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QPrinter>
#include <QSpinBox>


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
    QStringList getTeamTabNames() const;

    QList<StudentRecord> students;
    DataOptions *dataOptions = nullptr;

    inline static const int MAINWINDOWPADDING = 20;            // pixels of padding in buttons and above status message
    inline static const int MAINWINDOWFONT = 8;                // increase in font size for main window text
    inline static const int MAINWINDOWBUTTONFONT = 4;          // increase in font size for main window button text

signals:
    void closed();
    void generationComplete(const float *const allScores, const int *const orderedIndex,
                            const int generation, const float scoreStability, const bool unpenalizedGenomePresent);
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
    void rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    void changeIdealTeamSize();
    void chooseTeamSizes(int index);
    void startOptimization();
    void updateOptimizationProgress(const float *const allScores, const int *const orderedIndex,
                                    const int generation, const float scoreStability, const bool unpenalizedGenomePresent);
    void optimizationComplete();
    void dataDisplayTabClose(int closingTabIndex);
    void editDataDisplayTabName(int tabIndex);

private:
        // setup
    Ui::gruepr *ui;
    void loadUI();
    TeamingOptions *teamingOptions = nullptr;
    int numTeams = 1;
    inline void setTeamSizes(const QList<int> &teamSizes);
    inline void setTeamSizes(const int singleSize);
    inline QString writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB);

        // reading survey data
    long long numActiveStudents = MAX_STUDENTS;
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
                                const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, float _teamScores[],
                                std::vector<std::vector<float>> &_criteriaScores, std::vector<int> &_penaltyPoints);

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
    GroupingCriteriaCard *urmIdentityCard = nullptr;
    GroupingCriteriaCard *gradeBalanceCriteriaCard = nullptr;
    QList<Criterion::CriteriaType> teammateRulesExistence;
    QList<GroupingCriteriaCard*> initializedAttributeCriteriaCards;
    QList<AttributeWidget*> attributeWidgets;
        QList<int> addedAttributeNumbersList;
    QList<GroupingCriteriaCard*> criteriaCardsList;
    QPushButton *addNewCriteriaCardButton = nullptr;
    QMenu *addNewCriteriaMenu = nullptr;
    QAction *genderMenuAction = nullptr;
    QAction *urmMenuAction = nullptr;
    QAction *gradeMenuAction = nullptr;
    QAction *scheduleMenuAction = nullptr;
    QAction *requiredTeammatesMenuAction = nullptr;
    QAction *preventedTeammatesMenuAction = nullptr;
    QAction *requestedTeammatesMenuAction = nullptr;
    QList<QAction*> attributeMenuActions;
    void addCriteriaCard(Criterion::CriteriaType criteriaType);
    void addCriteriaCard(Criterion::CriteriaType criteriaType, int attribute);
    void initializeCriteriaCardPriorities();
    QFrame *m_dropIndicator = nullptr;
    QWidget *m_bottomDropZone = nullptr;
};

#endif // GRUEPR_H
