#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include <QFuture>
#include <QFutureWatcher>
#include <QTreeWidget>
#include "customDialogs.h"
#include "gruepr_structs_and_consts.h"


namespace Ui {
class gruepr;
}


class TimestampTableWidgetItem : public QTableWidgetItem
{
public:
    TimestampTableWidgetItem(const QString txt = "");
    bool operator <(const QTableWidgetItem &other) const;
};


class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseItem(QTreeWidgetItem *item);
    void expandItem(QTreeWidgetItem *item);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

signals:
    void swapPlaces(int studentAID, int studentBID);

private:
    QTreeWidgetItem* draggedItem;
    QTreeWidgetItem* droppedItem;
};


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

    void on_studentTable_cellEntered(int row, int /*unused column value*/);

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

    void updateOptimizationProgress(double score, int generation, double scoreStability);

    void askWhetherToContinueOptimizing(int generation);

    void optimizationComplete();

    void on_expandAllButton_clicked();

    void on_collapseAllButton_clicked();

    void on_teamNamesComboBox_currentIndexChanged(int index);

    void on_saveTeamsButton_clicked();

    void on_printTeamsButton_clicked();

    void on_loadSettingsButton_clicked();

    void on_saveSettingsButton_clicked();

    void on_clearSettingsButton_clicked();

    void swapTeammates(int studentAID, int studentBID);

    void on_HelpButton_clicked();

    void on_AboutButton_clicked();

    void on_registerButton_clicked();

    void on_mixedGenderCheckBox_stateChanged(int arg1);

signals:
    void generationComplete(double score, int generation, double scoreStability);

    void optimizationMightBeComplete(int generation);

    void haveOurKeepOptimizingValue();

    void turnOffBusyCursor();

private:
    Ui::gruepr *ui;
    QString registeredUser;
    DataOptions dataOptions;
    TeamingOptions teamingOptions;
    studentRecord *student;                             // array to hold the students' data
    int numStudents = maxStudents;
    int numTeams;
    void refreshStudentDisplay();
    int teamSize[maxStudents]={0};
    void setTeamSizes(int teamSizes[]);
    void setTeamSizes(int singleSize);
    bool loadSurveyData(QString fileName);              // returns false if file is invalid
    studentRecord readOneRecordFromFile(QStringList fields);
    QStringList ReadCSVLine(QString line);
    int *studentIDs;                                    // array of the IDs of students to be placed on teams
    QList<int> optimizeTeams(int *studentIDs);          // returns a single permutation-of-IDs
    QFuture<QList<int> > future;                        // needed so that optimization can happen in a separate thread
    int bestGenome[maxStudents];
    QFutureWatcher<void> futureWatcher;                 // used for signaling of optimization completion
    QMutex optimizationStoppedmutex;
    bool optimizationStopped;
    bool keepOptimizing;
    double getTeamScores(int teammates[], double teamScores[]);
    void refreshTeamInfo();
    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    QString sectionName;
    int finalGeneration;
    double teamSetScore;
    TeamTreeWidget *teamDataTree;
    QStringList teamNames;
};

#endif // GRUEPR_H