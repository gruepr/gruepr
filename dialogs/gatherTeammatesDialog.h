#ifndef GATHERTEAMMATESDIALOG
#define GATHERTEAMMATESDIALOG

#include "dataOptions.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QPushButton>
#include <QTableWidget>

class gatherTeammatesDialog : public QDialog
{
    Q_OBJECT

public:
    enum typeOfTeammates{required, prevented, requested};
    gatherTeammatesDialog(const typeOfTeammates whatTypeOfTeammate, const StudentRecord studentrecs[], int numStudentsComingIn,
                          const DataOptions *const dataOptions, const QString &sectionname, const QStringList *const currTeamSets, QWidget *parent = nullptr);
    ~gatherTeammatesDialog();

    StudentRecord *student = nullptr;
    bool teammatesSpecified = false;

private slots:
    void addOneTeammateSet();
    void clearAllTeammateSets();

private:
    typeOfTeammates whatType;
    bool requestsInSurvey = false;
    QString sectionName;
    int numStudents;
    const QStringList *teamSets = nullptr;
    QGridLayout *theGrid = nullptr;
    QTableWidget *currentListOfTeammatesTable = nullptr;
    inline static const int POSSIBLENUMIDS = 8;         // number of comboboxes in the dialog box, i.e., possible choices of teammates
    QComboBox possibleTeammates[POSSIBLENUMIDS + 1];    // +1 for the requesting student in typeOfTeammates == requested
    QPushButton *loadTeammates = nullptr;
    QComboBox *actionSelectBox = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    void refreshDisplay();
    bool saveCSVFile();                                    // these all return true on success, false on fail
    bool loadCSVFile();
    bool loadStudentPrefs();
    bool loadSpreadsheetFile();
    bool loadExistingTeamset();

    const QSize ICONSIZE = QSize(15,15);
};


#endif // GATHERTEAMMATESDIALOG
