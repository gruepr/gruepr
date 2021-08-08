#ifndef DAYNAMESDIALOG_H
#define DAYNAMESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>

class dayNamesDialog : public QDialog
{
    Q_OBJECT

public:
    dayNamesDialog(QCheckBox *dayselectors[], QLineEdit *daynames[], QWidget *parent = nullptr);

private:
    QGridLayout *theGrid;
    QDialogButtonBox *buttonBox;
};

#endif // DAYNAMESDIALOG_H
