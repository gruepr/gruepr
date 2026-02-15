#ifndef EDITORADDSTUDENTDIALOG_H
#define EDITORADDSTUDENTDIALOG_H

#include "dataOptions.h"
#include "studentRecord.h"
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStackedWidget>

class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(StudentRecord &student, const DataOptions *const dataOptions, QWidget *parent = nullptr, bool newStudent = false);
    ~editOrAddStudentDialog() override = default;
    editOrAddStudentDialog(const editOrAddStudentDialog&) = delete;
    editOrAddStudentDialog operator= (const editOrAddStudentDialog&) = delete;
    editOrAddStudentDialog(editOrAddStudentDialog&&) = delete;
    editOrAddStudentDialog& operator= (editOrAddStudentDialog&&) = delete;

private:
    void updateRecord(StudentRecord &student, const DataOptions *const dataOptions);
    void adjustSchedule(const StudentRecord &student, const DataOptions *const dataOptions);
    QList<QLineEdit*> datatext;
    QList<QPlainTextEdit*> datamultiline;
    QList<QWidget*> databox;                        // either a QGroupBox or QComboBox
    QStackedWidget *attributeStack = nullptr;
    QList<QPushButton*> attributeSelectorButtons;
    QList<QComboBox*> attributeCombobox;
    QList<QGroupBox*> attributeMultibox;
    bool tempUnavailability[MAX_DAYS][MAX_BLOCKS_PER_DAY];
};

#endif // EDITORADDSTUDENTDIALOG_H
