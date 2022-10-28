#ifndef TEAMSTABITEM_H
#define TEAMSTABITEM_H

#include "canvashandler.h"
#include "dataOptions.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "widgets/teamTreeWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class TeamsTabItem : public QWidget
{
Q_OBJECT

public:
    explicit TeamsTabItem(TeamingOptions *const incomingTeamingOptions, const DataOptions *const incomingDataOptions, CanvasHandler *const incomingCanvas,
                          TeamRecord *const incomingTeams, int incomingNumTeams, StudentRecord incomingStudents[],
                          QString incomingTabName, QWidget *parent = nullptr);
    ~TeamsTabItem();

    QString tabName;

public slots:
    void saveTeams();
    void postTeamsToCanvas();
    void printTeams();

private slots:
    void teamNamesChanged(int index);
    void randomizeTeamnames();
    void makePrevented();
    void swapStudents(const QVector<int> &arguments); // arguments = int studentAteam, int studentAID, int studentBteam, int studentBID
    void moveAStudent(const QVector<int> &arguments); // arguments = int oldTeam, int studentID, int newTeam
    void moveATeam(const QVector<int> &arguments);    // arguments = int teamA, int teamB
    void undoRedoDragDrop();

private:
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QVector<int> getTeamNumbersInDisplayOrder();

    QVBoxLayout *teamDataLayout = nullptr;

    QLabel *fileAndSectionLabel = nullptr;

    TeamTreeWidget *teamDataTree = nullptr;

    QHBoxLayout *dragDropLayout = nullptr;
    QLabel *dragDropExplanation = nullptr;
    struct UndoRedoItem{void (TeamsTabItem::*action)(const QVector<int> &arguments); QVector<int> arguments; QString ToolTip;};
    QList<UndoRedoItem> undoItems;
    QList<UndoRedoItem> redoItems;
    QPushButton *undoButton = nullptr;
    QPushButton *redoButton = nullptr;

    QHBoxLayout *teamOptionsLayout = nullptr;
    QPushButton *expandAllButton = nullptr;
    QPushButton *collapseAllButton = nullptr;
    QFrame *vertLine = nullptr;
    QLabel *setNamesLabel = nullptr;
    QComboBox *teamnamesComboBox = nullptr;
    QStringList teamnameCategories;
    QStringList teamnameLists;
    enum TeamNameType{numeric, repeated, repeated_spaced, sequeled, random_sequeled};    // see gruepr_globals.h for how teamname lists are signified
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

    QString instructorsFileContents;
    QString studentsFileContents;
    QString spreadsheetFileContents;
    void createFileContents();
    void printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);
    CanvasHandler *canvas = nullptr;

    const QSize SAVEPRINTICONSIZE = QSize(30, 30);
    const int BIGGERFONTSIZE = 12;
    const QFont PRINTFONT = QFont("Oxygen Mono", 10, QFont::Normal);

signals:
    void connectedToPrinter();
};

#endif // TEAMSTABITEM_H
