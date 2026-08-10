#ifndef TEAMMATESRULESDIALOG_H
#define TEAMMATESRULESDIALOG_H

#include "dataOptions.h"
#include "studentRecord.h"
#include "widgets/styledComboBox.h"
#include <QAbstractButton>
#include <QBoxLayout>
#include <QDialog>
#include <QTableWidget>

class gruepr;

namespace Ui {
    class TeammatesRulesDialog;
}

class TeammatesRulesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class TypeOfTeammates{groupTogether, splitApart};
    explicit TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions, const QString &sectionname,
                                  const QStringList &currTeamSets, TypeOfTeammates typeOfTeammates, int initialNumberGiven = REQUESTED_TEAMMATES_ALL,
                                  gruepr *parent = nullptr);
    ~TeammatesRulesDialog() override;
    TeammatesRulesDialog(const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog operator= (const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog(TeammatesRulesDialog&&) = delete;
    TeammatesRulesDialog& operator= (TeammatesRulesDialog&&) = delete;

    QList<StudentRecord> students;
    bool teammatesSpecified = false;
    int numberGroupTogethersGiven = REQUESTED_TEAMMATES_ALL;

    // Result of parsing a "spreadsheet file of previous teammates" -- teamNames[i] pairs with
    // teammateNames[i] (that team's teammates, by name, in file order). Deliberately UI-free (no
    // QFileDialog, no error QMessageBox) so it's directly testable; see parseTeamsFromSpreadsheetFile.
    struct ParsedSpreadsheetTeams {
        bool success = false;
        QStringList teamNames;
        QList<QStringList> teammateNames;
    };
    // Reads fileName (delimiter picked from its extension: .csv -> comma, else tab) and finds the
    // Team/Name (or First Name + Last Name) columns by header text rather than fixed position --
    // tolerant of a combined "Name" column (older exports), reordered/extra columns, or a missing
    // Section/Email. Static and UI-free on purpose: callable from a test without any dialog involved.
    static ParsedSpreadsheetTeams parseTeamsFromSpreadsheetFile(const QString &fileName);

    // Header widgets (public so layout can be managed)
    QHBoxLayout *headerLayout = nullptr;
    QWidget *headerWidget = nullptr;
    QAbstractButton *topLeftTableHeaderButton = nullptr;
    int initialWidthStudentHeader = 0;
    QTableWidget *tableWidget = nullptr;

private:
    Ui::TeammatesRulesDialog *ui;
    const TypeOfTeammates m_type;
    const QString m_typeText;

    bool requestsInSurvey = false;
    const int numStudents;
    QString sectionName;
    QStringList teamSets;
    gruepr *grueprParent = nullptr;

    QList <StyledComboBox *> possibleTeammates;

    void showToast(QWidget *parent, const QString &message, int duration = 3000);
    void initializeTableHeaders(QString searchBarText = "", bool initializeStatus = false);
    void refreshDisplay(int verticalScrollPos, int horizontalScrollPos, QString searchBarText="");
    void clearValues(bool verify = true);

    // these all return true on success, false on fail
    bool loadCSVFile();
    bool loadStudentPrefs();
    bool loadSpreadsheetFile();
    bool loadExistingTeamset();

    const QSize ICONSIZE = QSize(15,15);

    static QString typeToString(TypeOfTeammates type) {
        switch(type) {
        case TypeOfTeammates::splitApart: return tr("Split Apart");
        case TypeOfTeammates::groupTogether: return tr("Group Together");
        }
        return {};
    }
};

#endif // TEAMMATESRULESDIALOG_H
