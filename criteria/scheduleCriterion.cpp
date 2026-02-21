#include "scheduleCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

void ScheduleCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto *meetingScheduleContentLayout = new QVBoxLayout();
    auto *minimumAndDesiredButtonLayout = new QHBoxLayout();
    minMeetingTimes = new QSpinBox(parentCard);
    desiredMeetingTimes = new QSpinBox(parentCard);
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
        minMeetingTimes->setValue(teamingOptions->minTimeBlocksOverlap);
        desiredMeetingTimes->setMaximum(std::max(1.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
        desiredMeetingTimes->setValue(teamingOptions->desiredTimeBlocksOverlap);
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
        meetingLengthSpinBox->setValue(teamingOptions->meetingBlockSize);
        meetingLengthSpinBox->setSingleStep(dataOptions->scheduleResolution);
        meetingLengthSpinBox->setMinimum(dataOptions->scheduleResolution);
        //ui->scheduleWeight->setValue(teamingOptions->scheduleWeight);
    }

    connect(minMeetingTimes, &QSpinBox::valueChanged, this, [this, teamingOptions]() {
        teamingOptions->minTimeBlocksOverlap = (minMeetingTimes->value());
        if (desiredMeetingTimes->value() < (minMeetingTimes->value())) {
            desiredMeetingTimes->setValue(minMeetingTimes->value());
        }
    });

    connect(desiredMeetingTimes, &QSpinBox::valueChanged, this, [this, teamingOptions]() {
        teamingOptions->desiredTimeBlocksOverlap = (desiredMeetingTimes->value());
        if (minMeetingTimes->value() > (desiredMeetingTimes->value())) {
            minMeetingTimes->setValue(desiredMeetingTimes->value());
        }
    });

    connect(meetingLengthSpinBox, &QDoubleSpinBox::valueChanged, this, [this, teamingOptions]() {
        teamingOptions->meetingBlockSize = (meetingLengthSpinBox->value());
        meetingLengthSpinBox->setSuffix(meetingLengthSpinBox->value() > 1 ? tr(" hours") : tr(" hour"));
        if (dataOptions && !dataOptions->timeNames.empty() && !dataOptions->dayNames.empty()) {
            minMeetingTimes->setMaximum(std::max(0.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
            desiredMeetingTimes->setMaximum(std::max(1.0, int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (meetingLengthSpinBox->value())));
        }
    });
}

void ScheduleCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                              const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                              std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints)
{
    const int numDays = int(dataOptions->dayNames.size());
    const int numTimes = int(dataOptions->timeNames.size());
    const int numBlocksNeeded = teamingOptions->realMeetingBlockSize;

    // Allocated once per thread; subsequent calls just reuse the memory.
    // Using vector<uint8_t> rather than vector<bool> to avoid bit-packing overhead.
    thread_local std::vector<uint8_t> availabilityChart;
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
        if(!firstStudentOnTeam.ambiguousSchedule) {
            const auto &firstStudentUnavailability = firstStudentOnTeam.unavailable;
            for(int day = 0; day < numDays; day++) {
                const auto &firstStudentUnavailabilityThisDay = firstStudentUnavailability[day];
                for(int time = 0; time < numTimes; time++) {
                    availabilityChart[day * numTimes + time] = !firstStudentUnavailabilityThisDay[time];
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
            if(currStudent.ambiguousSchedule) {
                numStudentsWithAmbiguousSchedules++;
                studentNum++;
                continue;
            }
            const auto &currStudentUnavailability = currStudent.unavailable;
            for(int day = 0; day < numDays; day++) {
                const auto &currStudentUnavailabilityThisDay = currStudentUnavailability[day];
                for(int time = 0; time < numTimes; time++) {
                    // "and" each student's not-unavailability
                    availabilityChart[day * numTimes + time] = availabilityChart[day * numTimes + time] && !currStudentUnavailabilityThisDay[time];
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
                while(availabilityChart[day * numTimes + time] && (block < numBlocksNeeded) && (time < numTimes)) {
                    block++;
                    if(block < numBlocksNeeded) {
                        time++;
                    }
                }

                if((block == numBlocksNeeded) && (block > 0)){
                    criteriaScores[team]++;
                }
            }
        }

        // convert counts to a schedule score
        // normal schedule score is number of overlaps / desired number of overlaps
        if(criteriaScores[team] > teamingOptions->desiredTimeBlocksOverlap) {     // if team has > desiredTimeBlocksOverlap, each added overlap counts less
            const int numAdditionalOverlaps = int(criteriaScores[team]) - teamingOptions->desiredTimeBlocksOverlap;
            criteriaScores[team] = teamingOptions->desiredTimeBlocksOverlap;
            float factor = 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            for(int n = 1 ; n <= numAdditionalOverlaps; n++) {
                criteriaScores[team] += factor;
                factor *= 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            }
        }
        else if(criteriaScores[team] < teamingOptions->minTimeBlocksOverlap) {    // if team has fewer than minTimeBlocksOverlap, zero out the score and apply penalty
            criteriaScores[team] = 0;
            penaltyPoints[team]++;
        }
        criteriaScores[team] /= teamingOptions->desiredTimeBlocksOverlap;
        criteriaScores[team] *= weight;
    }
}
