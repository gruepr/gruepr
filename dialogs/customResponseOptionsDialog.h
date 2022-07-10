#ifndef CUSTOMRESPONSEOPTIONSDIALOG_H
#define CUSTOMRESPONSEOPTIONSDIALOG_H

#include "listTableDialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

class customResponseOptionsDialog : public listTableDialog
{
    Q_OBJECT

public:
    customResponseOptionsDialog(const QStringList &currentCustomOptions, QWidget *parent = nullptr);
    ~customResponseOptionsDialog();

    QStringList options;
    int numOptions;

private slots:
    void refreshDisplay(int numOptionsBoxValue);
    void optionChanged();
    void clearAll();

private:
    bool allFilled();

    QLabel numOptionsLabel;
    QSpinBox numOptionsBox;
    QLineEdit *optionLineEdit;
    QLabel numberingReminderLabel;

    inline const static int MAXRESPONSEOPTIONS = 100;
};


#endif // CUSTOMRESPONSEOPTIONSDIALOG_H
