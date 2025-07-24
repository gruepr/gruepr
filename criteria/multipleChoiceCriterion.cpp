#include "multipleChoiceCriterion.h"

void MultipleChoiceCriterion::generateCriteriaCard(const TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
}
