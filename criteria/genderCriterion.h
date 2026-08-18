#ifndef GENDERCRITERION_H
#define GENDERCRITERION_H

#include "criterion.h"
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <vector>

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

    // Caches identityRules' translation into integer form, plus each active student's gender identities folded into a single bitmask
    void prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions *dataOptions) override;

    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;

    QStringList identityOptions() const;
    void updateComplicatedRuleCountLabel() const;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    static QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions);

    const DataOptions *const dataOptions;
    QCheckBox *isolatedWomen = nullptr;
    QCheckBox *isolatedMen = nullptr;
    QCheckBox *isolatedNonbinary = nullptr;
    QCheckBox *mixedGender = nullptr;
    QPushButton *complicatedGenderRule = nullptr;
    QLabel *complicatedRuleCountLabel = nullptr;

    QMap<QString, IdentityRule> identityRules;   // key: "Woman" or "Man|Nonbinary"

    const QString womanKey = grueprGlobal::genderToString(Gender::woman);
    const QString manKey = grueprGlobal::genderToString(Gender::man);
    const QString nonbinaryKey = grueprGlobal::genderToString(Gender::nonbinary);

private:
    enum RuleOp {opNotEqual, opLessThan, opGreaterThan};
    struct RuleGroup {
        int identityMask;           // one bit per Gender, via genderBit()
        RuleOp op;
        qsizetype firstValue;       // into ruleValues
        qsizetype numValues;
    };

    // Translates identityRules into integer form. Called by prepareForOptimization (caching the
    // result into cachedRuleGroups/cachedRuleValues below) and, fresh/uncached, by
    // scoreForOneTeamInDisplay -- so the rule logic lives in exactly one place either way.
    void buildRuleTranslation(QList<RuleGroup> &ruleGroups, QList<int> &ruleValues) const;

    // Given one team's gender counts, applies every rule group's threshold check. Returns whether any
    // rule group's condition was met; if so and applyPenalty is true, also adds 1.0 to
    // penaltyPointsForTeam per triggering rule group.
    static bool anyRuleGroupTriggered(int numWomen, int numMen, int numNonbinary, const QList<RuleGroup> &ruleGroups,
                                      const QList<int> &ruleValues, bool applyPenalty, float &penaltyPointsForTeam);

    // Cached in prepareForOptimization
    QList<RuleGroup> cachedRuleGroups;
    QList<int> cachedRuleValues;

    // Cached in prepareForOptimization: each active student's gender identities folded into a single
    // bitmask (one bit per Gender, via genderBit()), indexed the same way calculateScore's teammates[]
    // already is (position in the original, unfiltered students[] array).
    std::vector<uint8_t> cachedGenderMask;
};

#endif // GENDERCRITERION_H
