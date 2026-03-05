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
                        std::vector<float> &/*criteriaScores*/, std::vector<int> &/*penaltyPoints*/) const override {};

    QString headerLabel(const DataOptions *) const override { return {}; }
    Qt::TextElideMode headerElideMode() const override { return Qt::ElideNone; }
    QString teamDisplayText(const TeamRecord &, const DataOptions *, float = 0) const override { return {}; }
    QVariant teamSortValue(const TeamRecord &, const DataOptions *, float = 0) const override { return 0; }
    QString studentDisplayText(const StudentRecord &, const DataOptions *) const override { return {}; }

    QComboBox *teamSizeBox = nullptr;
    QSpinBox *idealTeamSizeBox = nullptr;
};

#endif // TEAMSIZECRITERION_H
