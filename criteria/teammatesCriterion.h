#ifndef TEAMMATESCRITERION_H
#define TEAMMATESCRITERION_H

#include "criterion.h"
#include <QPushButton>

class TeammatesCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) override;

    QPushButton *setTeammateRulesButton = nullptr;
};

#endif // TEAMMATESCRITERION_H
