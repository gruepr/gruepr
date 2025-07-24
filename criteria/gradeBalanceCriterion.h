#ifndef GRADEBALANCECRITERION_H
#define GRADEBALANCECRITERION_H

#include "criteria/criterion.h"
class GradeBalanceCriterion : public Criterion
{
public:
    using Criterion::Criterion;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;
};

#endif // GRADEBALANCECRITERION_H
