#ifndef GATHERURMRESPONSESDIALOG_H
#define GATHERURMRESPONSESDIALOG_H

#include "listTableDialog.h"
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

class gatherURMResponsesDialog : public listTableDialog
{
    Q_OBJECT

public:
    gatherURMResponsesDialog(const QStringList &URMResponses, const QStringList &currURMResponsesConsideredUR, QWidget *parent = nullptr);
    ~gatherURMResponsesDialog();

    QStringList URMResponsesConsideredUR;

private:
    QLabel *explanation;
    QCheckBox *enableValue;
    QPushButton *responses;
};

#endif // GATHERURMRESPONSESDIALOG_H
