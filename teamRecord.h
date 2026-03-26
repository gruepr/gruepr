#ifndef TEAMRECORD_H
#define TEAMRECORD_H

#include "dataOptions.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include <QList>
#include <QString>

// all the info about one team

class TeamRecord
{
public:
    explicit TeamRecord(const DataOptions *const teamSetDataOptions, int teamSize) : size(teamSize), teamSetDataOptions(teamSetDataOptions) {};
    explicit TeamRecord(const DataOptions *const teamSetDataOptions, const QJsonObject &jsonTeamRecord, const QList<StudentRecord> &students);

    void createTooltip(const QList<StudentRecord> &students);
    void refreshTeamInfo(const QList<StudentRecord> &students, const int meetingBlockSize);

    QJsonObject toJson() const;

    long long LMSID = -1;           // team ID number according to the learning management system
    float score = 0;
    int size = 1;
    int numSections = 0;
    int numWomen = 0;
    int numMen = 0;
    int numNonbinary = 0;
    int numUnknown = 0;
    QString assignedOption;
    int numStudentsAvailable[MAX_DAYS][MAX_BLOCKS_PER_DAY] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    int numMeetingTimes = 0;
    QList<long long> studentIDs;
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
