#include "singlegendercriterion.h"

SingleGenderCriterion::SingleGenderCriterion(const QString& genderName, int priority, bool penaltyStatus)
    : Criterion(priority, penaltyStatus), genderName(genderName) {}
