#include "customTeamnamesDialog.h"
#include "gruepr_consts.h"
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team names
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamnamesDialog::customTeamnamesDialog(int numTeams, const QStringList &teamNames, QWidget *parent)
    :listTableDialog (tr("Choose custom team names"), true, true, parent)
{
    this->numTeams = numTeams;
    teamName = new QLineEdit[numTeams];

    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    //Table of team names
    theTable->setRowCount(numTeams);
    int widthCol0 = 0;
    for(int i = 0; i < numTeams; i++)
    {
        auto label = new QLabel(tr("Team ") + QString::number(i+1) + " ");
        theTable->setCellWidget(i, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamName[i].setPlaceholderText(tr("Custom name"));
        if(i < teamNames.size())
        {
            teamName[i].setText((teamNames.at(i) == QString::number(i+1))? "" : teamNames.at(i));
        }
        theTable->setCellWidget(i, 1, &teamName[i]);
    }
    theTable->horizontalHeader()->resizeSection(0, widthCol0 * TABLECOLUMN0OVERWIDTH);
    theTable->adjustSize();

    //A reset table button
    resetNamesButton = new QPushButton(this);
    resetNamesButton->setText(tr("&Clear All Names"));
    theGrid->addWidget(resetNamesButton, buttonBoxRowInGrid, 0, 1, 1);
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
    for(int i = 0; i < numTeams; i++)
    {
        teamName[i].clear();
    }
}
