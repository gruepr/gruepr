#ifndef MULTIPLECHOICESTYLECRITERION_H
#define MULTIPLECHOICESTYLECRITERION_H

#include "criterion.h"
#include "dataOptions.h"

class MultipleChoiceStyleCriterion : public Criterion {
public:
    DataOptions::AttributeType typeOfAttribute;
    int attributeIndex;

    MultipleChoiceStyleCriterion(float weight, bool penaltyStatus, DataOptions::AttributeType typeOfAttribute, int attributeIndex);
};

#endif // MULTIPLECHOICESTYLECRITERION_H
