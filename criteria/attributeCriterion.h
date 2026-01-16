#ifndef ATTRIBUTECRITERION_H
#define ATTRIBUTECRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include "widgets/attributeWidget.h"

class AttributeCriterion : public Criterion {
    Q_OBJECT

public:
    AttributeCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr, const int attribute = 0) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions), attributeIndex(attribute){ };

    const DataOptions *const dataOptions;
    DataOptions::AttributeType typeOfAttribute;
    const int attributeIndex;
    AttributeWidget *attributeWidget = nullptr;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
};

#endif // ATTRIBUTECRITERION_H
