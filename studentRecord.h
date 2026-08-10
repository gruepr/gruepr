#ifndef STUDENTRECORD_H
#define STUDENTRECORD_H

//the survey and other data from one student

#include "dataOptions.h"
#include "gruepr_globals.h"
#include <QDateTime>
#include <QJsonObject>
#include <cstdint>
#include <vector>

// A compact bit array for weekly schedule availability, replacing QList<bool> so ScheduleCriterion's
// per-team merge is O(1) words instead of O(N) bytes. Semantically still a flat array of
// numDays*numTimesPerDay bools, indexed as (day*numTimesPerDay + time), but with physical storage
// as packed 64-bit words instead of one byte per slot.
class ScheduleAvailabilityTable
{
public:
    ScheduleAvailabilityTable() = default;
    ScheduleAvailabilityTable(qsizetype size, bool value) { fill(value, size); }

    qsizetype size() const { return numBits; }
    bool isEmpty() const { return numBits == 0; }
    void resize(qsizetype newSize) { words.resize(size_t((newSize + 63) / 64), 0); numBits = newSize; }
    void fill(bool value, qsizetype newSize) {
        words.assign(size_t((newSize + 63) / 64), value ? ~uint64_t(0) : uint64_t(0));
        numBits = newSize;
    }
    void clear() { words.clear(); numBits = 0; }

    // Raw word access for ScheduleCriterion's merge/unpack (see calculateScore()). Bits beyond
    // numBits within the final word are unspecified -- callers must not read past size().
    const uint64_t *data() const { return words.data(); }
    qsizetype wordCount() const { return qsizetype(words.size()); }

    // Proxy standing in for bool& (a real reference into packed storage isn't possible).
    class Reference
    {
    public:
        Reference(uint64_t &word, int bit) : word(word), bit(bit) {}
        operator bool() const { return (word >> bit) & 1ULL; }
        Reference &operator=(bool value) {
            if(value) { word |= (1ULL << bit); } else { word &= ~(1ULL << bit); }
            return *this;
        }
        Reference &operator=(const Reference &other) { return operator=(bool(other)); }
    private:
        uint64_t &word;
        int bit;
    };

    bool operator[](qsizetype i) const { return (words[size_t(i / 64)] >> (i % 64)) & 1ULL; }
    Reference operator[](qsizetype i) { return {words[size_t(i / 64)], int(i % 64)}; }

private:
    std::vector<uint64_t> words;
    qsizetype numBits = 0;
};


class StudentRecord
{
public:
    StudentRecord();
    explicit StudentRecord(const QJsonObject &jsonStudentRecord);

    void reconcileScheduleDimensions(qsizetype numDays, qsizetype numTimesPerDay);

    void clear();

    void parseRecordFromStringList(const QStringList &fields, const DataOptions &dataOptions);

    void createTooltip(const DataOptions &dataOptions);

    QJsonObject toJson() const;

    bool deleted = false;                               // set true when user 'deletes' the student: no longer shows in lists, not actually removed from data
    bool duplicateRecord = false;                       // another record exists with the same firstname+lastname or email address
    long long ID = -1;                                  // ID is assigned in order of appearance in the data file
    long long LMSID = -1;                               // student ID number according to the learning management system
    QDateTime surveyTimestamp;                          // when the survey was submitted -- see TIMESTAMP_FORMAT for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QSet<Gender> gender = {Gender::unknown};
    QString URMIdentityResponse;                        // the text of the response the the race/ethnicity/culture question
    QList<QList<int>>   attributeVals_discrete;         // categorical index or discrete integer value for multiple choice attributes; -1 = unknown
    QList<QList<float>> attributeVals_continuous;       // float value for timezone and numerical attributes; empty = unknown
    QStringList assignmentPreferences;                  // ranked assignment preference option names, index 0 = 1st choice
    QString section;									// section data stored as text
    qsizetype numScheduleDays = 0;
    qsizetype numScheduleTimesPerDay = 0;
    ScheduleAvailabilityTable unavailable;                   // true if this is a busy block during week; stored flat: day * numTimesPerDay + time
    QString availabilityChart;
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    float timezone = 0;                                 // offset from GMT
    QString prefTeammates;
    QSet<long long> groupTogether;                      // set of student IDs that this student should be placed on a team with
    QString prefNonTeammates;
    QSet<long long> splitApart;                         // set of student IDs that this student should be prevented from being on a team with
    QString notes;										// any special notes for this student
    QStringList attributeResponse;                      // the text of the response to each attribute question
    QString tooltip;

private:
    inline static const int SIZE_OF_NOTES_IN_TOOLTIP = 300;
};

#endif // STUDENTRECORD_H
