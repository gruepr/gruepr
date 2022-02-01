#ifndef EDITORADDSTUDENTDIALOG_H
#define EDITORADDSTUDENTDIALOG_H

#include "dataOptions.h"
#include "studentRecord.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
//#include "widgets/categoricalSpinBox.h"

class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(StudentRecord &student, const DataOptions *const dataOptions, QWidget *parent = nullptr, bool newStudent = false);
    ~editOrAddStudentDialog();

private:
    void updateRecord(StudentRecord &student, const DataOptions *const dataOptions);
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QPlainTextEdit *datamultiline;
    QComboBox *databox;
    QTabWidget *attributeTabs;
//    CategoricalSpinBox *dataspinbox;
    QComboBox *attributeCombobox;
    QGroupBox *attributemulticategoricalbox;
    QDialogButtonBox *buttonBox;
    const int NUMSINGLELINES = 4;       // timestamp, first name, last name, email
    enum {timestamp, firstname, lastname, email};
    const int NUMCOMBOBOXES = 3;        // gender, ethnicity, section
    enum {gender, ethnicity, section};
    const int NUMMULTILINES = 3;        // pref. teammates, pref. non-teammates, notes
    enum {prefTeammates, prefNonTeammates, notes};
};

#endif // EDITORADDSTUDENTDIALOG_H
