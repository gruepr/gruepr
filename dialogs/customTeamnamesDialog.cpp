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
    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    //Table of team names
    theTable->setRowCount(numTeams);
    int widthCol0 = 0, rowHeight = 0;
    teamNameLineEdits.reserve(numTeams);
    for(int team = 0; team < numTeams; team++) {
        auto *label = new QLabel(tr("Team ") + QString::number(team+1) + " ", this);
        label->setStyleSheet(LABEL10PTSTYLE);
        theTable->setCellWidget(team, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamNameLineEdits << new QLineEdit(this);
        teamNameLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
        teamNameLineEdits.last()->setPlaceholderText(tr("Team ") + QString::number(team+1));
        if(team < incomingTeamNames.size()) {
            teamNameLineEdits.last()->setText((incomingTeamNames.at(team) == QString::number(team+1))? "" : incomingTeamNames.at(team));
        }
        connect(teamNameLineEdits.last(), &QLineEdit::textChanged, this, [this, numTeams](){teamNames.clear();
                                                                                  for(int i = 0; i < numTeams; i++)
                                                                                    {teamNames << (teamNameLineEdits[i]->text().isEmpty()?
                                                                                                  QString::number(i+1) : teamNameLineEdits[i]->text());}});
        theTable->setCellWidget(team, 1, teamNameLineEdits.last());
        rowHeight = std::max(rowHeight, std::max(label->height(), teamNameLineEdits.last()->height()));
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLEOVERSIZE));
    for(int i = 0; i < numTeams; i++) {
        theTable->verticalHeader()->resizeSection(i, int(float(rowHeight) * TABLEOVERSIZE));
    }
    theTable->adjustSize();

    //A reset table button
    auto *resetNamesButton = new QPushButton(tr("Clear All"), this);
    resetNamesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(resetNamesButton, &QPushButton::clicked, this, &customTeamnamesDialog::clearAllNames);
    addButton(resetNamesButton);

    adjustSize();
}


void customTeamnamesDialog::clearAllNames()
{
    for(auto &teamNameLineEdit : teamNameLineEdits) {
        teamNameLineEdit->clear();
    }
}
