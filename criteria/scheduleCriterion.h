#ifndef SCHEDULECRITERION_H
#define SCHEDULECRITERION_H

#include "criterion.h"
#include "dataOptions.h"
#include <QDoubleSpinBox>
#include <QSpinBox>

class ScheduleCriterion : public Criterion {
public:
    ScheduleCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    void generateCriteriaCard(const TeamingOptions *const teamingOptions) override;

    const DataOptions *const dataOptions;
    QSpinBox *minMeetingTimes = nullptr;
    QSpinBox *desiredMeetingTimes = nullptr;
    QDoubleSpinBox *meetingLengthSpinBox = nullptr;
};

#endif // SCHEDULECRITERION_H
