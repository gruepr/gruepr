#include "teammatesCriterion.h"
#include "gruepr_globals.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QVBoxLayout>

void TeammatesCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *requiredTeammatesContentAreaLayout = new QVBoxLayout();

    QString type;
    switch(criteriaType) {
    case Criterion::CriteriaType::requiredTeammates:
        type = " Required ";
        break;
    case Criterion::CriteriaType::preventedTeammates:
        type = " Prevented ";
        break;
    case Criterion::CriteriaType::requestedTeammates:
        type = " Requested ";
        break;
    default:
        return;
    }

    setTeammateRulesButton = new QPushButton(tr("Set") + type + tr("Teammate Rules"), parentCard);
    setTeammateRulesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setTeammateRulesButton->setMinimumHeight(30);
    setTeammateRulesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    requiredTeammatesContentAreaLayout->addWidget(setTeammateRulesButton);
    parentCard->setContentAreaLayout(*requiredTeammatesContentAreaLayout);
}

