#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>

class registerDialog : public QDialog
{
    Q_OBJECT

public:
    registerDialog(QWidget *parent = nullptr);

    QString name;
    QString institution;
    QString email;

private:
    QDialogButtonBox *buttonBox = nullptr;
    QLineEdit *nameLineEdit = nullptr;
    QLineEdit *institutionLineEdit = nullptr;
    QLineEdit *emailLineEdit = nullptr;
    inline static const QString EMAILADDRESSREGEX = "^[A-Z0-9.!#$%&*+\-_-~]+@[A-Z0-9.-]+\\.[A-Z]{2,64}$";
};

#endif // REGISTERDIALOG_H
