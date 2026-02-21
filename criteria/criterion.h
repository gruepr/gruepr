#ifndef CRITERION_H
#define CRITERION_H

#include "dataOptions.h"
#include "studentRecord.h"
#include <QObject>

class GroupingCriteriaCard;
class TeamingOptions;

class Criterion : public QObject {
    Q_OBJECT

public:
    enum class CriteriaType {section, teamSize, genderIdentity, urmIdentity, attributeQuestion, scheduleMeetingTimes,
                             requiredTeammates, preventedTeammates, requestedTeammates, gradeBalance};
    enum class Precedence{fixed, need, want};
    enum class AttributeDiversity {diverse, ignored, similar};  // diverse = heterogeneous (i.e., teammates have a range of values)
                                                                // similar = homogeneous (i.e., all teammates have the same value)
    Q_ENUM(AttributeDiversity)

    float weight;
    Precedence precedence = Precedence::want;
    bool penaltyStatus;
    CriteriaType criteriaType;

    Criterion(CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        weight(weight), penaltyStatus(penaltyStatus), criteriaType(criteriaType), parentCard(parent) {};
    ~Criterion() override = default;

    virtual void generateCriteriaCard(TeamingOptions *const teamingOptions) = 0;
    virtual void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) = 0;

protected:
    GroupingCriteriaCard *parentCard;
};

#endif // CRITERION_H
