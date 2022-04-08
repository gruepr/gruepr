#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

class registerDialog : public QDialog
{
    Q_OBJECT

public:
    registerDialog(QWidget *parent = nullptr);

    QLineEdit *name;
    QLineEdit *institution;
    QLineEdit *email;

private:
    QGridLayout *theGrid;
    QLabel *explanation;
    QDialogButtonBox *buttonBox;
    inline static const QString EMAILADDRESSREGEX = "^[A-Z0-9.!#$%&*+_-~]+@[A-Z0-9.-]+\\.[A-Z]{2,64}$";
};

#endif // REGISTERDIALOG_H
