#ifndef CUSTOMRESPONSEOPTIONSDIALOG_H
#define CUSTOMRESPONSEOPTIONSDIALOG_H

#include "listTableDialog.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

class customResponseOptionsDialog : public listTableDialog
{
    Q_OBJECT

public:
    customResponseOptionsDialog(const QStringList &currentCustomOptions, QWidget *parent = nullptr);
    ~customResponseOptionsDialog() override = default;
    customResponseOptionsDialog(const customResponseOptionsDialog&) = delete;
    customResponseOptionsDialog operator= (const customResponseOptionsDialog&) = delete;
    customResponseOptionsDialog(customResponseOptionsDialog&&) = delete;
    customResponseOptionsDialog& operator= (customResponseOptionsDialog&&) = delete;

    int numOptions;
    QStringList options;

private slots:
    void refreshDisplay();
    void clearAll();

private:
    inline bool allFilled();
    inline bool stripPrecedingOrderNumbers(QStringList &options);

    QSpinBox *numOptionsBox = nullptr;
    QCheckBox *orderedResponsesCheckbox = nullptr;
    QList<QLabel *> optionLabels;
    QList<QLineEdit *> optionLineEdits;

    inline const static int MAXRESPONSEOPTIONS = 100;
};


#endif // CUSTOMRESPONSEOPTIONSDIALOG_H
