#include "gradeBalanceCriterion.h"
#include "gruepr_globals.h"
#include "gruepr.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

void GradeBalanceCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto *grueprParent = qobject_cast<gruepr*>(parentCard->parent());

    // Calculate mean grade across all students
    float meanGrade = 0.0f;
    if (grueprParent != nullptr) {
        for (const auto &student : std::as_const(grueprParent->students))
            meanGrade += student.grade;
        if (!grueprParent->students.isEmpty())
            meanGrade /= grueprParent->students.size();
    }

    auto *gradeInfoFrame = new QFrame(parentCard);
    gradeInfoFrame->setFrameShadow(QFrame::Raised);
    auto *gradeInfoLayout = new QVBoxLayout(gradeInfoFrame);
    gradeInfoLayout->addWidget(new QLabel("<u>Mean Student Grade</u>"));
    gradeInfoLayout->addWidget(new QLabel(QString::number(double(meanGrade), 'f', 2)));

    minimumMeanGradeSpinBox = new QDoubleSpinBox(parentCard);
    minimumMeanGradeSpinBox->setPrefix("Min: ");
    minimumMeanGradeSpinBox->setMinimum(0.0);
    minimumMeanGradeSpinBox->setValue(0.0);
    minimumMeanGradeSpinBox->setSingleStep(1.0);
    minimumMeanGradeSpinBox->setDecimals(2);
    minimumMeanGradeSpinBox->setMinimumHeight(40);

    maximumMeanGradeSpinBox = new QDoubleSpinBox(parentCard);
    maximumMeanGradeSpinBox->setPrefix("Max: ");
    maximumMeanGradeSpinBox->setMinimum(0.0);
    maximumMeanGradeSpinBox->setMaximum(100.0);
    maximumMeanGradeSpinBox->setValue(100.0);
    maximumMeanGradeSpinBox->setSingleStep(1.0);
    maximumMeanGradeSpinBox->setDecimals(2);
    maximumMeanGradeSpinBox->setMinimumHeight(40);

    auto *spinBoxLayout = new QHBoxLayout();
    spinBoxLayout->addWidget(minimumMeanGradeSpinBox);
    spinBoxLayout->addWidget(maximumMeanGradeSpinBox);

    auto *spinBoxVLayout = new QVBoxLayout();
    spinBoxVLayout->addWidget(new QLabel("Target group average grade range:"));
    spinBoxVLayout->addLayout(spinBoxLayout);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(gradeInfoFrame, 1);
    contentLayout->addLayout(spinBoxVLayout, 3);
    parentCard->setContentAreaLayout(*contentLayout);

    connect(minimumMeanGradeSpinBox, &QDoubleSpinBox::valueChanged, this, [this, teamingOptions]() {
        teamingOptions->targetMinimumGroupGradeAverage = float(minimumMeanGradeSpinBox->value());
    });
    connect(maximumMeanGradeSpinBox, &QDoubleSpinBox::valueChanged, this, [this, teamingOptions]() {
        teamingOptions->targetMaximumGroupGradeAverage = float(maximumMeanGradeSpinBox->value());
    });
}

void GradeBalanceCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                  const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                  std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints)
{
    // optimize based on _teamingOptions->maximumDeviationOfGroupGradeAverage;

    // calculate group average grade. if absolute (overallmeangrade - currentgroupaveragegrade) > standarddeviationallowed, this group has a lower score

    int studentNum = 0;
    float overallMeanGrade = 0.0;
    QList<float> meanGroupGrades(numTeams);
    //calculate grade average of each team
    for(int team = 0; team < numTeams; team++) {
        float totalGroupGrade = 0.0;
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            totalGroupGrade += students[teammates[studentNum]].grade;
            studentNum++;
        }
        meanGroupGrades[team] = totalGroupGrade / teamSizes[team];
        overallMeanGrade += meanGroupGrades[team];
    }
    overallMeanGrade = overallMeanGrade / numTeams;

    //get overall mean grade
    for(int team = 0; team < numTeams; team++){
        if (meanGroupGrades[team] > teamingOptions->targetMaximumGroupGradeAverage || meanGroupGrades[team] < teamingOptions->targetMinimumGroupGradeAverage){
            criteriaScores[team] = 0;
        } else {
            criteriaScores[team] = 1;
        }
        criteriaScores[team] *= weight;
    }
}
