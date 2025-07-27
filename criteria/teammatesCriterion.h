#ifndef TEAMMATESCRITERION_H
#define TEAMMATESCRITERION_H

#include "criterion.h"
#include <QPushButton>

class TeammatesCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    QPushButton *setTeammateRulesButton = nullptr;
};

#endif // TEAMMATESCRITERION_H
