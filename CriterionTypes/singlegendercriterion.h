#ifndef SINGLEGENDERCRITERION_H
#define SINGLEGENDERCRITERION_H

#include "criterion.h"
#include <QString>
class SingleGenderCriterion : public Criterion {
public:
    QString genderName;

    SingleGenderCriterion(const QString& genderName, int priority, bool penaltyStatus);
};

#endif // SINGLEGENDERCRITERION_H
