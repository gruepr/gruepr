#ifndef CRITERION_H
#define CRITERION_H

#include <QObject>

class GroupingCriteriaCard;
class TeamingOptions;

class Criterion : public QObject {
    Q_OBJECT

public:
    enum class CriteriaType {section, teamSize, genderIdentity, urmIdentity, attributeQuestion, scheduleMeetingTimes, requiredTeammates, preventedTeammates, requestedTeammates, gradeBalance};
    enum class Precedence{need, want};
    enum class AttributeDiversity {diverse, ignored, similar};  // diverse = heterogeneous (i.e., teammates have a range of values)
                                                                // similar = homogeneous (i.e., all teammates have the same value)
    Q_ENUM(AttributeDiversity)

    float weight;
    Precedence precedence = Precedence::want;
    bool penaltyStatus;
    CriteriaType criteriaType;

    Criterion(CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
                weight(weight), penaltyStatus(penaltyStatus), criteriaType(criteriaType), parentCard(parent) {};
    virtual ~Criterion() = default;

    virtual void generateCriteriaCard(const TeamingOptions *const teamingOptions) = 0;

protected:
    GroupingCriteriaCard *parentCard;
};

#endif // CRITERION_H
