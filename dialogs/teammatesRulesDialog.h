#ifndef TEAMMATESRULESDIALOG_H
#define TEAMMATESRULESDIALOG_H

#include "dataOptions.h"
#include "studentRecord.h"
#include "teamingOptions.h"
#include <QAbstractButton>
#include <QBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QTableWidget>

namespace Ui {
    class TeammatesRulesDialog;
}

class TeammatesRulesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class TypeOfTeammates{required = 0, prevented = 1, requested = 2}; // int values correspond to the ui's tabwidget tab index
    explicit TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions, const TeamingOptions &teamingOptions,
                                  const QString &sectionname, const QStringList &currTeamSets, TypeOfTeammates typeOfTeammates, QWidget *parent = nullptr);
    ~TeammatesRulesDialog() override;
    TeammatesRulesDialog(const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog operator= (const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog(TeammatesRulesDialog&&) = delete;
    TeammatesRulesDialog& operator= (TeammatesRulesDialog&&) = delete;

    QList<StudentRecord> students;
    bool teammatesSpecified = false;
    int numberRequestedTeammatesGiven = 1;

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

    QList <QComboBox *> possibleTeammates;

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
        case TypeOfTeammates::required:  return tr("Required");
        case TypeOfTeammates::prevented: return tr("Prevented");
        case TypeOfTeammates::requested: return tr("Requested");
        }
        return {};
    }
};

#endif // TEAMMATESRULESDIALOG_H
