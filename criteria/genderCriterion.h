#ifndef GENDERCRITERION_H
#define GENDERCRITERION_H

#include "criterion.h"
#include <QCheckBox>
#include <QPushButton>

class GenderCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    QCheckBox *isolatedWomen = nullptr;
    QCheckBox *isolatedMen = nullptr;
    QCheckBox *isolatedNonbinary = nullptr;
    QCheckBox *mixedGender = nullptr;
    QPushButton *complicatedGenderRule = nullptr;
};

#endif // GENDERCRITERION_H
