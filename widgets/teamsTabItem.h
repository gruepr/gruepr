#ifndef TEAMSTABITEM_H
#define TEAMSTABITEM_H

#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include "dialogs/whichFilesDialog.h"
#include "widgets/teamTreeWidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QJsonObject>
#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class TeamsTabItem : public QWidget
{
Q_OBJECT

public:
    explicit TeamsTabItem(TeamingOptions &incomingTeamingOptions, const TeamSet &incomingTeamSet, QList<StudentRecord> &incomingStudents,
                          const QStringList &incomingSectionNames, const QString &incomingTabName, QPushButton *letsDoItButton, QWidget *parent = nullptr);
    explicit TeamsTabItem(const QJsonObject &jsonTeamsTab, TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents,
                          const QStringList &incomingSectionNames, QPushButton *letsDoItButton, QWidget *parent = nullptr);
    // handle the rest of the constructor work after loading data
    enum class TabType{newTab, fromJSON};
    void init(TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents, QPushButton *letsDoItButton, TabType tabType);
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
//FROMDEV    QWidget* createScoreLegend();
    void changeTeamNames(int index);
    void toggleSectionsInTeamNames(bool addSectionNames);
    void randomizeTeamnames(bool random);
    void updateTeamNamesInTableAndTooltips();

    void swapStudents(const QList<int> &arguments); // arguments = int studentAteam, int studentAID, int studentBteam, int studentBID
    void moveAStudent(const QList<int> &arguments); // arguments = int oldTeam, int studentID, int newTeam
    void moveATeam(const QList<int> &arguments);    // arguments = int teamA, int teamB
    void undoRedoDragDrop();

    void makeNewSetWithAllNewTeammates();

    void saveTeams();
    void printTeams();
    void postTeamsToCanvas();
//FROMDEV    void refreshSummaryTable(TeamingOptions teamingOptions);

private:
    TeamTreeWidget *teamDataTree = nullptr;
//FROMDEV    QTableWidget *summaryTable = nullptr;
    void refreshTeamDisplay();
    void refreshDisplayOrder();
    QList<int> getTeamNumbersInDisplayOrder() const;
    inline StudentRecord* findStudentFromID(const long long ID);

    TeamingOptions *teamingOptions = nullptr;
    QStringList sectionNames;
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
    QCheckBox *addSectionToTeamnamesCheckBox = nullptr;
    bool randomizedTeamNames = false;
    bool sectionsInTeamNames = false;

    //pointers to items back out in gruepr, so they can be used for "create new teams with all new teammates"
    TeamingOptions *externalTeamingOptions = nullptr;
    QList<StudentRecord> *externalStudents = nullptr;
    QPushButton *externalDoItButton = nullptr;

    enum files{studentFile = 0, instructorFile = 1, spreadsheetFile = 2, customFile = 3};
    inline static const int NUMEXPORTFILES = 4; // must line up with number of values in line above
    QStringList createStdFileContents();   // {studentsFileContents, instructorsFileContents, spreadsheetFileContents, ""}
    QString createCustomFileContents(WhichFilesDialog::CustomFileOptions customFileOptions);
    enum class PrintType{printer, printToPDF};
    void printFiles(const QStringList &fileContents, WhichFilesDialog::FileType filetype, PrintType printType);
    QPrinter *setupPrinter();
    void printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer);

    inline static const QSize SAVEPRINTICONSIZE = QSize(STD_ICON_SIZE, STD_ICON_SIZE);
    inline static const int FILEPREVIEWLENGTH = 1000;   // number of characters in each file preview, shown as tooltips in WhichFilesDialog
    inline static const int BIGGERFONTSIZE = 12;
    inline static const QFont PRINTFONT = QFont("Oxygen Mono", 10, QFont::Normal);
};

#endif // TEAMSTABITEM_H
