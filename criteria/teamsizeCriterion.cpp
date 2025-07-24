#include "teamsizeCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"

void TeamsizeCriterion::generateCriteriaCard(const TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto teamSizeContentAreaLayout = new QHBoxLayout();
    teamSizeContentAreaLayout->setSpacing(2);

    teamSizeBox = new QComboBox(parentCard);
    teamSizeBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    teamSizeBox->setMinimumHeight(30);

    idealTeamSizeBox = new QSpinBox(parentCard);
    idealTeamSizeBox->setValue(4);
    idealTeamSizeBox->setMinimum(2);
    idealTeamSizeBox->setSuffix(tr(" students"));
    idealTeamSizeBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    idealTeamSizeBox->setMinimumHeight(30);
    idealTeamSizeBox->setMinimumWidth(50);

    teamSizeContentAreaLayout->addWidget(idealTeamSizeBox);
    teamSizeContentAreaLayout->addWidget(teamSizeBox);
    parentCard->setContentAreaLayout(*teamSizeContentAreaLayout);
}
