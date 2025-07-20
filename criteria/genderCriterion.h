#ifndef GENDERCRITERION_H
#define GENDERCRITERION_H

#include "criterion.h"
#include <QString>
class GenderCriterion : public Criterion {
public:
    GenderCriterion(float weight, bool penaltyStatus);
};

#endif // GENDERCRITERION_H
