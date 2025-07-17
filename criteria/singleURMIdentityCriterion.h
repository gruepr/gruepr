#ifndef SINGLEURMIDENTITYCRITERION_H
#define SINGLEURMIDENTITYCRITERION_H

#include "criterion.h"
#include <QString>

class SingleURMIdentityCriterion : public Criterion {
public:
    QString urmName;

    SingleURMIdentityCriterion(const QString& urmName, float weight, bool penaltyStatus);
};

#endif // SINGLEURMIDENTITYCRITERION_H
