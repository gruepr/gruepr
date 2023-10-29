#ifndef GATHERURMRESPONSESDIALOG_H
#define GATHERURMRESPONSESDIALOG_H

#include "listTableDialog.h"
#include <QPushButton>

class gatherURMResponsesDialog : public listTableDialog
{
    Q_OBJECT

public:
    gatherURMResponsesDialog(const QStringList &URMResponses, const QStringList &currURMResponsesConsideredUR, QWidget *parent = nullptr);
    ~gatherURMResponsesDialog() = default;
    gatherURMResponsesDialog(const gatherURMResponsesDialog&) = delete;
    gatherURMResponsesDialog operator= (const gatherURMResponsesDialog&) = delete;
    gatherURMResponsesDialog(gatherURMResponsesDialog&&) = delete;
    gatherURMResponsesDialog& operator= (gatherURMResponsesDialog&&) = delete;

    QStringList URMResponsesConsideredUR;

private:
    QList<QPushButton*> responses;
};

#endif // GATHERURMRESPONSESDIALOG_H
