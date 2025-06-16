#ifndef GRADEBALANCECRITERION_H
#define GRADEBALANCECRITERION_H

#include "criteria/criterion.h"
class GradeBalanceCriterion : public Criterion
{
public:
    GradeBalanceCriterion(float weight, int penaltyStatus);
};

#endif // GRADEBALANCECRITERION_H
