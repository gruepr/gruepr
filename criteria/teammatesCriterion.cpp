#include "teammatesCriterion.h"
#include "gruepr_globals.h"
#include "gruepr.h"
#include "dialogs/teammatesRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QVBoxLayout>

void TeammatesCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    QString typeString;
    TeammatesRulesDialog::TypeOfTeammates type;
    switch(criteriaType) {
    case Criterion::CriteriaType::requiredTeammates:
        typeString = " Required ";
        type = TeammatesRulesDialog::TypeOfTeammates::required;
        break;
    case Criterion::CriteriaType::preventedTeammates:
        typeString = " Prevented ";
        type = TeammatesRulesDialog::TypeOfTeammates::prevented;
        break;
    case Criterion::CriteriaType::requestedTeammates:
        typeString = " Requested ";
        type = TeammatesRulesDialog::TypeOfTeammates::requested;
        break;
    default:
        return;
    }

    auto *requiredTeammatesContentAreaLayout = new QVBoxLayout();

    setTeammateRulesButton = new QPushButton(tr("Set") + typeString + tr("Teammate Rules"), parentCard);
    setTeammateRulesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setTeammateRulesButton->setMinimumHeight(30);
    setTeammateRulesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    requiredTeammatesContentAreaLayout->addWidget(setTeammateRulesButton);
    parentCard->setContentAreaLayout(*requiredTeammatesContentAreaLayout);

    // Connect button to open dialog
    auto *grueprParent = qobject_cast<gruepr*>(parentCard->parent());
    if (grueprParent == nullptr) {
        return;
    }

    connect(setTeammateRulesButton, &QPushButton::clicked, parentCard, [grueprParent, teamingOptions, type]() {
        const QStringList teamTabNames = grueprParent->getTeamTabNames();
        const QString sectionName = ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
                                     (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) ||
                                     (teamingOptions->sectionType == TeamingOptions::SectionType::noSections))
                                        ? "" : teamingOptions->sectionName;

        auto *win = new TeammatesRulesDialog(grueprParent->students, *grueprParent->dataOptions, *teamingOptions,
                                             sectionName, teamTabNames, type, grueprParent);
        if (win->exec() == QDialog::Accepted) {
            for (int i = 0; i < grueprParent->students.size(); i++) {
                grueprParent->students[i] = win->students[i];
            }
            teamingOptions->haveAnyRequiredTeammates  = win->required_teammatesSpecified;
            teamingOptions->haveAnyPreventedTeammates = win->prevented_teammatesSpecified;
            teamingOptions->haveAnyRequestedTeammates = win->requested_teammatesSpecified;
            teamingOptions->numberRequestedTeammatesGiven = win->numberRequestedTeammatesGiven;
            grueprParent->saveState();
        }
        delete win;
    });
}


void TeammatesCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints)
{
    // Get all IDs being teamed (so that we can make sure we only check the requireds/prevented/requesteds that are actually within this teamset)
    std::set<long long> IDsBeingTeamed;
    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            IDsBeingTeamed.insert(students[teammates[studentNum]].ID);
            studentNum++;
        }
    }

    // Loop through each team
    studentNum = 0;
    std::set<long long> IDsOnTeam;
    std::multiset<long long> relevantIDsOnTeam;
    std::vector<std::set<long long>> requestedIDs;

    for(int team = 0; team < numTeams; team++) {
        IDsOnTeam.clear();
        relevantIDsOnTeam.clear();
        requestedIDs.clear();

        //loop through each student on team and collect their ID and their required/prevented/requested IDs
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &currStudent = students[teammates[studentNum]];
            IDsOnTeam.insert(currStudent.ID);

            std::set<long long> requestedByThis;
            for (const auto id : IDsBeingTeamed) {
                if (criteriaType == CriteriaType::requiredTeammates  && currStudent.requiredWith.contains(id))
                    relevantIDsOnTeam.insert(id);
                if (criteriaType == CriteriaType::preventedTeammates && currStudent.preventedWith.contains(id))
                    relevantIDsOnTeam.insert(id);
                if (criteriaType == CriteriaType::requestedTeammates && currStudent.requestedWith.contains(id))
                    requestedByThis.insert(id);
            }
            if (criteriaType == CriteriaType::requestedTeammates)
                requestedIDs.push_back(requestedByThis);
            studentNum++;
        }

        criteriaScores[team] = 1;

        if (criteriaType == CriteriaType::requiredTeammates && teamingOptions->haveAnyRequiredTeammates) {
            for (const auto id : relevantIDsOnTeam) {
                if (IDsOnTeam.count(id) == 0) {
                    criteriaScores[team] = 0;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                }
            }
        } else if (criteriaType == CriteriaType::preventedTeammates && teamingOptions->haveAnyPreventedTeammates) {
            for (const auto id : relevantIDsOnTeam) {
                if (IDsOnTeam.count(id) != 0) {
                    criteriaScores[team] = 0;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                }
            }
        } else if (criteriaType == CriteriaType::requestedTeammates && teamingOptions->haveAnyRequestedTeammates) {
            for (const auto& reqSet : requestedIDs) {
                int found = 0;
                for (const auto id : reqSet)
                    if (IDsOnTeam.count(id)) {
                        found++;
                    }
                if (found < std::min(int(reqSet.size()), teamingOptions->numberRequestedTeammatesGiven)) {
                    criteriaScores[team] = 0;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                }
            }
        }

        criteriaScores[team] *= weight;
    }
}
