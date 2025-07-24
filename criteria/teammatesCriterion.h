#ifndef TEAMMATESCRITERION_H
#define TEAMMATESCRITERION_H

#include "criterion.h"
#include <QPushButton>

class TeammatesCriterion : public Criterion {
public:
    using Criterion::Criterion;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;

    QPushButton *setTeammateRulesButton = nullptr;
};

#endif // TEAMMATESCRITERION_H
