#ifndef SINGLEURMIDENTITYCRITERION_H
#define SINGLEURMIDENTITYCRITERION_H

#include "criterion.h"
#include <QString>

class SingleURMIdentityCriterion : public Criterion {
public:
    QString urmName;

    SingleURMIdentityCriterion(const QString& urmName, int priority, bool penaltyStatus);
};

#endif // SINGLEURMIDENTITYCRITERION_H
