#include "singleurmidentitycriterion.h"

SingleURMIdentityCriterion::SingleURMIdentityCriterion(const QString& urmName, float weight, bool penaltyStatus)
    : Criterion(weight, penaltyStatus), urmName(urmName) {}
