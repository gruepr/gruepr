#ifndef URMIDENTITYCRITERION_H
#define URMIDENTITYCRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include <QCheckBox>
#include <QPushButton>

class URMIdentityCriterion : public Criterion {
    Q_OBJECT

public:
    URMIdentityCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    const DataOptions *const dataOptions;
    QCheckBox *isolatedURM = nullptr;
    QPushButton *chooseURMResponses = nullptr;
//    QPushButton *complicatedURMRule = nullptr;
};

#endif // URMIDENTITYCRITERION_H
