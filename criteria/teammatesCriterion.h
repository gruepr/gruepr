#ifndef TEAMMATESCRITERION_H
#define TEAMMATESCRITERION_H

#include "criterion.h"
#include <QLabel>
#include <QPushButton>
#include <cstdint>
#include <vector>

class TeammatesCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;

    // Caches the full-active-roster "being teamed" stamp once, so calculateScore doesn't need to
    // rebuild it from teammates[]/numTeams on every single call during optimization.
    void prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions *dataOptions) override;

    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    // Need to override this one, because this criterion needs to see all teams for scoring any one team
    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;

    static TeammatesCriterion* findInCriteria(const TeamingOptions *teamingOptions, CriteriaType type);

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;

    QPushButton *setTeammateRulesButton = nullptr;
    QLabel *pairingCountLabel = nullptr;

    bool haveAnyTeammates = false;              // Do any of the students being teamed have any of these teammates?
    int numberGiven = REQUESTED_TEAMMATES_ALL;  // For groupTogether: at least how many of the requested teammates should we place on a student's team

private:
    // idsOnTeam/idsBeingTeamed are generation-stamped membership sets for student IDs (dense, assigned
    // in load order): idsOnTeamStamp[id] == idsOnTeamGeneration means "on this team," and analogously
    // for idsBeingTeamed -- O(1) array indexing instead of QSet<long long> hashing. See calculateScore().
    int scoreOneTeam(const QList<const StudentRecord *> &teamMembers,
                     const std::vector<uint32_t> &idsOnTeamStamp, uint32_t idsOnTeamGeneration,
                     const std::vector<uint32_t> &idsBeingTeamedStamp, uint32_t idsBeingTeamedGeneration) const;

    // Cached once in prepareForOptimization: every active student's ID, generation-stamped the same
    // way as calculateScore's own onTeamStamp. Fixed for the whole optimization run (the active
    // roster doesn't change generation to generation), so calculateScore can read it directly
    // instead of rebuilding it from teammates[]/numTeams on every call.
    std::vector<uint32_t> cachedBeingTeamedStamp;
    uint32_t cachedBeingTeamedGeneration = 0;
};

#endif // TEAMMATESCRITERION_H
