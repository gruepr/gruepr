#ifndef REQUESTEDTEAMMATESCRITERION_H
#define REQUESTEDTEAMMATESCRITERION_H

#include "criterion.h"

class RequestedTeammatesCriterion : public Criterion {
public:
    RequestedTeammatesCriterion(float weight, bool penaltyStatus);
};

#endif // REQUESTEDTEAMMATESCRITERION_H
