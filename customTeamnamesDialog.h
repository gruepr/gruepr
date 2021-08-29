#ifndef CUSTOMTEAMNAMESDIALOG_H
#define CUSTOMTEAMNAMESDIALOG_H

#include "listTableDialog.h"
#include <QPushButton>

class customTeamnamesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamnamesDialog(int numTeams = 1, const QStringList &teamNames = {}, QWidget *parent = nullptr);
    ~customTeamnamesDialog();

    QLineEdit *teamName;

private slots:
    void clearAllNames();

private:
    int numTeams;
    QPushButton *resetNamesButton;
};

#endif // CUSTOMTEAMNAMESDIALOG_H
