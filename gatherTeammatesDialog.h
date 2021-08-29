#ifndef GATHERTEAMMATESDIALOG
#define GATHERTEAMMATESDIALOG

#include <QDialog>
#include <QGridLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include "dataOptions.h"
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


#endif // GATHERTEAMMATESDIALOG
