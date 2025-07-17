#ifndef REQUIREDTEAMMATESCRITERION_H
#define REQUIREDTEAMMATESCRITERION_H

#include "criterion.h"

class RequiredTeammatesCriterion : public Criterion {
public:
    RequiredTeammatesCriterion(float weight, bool penaltyStatus);
};

#endif // REQUIREDTEAMMATESCRITERION_H
