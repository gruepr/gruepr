#ifndef TEAMRECORD_H
#define TEAMRECORD_H

#include <QString>
#include <QVector>
#include <set>
#include "dataOptions.h"
#include "gruepr_consts.h"
#include "studentRecord.h"

// all the info about one team

class TeamRecord
{
public:
    TeamRecord();
    void createTooltip(const DataOptions* const dataOptions);
    void refreshTeamInfo(const DataOptions* const dataOptions, const StudentRecord* const student);

    float score = 0;
    int size = 1;
    int numWomen = 0;
    int numMen = 0;
    int numNonbinary = 0;
    int numUnknown = 1;
    int numURM = 0;
    std::set<int> attributeVals[MAX_ATTRIBUTES];
    std::set<float> timezoneVals;
    int numStudentsAvailable[MAX_DAYS][MAX_BLOCKS_PER_DAY] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    QVector<int> studentIndexes;
    QString name;
    QString availabilityChart;
    QString tooltip;
};

#endif // TEAMRECORD_H
