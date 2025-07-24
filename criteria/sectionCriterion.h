#ifndef SECTIONCRITERION_H
#define SECTIONCRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include <QPushButton>
#include <QComboBox>

class SectionCriterion : public Criterion {
public:
    using Criterion::Criterion;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;

    DataOptions *dataOptions = nullptr;
    QPushButton *editSectionNameButton = nullptr;
    QComboBox *sectionSelectionBox = nullptr;
};

#endif // SECTIONCRITERION_H
