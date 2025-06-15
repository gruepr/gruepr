#ifndef GRADEBALANCECRITERION_H
#define GRADEBALANCECRITERION_H

#include "CriterionTypes/criterion.h"
class GradeBalanceCriterion : public Criterion
{
public:
    GradeBalanceCriterion(float weight, int penaltyStatus);
};

#endif // GRADEBALANCECRITERION_H
