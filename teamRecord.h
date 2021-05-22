#ifndef TEAMRECORD_H
#define TEAMRECORD_H

#include <QString>
#include <QVector>
#include <set>
#include "gruepr_consts.h"

// all the info about one team

class TeamRecord
{
public:
    TeamRecord();

    float score;
    int size;
    int numWomen;
    int numMen;
    int numNonbinary;
    int numUnknown;
    int numURM;
    std::set<int> attributeVals[MAX_ATTRIBUTES];
    std::set<float> timezoneVals;
    int numStudentsAvailable[MAX_DAYS][MAX_BLOCKS_PER_DAY] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    QVector<int> studentIDs;
    QString name;
    QString availabilityChart;
    QString tooltip;
};

#endif // TEAMRECORD_H
