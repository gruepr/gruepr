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
