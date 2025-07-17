#include "multipleChoiceStyleCriterion.h"

MultipleChoiceStyleCriterion::MultipleChoiceStyleCriterion(float weight, bool penaltyStatus, DataOptions::AttributeType typeOfAttribute, int attributeIndex)
    : Criterion(weight, penaltyStatus), typeOfAttribute(typeOfAttribute), attributeIndex(attributeIndex) {}
