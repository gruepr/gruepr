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
    explicit TeamsTabItem(TeamingOptions *const incomingTeamingOptions, const QString &incomingSectionName, const DataOptions *const incomingDataOptions,
                          TeamRecord incomingTeams[], int incomingNumTeams, StudentRecord incomingStudents[], QWidget *parent = nullptr);
    ~TeamsTabItem();

public slots:
    void saveTeams();
    void printTeams();

private slots:
    void teamNamesChanged(int index);
    void randomizeTeamnames();
    void makePrevented();
    void swapStudents(int studentAteam, int studentAID, int studentBteam, int studentBID);
    void moveAStudent(int oldTeam, int studentID, int newTeam);
    void moveATeam(int teamA, int teamB);

private:
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QVector<int> getTeamNumbersInDisplayOrder();

    QVBoxLayout *teamDataLayout = nullptr;

    QLabel *fileAndSectionLabel = nullptr;

    TeamTreeWidget *teamDataTree = nullptr;

    QLabel *dragDropExplanation = nullptr;

    QHBoxLayout *teamOptionsLayout = nullptr;
    QPushButton *expandAllButton = nullptr;
    QPushButton *collapseAllButton = nullptr;
    QFrame *vertLine = nullptr;
    QLabel *setNamesLabel = nullptr;
    QComboBox *teamnamesComboBox = nullptr;
    QStringList teamnameCategories;
    QStringList teamnameLists;
    enum TeamNameType{numeric, repeated, repeated_spaced, sequeled, random_sequeled};    // see gruepr_structs_and_consts.h for how teamname lists are signified
    QVector<TeamNameType> teamnameTypes;
    QCheckBox *randTeamnamesCheckBox = nullptr;
    QPushButton *sendToPreventedTeammates = nullptr;

    QHBoxLayout *savePrintLayout = nullptr;
    QPushButton *saveTeamsButton = nullptr;
    QPushButton *printTeamsButton = nullptr;

    TeamingOptions *teamingOptions = nullptr;
    bool *addedPreventedTeammates = nullptr;
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
