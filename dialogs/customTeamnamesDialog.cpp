#include "customTeamnamesDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team names
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamnamesDialog::customTeamnamesDialog(int numTeams, const QStringList &incomingTeamNames, QWidget *parent)
    :listTableDialog (tr("Enter custom team names"), true, true, parent)
{
    this->numTeams = numTeams;

    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    //Table of team names
    theTable->setRowCount(numTeams);
    int widthCol0 = 0;
    for(int team = 0; team < numTeams; team++)
    {
        auto *label = new QLabel(tr("Team ") + QString::number(team+1) + " ", this);
        label->setStyleSheet(LABELSTYLE);
        theTable->setCellWidget(team, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamNames << new QLineEdit(this);
        teamNames.last()->setStyleSheet(LINEEDITSTYLE);
        teamNames.last()->setPlaceholderText(tr("Team ") + QString::number(team+1));
        if(team < incomingTeamNames.size())
        {
            teamNames.last()->setText((incomingTeamNames.at(team) == QString::number(team+1))? "" : incomingTeamNames.at(team));
        }
        theTable->setCellWidget(team, 1, teamNames.last());
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLECOLUMN0OVERWIDTH));
    theTable->resizeColumnsToContents();
    theTable->resizeRowsToContents();
    theTable->adjustSize();

    //A reset table button
    resetNamesButton = new QPushButton(tr("Clear All"), this);
    resetNamesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(resetNamesButton, &QPushButton::clicked, this, &customTeamnamesDialog::clearAllNames);
    addButton(resetNamesButton);

    adjustSize();
}


void customTeamnamesDialog::clearAllNames()
{
    for(auto &teamName : teamNames)
    {
        teamName->clear();
    }
}
