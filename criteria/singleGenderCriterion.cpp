#include "singleGenderCriterion.h"

SingleGenderCriterion::SingleGenderCriterion(const QString& genderName, float weight, bool penaltyStatus)
    : Criterion(weight, penaltyStatus), genderName(genderName) {}
