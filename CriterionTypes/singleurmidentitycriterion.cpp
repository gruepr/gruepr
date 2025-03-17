#include "singleurmidentitycriterion.h"

SingleURMIdentityCriterion::SingleURMIdentityCriterion(const QString& urmName, int priority, bool penaltyStatus)
    : Criterion(priority, penaltyStatus), urmName(urmName) {}
