#ifndef GENDERCRITERION_H
#define GENDERCRITERION_H

#include "criterion.h"
#include <QCheckBox>
#include <QPushButton>

class GenderCriterion : public Criterion {
    Q_OBJECT

public:
    GenderCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                    GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        std::vector<float> & criteriaScores, std::vector<int> &penaltyPoints) const override;

    QStringList identityOptions() const;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions;
    QCheckBox *isolatedWomen = nullptr;
    QCheckBox *isolatedMen = nullptr;
    QCheckBox *isolatedNonbinary = nullptr;
    QCheckBox *mixedGender = nullptr;
    QPushButton *complicatedGenderRule = nullptr;

    QMap<QString, IdentityRule> identityRules;   // key: "Woman" or "Man|Nonbinary"

    const QString womanKey = grueprGlobal::genderToString(Gender::woman);
    const QString manKey = grueprGlobal::genderToString(Gender::man);
    const QString nonbinaryKey = grueprGlobal::genderToString(Gender::nonbinary);
};

#endif // GENDERCRITERION_H
