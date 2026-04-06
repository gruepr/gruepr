#include "scheduleCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

Criterion* ScheduleCriterion::clone() const
{
    auto *copy = new ScheduleCriterion(dataOptions, criteriaType, weight, penaltyStatus);
    copy->desiredTimeBlocksOverlap = desiredTimeBlocksOverlap;
    copy->minTimeBlocksOverlap = minTimeBlocksOverlap;
    copy->meetingBlockSize = meetingBlockSize;
    copy->numBlocksForOneMeeting = numBlocksForOneMeeting;
    return copy;
}

QJsonObject ScheduleCriterion::settingsToJson() const
{
    QJsonObject json = Criterion::settingsToJson();
    json["desiredTimeBlocksOverlap"] = desiredTimeBlocksOverlap;
    json["minTimeBlocksOverlap"] = minTimeBlocksOverlap;
    json["meetingBlockSize"] = static_cast<double>(meetingBlockSize);
    json["numBlocksForOneMeeting"] = numBlocksForOneMeeting;
    return json;
}

void ScheduleCriterion::settingsFromJson(const QJsonObject &json)
{
    Criterion::settingsFromJson(json);
    desiredTimeBlocksOverlap = json["desiredTimeBlocksOverlap"].toInt(8);
    minTimeBlocksOverlap = json["minTimeBlocksOverlap"].toInt(4);
    meetingBlockSize = static_cast<float>(json["meetingBlockSize"].toDouble(1));
    numBlocksForOneMeeting = json["numBlocksForOneMeeting"].toInt(1);
}

