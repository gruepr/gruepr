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
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QButtonGroup>
#include "categorialSpinBox.h"
#include "dataOptions.h"
#include "findMatchingNameDialog.h"
#include "listTableDialog.h"
#include "studentRecord.h"


class gatherTeammatesDialog : public QDialog
{
    Q_OBJECT

public:
    enum typeOfTeammates{required, prevented, requested};
    gatherTeammatesDialog(const typeOfTeammates whatTypeOfTeammate, const StudentRecord studentrecs[], int numStudentsComingIn,
                          const DataOptions *const dataOptions, const QString &sectionname, QWidget *parent = nullptr);
    ~gatherTeammatesDialog();

    StudentRecord *student;
    bool teammatesSpecified = false;

private slots:
    void addOneTeammateSet();
    void clearAllTeammateSets();

private:
    typeOfTeammates whatType;
    bool requestsInSurvey = false;
    QString sectionName;
    int numStudents;
    QGridLayout *theGrid;
    QTableWidget *currentListOfTeammatesTable;
    static const int possibleNumIDs = 8;                // number of comboboxes in the dialog box, i.e., possible choices of teammates
    QComboBox possibleTeammates[possibleNumIDs + 1];    // +1 for the requesting student in typeOfTeammates == requested
    QPushButton *loadTeammates;
    QComboBox *resetSaveOrLoad;
    QDialogButtonBox *buttonBox;
    void refreshDisplay();
    bool saveCSVFile();                                    // returns true on success, false on fail
    bool loadCSVFile();
    bool loadStudentPrefs();
    bool loadSpreadsheetFile();
};


class customTeamsizesDialog : public listTableDialog
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
    QLabel numTeamsLabel;
    QComboBox numTeamsBox;
    QSpinBox *teamsizeBox;
    QLabel remainingStudents;
};


class customTeamnamesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamnamesDialog(int numTeams = 1, const QStringList &teamNames = {}, QWidget *parent = nullptr);
    ~customTeamnamesDialog();

    QLineEdit *teamName;

private slots:
    void clearAllNames();

private:
    int numTeams;
    QPushButton *resetNamesButton;
};


class whichFilesDialog : public QDialog
{
    Q_OBJECT

public:
    enum action{save, print};

    whichFilesDialog(const action saveOrPrint, const QStringList &previews = {}, QWidget *parent = nullptr);
    ~whichFilesDialog();

    QCheckBox *studentFiletxt;
    QCheckBox *studentFilepdf;
    QCheckBox *instructorFiletxt;
    QCheckBox *instructorFilepdf;
    QCheckBox *spreadsheetFiletxt;

private slots:
    void boxToggled();

private:
    bool saveDialog;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLabel *textfile;
    QLabel *pdffile;
    QPushButton *studentFileLabel;
    QPushButton *instructorFileLabel;
    QPushButton *spreadsheetFileLabel;
    QDialogButtonBox *buttonBox;
    QFont previousToolTipFont;
    const int CHECKBOXSIZE = 30;
};


class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QStringList sectionNames, QWidget *parent = nullptr);
    ~editOrAddStudentDialog();

    StudentRecord student;

private slots:
    void recordEdited();

private:
    DataOptions internalDataOptions;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QPlainTextEdit *datamultiline;
    QComboBox *databox;
    CategoricalSpinBox *datacategorical;
    QDialogButtonBox *buttonBox;
};


class gatherIncompatibleResponsesDialog : public QDialog
{
    Q_OBJECT

public:
    gatherIncompatibleResponsesDialog(const int attribute, const DataOptions *const dataOptions, const QVector< QPair<int,int> > &currIncompats, QWidget *parent = nullptr);
    ~gatherIncompatibleResponsesDialog();

    QVector< QPair<int,int> > incompatibleResponses;

private slots:
    void addValues();
    void clearAllValues();

private:
    void updateExplanation();
    int numPossibleValues;
    QGridLayout *theGrid;
    QLabel *attributeQuestion;
    QLabel *incompatAttributePart1;
    QLabel *incompatAttributePart2;
    QRadioButton *primaryValues;
    QPushButton *primaryResponses;
    QButtonGroup *primaryValuesGroup;
    QCheckBox *incompatValues;
    QPushButton *incompatResponses;
    QPushButton *addValuesButton;
    QPushButton *resetValuesButton;
    QDialogButtonBox *buttonBox;
    QLabel *explanation;
};

#endif // CUSTOMDIALOGS
