#ifndef FINDMATCHINGNAMEDIALOG_H
#define FINDMATCHINGNAMEDIALOG_H

#include <QDialog>
#include "studentRecord.h"
#include <QComboBox>
#include <QRadioButton>

class findMatchingNameDialog : public QDialog
{
    Q_OBJECT

public:
    findMatchingNameDialog(int numStudents, const QList<StudentRecord> &students, const QString &searchName, QWidget *parent = nullptr,
                           const QString &nameOfStudentWhoAsked = "", const bool addStudentOption = false, const QString &searchEmail = "");
    ~findMatchingNameDialog() override = default;
    findMatchingNameDialog(const findMatchingNameDialog&) = delete;
    findMatchingNameDialog operator= (const findMatchingNameDialog&) = delete;
    findMatchingNameDialog(findMatchingNameDialog&&) = delete;
    findMatchingNameDialog& operator= (findMatchingNameDialog&&) = delete;

    bool addStudent = false;
    bool useRosterName = false;
    bool useRosterEmail = false;
    QString currSurveyName;
    QString currSurveyEmail;
    int currSurveyID = 0;

private:
    QComboBox *namesList = nullptr;
    QRadioButton *useSurveyNameCheckbox = nullptr;
    QRadioButton *useRosterNameCheckbox = nullptr;
    QRadioButton *useSurveyEmailCheckbox = nullptr;
    QRadioButton *useRosterEmailCheckbox = nullptr;
};

#endif // FINDMATCHINGNAMEDIALOG_H
