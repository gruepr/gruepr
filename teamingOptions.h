#ifndef TEAMINGOPTIONS_H
#define TEAMINGOPTIONS_H

#include "gruepr_globals.h"
#include <QJsonObject>
#include <QObject>
#include <QStringList>

//the teaming options set by the user when forming teams

class TeamingOptions
{
public:
    enum class AttributeDiversity {
        HOMOGENOUS,
        HETEROGENOUS,
        IGNORED
    };

    TeamingOptions();
    explicit TeamingOptions(const QJsonObject &jsonTeamingOptions);
    void reset();

    QJsonObject toJson() const;

    bool isolatedWomenPrevented = false;                // if true, will prevent teams with an isolated woman
    bool isolatedMenPrevented = false;                  // if true, will prevent teams with an isolated man
    bool isolatedNonbinaryPrevented = false;            // if true, will prevent teams with an isolated nonbinary student
    bool singleGenderPrevented = false;                 // if true, will penalize teams with all men or all women
    bool isolatedURMPrevented = false;                  // if true, will prevent teams with an isolated URM student
    QStringList URMResponsesConsideredUR;               // the list of responses to the race/ethnicity/culture question that are considered underrepresented
    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    float meetingBlockSize = 1;                         // the minimum length of schedule overlap to count as a meeting time (in units of hours)
    int realMeetingBlockSize = 1;                       // the minimum length of schedule overlap (in units of # of blocks in schedule)
    AttributeDiversity attributeDiversity[MAX_ATTRIBUTES]; 			// if true/false, tries to make all students on a team have similar/different levels of each attribute
    float attributeWeights[MAX_ATTRIBUTES];             // weights for each attribute as displayed to the user (i.e., non-normalized values)
    float realAttributeWeights[MAX_ATTRIBUTES];         // scoring weight of each attribute, normalized to total weight
    bool haveAnyRequiredAttributes[MAX_ATTRIBUTES];
    QList<int> requiredAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of required attribute value
    bool haveAnyIncompatibleAttributes[MAX_ATTRIBUTES];
    QList< QPair<int,int> > incompatibleAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of incompatible attribute value pairs
    float scheduleWeight = 1;
    float realScheduleWeight = 1;                       // scoring weight of the schedule, normalized to total weight
    int realNumScoringFactors = 1;                      // the total weight of all scoring factors, equal to the number of attributes + 1 for schedule if that is used
    bool haveAnyRequiredTeammates = false;
    bool haveAnyPreventedTeammates = false;
    bool haveAnyRequestedTeammates = false;
    int numberRequestedTeammatesGiven = 1;
    QList<int> smallerTeamsSizes;
    int smallerTeamsNumTeams = 1;
    QList<int> largerTeamsSizes;
    int largerTeamsNumTeams = 1;
    QList<int> teamSizesDesired;
    int numTeamsDesired = 1;
    QString sectionName;
    enum class SectionType {noSections, allTogether, allSeparately, oneSection} sectionType = SectionType::noSections;
    static QString attributeDiversityToString(TeamingOptions::AttributeDiversity value);
    static AttributeDiversity stringToAttributeDiversity(const QString& str);
    int teamsetNumber = 1;                              // which teamset are we working on now?
    inline static const int MAXWEIGHT = 10;             // the maximum value the user can assign for an attribute or schedule weight
    inline static const QString WEIGHTTOOLTIP = "<html>" + QObject::tr("The relative importance of this question in forming the teams. The range is from 0 to ") +
                                                QString::number(TeamingOptions::MAXWEIGHT) + ".</html>";
    inline static const QString SCHEDULEWEIGHTTOOLTIP = "<html>" + QObject::tr("The relative importance of the schedule in forming the teams. The range is from 0 to ") +
                                                        QString::number(TeamingOptions::MAXWEIGHT) + ".</html>";
};

#endif // TEAMINGOPTIONS_H
