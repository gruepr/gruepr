#ifndef TEAMINGOPTIONS_H
#define TEAMINGOPTIONS_H

#include "criteria/criterion.h"
#include "gruepr_globals.h"
#include <QJsonObject>
#include <QObject>
#include <QStringList>

//the teaming options set by the user when forming teams

class TeamingOptions
{
public:

    TeamingOptions();
    explicit TeamingOptions(const QJsonObject &jsonTeamingOptions);
    void reset();

    QJsonObject toJson() const;

    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    float meetingBlockSize = 1;                         // the minimum length of schedule overlap to count as a meeting time (in units of hours)
    int realMeetingBlockSize = 1;                       // the minimum length of schedule overlap (in units of # of blocks in schedule)

    Criterion* criteria[MAX_CRITERIA] = {nullptr};
    int realNumScoringFactors = 1;                      // the total weight of all scoring factors, equal to the number of criteria to group by (excluding teamsize and section)
    bool haveAnyRequiredAttributes[MAX_ATTRIBUTES];
    QList<int> requiredAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of required attribute value
    bool haveAnyIncompatibleAttributes[MAX_ATTRIBUTES];
    QList< QPair<int,int> > incompatibleAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of incompatible attribute value pairs
    bool haveAnyGroupTogethers = false;
    bool haveAnySplitAparts = false;
    int numberGroupTogethersGiven = REQUESTED_TEAMMATES_ALL;
    int idealTeamSize = 4;
    QList<int> smallerTeamsSizes;
    int smallerTeamsNumTeams = 1;
    QList<int> largerTeamsSizes;
    int largerTeamsNumTeams = 1;
    QList<int> teamSizesDesired;
    int numTeamsDesired = 1;
    QString sectionName;
    enum class SectionType {noSections, allTogether, allSeparately, oneSection} sectionType = SectionType::noSections;
    int teamsetNumber = 1;                              // which teamset are we working on now?
};

#endif // TEAMINGOPTIONS_H
