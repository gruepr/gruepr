#ifndef SINGLEURMIDENTITYCRITERION_H
#define SINGLEURMIDENTITYCRITERION_H

#include "criterion.h"
#include <QPushButton>
#include <QString>

class SingleURMIdentityCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    QString urmName;
    QPushButton *complicatedURMRule = nullptr;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
};

#endif // SINGLEURMIDENTITYCRITERION_H
