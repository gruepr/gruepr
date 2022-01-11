#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include <QFuture>
#include <QFutureWatcher>
#include <QPrinter>
#ifdef Q_OS_WIN32
  #include <QWinTaskbarButton>
  #include <QWinTaskbarProgress>
#endif
#include "gruepr_consts.h"
#include "boxwhiskerplot.h"
#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/progressDialog.h"
#include "studentRecord.h"
#include "teamingOptions.h"
#include "teamRecord.h"
#include "widgets/attributeTabItem.h"


namespace Ui {class gruepr;}


class gruepr : public QMainWindow
{
    Q_OBJECT

public:
    explicit gruepr(QWidget *parent = nullptr);
    ~gruepr();

    static void getTeamScores(const StudentRecord _student[], const int _numStudents, TeamRecord _teams[], const int _numTeams,
                              const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_loadSurveyFileButton_clicked();
    void loadStudentRoster();
    void on_sectionSelectionBox_currentIndexChanged(const QString &desiredSection);
    void editAStudent();
    void removeAStudent(int index, const QString &name = "", bool delayVisualUpdate = false);
    void on_addStudentPushButton_clicked();
    void rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    void on_saveSurveyFilePushButton_clicked();
    void on_isolatedWomenCheckBox_stateChanged(int arg1);
    void on_isolatedMenCheckBox_stateChanged(int arg1);
    void on_isolatedNonbinaryCheckBox_stateChanged(int arg1);
    void on_mixedGenderCheckBox_stateChanged(int arg1);
    void on_isolatedURMCheckBox_stateChanged(int arg1);
    void on_URMResponsesButton_clicked();
    void requiredResponsesButton_clicked();
    void incompatibleResponsesButton_clicked();
    void on_minMeetingTimes_valueChanged(int arg1);
    void on_desiredMeetingTimes_valueChanged(int arg1);
    void on_meetingLength_currentIndexChanged(int index);
    void on_scheduleWeight_valueChanged(double arg1);
    void on_idealTeamSizeBox_valueChanged(int arg1);
    void on_teamSizeBox_currentIndexChanged(int index);
    void on_requiredTeammatesButton_clicked();
    void on_preventedTeammatesButton_clicked();
    void on_requestedTeammatesButton_clicked();
    void on_requestedTeammateNumberBox_valueChanged(int arg1);
    void on_letsDoItButton_clicked();
    void updateOptimizationProgress(const QVector<float> &allScores, const int *const orderedIndex, const int generation, const float scoreStability);
    void optimizationComplete();
    void dataDisplayTabSwitch(int newTabIndex);
    void dataDisplayTabClose(int closingTabIndex);
    void editDataDisplayTabName(int tabIndex);
    void loadOptionsFile();
    void saveOptionsFile();
    void settingsWindow();
    void helpWindow();
    void aboutWindow();

signals:
    void generationComplete(const QVector<float> &allScores, const int *orderedIndex, int generation, float scoreStability);
    void turnOffBusyCursor();

private:
        // setup
    Ui::gruepr *ui;
    QLabel *statusBarLabel;
    void loadDefaultSettings();
    void resetUI();
    void loadUI();
    DataOptions *dataOptions = nullptr;
    TeamingOptions *teamingOptions = nullptr;
    int numTeams = 1;
    void setTeamSizes(const int teamSizes[]);
    void setTeamSizes(const int singleSize);

        // reading survey data file
    int numStudents = MAX_STUDENTS;
    StudentRecord *student = nullptr;                   // array to hold the students' data
    bool loadSurveyData(CsvFile &surveyFile);           // returns false if file is invalid
    bool loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails);           // returns false if file is invalid; checks survey names and emails against roster
    void refreshStudentDisplay();
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column
    attributeTabItem *attributeTab = nullptr;
    QString sectionName;
    const QColor HIGHLIGHTYELLOW = QColor(0xff, 0xff, 0x3b);

        // team set optimization
    int *studentIndexes = nullptr;                                  // array of the indexes of students to be placed on teams
    QVector<int> optimizeTeams(const int *const studentIndexes);    // returns a single permutation-of-indexes
    QFuture< QVector<int> > future;                                 // needed so that optimization can happen in a separate thread
    QFutureWatcher<void> futureWatcher;                             // used for signaling of optimization completion
    BoxWhiskerPlot *progressChart = nullptr;
    progressDialog *progressWindow = nullptr;
#ifdef Q_OS_WIN32
    QWinTaskbarButton *taskbarButton = nullptr;
    QWinTaskbarProgress *taskbarProgress = nullptr;
#endif
    static float getGenomeScore(const StudentRecord _student[], const int _teammates[], const int _numTeams, const int _teamSizes[],
                                const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions,
                                float _teamScores[], float **_attributeScore, float *_schedScore, bool **_availabilityChart, int *_penaltyPoints);
    float teamSetScore = 0;
    int finalGeneration = 1;
    QMutex optimizationStoppedmutex;
    bool optimizationStopped = false;
    bool keepOptimizing = false;

        // reporting results
    TeamRecord *teams = nullptr;
};

#endif // GRUEPR_H
