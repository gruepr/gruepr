#ifndef GRADEBALANCECRITERION_H
#define GRADEBALANCECRITERION_H

#include "criterion.h"
#include <QDoubleSpinBox>
#include <QLabel>

class GradeBalanceCriterion : public Criterion {
    Q_OBJECT

public:
    GradeBalanceCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                          GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions) {}
    Criterion* clone() const override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const override;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions = nullptr;
    QDoubleSpinBox *minimumMeanGradeSpinBox = nullptr;
    QDoubleSpinBox *maximumMeanGradeSpinBox = nullptr;
};

#endif // GRADEBALANCECRITERION_H
