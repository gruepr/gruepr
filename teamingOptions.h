#ifndef TEAMINGOPTIONS_H
#define TEAMINGOPTIONS_H

#include "gruepr_consts.h"
#include <QStringList>

//the teaming options set by the user when forming teams

class TeamingOptions
{
public:
    TeamingOptions();
    void reset();

    bool isolatedWomenPrevented = false;                // if true, will prevent teams with an isolated woman
    bool isolatedMenPrevented = false;                  // if true, will prevent teams with an isolated man
    bool isolatedNonbinaryPrevented = false;            // if true, will prevent teams with an isolated nonbinary student
    bool singleGenderPrevented = false;                 // if true, will penalize teams with all men or all women
    bool isolatedURMPrevented = false;                  // if true, will prevent teams with an isolated URM student
    QStringList URMResponsesConsideredUR;               // the list of responses to the race/ethnicity/culture question that are considered underrepresented
    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    int meetingBlockSize = 1;                           // count available meeting times in units of 1 hour or 2 hours long
    bool desireHomogeneous[MAX_ATTRIBUTES]; 			// if true/false, tries to make all students on a team have similar/different levels of each attribute
    float attributeWeights[MAX_ATTRIBUTES];             // weights for each attribute as displayed to the user (i.e., non-normalized values)
    float realAttributeWeights[MAX_ATTRIBUTES];         // scoring weight of each attribute, normalized to total weight
    bool haveAnyRequiredAttributes[MAX_ATTRIBUTES];
    QVector<int> requiredAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of required attribute value
    bool haveAnyIncompatibleAttributes[MAX_ATTRIBUTES];
    QVector< QPair<int,int> > incompatibleAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of incompatible attribute value pairs
    float scheduleWeight = 1;
    float realScheduleWeight = 1;                       // scoring weight of the schedule, normalized to total weight
    int realNumScoringFactors = 1;                      // the total weight of all scoring factors, equal to the number of attributes + 1 for schedule if that is used
    static const int MAXWEIGHT = 100;                   // the maximum value the user can assign for an attribute or schedule weight
    bool haveAnyRequiredTeammates = false;
    bool haveAnyPreventedTeammates = false;
    bool haveAnyRequestedTeammates = false;
    int numberRequestedTeammatesGiven = 1;
    int smallerTeamsSizes[MAX_STUDENTS] = {0};
    int smallerTeamsNumTeams = 1;
    int largerTeamsSizes[MAX_STUDENTS] = {0};
    int largerTeamsNumTeams = 1;
    int numTeamsDesired = 1;
    int teamSizesDesired[MAX_STUDENTS] = {0};
    QString sectionName;
};

#endif // TEAMINGOPTIONS_H
