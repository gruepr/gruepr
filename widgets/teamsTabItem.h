#ifndef TEAMSTABITEM_H
#define TEAMSTABITEM_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QVBoxLayout>
#include "widgets/teamTreeWidget.h"
#include "dataOptions.h"
#include "studentRecord.h"
#include "teamingOptions.h"
#include "teamRecord.h"

class TeamsTabItem : public QWidget
{
Q_OBJECT

public:
    explicit TeamsTabItem(const TeamingOptions *const incomingTeamingOptions, const QString &incomingSectionName, const DataOptions *const incomingDataOptions,
                          TeamRecord incomingTeams[], int incomingNumTeams, StudentRecord incomingStudents[], QWidget *parent = nullptr);
    ~TeamsTabItem();

private slots:
    void teamNamesChanged(int index);
    void randomizeTeamnames();
    void swapStudents(int studentAteam, int studentAID, int studentBteam, int studentBID);
    void moveAStudent(int oldTeam, int studentID, int newTeam);
    void moveATeam(int teamA, int teamB);

public slots:
    void saveTeams();
    void printTeams();

private:
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QVector<int> getTeamNumbersInDisplayOrder();

    QVBoxLayout *teamDataLayout = nullptr;

    TeamTreeWidget *teamDataTree = nullptr;

    QLabel *dragDropExplanation = nullptr;

    QHBoxLayout *teamOptionsLayout = nullptr;
    QPushButton *expandAllButton = nullptr;
    QPushButton *collapseAllButton = nullptr;
    QFrame *vertLine = nullptr;
    QLabel *setNamesLabel = nullptr;
    QComboBox *teamnamesComboBox = nullptr;
    QCheckBox *randTeamnamesCheckBox = nullptr;

    QHBoxLayout *savePrintLayout = nullptr;
    QPushButton *saveTeamsButton = nullptr;
    QPushButton *printTeamsButton = nullptr;

    TeamingOptions *teamingOptions = nullptr;
    DataOptions *dataOptions = nullptr;
    TeamRecord *teams = nullptr;
    int numTeams = 1;
    StudentRecord *students = nullptr;
    int numStudents = 1;

    QString sectionName;
    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    void createFileContents();
    void printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);

    const QSize SAVEPRINTICONSIZE = QSize(30, 30);
    const int BIGGERFONTSIZE = 12;
    const QFont PRINTFONT = QFont("Oxygen Mono", 10, QFont::Normal);

signals:
    void connectedToPrinter();
};

#endif // TEAMSTABITEM_H
