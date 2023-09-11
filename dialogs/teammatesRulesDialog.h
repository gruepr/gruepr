#ifndef TEAMMATESRULESDIALOG_H
#define TEAMMATESRULESDIALOG_H

#include <QDialog>
#include "dataOptions.h"
#include "studentRecord.h"
#include "teamingOptions.h"
#include <QComboBox>

namespace Ui {
class TeammatesRulesDialog;
}

class TeammatesRulesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class TypeOfTeammates{required, prevented, requested};
    explicit TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions, const TeamingOptions &teamingOptions,
                                  const QString &sectionname, const QStringList &currTeamSets, QWidget *parent = nullptr);
    ~TeammatesRulesDialog();
    TeammatesRulesDialog(const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog operator= (const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog(TeammatesRulesDialog&&) = delete;
    TeammatesRulesDialog& operator= (TeammatesRulesDialog&&) = delete;

    QList<StudentRecord> students;
    bool required_teammatesSpecified = false;
    bool prevented_teammatesSpecified = false;
    bool requested_teammatesSpecified = false;
    int numberRequestedTeammatesGiven = 1;

private slots:
    void clearAllValues();

private:
    Ui::TeammatesRulesDialog *ui;
    QPushButton *clearAllValuesButton = nullptr;

    bool positiverequestsInSurvey = false;
    bool negativerequestsInSurvey = false;
    const int numStudents;
    QString sectionName;
    QStringList teamSets;

    QList <QComboBox *> possibleRequiredTeammates;
    QList <QComboBox *> possiblePreventedTeammates;
    QList <QComboBox *> possibleRequestedTeammates;
    void addTeammateSelector(TypeOfTeammates typeOfTeammates);

    void refreshDisplay(TypeOfTeammates typeOfTeammates);

    void addOneTeammateSet(TypeOfTeammates typeOfTeammates);
    void clearValues(TypeOfTeammates typeOfTeammates, bool verify = true);

     // these all return true on success, false on fail
    bool saveCSVFile(TypeOfTeammates typeOfTeammates);
    bool loadCSVFile(TypeOfTeammates typeOfTeammates);
    bool loadStudentPrefs(TypeOfTeammates typeOfTeammates);
    bool loadSpreadsheetFile(TypeOfTeammates typeOfTeammates);
    bool loadExistingTeamset(TypeOfTeammates typeOfTeammates);

    const QSize ICONSIZE = QSize(15,15);
};

#endif // TEAMMATESRULESDIALOG_H
