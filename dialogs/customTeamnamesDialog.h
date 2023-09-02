#ifndef CUSTOMTEAMNAMESDIALOG_H
#define CUSTOMTEAMNAMESDIALOG_H

#include "listTableDialog.h"
#include <QPushButton>

class customTeamnamesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamnamesDialog(int numTeams = 1, const QStringList &incomingTeamNames = {}, QWidget *parent = nullptr);
    customTeamnamesDialog(const customTeamnamesDialog&) = delete;
    customTeamnamesDialog operator= (const customTeamnamesDialog&) = delete;
    customTeamnamesDialog(customTeamnamesDialog&&) = delete;
    customTeamnamesDialog& operator= (customTeamnamesDialog&&) = delete;

    QList<QLineEdit *> teamNames;

private slots:
    void clearAllNames();

private:
    int numTeams;
    QPushButton *resetNamesButton = nullptr;
};

#endif // CUSTOMTEAMNAMESDIALOG_H
