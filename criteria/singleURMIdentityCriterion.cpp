#include "singleURMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"

void SingleURMIdentityCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *urmContentLayout = new QVBoxLayout();
    auto *urmNameLabel = new QLineEdit(urmName, parentCard);
    complicatedURMRule = new QPushButton(tr("Something more complicated"), parentCard);
    complicatedURMRule->setFixedHeight(40);
    complicatedURMRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    urmContentLayout->addWidget(urmNameLabel);
    urmContentLayout->addWidget(complicatedURMRule);
    parentCard->setContentAreaLayout(*urmContentLayout);
}
