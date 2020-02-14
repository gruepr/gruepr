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
#include <customWidgets.h>
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
    static const int possibleNumIDs = 8;                // number of comboboxes in the dialog box, i.e., possible choices of teammates
    QComboBox possibleTeammates[possibleNumIDs + 1];    // +1 for the requesting student in typeOfTeammates == requested
    QPushButton *loadTeammates;
    QPushButton *resetTableButton;
    QPushButton *loadFileOfTeammates;
    QDialogButtonBox *buttonBox;
    void refreshDisplay();
    bool loadFile();                                    // returns true on success, false on fail
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
    customTeamnamesDialog(int numTeams = 1, const QStringList &teamNames = {}, QWidget *parent = nullptr);
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
};


class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(const studentRecord &studentToBeEdited, const DataOptions &dataOptions, const QStringList &sectionNames, QWidget *parent = nullptr);
    ~editOrAddStudentDialog();

    studentRecord student;

private slots:
    void recordEdited();

private:
    DataOptions dataOptions;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QComboBox *databox;
    QSpinBox *datanumber;
    CategoricalSpinBox *datacategorical;
    QDialogButtonBox *buttonBox;
};


class gatherIncompatibleResponsesDialog : public QDialog
{
    Q_OBJECT

public:
    gatherIncompatibleResponsesDialog(const int attribute, const DataOptions &dataOptions, const QList< QPair<int,int> > &currIncompats, QWidget *parent = nullptr);
    ~gatherIncompatibleResponsesDialog();

    QList< QPair<int,int> > incompatibleResponses;

private slots:
    void addValues();
    void clearAllValues();

private:
    void updateExplanation();
    int numPossibleValues;
    QGridLayout *theGrid;
    QLabel *attributeDescription;
    QCheckBox *enableValue;
    QPushButton *responses;
    QPushButton *addValuesButton;
    QPushButton *resetValuesButton;
    QDialogButtonBox *buttonBox;
    QLabel *explanation;
};


class gatherURMResponsesDialog : public QDialog
{
    Q_OBJECT

public:
    gatherURMResponsesDialog(const DataOptions &dataOptions, const QStringList &currURMResponsesConsideredUR, QWidget *parent = nullptr);
    ~gatherURMResponsesDialog();

    QStringList URMResponsesConsideredUR;

private:
    QGridLayout *theGrid;
    QLabel *explanation;
    QCheckBox *enableValue;
    QPushButton *responses;
    QDialogButtonBox *buttonBox;
};


#endif // CUSTOMDIALOGS
