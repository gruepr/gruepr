#ifndef TEAMMATESCRITERION_H
#define TEAMMATESCRITERION_H

#include "criterion.h"
#include <QLabel>
#include <QPushButton>

class TeammatesCriterion : public Criterion {
    Q_OBJECT

public:
    using Criterion::Criterion;

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

    static TeammatesCriterion* findInCriteria(const TeamingOptions *teamingOptions, CriteriaType type);

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    QPushButton *setTeammateRulesButton = nullptr;
    QLabel *pairingCountLabel = nullptr;

    bool haveAnyTeammates = false;              // Do any of the students being teamed have any of these teammates?
    int numberGiven = REQUESTED_TEAMMATES_ALL;  // For groupTogether: at least how many of the requested teammates should we place on a student's team

private:
    int scoreOneTeam(const QList<const StudentRecord *> &teamMembers, const QSet<long long> &idsOnTeam,
                     const QSet<long long> &idsBeingTeamed, const TeamingOptions *const teamingOptions) const;
};

#endif // TEAMMATESCRITERION_H
