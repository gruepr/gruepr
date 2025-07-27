#ifndef GRADEBALANCECRITERION_H
#define GRADEBALANCECRITERION_H

#include "criteria/criterion.h"
class GradeBalanceCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
};

#endif // GRADEBALANCECRITERION_H
