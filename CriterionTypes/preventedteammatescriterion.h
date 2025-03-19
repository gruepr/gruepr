#ifndef PREVENTEDTEAMMATESCRITERION_H
#define PREVENTEDTEAMMATESCRITERION_H

#include "criterion.h"

class PreventedTeammatesCriterion : public Criterion {
public:
    PreventedTeammatesCriterion(float weight, bool penaltyStatus);
};

#endif // PREVENTEDTEAMMATESCRITERION_H
