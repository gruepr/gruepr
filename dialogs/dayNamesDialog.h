#ifndef DAYNAMESDIALOG_H
#define DAYNAMESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>

class dayNamesDialog : public QDialog
{
    Q_OBJECT

public:
    dayNamesDialog(QList<QCheckBox *> dayselectors, QList<QLineEdit *> daynames, QWidget *parent = nullptr);
};

#endif // DAYNAMESDIALOG_H
