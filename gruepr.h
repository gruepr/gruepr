#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/progressDialog.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "widgets/attributeWidget.h"
#include "widgets/boxwhiskerplot.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QPrinter>


namespace Ui {class gruepr;}


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

    static void calcTeamScores(const StudentRecord *const _student, const int _numStudents, TeamRecord *const _teams, const int _numTeams,
                               const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions);

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
    void editAStudent();
    void removeAStudent(const QString &name, bool delayVisualUpdate);
    void removeAStudent(int index, bool delayVisualUpdate);
    void addAStudent();
    void compareStudentsToRoster();
    void rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    void simpleUIItemUpdate(QObject *sender = nullptr);
    void selectURMResponses();
    void responsesRulesButton_clicked();
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

private:
        // setup
    Ui::gruepr *ui;
    void loadDefaultSettings();
    void loadUI();
    DataOptions *dataOptions = nullptr;
    TeamingOptions *teamingOptions = nullptr;
    int numTeams = 1;
    inline void setTeamSizes(const int teamSizes[]);
    inline void setTeamSizes(const int singleSize);
    inline QString writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB);

        // reading survey data
    int numActiveStudents = MAX_STUDENTS;
    QList<StudentRecord> students;
    bool loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails);   // returns false if file is invalid; checks survey names and emails against roster
    void refreshStudentDisplay();
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column
    QList<QPushButton *> attributeSelectorButtons;
    QList<AttributeWidget *> attributeWidgets;

        // team set optimization
    int *studentIndexes = nullptr;                                // array of the indexes of students to be placed on teams
    QList<int> optimizeTeams(const int *const studentIndexes);    // return value is a single permutation-of-indexes
    QFuture< QList<int> > future;                                 // needed so that optimization can happen in a separate thread
    QFutureWatcher< QList<int> > futureWatcher;                   // used for signaling of optimization completion
    BoxWhiskerPlot *progressChart = nullptr;
    progressDialog *progressWindow = nullptr;
    static float getGenomeScore(const StudentRecord *const _student, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions,
                                float _teamScores[], float **_attributeScore, float *_schedScore, bool **_availabilityChart, int *_penaltyPoints);
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
};

#endif // GRUEPR_H
