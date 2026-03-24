#ifndef SECTIONCRITERION_H
#define SECTIONCRITERION_H

#include "criterion.h"
#include "widgets/styledComboBox.h"
#include <QPushButton>

class SectionCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    Criterion* clone() const override { return nullptr; }

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const /*students*/, const int /*teammates*/[], const int /*numTeams*/, const int /*teamSizes*/[],
                                const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                std::vector<float> &/*criteriaScores*/, std::vector<int> &/*penaltyPoints*/) const override {};

    QString headerLabel(const DataOptions *) const override { return {}; }
    Qt::TextElideMode headerElideMode() const override { return Qt::ElideNone; }
    QString teamDisplayText(const TeamRecord &, const DataOptions *, float, const QList<StudentRecord> &) const override { return {}; }
    QVariant teamSortValue(const TeamRecord &, const DataOptions *, float, const QList<StudentRecord> &) const override { return 0; }
    QString studentDisplayText(const StudentRecord &, const DataOptions *) const override { return {}; }

    DataOptions *dataOptions = nullptr;
    QPushButton *editSectionNameButton = nullptr;
    StyledComboBox *sectionSelectionBox = nullptr;
};

#endif // SECTIONCRITERION_H
