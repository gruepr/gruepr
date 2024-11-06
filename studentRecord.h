#ifndef STUDENTRECORD_H
#define STUDENTRECORD_H

//the survey and other data from one student

#include "dataOptions.h"
#include "gruepr_globals.h"
#include <QDateTime>
#include <QJsonObject>

class StudentRecord
{
public:
    StudentRecord();
    explicit StudentRecord(const QJsonObject &jsonStudentRecord);

    void clear();

    void parseRecordFromStringList(const QStringList &fields, const DataOptions &dataOptions);
    void createTooltip(const DataOptions &dataOptions);

    QJsonObject toJson() const;

    bool deleted = false;                               // set true when user 'deletes' the student; no longer shows in lists
    long long ID = -1;                                  // ID is assigned in order of appearance in the data file
    long long LMSID = -1;                               // student ID number according to the learning management system
    bool duplicateRecord = false;                       // another record exists with the same firstname+lastname or email address
    Gender gender = Gender::unknown;
    bool URM = false;                                   // true if this student is from an underrepresented minority group
    bool unavailable[MAX_DAYS][MAX_BLOCKS_PER_DAY];     // true if this is a busy block during week
    float timezone = 0;                                 // offset from GMT
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    QSet<long long> preventedWith;                      // set of student IDs that this student is prevented from being on a team with
    QSet<long long> requiredWith;                       // set of student IDs that this student is required to be on a team with
    QSet<long long> requestedWith;                      // set of student IDs that this student desires to be on a team with
    QList<int> attributeVals[MAX_ATTRIBUTES];           // rating for each attribute (when set, each rating is numerical value from 1 -> attributeLevels[attribute])
    QDateTime surveyTimestamp;                          // date/time that the survey was submitted -- see TIMESTAMP_FORMAT definition for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QString section;									// section data stored as text
    QString prefTeammates;
    QString prefNonTeammates;
    QString notes;										// any special notes for this student
    QString attributeResponse[MAX_ATTRIBUTES];          // the text of the response to each attribute question
    QString URMResponse;                                // the text of the response the the race/ethnicity/culture question
    QString availabilityChart;
    QString tooltip;

private:
    inline static const int SIZE_OF_NOTES_IN_TOOLTIP = 300;
};

#endif // STUDENTRECORD_H
