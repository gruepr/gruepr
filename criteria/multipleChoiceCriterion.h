#ifndef MULTIPLECHOICECRITERION_H
#define MULTIPLECHOICECRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include "widgets/attributeWidget.h"

class MultipleChoiceCriterion : public Criterion {
public:
    MultipleChoiceCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    const DataOptions *const dataOptions;
    DataOptions::AttributeType typeOfAttribute;
    int attributeIndex;
    AttributeWidget *attributeWidgets = nullptr;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;
};

#endif // MULTIPLECHOICECRITERION_H
