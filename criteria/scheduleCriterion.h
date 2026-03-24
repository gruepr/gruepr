#ifndef SCHEDULECRITERION_H
#define SCHEDULECRITERION_H

#include "criterion.h"
#include <QDoubleSpinBox>
#include <QSpinBox>

class ScheduleCriterion : public Criterion {
    Q_OBJECT

public:
    ScheduleCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                      GroupingCriteriaCard *parent = nullptr) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions){ };

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const /*teamingOptions*/) override;
    void prepareForOptimization(const StudentRecord *students, int numStudents, const DataOptions *dataOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const /*teamingOptions*/, const DataOptions *const dataOptions,
                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const override;

    static int getNumBlocksForOneMeeting(const TeamingOptions *teamingOptions);

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    float meetingBlockSize = 1;                         // the minimum length of schedule overlap to count as a meeting time (in units of hours)

    const DataOptions *const dataOptions;
    QSpinBox *minMeetingTimes = nullptr;
    QSpinBox *desiredMeetingTimes = nullptr;
    QDoubleSpinBox *meetingLengthSpinBox = nullptr;

private:
    int numBlocksForOneMeeting = 1;                     // the minimum length of schedule overlap (in units of # of blocks in schedule)
};

#endif // SCHEDULECRITERION_H
