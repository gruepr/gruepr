#ifndef TEAMSIZECRITERION_H
#define TEAMSIZECRITERION_H

#include "criterion.h"
#include <QComboBox>
#include <QSpinBox>

class TeamsizeCriterion : public Criterion {
public:
    using Criterion::Criterion;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;

    QComboBox *teamSizeBox = nullptr;
    QSpinBox *idealTeamSizeBox = nullptr;
};

#endif // TEAMSIZECRITERION_H
