#ifndef GENDERCRITERION_H
#define GENDERCRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include <QCheckBox>
#include <QPushButton>

class GenderCriterion : public Criterion {
    Q_OBJECT

public:
    GenderCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    const DataOptions *const dataOptions;
    QCheckBox *isolatedWomen = nullptr;
    QCheckBox *isolatedMen = nullptr;
    QCheckBox *isolatedNonbinary = nullptr;
    QCheckBox *mixedGender = nullptr;
//    QPushButton *complicatedGenderRule = nullptr;
};

#endif // GENDERCRITERION_H
