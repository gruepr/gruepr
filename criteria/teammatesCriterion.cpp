#include "teammatesCriterion.h"
#include "gruepr_globals.h"
#include "gruepr.h"
#include "dialogs/teammatesRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
// Starts a new round for one of the generation-stamped membership arrays used below. If the next
// increment would wrap back to 0 -- the value every never-yet-inserted slot already holds --
// untouched slots would start looking like members again, so guard explicitly: clear the array once
// and restart the counter at 1.
void beginNewRound(std::vector<uint32_t> &stamp, uint32_t &generation) {
    if (generation == std::numeric_limits<uint32_t>::max()) {
        std::fill(stamp.begin(), stamp.end(), 0);
        generation = 0;
    }
    generation++;
}

void markID(std::vector<uint32_t> &stamp, uint32_t generation, long long id) {
    if (id < 0) { return; }
    if (static_cast<long long>(stamp.size()) <= id) { stamp.resize(static_cast<size_t>(id) + 1, 0); }
    stamp[static_cast<size_t>(id)] = generation;
}
}

Criterion* TeammatesCriterion::clone() const {
    auto *copy = new TeammatesCriterion(criteriaType, weight, penaltyStatus);
    copy->haveAnyTeammates = haveAnyTeammates;
    copy->numberGiven = numberGiven;
    return copy;
}

QJsonObject TeammatesCriterion::settingsToJson() const {
    QJsonObject json = Criterion::settingsToJson();
    json["haveAnyTeammates"] = haveAnyTeammates;
    json["numberGiven"] = numberGiven;
    return json;
}

void TeammatesCriterion::settingsFromJson(const QJsonObject &json) {
    Criterion::settingsFromJson(json);
    haveAnyTeammates = json["haveAnyTeammates"].toBool(false);
    numberGiven = json["numberGiven"].toInt(REQUESTED_TEAMMATES_ALL);
}

void TeammatesCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
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
        haveAnyTeammates = (count > 0);
        pairingCountLabel->setText(count == 0 ? tr("No pairings set")
                                              : QString::number(count) + (count == 1 ? tr(" pairing set") : tr(" pairings set")));
    };

    updatePairingCount();

    connect(setTeammateRulesButton, &QPushButton::clicked, parentCard, [this, grueprParent, teamingOptions, type, updatePairingCount]() {
        const QStringList teamTabNames = grueprParent->getTeamTabNames();
        const QString sectionName = ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
                                     (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) ||
                                     (teamingOptions->sectionType == TeamingOptions::SectionType::noSections))
                                        ? "" : teamingOptions->sectionName;

        auto *win = new TeammatesRulesDialog(grueprParent->students, *grueprParent->dataOptions, sectionName,
                                             teamTabNames, type, numberGiven, grueprParent);
        if (win->exec() == QDialog::Accepted) {
            for (int i = 0; i < grueprParent->students.size(); i++) {
                grueprParent->students[i] = win->students[i];
            }
            haveAnyTeammates = win->teammatesSpecified;
            if(type == TeammatesRulesDialog::TypeOfTeammates::groupTogether) {
                numberGiven = win->numberGroupTogethersGiven;
            }
            grueprParent->saveState();
            updatePairingCount();
        }
        delete win;
    });
}


void TeammatesCriterion::prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions * /*dataOptions*/)
{
    // Every active student is, by definition, being teamed. This is the membership set
    // calculateScore() needs, but it's fixed for the whole optimization run (or per-section, when
    // teaming sections separately so this is called fresh for each section), so it's cheaper to cache
    // it once here. Deleted students and students from a different section, are not included.
    beginNewRound(cachedBeingTeamedStamp, cachedBeingTeamedGeneration);
    for(int i = 0; i < numStudents; i++) {
        markID(cachedBeingTeamedStamp, cachedBeingTeamedGeneration, students[studentIndexes[i]].ID);
    }
}

void TeammatesCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                        const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    // thread_local because scoring runs inside an OpenMP parallel region, so the buffer must be per-thread.
    // Generation-stamped membership set for student IDs (dense, assigned in load order): stamp[id] ==
    // generation means "member of this round" -- O(1) array indexing, rebuilt fresh every call/team.
    thread_local std::vector<uint32_t> onTeamStamp;
    thread_local uint32_t onTeamGeneration = 0;

    // Loop through each team
    int studentNum = 0;
    QList<const StudentRecord *> teamMembers;

    for(int team = 0; team < numTeams; team++) {
        beginNewRound(onTeamStamp, onTeamGeneration);
        teamMembers.clear();
        teamMembers.reserve(teamSizes[team]);

        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &currStudent = students[teammates[studentNum]];
            markID(onTeamStamp, onTeamGeneration, currStudent.ID);
            teamMembers.append(&currStudent);
            studentNum++;
        }

        // Uses the roster-wide stamp cached once in prepareForOptimization instead of rebuilding
        // it from teammates[]/numTeams on every call -- the active roster is fixed for the run.
        const int penalties = scoreOneTeam(teamMembers, onTeamStamp, onTeamGeneration, cachedBeingTeamedStamp, cachedBeingTeamedGeneration);
        if (penalties > 0 && penaltyStatus) {
            penaltyPoints[team] += penalties;
        }

        criteriaScores[team] = (penalties == 0) ? weight : 0;
        penaltyPoints[team] *= weight;
    }
}

float TeammatesCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions* /*teamingOptions*/,
                                                   const DataOptions* /*dataOptions*/, const QSet<long long> &allIDsBeingTeamed)
{
    // "generation" is just a fixed 1 since these are fresh local vectors that start empty every call.
    std::vector<uint32_t> beingTeamedStamp;
    for (const auto id : allIDsBeingTeamed) {
        markID(beingTeamedStamp, 1, id);
    }

    std::vector<uint32_t> IDsOnTeam;
    for (const auto id : team.studentIDs) {
        markID(IDsOnTeam, 1, id);
    }

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
            thisTeamHasGroupTogethers = thisTeamHasGroupTogethers || !allStudents[i].groupTogether.empty();
            thisTeamHasSplitAparts = thisTeamHasSplitAparts || !allStudents[i].splitApart.empty();
        }
    }

    if (criteriaType == CriteriaType::groupTogether && !thisTeamHasGroupTogethers) {
        return Criterion::NO_SCORE;
    }
    if (criteriaType == CriteriaType::splitApart && !thisTeamHasSplitAparts) {
        return Criterion::NO_SCORE;
    }

    // "generation" is just a fixed 1 since each call starts empty.
    const int penalties = scoreOneTeam(teamMembers, IDsOnTeam, 1, beingTeamedStamp, 1);
    return (penalties == 0) ? 1 : 0;
}

TeammatesCriterion* TeammatesCriterion::findInCriteria(const TeamingOptions *teamingOptions, CriteriaType type)
{
    for (auto *criterion : std::as_const(teamingOptions->criteria)) {
        if (criterion->criteriaType == type) {
            return static_cast<TeammatesCriterion*>(criterion);
        }
    }
    return nullptr;
}

int TeammatesCriterion::scoreOneTeam(const QList<const StudentRecord *> &teamMembers,
                                     const std::vector<uint32_t> &idsOnTeamStamp, uint32_t idsOnTeamGeneration,
                                     const std::vector<uint32_t> &idsBeingTeamedStamp, uint32_t idsBeingTeamedGeneration) const
{
    int penalties = 0;

    auto isOnTeam = [&](long long id) {
        return id >= 0 && id < static_cast<long long>(idsOnTeamStamp.size()) && idsOnTeamStamp[static_cast<size_t>(id)] == idsOnTeamGeneration;
    };
    auto isBeingTeamed = [&](long long id) {
        return id >= 0 && id < static_cast<long long>(idsBeingTeamedStamp.size()) && idsBeingTeamedStamp[static_cast<size_t>(id)] == idsBeingTeamedGeneration;
    };

    if (criteriaType == CriteriaType::groupTogether && haveAnyTeammates) {
        for (const auto *const student : teamMembers) {
            int found = 0;
            int needed = 0;
            for (const auto id : std::as_const(student->groupTogether)) {
                if (isBeingTeamed(id)) {
                    needed++;
                    if (isOnTeam(id)) {
                        found++;
                    }
                }
            }
            if (found < std::min(needed, numberGiven)) {
                penalties++;
            }
        }
    }
    else if (criteriaType == CriteriaType::splitApart && haveAnyTeammates) {
        for (const auto *const student : teamMembers) {
            for (const auto id : std::as_const(student->splitApart)) {
                if (isBeingTeamed(id) && isOnTeam(id)) {
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

QString TeammatesCriterion::teamDisplayText(const TeamRecord &, const DataOptions *, float criterionScore, const QList<StudentRecord> &/*students*/) const
{
    if (IS_NO_SCORE(criterionScore)) {
        return QString::fromUtf8(" ");
    }
    if (criterionScore > 0) {
        return QString::fromUtf8("✓");
    }
    return QString::fromUtf8("✗");
}

QVariant TeammatesCriterion::teamSortValue(const TeamRecord &, const DataOptions *, float criterionScore, const QList<StudentRecord> &/*students*/) const
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
        return student.groupTogether.empty() ? " " : QString(BULLET);
    }
    return student.splitApart.empty() ? " " : QString(BULLET);
}

QString TeammatesCriterion::exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions */*dataOptions*/) const
{
    if (criteriaType == CriteriaType::groupTogether && haveAnyTeammates) {
        QString text = "\n" + tr("Required teammates active");
        if (numberGiven == REQUESTED_TEAMMATES_ALL) {
            text += tr(", all requests granted");
        }
        else {
            text += tr(", up to ") + QString::number(numberGiven) +
                    tr(" per student granted");
        }
        return text;
    }

    if (criteriaType == CriteriaType::splitApart && haveAnyTeammates) {
        return "\n" + tr("Prevented teammates active");
    }

    return {};
}