void ScheduleCriterion::generateCriteriaCard(TeamingOptions *const /*teamingOptions*/)
{
    auto *meetingScheduleContentLayout = new QVBoxLayout();
    auto *minimumAndDesiredButtonLayout = new QHBoxLayout();
    minMeetingTimes = new QSpinBox(parentCard);
    desiredMeetingTimes = new QSpinBox(parentCard);
    minMeetingTimes->setStyleSheet(SPINBOXSTYLE);
    desiredMeetingTimes->setStyleSheet(SPINBOXSTYLE);
    minMeetingTimes->setMinimumHeight(40);
    desiredMeetingTimes->setMinimumHeight(40);
    minMeetingTimes->setPrefix(QString("Minimum: "));
    minMeetingTimes->setMinimum(1);
    minMeetingTimes->setValue(4);
    desiredMeetingTimes->setPrefix(QString("Desired: "));
    desiredMeetingTimes->setValue(8);
    desiredMeetingTimes->setMinimum(1);
    desiredMeetingTimes->installEventFilter(new MouseWheelBlocker(desiredMeetingTimes));
    desiredMeetingTimes->setFocusPolicy(Qt::StrongFocus);
    minMeetingTimes->installEventFilter(new MouseWheelBlocker(minMeetingTimes));
    minMeetingTimes->setFocusPolicy(Qt::StrongFocus);
    minimumAndDesiredButtonLayout->addWidget(minMeetingTimes);
    minimumAndDesiredButtonLayout->addWidget(desiredMeetingTimes);

    meetingLengthSpinBox = new QDoubleSpinBox(parentCard);
    meetingLengthSpinBox->setStyleSheet(DOUBLESPINBOXSTYLE);
    meetingLengthSpinBox->setMinimumHeight(40);
    meetingLengthSpinBox->setPrefix(QString("Duration: "));
    meetingLengthSpinBox->setSuffix(QString(" hour"));
    meetingScheduleContentLayout->addLayout(minimumAndDesiredButtonLayout);
    meetingScheduleContentLayout->addWidget(meetingLengthSpinBox);
    meetingLengthSpinBox->setMinimum(0.25);
    meetingLengthSpinBox->setMaximum(3.0);
    meetingLengthSpinBox->setSingleStep(0.25);
    meetingLengthSpinBox->setValue(1.0);
    meetingLengthSpinBox->setDecimals(2);
    meetingLengthSpinBox->installEventFilter(new MouseWheelBlocker(meetingLengthSpinBox));
    meetingLengthSpinBox->setFocusPolicy(Qt::StrongFocus);
    parentCard->setContentAreaLayout(*meetingScheduleContentLayout);

    if(!dataOptions->dayNames.isEmpty()) {
        minMeetingTimes->setMaximum(std::max(0.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
        minMeetingTimes->setValue(minTimeBlocksOverlap);
        desiredMeetingTimes->setMaximum(std::max(1.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
        desiredMeetingTimes->setValue(desiredTimeBlocksOverlap);
        //display no decimals if whole number of hours, 1 decimal if on the half-hour, 2 decimals otherwise
        const int roundedVal = std::round(100*dataOptions->scheduleResolution);
        if((roundedVal == 100) || (roundedVal == 200) || (roundedVal == 300)) {
            meetingLengthSpinBox->setDecimals(0);
        }
        else if ((roundedVal == 50) || (roundedVal == 150)) {
            meetingLengthSpinBox->setDecimals(1);
        }
        else {
            meetingLengthSpinBox->setDecimals(2);
        }
        meetingLengthSpinBox->setValue(meetingBlockSize);
        meetingLengthSpinBox->setSingleStep(dataOptions->scheduleResolution);
        meetingLengthSpinBox->setMinimum(dataOptions->scheduleResolution);
        //ui->scheduleWeight->setValue(scheduleWeight);
    }

    connect(minMeetingTimes, &QSpinBox::valueChanged, this, [this]() {
        minTimeBlocksOverlap = (minMeetingTimes->value());
        if (desiredMeetingTimes->value() < (minMeetingTimes->value())) {
            desiredMeetingTimes->setValue(minMeetingTimes->value());
        }
    });

    connect(desiredMeetingTimes, &QSpinBox::valueChanged, this, [this]() {
        desiredTimeBlocksOverlap = (desiredMeetingTimes->value());
        if (minMeetingTimes->value() > (desiredMeetingTimes->value())) {
            minMeetingTimes->setValue(desiredMeetingTimes->value());
        }
    });

    connect(meetingLengthSpinBox, &QDoubleSpinBox::valueChanged, this, [this]() {
        meetingBlockSize = (meetingLengthSpinBox->value());
        meetingLengthSpinBox->setSuffix(meetingLengthSpinBox->value() > 1 ? tr(" hours") : tr(" hour"));
        if (dataOptions && !dataOptions->timeNames.empty() && !dataOptions->dayNames.empty()) {
            minMeetingTimes->setMaximum(std::max(0.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
            desiredMeetingTimes->setMaximum(std::max(1.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
        }
    });
}

void ScheduleCriterion::prepareForOptimization(const StudentRecord *, int, const DataOptions *dataOptions)
{
    numBlocksForOneMeeting = static_cast<int>(std::ceil(meetingBlockSize / dataOptions->scheduleResolution));
}

void ScheduleCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                       const TeamingOptions *const /*teamingOptions*/, const DataOptions *const dataOptions,
                                       QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    const int numDays = int(dataOptions->dayNames.size());
    const int numTimes = int(dataOptions->timeNames.size());

    // Allocated once per thread; subsequent calls just reuse the memory.
    // Using vector<uint8_t> rather than vector<bool> to avoid bit-packing overhead.
    thread_local QList<uint8_t> availabilityChart;
    const int chartSize = numDays * numTimes;
    if (int(availabilityChart.size()) < chartSize) {
        availabilityChart.resize(chartSize);
    }

    // combine each student's schedule array into a team schedule array
    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // start compiling a team availability chart; begin with that of the first student on team (unless they have ambiguous schedule)
        int numStudentsWithAmbiguousSchedules = 0;
        const auto &firstStudentOnTeam = students[teammates[studentNum]];
        if(!firstStudentOnTeam.ambiguousSchedule && firstStudentOnTeam.unavailable.size() >= chartSize) {
            const auto &firstStudentUnavailability = firstStudentOnTeam.unavailable;
            for(int day = 0; day < numDays; day++) {
                for(int time = 0; time < numTimes; time++) {
                    availabilityChart[day * numTimes + time] = !firstStudentUnavailability[day * numTimes + time];
                }
            }
        }
        else {
            // ambiguous schedule, so note it and start with all timeslots available
            numStudentsWithAmbiguousSchedules++;
            for(int day = 0; day < numDays; day++) {
                for(int time = 0; time < numTimes; time++) {
                    availabilityChart[day * numTimes + time] = true;
                }
            }
        }
        studentNum++;

        // now move on to each subsequent student and, unless they have ambiguous schedule, merge their availability into the team's
        for(int teammate = 1; teammate < teamSizes[team]; teammate++) {
            const auto &currStudent = students[teammates[studentNum]];
            if(currStudent.ambiguousSchedule || currStudent.unavailable.size() < chartSize) {
                numStudentsWithAmbiguousSchedules++;
                studentNum++;
                continue;
            }
            const auto &currStudentUnavailability = currStudent.unavailable;
            for(int day = 0; day < numDays; day++) {
                for(int time = 0; time < numTimes; time++) {
                    availabilityChart[day * numTimes + time] = availabilityChart[day * numTimes + time] && !currStudentUnavailability[day * numTimes + time];
                }
            }
            studentNum++;
        }

        // keep schedule score at 0 unless 2+ students have unambiguous sched (avoid runaway score by grouping students w/ambiguous scheds)
        if((teamSizes[team] - numStudentsWithAmbiguousSchedules) < 2) {
            criteriaScores[team] = 0;
            continue;
        }

        //count when there's the correct number of consecutive time blocks, but don't count wrap-around past end of 1 day!
        for(int day = 0; day < numDays; day++) {
            for(int time = 0; time < numTimes; time++) {
                int block = 0;
                while(availabilityChart[day * numTimes + time] && (block < numBlocksForOneMeeting) && (time < numTimes)) {
                    block++;
                    if(block < numBlocksForOneMeeting) {
                        time++;
                    }
                }

                if((block == numBlocksForOneMeeting) && (block > 0)){
                    criteriaScores[team]++;
                }
            }
        }

        // convert counts to a schedule score
        // normal schedule score is number of overlaps / desired number of overlaps
        if(criteriaScores[team] > desiredTimeBlocksOverlap) {     // if team has > desiredTimeBlocksOverlap, each added overlap counts less
            const int numAdditionalOverlaps = int(criteriaScores[team]) - desiredTimeBlocksOverlap;
            criteriaScores[team] = desiredTimeBlocksOverlap;
            float factor = 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            for(int n = 1 ; n <= numAdditionalOverlaps; n++) {
                criteriaScores[team] += factor;
                factor *= 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            }
        }
        else if(criteriaScores[team] < minTimeBlocksOverlap) {    // if team has fewer than minTimeBlocksOverlap, apply penalty
            penaltyPoints[team] += 1.0f;
        }

        criteriaScores[team] /= desiredTimeBlocksOverlap;
        criteriaScores[team] *= weight;
        penaltyPoints[team] *= weight;
    }
}

int ScheduleCriterion::getNumBlocksForOneMeeting(const TeamingOptions *teamingOptions)
{
    for (const auto *criterion : std::as_const(teamingOptions->criteria)) {
        if (criterion->criteriaType == CriteriaType::scheduleMeetingTimes) {
            return static_cast<const ScheduleCriterion*>(criterion)->numBlocksForOneMeeting;
        }
    }
    return 1;
}

QString ScheduleCriterion::headerLabel(const DataOptions *) const {
    return tr("Meeting\ntimes");
}

Qt::TextElideMode ScheduleCriterion::headerElideMode() const {
    return Qt::ElideNone;
}

QString ScheduleCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *, float /*criterionScore*/, const QList<StudentRecord> &/*students*/) const {
    if (team.size > 1) {
        return QString::number(team.numMeetingTimes);
    }
    return "  --  ";
}

QVariant ScheduleCriterion::teamSortValue(const TeamRecord &team, const DataOptions *, float /*criterionScore*/, const QList<StudentRecord> &/*students*/) const {
    return team.numMeetingTimes;
}

QString ScheduleCriterion::studentDisplayText(const StudentRecord &, const DataOptions *) const {
    return "";
}

QString ScheduleCriterion::exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions *) const {
    QString text;
    text += "\n" + tr("Meeting block size is ") + QString::number(meetingBlockSize) +
            tr(" hour") + ((meetingBlockSize == 1) ? "" : tr("s"));
    text += "\n" + tr("Minimum number of meeting times = ") + QString::number(minTimeBlocksOverlap);
    text += "\n" + tr("Desired number of meeting times = ") + QString::number(desiredTimeBlocksOverlap);
    text += "\n" + tr("Schedule weight = ") + QString::number(double(weight));
    return text;
}

QString ScheduleCriterion::exportStudentText(const StudentRecord &, const DataOptions *) const {
    return {};
}
