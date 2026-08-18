#ifndef URMIDENTITYCRITERION_H
#define URMIDENTITYCRITERION_H

#include "criterion.h"
#include <QLabel>
#include <QPushButton>
#include <vector>

class URMIdentityCriterion : public Criterion {
    Q_OBJECT

public:
    URMIdentityCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                         GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    // Caches identityRules' translation into integer form, plus each active student's precomputed identity slot
    void prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions *dataOptions) override;

    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;

    QStringList identityOptions() const;
    void updateRuleCountLabel() const;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    static QString exportStudentText(const StudentRecord &student);

    const DataOptions *const dataOptions;
    QPushButton *editRulesButton = nullptr;
    QLabel *ruleCountLabel = nullptr;

    QMap<QString, IdentityRule> identityRules;

private:
    enum RuleOp {opNotEqual, opLessThan, opGreaterThan};
    struct RuleGroup {
        qsizetype firstIdentity;    // into groupIdentities
        qsizetype numIdentities;
        RuleOp op;
        qsizetype firstValue;       // into ruleValues
        qsizetype numValues;
    };

    // Translates identityRules into a flat, fast-to-evaluate form. identityIndex maps each distinct
    // identity string named by any rule to a small integer slot -- the same identity can be named by
    // more than one rule, and they all share its slot. groupIdentities/ruleGroups/ruleValues then
    // record which slots and thresholds each rule group cares about. Cost is bounded by the number of
    // rules, not the number of students, so this is cheap enough to call fresh (see
    // scoreForOneTeamInDisplay) as well as once per optimization run (see prepareForOptimization).
    void buildRuleTranslation(QHash<QString, int> &identityIndex, QList<int> &groupIdentities,
                              QList<RuleGroup> &ruleGroups, QList<int> &ruleValues) const;

    // Given per-identity-slot counts for one team, applies every rule group's threshold check.
    // Returns whether any rule group's condition was met (i.e. a penalty situation); if so and
    // applyPenalty is true, also adds 1.0 to penaltyPointsForTeam per triggering rule group.
    static bool anyRuleGroupTriggered(const QList<int> &responseCounts, const QList<int> &groupIdentities,
                                      const QList<RuleGroup> &ruleGroups, const QList<int> &ruleValues,
                                      bool applyPenalty, float &penaltyPointsForTeam);

    // Cached in prepareForOptimization
    QList<int> cachedGroupIdentities;
    QList<RuleGroup> cachedRuleGroups;
    QList<int> cachedRuleValues;
    int cachedNumIdentitySlots = 0;

    // Cached in prepareForOptimization: each active student's precomputed identity slot (or -1 if
    // their response names no rule), indexed the same way calculateScore's teammates[] already is
    // (position in the original, unfiltered students[] array).
    std::vector<int> cachedIdentitySlot;
};

#endif // URMIDENTITYCRITERION_H
