#ifndef TEAMRECORD_H
#define TEAMRECORD_H

#include "dataOptions.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include <QList>
#include <QString>
#include <set>

// all the info about one team

class TeamRecord
{
public:
    explicit TeamRecord(const DataOptions *const teamSetDataOptions, int teamSize) : size(teamSize), teamSetDataOptions(teamSetDataOptions) {};
    explicit TeamRecord(const DataOptions *const teamSetDataOptions, const QJsonObject &jsonTeamRecord);

    void createTooltip();
    void refreshTeamInfo(const StudentRecord* const student, const int meetingBlockSize);

    QJsonObject toJson() const;

    int LMSID = -1;         // ID number for this team according to the learning management system
    float score = 0;
    int size = 1;
    int numSections = 0;
    int numWomen = 0;
    int numMen = 0;
    int numNonbinary = 0;
    int numUnknown = 0;
    int numURM = 0;
    std::set<int> attributeVals[MAX_ATTRIBUTES];
    std::set<float> timezoneVals;
    int numStudentsAvailable[MAX_DAYS][MAX_BLOCKS_PER_DAY] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    int numMeetingTimes = 0;
    QList<int> studentIndexes;
    QString name;
    QString tooltip;

private:
    const DataOptions *teamSetDataOptions;
};


class TeamSet : public QList<TeamRecord>
{
public:
    DataOptions dataOptions;
};

#endif // TEAMRECORD_H
