#ifndef EDITORADDSTUDENTDIALOG_H
#define EDITORADDSTUDENTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include "widgets\categoricalSpinBox.h"
#include "dataOptions.h"
#include "studentRecord.h"

class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QWidget *parent = nullptr);
    ~editOrAddStudentDialog();

    StudentRecord student;

private:
    void updateRecord();
    DataOptions internalDataOptions;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QPlainTextEdit *datamultiline;
    QComboBox *databox;
    CategoricalSpinBox *datacategorical;
    QDialogButtonBox *buttonBox;
    const int NUMSINGLELINES = 4;       // timestamp, first name, last name, email
    enum {timestamp, firstname, lastname, email};
    const int NUMCOMBOBOXES = 3;        // gender, ethnicity, section
    enum {gender, ethnicity, section};
    const int NUMMULTILINES = 3;        // pref. teammates, pref. non-teammates, notes
    enum {prefTeammates, prefNonTeammates, notes};
};

#endif // EDITORADDSTUDENTDIALOG_H
