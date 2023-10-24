#ifndef EDITORADDSTUDENTDIALOG_H
#define EDITORADDSTUDENTDIALOG_H

#include "dataOptions.h"
#include "studentRecord.h"
#include "widgets/comboBoxThatPassesScrollwheel.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStackedWidget>

class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(StudentRecord &student, const DataOptions *const dataOptions, QWidget *parent = nullptr, bool newStudent = false);
    ~editOrAddStudentDialog() = default;
    editOrAddStudentDialog(const editOrAddStudentDialog&) = delete;
    editOrAddStudentDialog operator= (const editOrAddStudentDialog&) = delete;
    editOrAddStudentDialog(editOrAddStudentDialog&&) = delete;
    editOrAddStudentDialog& operator= (editOrAddStudentDialog&&) = delete;

private:
    void updateRecord(StudentRecord &student, const DataOptions *const dataOptions);
    void adjustSchedule(const StudentRecord &student, const DataOptions *const dataOptions);
    QGridLayout *theGrid = nullptr;
    QList<QLabel*> explanation;
    QList<QLineEdit*> datatext;
    QList<QPlainTextEdit*> datamultiline;
    QList<ComboBoxThatPassesScrollwheel*> databox;
    QStackedWidget *attributeStack = nullptr;
    QList<QPushButton*> attributeSelectorButtons;
    QList<ComboBoxThatPassesScrollwheel*> attributeCombobox;
    QList<QGroupBox*> attributeMultibox;
    bool tempUnavailability[MAX_DAYS][MAX_BLOCKS_PER_DAY];
    QDialogButtonBox *buttonBox = nullptr;
    inline static const int NUMSINGLELINES = 4;       // timestamp, first name, last name, email
    enum {timestamp, firstname, lastname, email};
    inline static const int NUMCOMBOBOXES = 3;        // gender, ethnicity, section
    enum {gender, ethnicity, section};
    inline static const int NUMMULTILINES = 3;        // pref. teammates, pref. non-teammates, notes
    enum {prefTeammates, prefNonTeammates, notes};
};

#endif // EDITORADDSTUDENTDIALOG_H
