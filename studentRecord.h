#ifndef STUDENTRECORD_H
#define STUDENTRECORD_H

//the survey and other data from one student

#include "dataOptions.h"
#include "gruepr_globals.h"
#include <QDateTime>

class StudentRecord
{
public:
    StudentRecord();
    void parseRecordFromStringList(const QStringList &fields, const DataOptions* const dataOptions);
    void createTooltip(const DataOptions* const dataOptions);

    int ID = -1;                                        // ID is assigned in order of appearance in the data file
    int LMSID = -1;                                     // student ID number according to the learning management system
    bool duplicateRecord = false;                       // another record exists with the same firstname+lastname or email address
    Gender gender = Gender::unknown;
    bool URM = false;                                   // true if this student is from an underrepresented minority group
    bool unavailable[MAX_DAYS][MAX_BLOCKS_PER_DAY];     // true if this is a busy block during week
    float timezone = 0;                                 // offset from GMT
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    bool preventedWith[MAX_IDS] = {false};              // true if this student is prevented from working with the student with this ID value
    bool requiredWith[MAX_IDS] = {false};       		// true if this student is required to work with the student with this ID value
    bool requestedWith[MAX_IDS] = {false};      		// true if this student desires to work with the student with this ID value
    QVector<int> attributeVals[MAX_ATTRIBUTES];         // rating for each attribute (when set, each rating is numerical value from 1 -> attributeLevels[attribute])
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
