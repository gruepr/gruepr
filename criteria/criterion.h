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
    enum class CriteriaType {section, teamSize, genderIdentity, urmIdentity, attributeQuestion,
                             scheduleMeetingTimes, groupTogether, splitApart, assignmentPreference};
    Q_ENUM(CriteriaType)
    static int resolveCriteriaTypeKey(const QMetaEnum &e, const QString &name);  // needed to migrate old enum names to new

    enum class AttributeDiversity {diverse, ignored, similar, average};  // diverse = heterogeneous (i.e., teammates have a range of values)
                                                                      // similar = homogeneous (i.e., all teammates have the same value)
                                                                      // average = each team aims to have an average value near the population average
    Q_ENUM(AttributeDiversity)

    using IdentityRule = QMap<QString, QList<int>>;             // A map from a logic operation (e.g., "!=") to a set of values (e.g., 1, 2)

    Criterion(CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false, GroupingCriteriaCard *parent = nullptr) :
        weight(weight), penaltyStatus(penaltyStatus), criteriaType(criteriaType), parentCard(parent) {};
    ~Criterion() override = default;

    // function to create criterion copies owned by each teamsTabWidget
    virtual Criterion* clone() const = 0;
    virtual QJsonObject settingsToJson() const;
    virtual void settingsFromJson(const QJsonObject &json);

    // generate the UI for the criterion display in gruepr
    virtual void generateCriteriaCard(TeamingOptions *const teamingOptions) = 0;

    // called once before optimization begins to cache any values derived from the student data
    virtual void prepareForOptimization(const StudentRecord */*students*/, int /*numStudents*/, const DataOptions */*dataOptions*/) {}

    // calculate the score for the criterion for all the teams in a genome, used in the optimization algorithm
    virtual void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                QList<float> &criteriaScores, QList<float> &penaltyPoints) const = 0;

    // a convenience wrapper around calculateScore to calculate for one team, used to color the TeamTree display
    virtual float scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                           const DataOptions *dataOptions, const QSet<long long> &allIDsBeingTeamed = {});

    static constexpr float NO_SCORE = std::numeric_limits<float>::quiet_NaN();
    static inline bool IS_NO_SCORE(float score) { return std::isnan(score); }

    // Description of this criterion's settings for the teaming options summary in exports
    virtual QString exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions */*dataOptions*/) const { return {}; }

    // Per-student data for this criterion in export files (fixed-width formatted)
    virtual QString exportStudentText(const StudentRecord &/*student*/, const DataOptions */*dataOptions*/) const { return {}; }

    float weight;
    enum class Precedence {fixed, need, want} precedence = Precedence::want;
    bool penaltyStatus;
    CriteriaType criteriaType;

    // functions for displaying the criterion results in the TeamTree data display
    virtual QString headerLabel(const DataOptions *dataOptions) const = 0;
    virtual Qt::TextElideMode headerElideMode() const = 0;
    virtual void prepareForDisplay(const QList<StudentRecord> &/*students*/, const TeamSet &/*teams*/) {}
    virtual QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const = 0;
    virtual QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const = 0;
    virtual Qt::AlignmentFlag teamTextAlignment() const { return Qt::AlignCenter; }
    virtual QColor teamDisplayColor(float criterionScore) const;    // default is a 0->1 red->green gradient
    virtual QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const = 0;
    virtual Qt::AlignmentFlag studentTextAlignment() const { return Qt::AlignCenter; }

protected:
    GroupingCriteriaCard *parentCard;
};


/*
 * PROCESS TO CREATE A NEW TYPE OF CRITERION:

1. Add enum value in criterion.h → CriteriaType enum
2. Create the subclass — new .h and .cpp files inheriting from Criterion. Implement all required methods:

    generateCriteriaCard — builds the UI card
    calculateScore — batch scoring for the GA
    clone — returns a copy for team tabs
    headerLabel — column header text
    headerElideMode — how to elide the header
    teamDisplayText — what to show in the team row
    teamSortValue — sort key for the column
    studentDisplayText — what to show in the student row
    exportTeamingOptionText — text for the instructor export header
    exportStudentText — per-student text for the instructor export
    Optionally override: teamTextAlignment, studentTextAlignment, teamDisplayColor, scoreForOneTeamInDisplay, settingsToJson, settingsFromJson

3. gruepr.cpp — add menu action in the constructor, add case in addCriteriaCard, add case in deleteCriteriaCard
4. teamsTabItem.cpp — add #include for the new subclass, add case in restoreCriteria's switch
5. gruepr.h — add member pointers if the criterion is singleton (like gender/URM), or add to a tracking list if multiple can exist
6. Add to .pro — include the new source files


*/

#endif // CRITERION_H
