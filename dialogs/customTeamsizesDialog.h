#ifndef CUSTOMTEAMSIZESDIALOG_H
#define CUSTOMTEAMSIZESDIALOG_H

#include "listTableDialog.h"
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>

class customTeamsizesDialog : public listTableDialog
{
    Q_OBJECT

public:
    customTeamsizesDialog(int numStudents, int idealTeamsize, QWidget *parent = nullptr);
    ~customTeamsizesDialog();
    customTeamsizesDialog(const customTeamsizesDialog&) = delete;
    customTeamsizesDialog operator= (const customTeamsizesDialog&) = delete;
    customTeamsizesDialog(customTeamsizesDialog&&) = delete;
    customTeamsizesDialog& operator= (customTeamsizesDialog&&) = delete;

    int *teamsizes;
    int numTeams;

private slots:
    void refreshDisplay(int numTeamsBoxIndex);
    void teamsizeChanged(int);

private:
    int numStudents;
    QLabel numTeamsLabel;
    QComboBox numTeamsBox;
    QSpinBox *teamsizeBox;
    QLabel remainingStudents;
};


#endif // CUSTOMTEAMSIZESDIALOG_H
