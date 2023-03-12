#include "customTeamnamesDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team names
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamnamesDialog::customTeamnamesDialog(int numTeams, const QStringList &teamNames, QWidget *parent)
    :listTableDialog (tr("Enter custom team names"), true, true, parent)
{
    this->numTeams = numTeams;
    teamName = new QLineEdit[numTeams];

    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    //Table of team names
    theTable->setRowCount(numTeams);
    int widthCol0 = 0;
    for(int team = 0; team < numTeams; team++)
    {
        auto *label = new QLabel(tr("Team ") + QString::number(team+1) + " ");
        theTable->setCellWidget(team, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamName[team].setPlaceholderText(tr("Custom name"));
        if(team < teamNames.size())
        {
            teamName[team].setText((teamNames.at(team) == QString::number(team+1))? "" : teamNames.at(team));
        }
        theTable->setCellWidget(team, 1, &teamName[team]);
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLECOLUMN0OVERWIDTH));
    theTable->adjustSize();

    //A reset table button
    resetNamesButton = new QPushButton(this);
    resetNamesButton->setText(tr("&Clear All Names"));
    theGrid->addWidget(resetNamesButton, BUTTONBOXROWINGRID, 0, 1, 1);
    connect(resetNamesButton, &QPushButton::clicked, this, &customTeamnamesDialog::clearAllNames);

    adjustSize();
}


customTeamnamesDialog::~customTeamnamesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] teamName;
}


void customTeamnamesDialog::clearAllNames()
{
    for(int team = 0; team < numTeams; team++)
    {
        teamName[team].clear();
    }
}
