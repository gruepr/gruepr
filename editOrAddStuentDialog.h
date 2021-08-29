#ifndef EDITORADDSTUENTDIALOG_H
#define EDITORADDSTUENTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include "categoricalSpinBox.h"
#include "dataOptions.h"
#include "studentRecord.h"

class editOrAddStudentDialog : public QDialog
{
    Q_OBJECT

public:
    editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QStringList sectionNames, QWidget *parent = nullptr);
    ~editOrAddStudentDialog();

    StudentRecord student;

private slots:
    void recordEdited();

private:
    DataOptions internalDataOptions;
    QGridLayout *theGrid;
    QLabel *explanation;
    QLineEdit *datatext;
    QPlainTextEdit *datamultiline;
    QComboBox *databox;
    CategoricalSpinBox *datacategorical;
    QDialogButtonBox *buttonBox;
};

#endif // EDITORADDSTUENTDIALOG_H
