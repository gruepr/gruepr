#ifndef TEAMSTABITEM_H
#define TEAMSTABITEM_H

#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "widgets/teamTreeWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QJsonObject>
#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QWidget>

class TeamsTabItem : public QWidget
{
Q_OBJECT

public:
    explicit TeamsTabItem(TeamingOptions &incomingTeamingOptions, const TeamSet &incomingTeamSet, QList<StudentRecord> &incomingStudents,
                          const QString &incomingTabName, QPushButton *letsDoItButton, QWidget *parent = nullptr);
    explicit TeamsTabItem(const QJsonObject &jsonTeamsTab, TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents,
                          QPushButton *letsDoItButton, QWidget *parent = nullptr);
    void init(TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents, QPushButton *letsDoItButton);    // handle the constructor work after loading data
    ~TeamsTabItem() override;
    TeamsTabItem(const TeamsTabItem&) = delete;
    TeamsTabItem operator= (const TeamsTabItem&) = delete;
    TeamsTabItem(TeamsTabItem&&) = delete;
    TeamsTabItem& operator= (TeamsTabItem&&) = delete;

    QJsonObject toJson() const;

    QString tabName;

signals:
    void connectedToPrinter();
    void saveState();

private slots:
    void teamNamesChanged(int index);
    void randomizeTeamnames();
    void makeNewSetWithAllNewTeammates();
    void swapStudents(const QList<int> &arguments); // arguments = int studentAteam, int studentAID, int studentBteam, int studentBID
    void moveAStudent(const QList<int> &arguments); // arguments = int oldTeam, int studentID, int newTeam
    void moveATeam(const QList<int> &arguments);    // arguments = int teamA, int teamB
    void undoRedoDragDrop();
    void saveTeams();
    void printTeams();
    void postTeamsToCanvas();

private:
    TeamTreeWidget *teamDataTree = nullptr;
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QList<int> getTeamNumbersInDisplayOrder();
    inline StudentRecord* findStudentFromID(const long long ID);

    TeamingOptions *teamingOptions = nullptr;
    TeamSet teams;
    QList<StudentRecord> students;
    int numStudents = 1;

    struct UndoRedoItem{void (TeamsTabItem::*action)(const QList<int> &arguments);
                        QList<int> arguments;
                        QString ToolTip;};
    QList<UndoRedoItem> undoItems;
    QList<UndoRedoItem> redoItems;
    QPushButton *undoButton = nullptr;
    QPushButton *redoButton = nullptr;

    static const QStringList teamnameCategories;
    static const QStringList teamnameLists;
    QComboBox *teamnamesComboBox = nullptr;
    QCheckBox *randTeamnamesCheckBox = nullptr;

    //pointers to items back out in gruepr, so they can be used for "create new teams with all new teammates"
    TeamingOptions *externalTeamingOptions = nullptr;
    QList<StudentRecord> *externalStudents = nullptr;
    QPushButton *externalDoItButton = nullptr;

    enum files{student = 0, instructor = 1, spreadsheet = 2};
    QStringList createFileContents();   // {studentsFileContents, instructorsFileContents, spreadsheetFileContents}
    void printFiles(const QStringList &fileContents, bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);

    inline static const QSize SAVEPRINTICONSIZE = QSize(STD_ICON_SIZE, STD_ICON_SIZE);
    inline static const int BIGGERFONTSIZE = 12;
    inline static const QFont PRINTFONT = QFont("Oxygen Mono", 10, QFont::Normal);
};

#endif // TEAMSTABITEM_H
