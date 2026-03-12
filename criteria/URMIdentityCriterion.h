#ifndef URMIDENTITYCRITERION_H
#define URMIDENTITYCRITERION_H

#include "criterion.h"
#include <QLabel>
#include <QPushButton>

class URMIdentityCriterion : public Criterion {
    Q_OBJECT

public:
    URMIdentityCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const override;
    // Need to override this one, because this criterion needs to see all teams for scoring any one team
    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;
    Criterion* clone() const override;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions;
    QPushButton *editRulesButton = nullptr;
    QLabel *ruleCountLabel = nullptr;
};

#endif // URMIDENTITYCRITERION_H
