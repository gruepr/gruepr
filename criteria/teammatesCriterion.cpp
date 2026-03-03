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
    if(criteriaType == Criterion::CriteriaType::groupTogether) {
        typeString = tr("group together");
        type = TeammatesRulesDialog::TypeOfTeammates::groupTogether;
    }
    else {
        typeString = tr("split apart");
        type = TeammatesRulesDialog::TypeOfTeammates::splitApart;
    }

    auto *teammatesContentAreaLayout = new QVBoxLayout();

    setTeammateRulesButton = new QPushButton(tr("Select which students to ") + typeString, parentCard);
    setTeammateRulesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setTeammateRulesButton->setMinimumHeight(30);
    setTeammateRulesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    teammatesContentAreaLayout->addWidget(setTeammateRulesButton);

    pairingCountLabel = new QLabel(parentCard);
    teammatesContentAreaLayout->addWidget(pairingCountLabel);

    parentCard->setContentAreaLayout(*teammatesContentAreaLayout);

    // Connect button to open dialog
    auto *grueprParent = qobject_cast<gruepr*>(parentCard->parent());
    if (grueprParent == nullptr) {
        return;
    }

    auto updatePairingCount = [this, grueprParent, type]() {
        int count = 0;
        for (const auto &student : std::as_const(grueprParent->students)) {
            if (!student.deleted) {
                if (type == TeammatesRulesDialog::TypeOfTeammates::splitApart) {
                    count += student.splitApart.size();
                }
                else {
                    count += student.groupTogether.size();
                }
            }
        }
        count /= 2; // undo double-count of Student A -> Student B and then again Student B -> Student A
        pairingCountLabel->setText(count == 0 ? tr("No pairings set")
                                              : QString::number(count) + (count == 1 ? tr(" pairing set") : tr(" pairings set")));
    };

    updatePairingCount();

    connect(setTeammateRulesButton, &QPushButton::clicked, parentCard, [grueprParent, teamingOptions, type, updatePairingCount]() {
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
            if(type == TeammatesRulesDialog::TypeOfTeammates::groupTogether) {
                teamingOptions->haveAnyGroupTogethers = win->teammatesSpecified;
                teamingOptions->numberGroupTogethersGiven = win->numberGroupTogethersGiven;
            }
            else {
                teamingOptions->haveAnySplitAparts = win->teammatesSpecified;
            }
            grueprParent->saveState();
            updatePairingCount();
        }
        delete win;
    });
}


void TeammatesCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                        const TeamingOptions *const teamingOptions, const DataOptions *const /*dataOptions*/,
                                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints)
{
    // Get all IDs being teamed (so that we can make sure we only check the groupTogethers/splitAparts that are actually within this teamset)
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
    std::vector<std::set<long long>> allRequestedIDs;
    std::multiset<long long> allPreventedIDs;

    for(int team = 0; team < numTeams; team++) {
        IDsOnTeam.clear();
        allRequestedIDs.clear();
        allPreventedIDs.clear();

        //loop through each student on team and collect their ID and their groupTogether/splitApart IDs
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &currStudent = students[teammates[studentNum]];
            IDsOnTeam.insert(currStudent.ID);

            std::set<long long> requestedByThisStudent;
            for (const auto id : IDsBeingTeamed) {
                if (criteriaType == CriteriaType::groupTogether  && currStudent.groupTogether.contains(id))
                    requestedByThisStudent.insert(id);
                if (criteriaType == CriteriaType::splitApart && currStudent.splitApart.contains(id))
                    allPreventedIDs.insert(id);
            }
            allRequestedIDs.push_back(requestedByThisStudent);
            studentNum++;
        }

        criteriaScores[team] = 1;

        if (criteriaType == CriteriaType::groupTogether && teamingOptions->haveAnyGroupTogethers) {
            for (const auto& reqSet : allRequestedIDs) {
                int found = 0;
                for (const auto id : reqSet)
                    if (IDsOnTeam.count(id)) {
                        found++;
                    }
                if (found < std::min(int(reqSet.size()), teamingOptions->numberGroupTogethersGiven)) {
                    criteriaScores[team] = 0;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                }
            }
        }
        else if (criteriaType == CriteriaType::splitApart && teamingOptions->haveAnySplitAparts) {
            for (const auto id : allPreventedIDs) {
                if (IDsOnTeam.count(id) != 0) {
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
