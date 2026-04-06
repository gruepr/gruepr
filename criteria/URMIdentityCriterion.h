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

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    // Need to override this one, because this criterion needs to see all teams for scoring any one team
    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;
    QStringList identityOptions() const;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions;
    QPushButton *editRulesButton = nullptr;
    QLabel *ruleCountLabel = nullptr;

    QMap<QString, IdentityRule> identityRules;
};

#endif // URMIDENTITYCRITERION_H
