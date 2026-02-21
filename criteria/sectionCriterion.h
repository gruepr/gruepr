#ifndef SECTIONCRITERION_H
#define SECTIONCRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include <QComboBox>
#include <QPushButton>

class SectionCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const /*students*/, const int /*teammates*/[], const int /*numTeams*/, const int /*teamSizes*/[],
                                const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                std::vector<float> &/*criteriaScores*/, std::vector<int> &/*penaltyPoints*/) override {};

    DataOptions *dataOptions = nullptr;
    QPushButton *editSectionNameButton = nullptr;
    QComboBox *sectionSelectionBox = nullptr;
};

#endif // SECTIONCRITERION_H
