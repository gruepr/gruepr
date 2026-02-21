#ifndef TEAMSIZECRITERION_H
#define TEAMSIZECRITERION_H

#include "criterion.h"
#include <QComboBox>
#include <QSpinBox>

class TeamsizeCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const /*students*/, const int /*teammates*/[], const int /*numTeams*/, const int /*teamSizes*/[],
                        const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                        std::vector<float> &/*criteriaScores*/, std::vector<int> &/*penaltyPoints*/) override {};

    QComboBox *teamSizeBox = nullptr;
    QSpinBox *idealTeamSizeBox = nullptr;
};

#endif // TEAMSIZECRITERION_H
