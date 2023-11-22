#ifndef CUSTOMTEAMNAMESDIALOG_H
#define CUSTOMTEAMNAMESDIALOG_H

#include "listTableDialog.h"

class customTeamnamesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamnamesDialog(int numTeams = 1, const QStringList &incomingTeamNames = {}, QWidget *parent = nullptr);
    ~customTeamnamesDialog() override = default;
    customTeamnamesDialog(const customTeamnamesDialog&) = delete;
    customTeamnamesDialog operator= (const customTeamnamesDialog&) = delete;
    customTeamnamesDialog(customTeamnamesDialog&&) = delete;
    customTeamnamesDialog& operator= (customTeamnamesDialog&&) = delete;

    QStringList teamNames;

private slots:
    void clearAllNames();

private:
    QList<QLineEdit *> teamNameLineEdits;
};

#endif // CUSTOMTEAMNAMESDIALOG_H
