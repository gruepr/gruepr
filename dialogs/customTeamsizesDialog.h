#ifndef CUSTOMTEAMSIZESDIALOG_H
#define CUSTOMTEAMSIZESDIALOG_H

#include "listTableDialog.h"
#include <QLabel>
#include <QSpinBox>

class customTeamsizesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamsizesDialog(long long numStudents, int idealTeamsize, QWidget *parent = nullptr);
    ~customTeamsizesDialog() override = default;
    customTeamsizesDialog(const customTeamsizesDialog&) = delete;
    customTeamsizesDialog operator= (const customTeamsizesDialog&) = delete;
    customTeamsizesDialog(customTeamsizesDialog&&) = delete;
    customTeamsizesDialog& operator= (customTeamsizesDialog&&) = delete;

    QList<int> teamsizes;
    int numTeams;

private slots:
    void refreshDisplay();
    void teamsizeChanged(int);

private:
    long long numStudents;
    QSpinBox *numTeamsBox = nullptr;
    QList<QSpinBox*> teamsizeBox;
    QLabel *remainingStudents = nullptr;
};


#endif // CUSTOMTEAMSIZESDIALOG_H
