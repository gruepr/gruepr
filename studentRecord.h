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
    bool duplicateRecord = false;                       // another record exists with the same firstname+lastname or email address
    long long ID = -1;                                  // ID is assigned in order of appearance in the data file
    long long LMSID = -1;                               // student ID number according to the learning management system
    QDateTime surveyTimestamp;                          // date/time that the survey was submitted -- see TIMESTAMP_FORMAT definition for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QSet<Gender> gender = {Gender::unknown};
    QString URMResponse;                                // the text of the response the the race/ethnicity/culture question
    QList<int>   attributeVals_discrete[MAX_ATTRIBUTES];   // categorical index or discrete integer value for multiple choice attributes; -1 = unknown
    QList<float> attributeVals_continuous[MAX_ATTRIBUTES]; // float value for timezone and numerical attributes; empty = unknown
    QStringList assignmentPreferences;                  // ranked assignment preference option names, index 0 = 1st choice
    QString section;									// section data stored as text
    bool unavailable[MAX_DAYS][MAX_BLOCKS_PER_DAY];     // true if this is a busy block during week
    QString availabilityChart;
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    float timezone = 0;                                 // offset from GMT
    QString prefTeammates;
    QSet<long long> groupTogether;                      // set of student IDs that this student should be placed on a team with
    QString prefNonTeammates;
    QSet<long long> splitApart;                         // set of student IDs that this student should be prevented from being on a team with
    QString notes;										// any special notes for this student
    QString attributeResponse[MAX_ATTRIBUTES];          // the text of the response to each attribute question
    QString tooltip;

private:
    inline static const int SIZE_OF_NOTES_IN_TOOLTIP = 300;
};

#endif // STUDENTRECORD_H
