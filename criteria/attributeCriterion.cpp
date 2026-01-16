#include "attributeCriterion.h"
#include "widgets/groupingCriteriaCardWidget.h"

void AttributeCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto *attributeContentLayout = new QHBoxLayout();
    attributeWidget = new AttributeWidget(attributeIndex, dataOptions, teamingOptions, parentCard);
    attributeContentLayout->addWidget(attributeWidget);

    parentCard->setContentAreaLayout(*attributeContentLayout);
}
