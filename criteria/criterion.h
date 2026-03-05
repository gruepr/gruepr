#ifndef CRITERION_H
#define CRITERION_H

#include "dataOptions.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include <QMetaEnum>
#include <QObject>

class GroupingCriteriaCard;
class TeamingOptions;

class Criterion : public QObject {
    Q_OBJECT

public:
    enum class CriteriaType {section, teamSize, genderIdentity, urmIdentity, attributeQuestion, scheduleMeetingTimes,
                             groupTogether, splitApart, gradeBalance};
    Q_ENUM(CriteriaType)
    static int resolveCriteriaTypeKey(const QMetaEnum &e, const QString &name);  // needed to migrate old enum names to new

    enum class AttributeDiversity {diverse, ignored, similar};  // diverse = heterogeneous (i.e., teammates have a range of values)
                                                                // similar = homogeneous (i.e., all teammates have the same value)
    Q_ENUM(AttributeDiversity)

    Criterion(CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        weight(weight), penaltyStatus(penaltyStatus), criteriaType(criteriaType), parentCard(parent) {};
    ~Criterion() override = default;

    // generate the UI for the criterion display in gruepr
    virtual void generateCriteriaCard(TeamingOptions *const teamingOptions) = 0;

    // calculate the score for the criterion for all the teams in a genome, used in the optimization algorithm
    virtual void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const = 0;

    // a convenience wrapper around calculateScore to calculate for one team, used to color the TeamTree display
    virtual float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                           const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed = {});

    static constexpr float NO_SCORE = std::numeric_limits<float>::quiet_NaN();
    static inline bool IS_NO_SCORE(float score) { return std::isnan(score); }

    float weight;
    enum class Precedence {fixed, need, want} precedence = Precedence::want;
    bool penaltyStatus;
    CriteriaType criteriaType;

    // functions for displaying the criterion results in the TeamTree data display
    virtual QString headerLabel(const DataOptions *dataOptions) const = 0;
    virtual Qt::TextElideMode headerElideMode() const = 0;
    virtual QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const = 0;
    virtual QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore = 0) const = 0;
    virtual Qt::AlignmentFlag teamTextAlignment() const { return Qt::AlignCenter; }
    virtual QColor teamDisplayColor(float criterionScore) const;    // default is a 0->1 red->green gradient
    virtual QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const = 0;
    virtual Qt::AlignmentFlag studentTextAlignment() const { return Qt::AlignCenter; }

protected:
    GroupingCriteriaCard *parentCard;
};

#endif // CRITERION_H
