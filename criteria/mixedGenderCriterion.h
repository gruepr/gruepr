#ifndef MIXEDGENDERCRITERION_H
#define MIXEDGENDERCRITERION_H

#include "criterion.h"

class MixedGenderCriterion : public Criterion {
public:
    MixedGenderCriterion(float weight, bool penaltyStatus);
};

#endif // MIXEDGENDERCRITERION_H
