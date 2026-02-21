#include "URMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/gatherURMResponsesDialog.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"

void URMIdentityCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *urmContentLayout = new QVBoxLayout();
    isolatedURM = new QCheckBox(tr("Prevent Isolated URM"), parentCard);
    isolatedURM->setChecked(teamingOptions->isolatedURMPrevented);
    chooseURMResponses = new QPushButton(tr("Set Responses Considered Underrepresented"), parentCard);
    chooseURMResponses->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    // complicatedURMRule = new QPushButton(tr("Something more complicated"), parentCard);
    // complicatedURMRule->setFixedHeight(40);
    // complicatedURMRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    urmContentLayout->addWidget(isolatedURM);
    urmContentLayout->addWidget(chooseURMResponses);
    // urmContentLayout->addWidget(complicatedURMRule);
    parentCard->setContentAreaLayout(*urmContentLayout);

    connect(isolatedURM, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        teamingOptions->isolatedURMPrevented = (state == Qt::Checked);
    });
    connect(chooseURMResponses, &QPushButton::clicked, this, [this, teamingOptions]() {
        auto *win = new gatherURMResponsesDialog(dataOptions->URMResponses, teamingOptions->URMResponsesConsideredUR, this->parentCard);

        //If user clicks OK, replace the responses considered underrepresented with the set from the window
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            teamingOptions->URMResponsesConsideredUR = win->URMResponsesConsideredUR;
            teamingOptions->URMResponsesConsideredUR.removeDuplicates();
        }

        delete win;
    });

    // connect(complicatedURMRule, &QPushButton::clicked, this, [this, teamingOptions]() {
    //     //FROMDEV: Need to generalize so that not only one identity at a time
    //     auto *window = new IdentityRulesDialog(this->parentCard, /*Gender::woman*/, teamingOptions, dataOptions);
    //     window->exec();
    //     isolatedURM->setChecked(teamingOptions->URMIdentityRules[/*Gender::woman*/]["!="].contains(1));
    // });
}

void URMIdentityCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                          const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                          std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints)
{
    if(!teamingOptions->isolatedURMPrevented) {
        return;
    }

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        //for now no positive "score", just penalties related to URM isolation
        criteriaScores[team] = weight;
        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // Count how many URM students on the team
        int numURM = 0;
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            if(students[teammates[studentNum]].URM) {
                numURM++;
            }
            studentNum++;
        }

        if(numURM == 1) {
            penaltyPoints[team]++;
        }
        /*
        // Count how many URM on the team
        QMap <QString, int> urmToCount;
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            if (urmToCount.contains(_students[_teammates[studentNum]].URMResponse)){
                urmToCount[_students[_teammates[studentNum]].URMResponse] += 1;
            } else {
                urmToCount[_students[_teammates[studentNum]].URMResponse] = 1;
            }
            studentNum++;
        }

        const QList<int> &unallowed_values = _teamingOptions->urmIdentityRules[criterion->urmName]["!="];
        for (int unallowed_value : unallowed_values) {
            if (urmToCount[criterion->urmName] == unallowed_value) {
                if (criterion->penaltyStatus){
                    _penaltyPoints[team]++;
                }
                break;
            }
        }
*/
    }
}
