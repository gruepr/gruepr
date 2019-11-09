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
    enum typeOfTeammates{required, prevented, requested};
    gatherTeammatesDialog(const typeOfTeammates whatTypeOfTeammate, const studentRecord studentrecs[], int numStudentsComingIn, const QString &sectionname, QWidget *parent = nullptr);
    ~gatherTeammatesDialog();

    studentRecord *student;
    bool teammatesSpecified;

private slots:
    void addOneTeammateSet();
    void clearAllTeammateSets();

private:
    typeOfTeammates whatType;
    QString sectionName;
    int numStudents;
    QGridLayout *theGrid;
    QTableWidget *currentListOfTeammatesTable;
    static const int possibleNumIDs = 8;                //number of comboboxes in the dialog box, i.e., possible choices of teammates
    QComboBox possibleTeammates[possibleNumIDs + 1];    // +1 for the requesting student in typeOfTeammates == requested
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

private slots:
    void clearAllNames();

private:
    int numTeams;
    QGridLayout *theGrid;
    QLabel *teamNumberLabel;
    QPushButton *resetNamesButton;
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
    enum saveFormat{txt, pdf};

    whichFilesDialog(const action saveOrPrint, const QStringList previews = {}, QWidget *parent = nullptr);

    QCheckBox *studentFile;
    QCheckBox *instructorFile;
    QCheckBox *spreadsheetFile;

private:
    QGridLayout *theGrid;
    QLabel *explanation;
    QDialogButtonBox *buttonBox;
};


class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(studentRecord studentToBeEdited, DataOptions dataOptions, QStringList sectionNames, QWidget *parent = nullptr);
    ~editOrAddStudentDialog();

    studentRecord student;

private slots:
    void recordEdited();

private:

    // a subclassed QSpinBox that replaces numerical values with categorical attribute responses in display
    class CategoricalSpinBox : public QSpinBox
    {
    public:
        CategoricalSpinBox(QWidget *parent = nullptr);
        QString textFromValue(int value) const;
        int valueFromText(const QString &text) const;
        QValidator::State validate (QString &input, int &pos) const;
        QStringList responseTexts;
    };

    DataOptions dataOptions;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QComboBox *databox;
    QSpinBox *datanumber;
    CategoricalSpinBox *datacategorical;
    QDialogButtonBox *buttonBox;
};


#endif // CUSTOMDIALOGS
