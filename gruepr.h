#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include <QFuture>
#include <QFutureWatcher>
#include <QPrinter>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#include "customDialogs.h"
#include "customWidgets.h"
#include "gruepr_structs_and_consts.h"


namespace Ui {
class gruepr;
}


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

    void on_sectionSelectionBox_currentIndexChanged(const QString &arg1);

    void on_studentTable_cellEntered(int row, int column);

    void removeAStudent();

    void on_addStudentPushButton_clicked();

    void on_attributeScrollBar_valueChanged(int value);

    void on_attributeWeight_valueChanged(double arg1);

    void on_attributeHomogeneousBox_stateChanged(int arg1);

    void on_isolatedWomenCheckBox_stateChanged(int arg1);

    void on_isolatedMenCheckBox_stateChanged(int arg1);

    void on_isolatedURMCheckBox_stateChanged(int arg1);

    void on_minMeetingTimes_valueChanged(int arg1);

    void on_desiredMeetingTimes_valueChanged(int arg1);

    void on_meetingLength_currentIndexChanged(int index);

    void on_scheduleWeight_valueChanged(double arg1);

    void on_idealTeamSizeBox_valueChanged(int arg1);

    void on_teamSizeBox_currentIndexChanged(int index);

    void on_requiredTeammatesButton_clicked();

    void on_preventedTeammatesButton_clicked();

    void on_letsDoItButton_clicked();

    void on_cancelOptimizationButton_clicked();

    void updateOptimizationProgress(float score, int generation, float scoreStability);

    void askWhetherToContinueOptimizing(int generation);

    void optimizationComplete();

    void on_expandAllButton_clicked();

    void on_collapseAllButton_clicked();

    void on_sortTeamsButton_clicked();

    void on_teamNamesComboBox_currentIndexChanged(int index);

    void on_saveTeamsButton_clicked();

    void on_printTeamsButton_clicked();

    void on_loadSettingsButton_clicked();

    void on_saveSettingsButton_clicked();

    void on_clearSettingsButton_clicked();

    void swapTeammates(int studentAID, int studentBID);

    void swapTeams(int teamA, int teamB);

    void refreshTeamInfo();

    void on_HelpButton_clicked();

    void on_AboutButton_clicked();

    void on_registerButton_clicked();

    void on_mixedGenderCheckBox_stateChanged(int arg1);

signals:
    void generationComplete(float score, int generation, float scoreStability);

    void optimizationMightBeComplete(int generation);

    void haveOurKeepOptimizingValue();

    void turnOffBusyCursor();

    void connectedToPrinter();

private:
    Ui::gruepr *ui;
    QString registeredUser;
    DataOptions dataOptions;
    TeamingOptions teamingOptions;
    studentRecord *student = nullptr;                   // array to hold the students' data
    int numStudents = maxStudents;
    int numTeams;
    void refreshStudentDisplay();
    int teamSize[maxTeams]={0};
    void setTeamSizes(const int teamSizes[]);
    void setTeamSizes(const int singleSize);
    bool loadSurveyData(QString fileName);              // returns false if file is invalid
    studentRecord readOneRecordFromFile(QStringList fields);
    QStringList ReadCSVLine(QString line);              // read one line from a CSV file, smartly handling commas inside quotation mark-encased fields, and returning fields as QStringList
    QStringList ReadCSVLine(QString line, int minFields); // as above, but (unless line is empty) append empty fields so that always return QStringList with at least minFields entries
    int *studentIDs;                                    // array of the IDs of students to be placed on teams
    // pre-allocated arrays for calculating scores (which happens a lot during optimization)
    float realAttributeWeights[maxAttributes];
    float realScheduleWeight;
    float attributeScore[maxAttributes][maxTeams];
    float schedScore[maxTeams];
    int genderAdj[maxTeams];
    int URMAdj[maxTeams];
    int prevTeammateAdj[maxTeams];
    int reqTeammateAdj[maxTeams];
    QList<int> optimizeTeams(int *studentIDs);          // returns a single permutation-of-IDs
    QFuture<QList<int> > future;                        // needed so that optimization can happen in a separate thread
    int bestGenome[maxStudents];
    QFutureWatcher<void> futureWatcher;                 // used for signaling of optimization completion
    QWinTaskbarButton *taskbarButton = nullptr;
    QWinTaskbarProgress *taskbarProgress = nullptr;
    QMutex optimizationStoppedmutex;
    bool optimizationStopped;
    bool keepOptimizing;
    float getTeamScores(const int teammates[], float teamScores[]);
    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    QString sectionName;
    int finalGeneration;
    float teamSetScore;
    TeamTreeWidget *teamDataTree;
    QStringList teamNames;
    void printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter* setupPrinter();
    void printOneFile(QString file, QString delimiter, QFont &font, QPrinter *printer);
};

#endif // GRUEPR_H
