#include "scheduleCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

void ScheduleCriterion::generateCriteriaCard(const TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    QVBoxLayout *meetingScheduleContentLayout = new QVBoxLayout();
    QHBoxLayout *minimumAndDesiredButtonLayout = new QHBoxLayout();
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
    parentCard->setContentAreaLayout(*meetingScheduleContentLayout);

    //from loadUI()
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
}
