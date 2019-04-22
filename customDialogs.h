#ifndef CUSTOMDIALOGS
#define CUSTOMDIALOGS

// Code related to the subclassed dialog boxes used in gruepr

#include <QDialog>
#include <QGridLayout>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QCheckBox>
#include "gruepr_structs_and_consts.h"


class gatherTeammatesDialog : public QDialog
{
    Q_OBJECT

public:
    enum typeOfTeammates{required, prevented};

    gatherTeammatesDialog(const typeOfTeammates requiredOrPrevented, const studentRecord student[], int numStudentsInSystem, const QString &sectionName, QWidget *parent = nullptr);
    ~gatherTeammatesDialog();

    studentRecord *student;

private slots:
    void addOneTeammateSet();
    void clearAllTeammateSets();

private:
    typeOfTeammates requiredOrPrevented;
    QString sectionName;
    int numStudents;
    QGridLayout *theGrid;
    QTableWidget *currentListOfTeammatesTable;
    QComboBox possibleTeammates[8];
    QPushButton *loadTeammates;
    QPushButton *resetTableButton;
    QDialogButtonBox *buttonBox;
    void refreshDisplay();
};


class customTeamsizesDialog : public QDialog
{
    Q_OBJECT

public:
    customTeamsizesDialog(int numStudents, int idealTeamsize, QWidget *parent = nullptr);
    ~customTeamsizesDialog();

    int *teamsizes;
    int numTeams;

private slots:
    void refreshDisplay(int numTeamsBoxIndex);
    void teamsizeChanged(int);

private:
    int numStudents;
    QGridLayout *theGrid;
    QLabel numTeamsLabel;
    QComboBox numTeamsBox;
    QLabel *teamNumberLabel;
    QSpinBox *teamsizeBox;
    QLabel remainingStudents;
    QDialogButtonBox *buttonBox;
};


class customTeamnamesDialog : public QDialog
{
    Q_OBJECT

public:
    customTeamnamesDialog(int numTeams = 1, QStringList teamNames = {}, QWidget *parent = nullptr);
    ~customTeamnamesDialog();

    QLineEdit *teamName;

private:
    QGridLayout *theGrid;
    QLabel *teamNumberLabel;
    QDialogButtonBox *buttonBox;
};


class registerDialog : public QDialog
{
    Q_OBJECT

public:
    registerDialog(QWidget *parent = nullptr);

    QLineEdit *name;
    QLineEdit *institution;
    QLineEdit *email;

private:
    QGridLayout *theGrid;
    QLabel *explanation;
    QDialogButtonBox *buttonBox;
};


class whichFilesDialog : public QDialog
{
    Q_OBJECT

public:
    enum action{save, print};

    whichFilesDialog(const action saveOrPrint, QWidget *parent = nullptr);

    QCheckBox *studentFile;
    QCheckBox *instructorFile;
    QCheckBox *spreadsheetFile;

private:
    QGridLayout *theGrid;
    QLabel *explanation;
    QDialogButtonBox *buttonBox;
};


#endif // CUSTOMDIALOGS
