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
#include "csvfile.h"
#include "customDialogs.h"
#include "comboBoxWithElidedContents.h"
#include "categorialSpinBox.h"
#include "pushButtonWithMouseEnter.h"
#include "sortableTableWidgetItem.h"
#include "studentRecord.h"
#include "teamTreeWidget.h"
#include "boxwhiskerplot.h"
#include "gruepr_structs_and_consts.h"


namespace Ui {class gruepr;}


class gruepr : public QMainWindow
{
    Q_OBJECT

public:
    explicit gruepr(QWidget *parent = nullptr);
    ~gruepr();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_loadSurveyFileButton_clicked();
    void loadStudentRoster();
    void on_sectionSelectionBox_currentIndexChanged(const QString &desiredSection);
    void removeAStudent(const QString &name, bool delayVisualUpdate = false);
    void editAStudent();
    void on_addStudentPushButton_clicked();
    void on_saveSurveyFilePushButton_clicked();
    void on_attributeScrollBar_valueChanged(int value);
    void on_attributeWeight_valueChanged(double arg1);
    void on_attributeHomogeneousBox_stateChanged(int arg1);
    void on_incompatibleResponsesButton_clicked();
    void on_isolatedWomenCheckBox_stateChanged(int arg1);
    void on_isolatedMenCheckBox_stateChanged(int arg1);
    void on_isolatedNonbinaryCheckBox_stateChanged(int arg1);
    void on_mixedGenderCheckBox_stateChanged(int arg1);
    void on_isolatedURMCheckBox_stateChanged(int arg1);
    void on_URMResponsesButton_clicked();
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
    void on_teamNamesComboBox_activated(int index);
    void on_saveTeamsButton_clicked();
    void on_printTeamsButton_clicked();
    void loadOptionsFile();
    void saveOptionsFile();
    void swapTeammates(int studentAteam, int studentAID, int studentBteam, int studentBID);
    void moveTeammate(int studentTeam, int studentID, int newTeam);
    void reorderTeams(int teamA, int teamB);
    void reorderedTeams();
    void settingsWindow();
    void helpWindow();
    void aboutWindow();

signals:
    void generationComplete(const QVector<float> &allScores, const int *orderedIndex, int generation, float scoreStability);
    void turnOffBusyCursor();
    void connectedToPrinter();

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
    StudentRecord parseOneRecord(const QStringList &fields);
    bool loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails);           // returns false if file is invalid; loads fullnames and emails into lists
    void refreshStudentDisplay();
    int prevSortColumn = 0;                             // column sorting the student table, used when trying to sort by edit info or remove student column
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;   // order of sorting the student table, used when trying to sort by edit info or remove student column

        // team set optimization
    int *studentIDs = nullptr;                              // array of the IDs of students to be placed on teams
    QVector<int> optimizeTeams(const int *const studentIDs);  // returns a single permutation-of-IDs
    QFuture< QVector<int> > future;                         // needed so that optimization can happen in a separate thread
    QFutureWatcher<void> futureWatcher;                     // used for signaling of optimization completion
    BoxWhiskerPlot *progressChart = nullptr;
    progressDialog *progressWindow = nullptr;
#ifdef Q_OS_WIN32
    QWinTaskbarButton *taskbarButton = nullptr;
    QWinTaskbarProgress *taskbarProgress = nullptr;
#endif
    float getTeamScores(const int teammates[], float teamScores[], float **attributeScore, float *schedScore, int *penaltyPoints);
    float teamSetScore = 0;
    int finalGeneration = 1;
    QMutex optimizationStoppedmutex;
    bool optimizationStopped = false;
    bool keepOptimizing = false;

        // reporting results
    TeamInfo *teams = nullptr;
    void refreshTeamInfo(QVector<int> teamNums = {-1});
    void refreshTeamToolTips(QVector<int> teamNums = {-1});
    void refreshTeamDisplay();
    QVector<int> getTeamNumbersInDisplayOrder();
    QString sectionName;
    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    void createFileContents();
    void printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);
};

#endif // GRUEPR_H
