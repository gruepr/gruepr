#include "teammatesCriterion.h"
#include "gruepr_globals.h"
#include "gruepr.h"
#include "dialogs/teammatesRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QVBoxLayout>

Criterion* TeammatesCriterion::clone() const {
    return new TeammatesCriterion(criteriaType, weight, penaltyStatus);
}

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
                                        std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const
{
    // Get all IDs being teamed (so that we can make sure we only check the groupTogethers/splitAparts that are actually within this teamset)
    QSet<long long> IDsBeingTeamed;
    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            IDsBeingTeamed.insert(students[teammates[studentNum]].ID);
            studentNum++;
        }
    }

    // Loop through each team
    studentNum = 0;
    QSet<long long> IDsOnTeam;
    QList<const StudentRecord *> teamMembers;

    for(int team = 0; team < numTeams; team++) {
        IDsOnTeam.clear();
        teamMembers.clear();
        teamMembers.reserve(teamSizes[team]);

        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &currStudent = students[teammates[studentNum]];
            IDsOnTeam.insert(currStudent.ID);
            teamMembers.append(&currStudent);
            studentNum++;
        }

        const int penalties = scoreOneTeam(teamMembers, IDsOnTeam, IDsBeingTeamed, teamingOptions);
        criteriaScores[team] = (penalties == 0) ? weight : 0;

        if (penalties > 0 && penaltyStatus) {
            penaltyPoints[team] += penalties;
        }
    }
}

float TeammatesCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                                   const DataOptions *, const QSet<long long> &allIDsBeingTeamed)
{
    const QSet<long long> IDsOnTeam(team.studentIDs.begin(), team.studentIDs.end());

    QList<const StudentRecord *> teamMembers;
    teamMembers.reserve(team.size);
    bool thisTeamHasGroupTogethers = false, thisTeamHasSplitAparts = false;
    for (const auto studentID : team.studentIDs) {
        int i = 0;
        while (i < allStudents.size() && allStudents[i].ID != studentID) {
            i++;
        }
        if (i < allStudents.size()) {
            teamMembers.append(&allStudents[i]);
            thisTeamHasGroupTogethers = thisTeamHasGroupTogethers || !allStudents[i].groupTogether.isEmpty();
            thisTeamHasSplitAparts = thisTeamHasSplitAparts || !allStudents[i].splitApart.isEmpty();
        }
    }

    if (criteriaType == CriteriaType::groupTogether && !thisTeamHasGroupTogethers) {
        return Criterion::NO_SCORE;
    }
    if (criteriaType == CriteriaType::splitApart && !thisTeamHasSplitAparts) {
        return Criterion::NO_SCORE;
    }

    const int penalties = scoreOneTeam(teamMembers, IDsOnTeam, allIDsBeingTeamed, teamingOptions);
    return (penalties == 0) ? 1 : 0;
}

int TeammatesCriterion::scoreOneTeam(const QList<const StudentRecord *> &teamMembers, const QSet<long long> &idsOnTeam,
                                     const QSet<long long> &idsBeingTeamed, const TeamingOptions *const teamingOptions) const
{
    int penalties = 0;

    if (criteriaType == CriteriaType::groupTogether && teamingOptions->haveAnyGroupTogethers) {
        for (const auto *const student : teamMembers) {
            int found = 0;
            int needed = 0;
            for (const auto id : student->groupTogether) {
                if (idsBeingTeamed.contains(id)) {
                    needed++;
                    if (idsOnTeam.contains(id)) {
                        found++;
                    }
                }
            }
            if (found < std::min(needed, teamingOptions->numberGroupTogethersGiven)) {
                penalties++;
            }
        }
    }
    else if (criteriaType == CriteriaType::splitApart && teamingOptions->haveAnySplitAparts) {
        for (const auto *student : teamMembers) {
            for (const auto id : student->splitApart) {
                if (idsBeingTeamed.contains(id) && idsOnTeam.contains(id)) {
                    penalties++;
                }
            }
        }
    }

    return penalties;
}

QString TeammatesCriterion::headerLabel(const DataOptions *) const
{
    if (criteriaType == CriteriaType::groupTogether) {
        return tr("Required\nteammates");
    }
    return tr("Prevented\nteammates");
}

Qt::TextElideMode TeammatesCriterion::headerElideMode() const
{
    return Qt::ElideNone;
}

QString TeammatesCriterion::teamDisplayText(const TeamRecord &, const DataOptions *, float criterionScore) const
{
    if (IS_NO_SCORE(criterionScore)) {
        return QString::fromUtf8(" ");
    }
    if (criterionScore > 0) {
        return QString::fromUtf8("✓");
    }
    return QString::fromUtf8("✗");
}

QVariant TeammatesCriterion::teamSortValue(const TeamRecord &, const DataOptions *, float criterionScore) const
{
    if (IS_NO_SCORE(criterionScore)) {
        return 0;
    }
    if (criterionScore > 0) {
        return 1;
    }
    return -1;
}

QString TeammatesCriterion::studentDisplayText(const StudentRecord &student, const DataOptions *) const
{
    if (criteriaType == CriteriaType::groupTogether) {
        return student.groupTogether.isEmpty() ? " " : QString(BULLET);
    }
    return student.splitApart.isEmpty() ? " " : QString(BULLET);
}

QString TeammatesCriterion::exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *) const
{
    if (criteriaType == CriteriaType::groupTogether && teamingOptions->haveAnyGroupTogethers) {
        QString text = "\n" + tr("Required teammates active");
        if (teamingOptions->numberGroupTogethersGiven >= MAX_STUDENTS) {
            text += tr(", all requests granted");
        }
        else {
            text += tr(", up to ") + QString::number(teamingOptions->numberGroupTogethersGiven) +
                    tr(" per student granted");
        }
        return text;
    }

    if (criteriaType == CriteriaType::splitApart && teamingOptions->haveAnySplitAparts) {
        return "\n" + tr("Prevented teammates active");
    }

    return {};
}

QString TeammatesCriterion::exportStudentText(const StudentRecord &, const DataOptions *) const
{
    return {};
}
