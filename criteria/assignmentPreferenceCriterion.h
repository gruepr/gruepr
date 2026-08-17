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
    void prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions *dataOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    // Must override: assignment is inherently multi-team, so single-team display scoring needs the full assignment
    float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                   const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed) override;

    // Assignment is a joint property of the whole team set (a swap anywhere can change the optimal
    // assignment everywhere), so no per-team rescoring could ever be correct -- opt out of the hill
    // climb's single-team scoring entirely rather than have a per-team score silently mean the wrong thing.
    bool supportsSingleTeamScoring() const override { return false; }
    float scoreForOneTeamInOptimization(const StudentRecord *const students, const int teamRoster[], const int teamSize,
                                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                        float &penaltyPoints) const override;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    void prepareForDisplay(const QList<StudentRecord> &students, const TeamSet &teams, const TeamingOptions *teamingOptions) override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &students) const override;
    Qt::AlignmentFlag teamTextAlignment() const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &students) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    static QString exportStudentText(const StudentRecord &student);

    const DataOptions *const dataOptions;

    // Whether to penalize teams where no member ranked the assigned option
    bool penalizeNoOneRanked = false;
    bool penalizeAnyOneUnranked = false;

    QLabel *infoLabel = nullptr;

    // Solved in prepareForDisplay, read in scoreForOneTeamInDisplay, teamDisplayText, and teamSortValue
    QMap<long long, QString> displayAssignment;   // team first-student-ID -> assigned option name
    QMap<long long, int> displayOptionIndex;      // team first-student-ID -> assigned option's index in allOptionNames (sort key)
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

    // Solve the assignment for one independent pool of teams (e.g., one section when sections were teamed
    // separately) and merge the results into displayAssignment/displayScore/displayStudentAssignment
    void solveAndCacheDisplayForTeams(const QList<StudentRecord> &students, const QList<TeamRecord> &teams);

    // Cache for display: last solved assignment (team studentIDs hash -> option name)
    // Mutable because scoreForOneTeamInDisplay needs to cache results from a const-like context
    mutable QMap<long long, QString> lastAssignmentByTeamID;
    mutable QMap<long long, float> lastScoreByTeamID;
};

#endif // ASSIGNMENTPREFERENCECRITERION_H
