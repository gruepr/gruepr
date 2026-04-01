#ifndef ASSIGNMENTPREFERENCECRITERION_H
#define ASSIGNMENTPREFERENCECRITERION_H

#include "criterion.h"
#include <QLabel>
#include <QList>
#include <QMap>

class AssignmentPreferenceCriterion : public Criterion {
    Q_OBJECT

public:
    AssignmentPreferenceCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                                  GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions) {};

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void prepareForOptimization(const StudentRecord *students, int numStudents, const DataOptions *dataOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    // Must override: assignment is inherently multi-team, so single-team display scoring needs the full assignment
    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    void prepareForDisplay(const QList<StudentRecord> &students, const TeamSet &teams) override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &students) const override;
    Qt::AlignmentFlag teamTextAlignment() const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &students) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions;

    // Whether to penalize teams where no member ranked the assigned option
    bool penalizeNoOneRanked = false;
    bool penalizeAnyOneUnranked = false;

    QLabel *infoLabel = nullptr;

    // Solved in prepareForDisplay, read in scoreForOneTeamInDisplay and teamDisplayText
    QMap<long long, QString> displayAssignment;   // team first-student-ID -> assigned option name
    QMap<long long, float> displayScore;          // team first-student-ID -> normalized score (0-1)
    QMap<long long, QString> displayStudentAssignment;  // student ID -> option name assigned to their team


private:
    // Cached in prepareForOptimization
    QStringList allOptionNames;                     // universe of option names discovered from student data
    QMap<QString, int> optionNameToIndex;            // option name -> index in allOptionNames
    int numOptions = 0;
    int numRankedChoices = 0;                        // k: how many choices each student ranked

    // Hungarian algorithm: solves min-cost assignment on a square cost matrix
    // Returns the column assigned to each row (result[row] = col)
    static QList<int> hungarianAlgorithm(const QList<QList<float>> &costMatrix);

    // Build utility matrix and solve assignment for a given set of teams
    // Returns a map from team index -> assigned option index
    // Also fills teamScores with the per-team normalized score
    QList<int> solveAssignment(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                     QList<float> &teamScores) const;

    // Cache for display: last solved assignment (team studentIDs hash -> option name)
    // Mutable because scoreForOneTeamInDisplay needs to cache results from a const-like context
    mutable QMap<long long, QString> lastAssignmentByTeamID;
    mutable QMap<long long, float> lastScoreByTeamID;
};

#endif // ASSIGNMENTPREFERENCECRITERION_H
