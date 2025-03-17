#include "multiplechoicestylecriterion.h"

MultipleChoiceStyleCriterion::MultipleChoiceStyleCriterion(int priority, bool penaltyStatus, AttributeType typeOfAttribute, int attributeIndex)
    : Criterion(priority, penaltyStatus), typeOfAttribute(typeOfAttribute), attributeIndex(attributeIndex) {}
