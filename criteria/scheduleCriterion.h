#ifndef SCHEDULECRITERION_H
#define SCHEDULECRITERION_H

#include "criterion.h"

class ScheduleCriterion : public Criterion {
public:
    ScheduleCriterion(float weight, bool penaltyStatus);
};

#endif // SCHEDULECRITERION_H
