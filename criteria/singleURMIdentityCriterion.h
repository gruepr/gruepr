#ifndef SINGLEURMIDENTITYCRITERION_H
#define SINGLEURMIDENTITYCRITERION_H

#include "criterion.h"
#include <QString>

class SingleURMIdentityCriterion : public Criterion {
public:
    using Criterion::Criterion;

    QString urmName;

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;
};

#endif // SINGLEURMIDENTITYCRITERION_H
