#ifndef GRUEPR_H
#define GRUEPR_H

#include <QMainWindow>
#include <QString>
#include <QFuture>
#include <QFutureWatcher>
#include "GA.h"
#include "customDialogs.h"
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

    void removeAStudent();

    void on_addStudentPushButton_clicked();

    void on_attributeScrollBar_valueChanged(int value);

    void on_attributeWeight_valueChanged(double arg1);

    void on_attributeHomogeneousBox_stateChanged(int arg1);

    void on_isolatedWomenCheckBox_stateChanged(int arg1);

    void on_isolatedMenCheckBox_stateChanged(int arg1);

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

    void on_saveTeamsButton_clicked();

    void on_loadSettingsButton_clicked();

    void on_saveSettingsButton_clicked();

    void on_clearSettingsButton_clicked();

    void on_adjustTeamsButton_clicked();

    void on_HelpButton_clicked();

    void on_AboutButton_clicked();

    void on_registerButton_clicked();

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
    studentRecord student[maxStudents];                 // array to hold the students' data
    int numStudents;
    int numTeams;
    void refreshStudentDisplay();
    int teamSize[maxStudents]={0};
    void setTeamSizes(int teamSizes[]);
    void setTeamSizes(int singleSize);
    bool loadSurveyData(QString fileName);              // returns false if file is invalid
    studentRecord readOneRecordFromFile(QStringList fields);
    QStringList ReadCSVLine(QString line);
    QList<int> optimizeTeams();                         // returns a single permutation-of-IDs
    QFuture<QList<int> > future;                        // needed so that optimization can happen in a separate thread
    int bestGenome[maxStudents];
    QFutureWatcher<void> futureWatcher;                 // used for signaling of optimization completion
    QMutex optimizationStoppedmutex;
    bool optimizationStopped;
    bool keepOptimizing;
    double getTeamScores(int teammates[], double teamScores[]);
    void printTeams(int teammates[], double teamScores[], QString filename);
    // four strings used to show various values in the top row of teams printout
    QString sectionName;
    QString finalGeneration;
    QString finalTeamSetScore;
};


class TimestampTableWidgetItem : public QTableWidgetItem
{
public:
    TimestampTableWidgetItem(const QString txt = "")
        :QTableWidgetItem(txt)
    {
    }

    bool operator <(const QTableWidgetItem &other) const
    {
        return QDateTime::fromString(text(), "d-MMM. h:mm AP") < QDateTime::fromString(other.text(), "d-MMM. h:mm AP");
    }
};


#endif // GRUEPR_H
