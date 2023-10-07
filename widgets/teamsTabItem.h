#ifndef TEAMSTABITEM_H
#define TEAMSTABITEM_H

#include "canvashandler.h"
#include "dataOptions.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "widgets/labelWithInstantTooltip.h"
#include "widgets/teamTreeWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class TeamsTabItem : public QWidget
{
Q_OBJECT

public:
    explicit TeamsTabItem(TeamingOptions &incomingTeamingOptions, const DataOptions &incomingDataOptions,
                          const QList<TeamRecord> &incomingTeams, QList<StudentRecord> &incomingStudents, const QString &incomingTabName,
                          QPushButton *letsDoItButton, QWidget *parent = nullptr);
    explicit TeamsTabItem(const QJsonObject &jsonTeamsTab, TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents, QPushButton *letsDoItButton, QWidget *parent = nullptr);
    void init(TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents, QPushButton *letsDoItButton);    // handle the constructor work after loading data
    ~TeamsTabItem();
    TeamsTabItem(const TeamsTabItem&) = delete;
    TeamsTabItem operator= (const TeamsTabItem&) = delete;
    TeamsTabItem(TeamsTabItem&&) = delete;
    TeamsTabItem& operator= (TeamsTabItem&&) = delete;

    QJsonObject toJson() const;

    QString tabName;

public slots:
    void saveTeams();
    void postTeamsToCanvas();
    void printTeams();

private slots:
    void teamNamesChanged(int index);
    void randomizeTeamnames();
    void makeNewSetWithAllNewTeammates();
    void swapStudents(const QList<int> &arguments); // arguments = int studentAteam, int studentAID, int studentBteam, int studentBID
    void moveAStudent(const QList<int> &arguments); // arguments = int oldTeam, int studentID, int newTeam
    void moveATeam(const QList<int> &arguments);    // arguments = int teamA, int teamB
    void undoRedoDragDrop();

private:
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QList<int> getTeamNumbersInDisplayOrder();

    QVBoxLayout *teamDataLayout = nullptr;

    TeamTreeWidget *teamDataTree = nullptr;

    QHBoxLayout *rowsLayout = nullptr;
    LabelWithInstantTooltip *dragDropExplanation = nullptr;
    struct UndoRedoItem{void (TeamsTabItem::*action)(const QList<int> &arguments); QList<int> arguments; QString ToolTip;};
    QList<UndoRedoItem> undoItems;
    QList<UndoRedoItem> redoItems;
    QPushButton *undoButton = nullptr;
    QPushButton *redoButton = nullptr;
    QPushButton *expandAllButton = nullptr;
    QPushButton *collapseAllButton = nullptr;

    QFrame *horLine = nullptr;

    QHBoxLayout *teamOptionsLayout = nullptr;
    QComboBox *teamnamesComboBox = nullptr;
    QStringList teamnameCategories;
    QStringList teamnameLists;
    enum TeamNameType{numeric, repeated, repeated_spaced, sequeled, random_sequeled};    // see gruepr_globals.h for how teamname lists are signified
    QList<TeamNameType> teamnameTypes;
    QCheckBox *randTeamnamesCheckBox = nullptr;
    QPushButton *sendToPreventedTeammates = nullptr;

    QHBoxLayout *savePrintLayout = nullptr;
    QPushButton *saveTeamsButton = nullptr;
    QPushButton *printTeamsButton = nullptr;
    QPushButton *postTeamsButton = nullptr;

    TeamingOptions *teamingOptions = nullptr;
    DataOptions *dataOptions = nullptr;
    QList<TeamRecord> teams;
    QList<StudentRecord> students;
    int numStudents = 1;

    //pointers to items back out in gruepr, so they can be used for "create new teams with all new teammates"
    TeamingOptions *externalTeamingOptions = nullptr;
    QList<StudentRecord> *externalStudents = nullptr;
    QPushButton *externalDoItButton = nullptr;

    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    void createFileContents();
    void printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);
    CanvasHandler *canvas = nullptr;

    const QSize SAVEPRINTICONSIZE = QSize(STD_ICON_SIZE, STD_ICON_SIZE);
    const int BIGGERFONTSIZE = 12;
    const QFont PRINTFONT = QFont("Oxygen Mono", 10, QFont::Normal);

signals:
    void connectedToPrinter();
};

#endif // TEAMSTABITEM_H
