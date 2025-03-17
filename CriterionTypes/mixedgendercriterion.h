#ifndef MIXEDGENDERCRITERION_H
#define MIXEDGENDERCRITERION_H

#include "criterion.h"

class MixedGenderCriterion : public Criterion {
public:
    MixedGenderCriterion(int priority, int penaltyStatus);
};

#endif // MIXEDGENDERCRITERION_H
