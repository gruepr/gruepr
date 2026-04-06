#include "assignmentPreferenceCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include "widgets/styledComboBox.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>


Criterion* AssignmentPreferenceCriterion::clone() const
{
    auto *copy = new AssignmentPreferenceCriterion(dataOptions, criteriaType, weight, penaltyStatus);
    copy->penalizeNoOneRanked = penalizeNoOneRanked;
    copy->penalizeAnyOneUnranked = penalizeAnyOneUnranked;
    copy->allOptionNames = allOptionNames;
    copy->optionNameToIndex = optionNameToIndex;
    copy->numOptions = numOptions;
    copy->numRankedChoices = numRankedChoices;
    copy->displayAssignment = displayAssignment;
    copy->displayScore = displayScore;
    copy->displayStudentAssignment = displayStudentAssignment;
    return copy;
}

QJsonObject AssignmentPreferenceCriterion::settingsToJson() const
{
    QJsonObject json = Criterion::settingsToJson();
    json["penalizeNoOneRanked"] = penalizeNoOneRanked;
    json["penalizeAnyOneUnranked"] = penalizeAnyOneUnranked;
    json["allOptionNames"] = QJsonArray::fromStringList(allOptionNames);
    json["numRankedChoices"] = numRankedChoices;
    return json;
}

void AssignmentPreferenceCriterion::settingsFromJson(const QJsonObject &json)
{
    Criterion::settingsFromJson(json);
    penalizeNoOneRanked = json["penalizeNoOneRanked"].toBool(false);
    penalizeAnyOneUnranked = json["penalizeAnyOneUnranked"].toBool(false);
    allOptionNames = {};
    const QJsonArray optionNamesArray = json["allOptionNames"].toArray();
    for(const auto &item : optionNamesArray) {
        allOptionNames << item.toString();
    }
    numOptions = allOptionNames.size();
    optionNameToIndex.clear();
    for(int i = 0; i < allOptionNames.size(); i++) {
        optionNameToIndex[allOptionNames[i]] = i;
    }
    numRankedChoices = json["numRankedChoices"].toInt(0);
}


/////////////////////////////////////////////////////////////////////
// UI card
/////////////////////////////////////////////////////////////////////

void AssignmentPreferenceCriterion::generateCriteriaCard(TeamingOptions *const /*teamingOptions*/)
{
    auto *contentLayout = new QVBoxLayout();
    contentLayout->setSpacing(2);

    infoLabel = new QLabel(parentCard);
    infoLabel->setStyleSheet(LABEL12PTSTYLE);
    infoLabel->setWordWrap(true);
    infoLabel->setText(tr("Assign each team a unique option, optimizing for students' ranked preferences."));
    contentLayout->addWidget(infoLabel);

    auto *requirementComboBox = new StyledComboBox(parentCard);
    requirementComboBox->installEventFilter(new MouseWheelBlocker(requirementComboBox));
    requirementComboBox->setFocusPolicy(Qt::StrongFocus);
    requirementComboBox->addItem(tr("No ranking requirement"));
    requirementComboBox->addItem(tr("At least one teammate should have ranked the team's assigned option"));
    requirementComboBox->addItem(tr("All teammates should have ranked the team's assigned option"));
    if(penalizeAnyOneUnranked) {
        requirementComboBox->setCurrentIndex(2);
    } else if(penalizeNoOneRanked) {
        requirementComboBox->setCurrentIndex(1);
    } else {
        requirementComboBox->setCurrentIndex(0);
    }
    connect(requirementComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        penalizeNoOneRanked = (index >= 1);
        penalizeAnyOneUnranked = (index == 2);
    });
    contentLayout->addWidget(requirementComboBox);

    parentCard->setContentAreaLayout(*contentLayout);
}


/////////////////////////////////////////////////////////////////////
// Prepare for optimization — cache option names and index map
/////////////////////////////////////////////////////////////////////

void AssignmentPreferenceCriterion::prepareForOptimization(const StudentRecord *students, int numStudents, const DataOptions */*dataOptions*/)
{
    // Discover all option names from student preferences
    QSet<QString> optionSet;
    int maxK = 0;
    for(int i = 0; i < numStudents; i++) {
        const auto &prefs = students[i].assignmentPreferences;
        maxK = std::max(maxK, static_cast<int>(prefs.size()));
        for(const auto &pref : prefs) {
            if(!pref.isEmpty()) {
                optionSet.insert(pref);
            }
        }
    }

    allOptionNames = optionSet.values();
    std::sort(allOptionNames.begin(), allOptionNames.end());
    optionNameToIndex.clear();
    for(int i = 0; i < allOptionNames.size(); i++) {
        optionNameToIndex[allOptionNames[i]] = i;
    }
    numOptions = allOptionNames.size();
    numRankedChoices = maxK;

    // Clear display caches
    lastAssignmentByTeamID.clear();
    lastScoreByTeamID.clear();
}


/////////////////////////////////////////////////////////////////////
// Hungarian algorithm — O(N^3) min-cost assignment
// Input: square NxN cost matrix (minimize)
// Output: result[row] = column assigned to row
/////////////////////////////////////////////////////////////////////

QList<int> AssignmentPreferenceCriterion::hungarianAlgorithm(const QList<QList<float>> &costMatrix)
{
    const int n = static_cast<int>(costMatrix.size());
    if(n == 0) {
        return {};
    }

    // Uses 1-indexed arrays for clarity (standard textbook formulation)
    const float INF = std::numeric_limits<float>::max();

    QList<float> u(n + 1, 0), v(n + 1, 0);  // potentials for rows and columns
    QList<int> p(n + 1, 0);                   // p[j] = row assigned to column j (0 = unassigned)
    QList<int> way(n + 1, 0);                 // way[j] = column preceding j in the augmenting path

    for(int i = 1; i <= n; i++) {
        // Try to assign row i
        p[0] = i;
        int j0 = 0;  // virtual column 0
        QList<float> minv(n + 1, INF);
        QList<bool> used(n + 1, false);

        do {
            used[j0] = true;
            const int i0 = p[j0];
            float delta = INF;
            int j1 = -1;

            for(int j = 1; j <= n; j++) {
                if(!used[j]) {
                    const float cur = costMatrix[i0 - 1][j - 1] - u[i0] - v[j];
                    if(cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if(minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            }

            for(int j = 0; j <= n; j++) {
                if(used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                }
                else {
                    minv[j] -= delta;
                }
            }

            j0 = j1;
        } while(p[j0] != 0);

        // Update assignment along the augmenting path
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while(j0 != 0);
    }

    // Convert to 0-indexed: result[row] = column
    QList<int> result(n);
    for(int j = 1; j <= n; j++) {
        if(p[j] != 0) {
            result[p[j] - 1] = j - 1;
        }
    }

    return result;
}


/////////////////////////////////////////////////////////////////////
// Build utility matrix and solve assignment
// Returns assignment[team] = option index
// Fills teamScores with per-team normalized scores (0 to 1)
/////////////////////////////////////////////////////////////////////

QList<int> AssignmentPreferenceCriterion::solveAssignment(const StudentRecord *const students, const int teammates[],
                                                                const int numTeams, const int teamSizes[],
                                                                QList<float> &teamScores) const
{
    if(numOptions == 0 || numRankedChoices == 0) {
        teamScores.assign(numTeams, 0.0f);
        return {numTeams, -1};
    }

    // Build square matrix of size max(numTeams, numOptions)
    // We maximize utility, but Hungarian minimizes cost, so we use cost = maxUtility - utility
    const int dim = std::max(numTeams, numOptions);

    // First pass: compute utility matrix and find max utility for cost conversion
    QList<QList<float>> utilityMatrix(dim, QList<float>(dim, 0.0f));

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        for(int m = 0; m < teamSizes[team]; m++) {
            const auto &prefs = students[teammates[studentNum]].assignmentPreferences;
            for(int r = 0; r < prefs.size() && r < numRankedChoices; r++) {
                auto it = optionNameToIndex.find(prefs[r]);
                if(it != optionNameToIndex.end()) {
                    utilityMatrix[team][it.value()] += static_cast<float>(numRankedChoices - r);
                }
            }
            studentNum++;
        }
    }
    // Dummy rows (teams beyond numTeams) and dummy columns (options beyond numOptions) stay at 0 utility

    float maxUtility = 0.0f;
    for(int i = 0; i < dim; i++) {
        for(int j = 0; j < dim; j++) {
            maxUtility = std::max(maxUtility, utilityMatrix[i][j]);
        }
    }

    // Convert to cost matrix: cost = maxUtility - utility
    QList<QList<float>> costMatrix(dim, QList<float>(dim, 0.0f));
    for(int i = 0; i < dim; i++) {
        for(int j = 0; j < dim; j++) {
            costMatrix[i][j] = maxUtility - utilityMatrix[i][j];
        }
    }

    // Solve
    const QList<int> assignment = hungarianAlgorithm(costMatrix);

    // Extract per-team scores
    teamScores.resize(numTeams);
    for(int team = 0; team < numTeams; team++) {
        const int assignedOption = assignment[team];
        const float utility = utilityMatrix[team][assignedOption];
        const auto maxPossible = static_cast<float>(teamSizes[team] * numRankedChoices);
        teamScores[team] = (maxPossible > 0.0f) ? (utility / maxPossible) : 0.0f;
    }

    return assignment;
}


/////////////////////////////////////////////////////////////////////
// calculateScore — called by the GA for every genome
/////////////////////////////////////////////////////////////////////

void AssignmentPreferenceCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                                   const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                                   QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    QList<float> teamScores;
    const QList<int> assignment = solveAssignment(students, teammates, numTeams, teamSizes, teamScores);

    for(int team = 0; team < numTeams; team++) {

        // Penalty: if enabled and the assigned option was left unranked by any or all team members
        if((penalizeNoOneRanked || penalizeAnyOneUnranked) && assignment[team] >= 0 && assignment[team] < numOptions) {
            const QString &assignedName = allOptionNames[assignment[team]];
            int studentNum = 0;
            for(int t = 0; t < team; t++) {
                studentNum += teamSizes[t];
            }
            bool anyoneRanked = false;
            bool everyoneRanked = true;
            for(int m = 0; m < teamSizes[team]; m++) {
                if(students[teammates[studentNum + m]].assignmentPreferences.contains(assignedName)) {
                    anyoneRanked = true;
                }
                else {
                    everyoneRanked = false;
                }
            }
            if(penalizeNoOneRanked && !anyoneRanked) {
                penaltyPoints[team] += 1.0f;
            }
            if(penalizeAnyOneUnranked && !everyoneRanked) {
                penaltyPoints[team] += 1.0f;
            }
        }

        criteriaScores[team] = teamScores[team] * weight;
        penaltyPoints[team] *= weight;
    }
}


/////////////////////////////////////////////////////////////////////
// prepareForDisplay — solve the full assignment for all teams
/////////////////////////////////////////////////////////////////////

void AssignmentPreferenceCriterion::prepareForDisplay(const QList<StudentRecord> &students, const TeamSet &teams)
{
    displayAssignment.clear();
    displayScore.clear();
    displayStudentAssignment.clear();

    if(numOptions == 0 || numRankedChoices == 0 || teams.isEmpty()) {
        return;
    }

    const int numTeams = teams.size();
    const int dim = std::max(numTeams, numOptions);

    // Build utility matrix
    QList<QList<float>> utilityMatrix(dim, QList<float>(dim, 0.0f));

    for(int t = 0; t < numTeams; t++) {
        const auto &team = teams[t];
        for(const auto studentID : team.studentIDs) {
            for(const auto &student : students) {
                if(student.ID == studentID) {
                    const auto &prefs = student.assignmentPreferences;
                    for(int r = 0; r < prefs.size() && r < numRankedChoices; r++) {
                        auto it = optionNameToIndex.find(prefs[r]);
                        if(it != optionNameToIndex.end()) {
                            utilityMatrix[t][it.value()] += static_cast<float>(numRankedChoices - r);
                        }
                    }
                    break;
                }
            }
        }
    }

    // Convert to cost matrix
    float maxUtility = 0.0f;
    for(int i = 0; i < dim; i++) {
        for(int j = 0; j < dim; j++) {
            maxUtility = std::max(maxUtility, utilityMatrix[i][j]);
        }
    }
    QList<QList<float>> costMatrix(dim, QList<float>(dim, 0.0f));
    for(int i = 0; i < dim; i++) {
        for(int j = 0; j < dim; j++) {
            costMatrix[i][j] = maxUtility - utilityMatrix[i][j];
        }
    }

    // Solve
    const QList<int> assignment = hungarianAlgorithm(costMatrix);

    // Cache results for every team
    for(int t = 0; t < numTeams; t++) {
        const auto &team = teams[t];
        if(team.studentIDs.isEmpty()) {
            continue;
        }
        const long long teamKey = team.studentIDs.first();
        const int assignedOption = assignment[t];
        const float utility = utilityMatrix[t][assignedOption];
        const auto maxPossible = static_cast<float>(team.size * numRankedChoices);
        float score = (maxPossible > 0.0f) ? (utility / maxPossible) : 0.0f;

        // Apply penalty for display if enabled and any team member or no team member ranked the assigned option
        if((penalizeNoOneRanked || penalizeAnyOneUnranked) && assignedOption >= 0 && assignedOption < numOptions) {
            const QString &assignedName = allOptionNames[assignedOption];
            bool anyoneRanked = false;
            bool everyoneRanked = true;
            for(const auto studentID : team.studentIDs) {
                for(const auto &student : students) {
                    if(student.ID == studentID) {
                        if(student.assignmentPreferences.contains(assignedName)) {
                            anyoneRanked = true;
                        }
                        else {
                            everyoneRanked = false;
                        }
                        break;
                    }
                }
            }
            if(penalizeNoOneRanked && !anyoneRanked) {
                score = 0.0f;
            }
            if(penalizeAnyOneUnranked && !everyoneRanked) {
                score = 0.0f;
            }
        }

        displayScore[teamKey] = score;
        if(assignedOption >= 0 && assignedOption < numOptions) {
            displayAssignment[teamKey] = allOptionNames[assignedOption];
        }
    }

    // Also cache the assignment for each student (so studentDisplayText can look it up)
    displayStudentAssignment.clear();
    for(int t = 0; t < numTeams; t++) {
        const auto &team = teams[t];
        const int assignedOption = assignment[t];
        const QString optionName = (assignedOption >= 0 && assignedOption < numOptions) ? allOptionNames[assignedOption] : QString();
        for(const auto studentID : team.studentIDs) {
            displayStudentAssignment[studentID] = optionName;
        }
    }
}


/////////////////////////////////////////////////////////////////////
// scoreForOneTeamInDisplay — return cached score from prepareForDisplay
/////////////////////////////////////////////////////////////////////

float AssignmentPreferenceCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &/*allStudents*/, const TeamRecord &team,
                                                              const TeamingOptions */*teamingOptions*/, const DataOptions */*dataOptions*/,
                                                              const QSet<long long> &/*allIDsBeingTeamed*/)
{
    if(numOptions == 0 || numRankedChoices == 0 || team.studentIDs.isEmpty()) {
        return NO_SCORE;
    }

    const long long teamKey = team.studentIDs.first();
    auto it = displayScore.find(teamKey);
    if(it != displayScore.end()) {
        return it.value();
    }

    return NO_SCORE;
}


/////////////////////////////////////////////////////////////////////
// Display methods
/////////////////////////////////////////////////////////////////////

QString AssignmentPreferenceCriterion::headerLabel(const DataOptions */*dataOptions*/) const
{
    return QObject::tr("Assignment");
}

Qt::TextElideMode AssignmentPreferenceCriterion::headerElideMode() const
{
    return Qt::ElideRight;
}

QString AssignmentPreferenceCriterion::teamDisplayText(const TeamRecord &team, const DataOptions */*dataOptions*/,
                                                       float /*criterionScore*/, const QList<StudentRecord> &/*students*/) const
{
    if(team.studentIDs.isEmpty()) {
        return {};
    }

    const long long teamKey = team.studentIDs.first();
    auto it = displayAssignment.find(teamKey);
    if(it != displayAssignment.end()) {
        return it.value();
    }

    return "?";
}

QVariant AssignmentPreferenceCriterion::teamSortValue(const TeamRecord &team, const DataOptions */*dataOptions*/,
                                                      float criterionScore, const QList<StudentRecord> &/*students*/) const
{
    if(team.studentIDs.isEmpty()) {
        return 0.0f;
    }
    return criterionScore;
}

QString AssignmentPreferenceCriterion::studentDisplayText(const StudentRecord &student, const DataOptions */*dataOptions*/) const
{
    if(student.assignmentPreferences.isEmpty()) {
        return "—";
    }

    // Look up which option was assigned to this student's team
    auto it = displayStudentAssignment.find(student.ID);
    if(it != displayStudentAssignment.end() && !it.value().isEmpty()) {
        const QString &assignedOption = it.value();
        // Find what rank the student gave this option
        const int rank = student.assignmentPreferences.indexOf(assignedOption);
        if(rank >= 0) {
            return (rank == 0? QString(RANKYOURFIRSTCHOICE) : ( QString(RANKYOURCHOICE) + " " + QString::number(rank + 1)));
        }
        return "—";  // student didn't rank the assigned option
    }

    // Fallback if prepareForDisplay hasn't run: show compact list
    QStringList display;
    for(int i = 0; i < student.assignmentPreferences.size(); i++) {
        display << QString::number(i + 1) + ". " + student.assignmentPreferences[i];
    }
    return display.join(", ");
}

Qt::AlignmentFlag AssignmentPreferenceCriterion::teamTextAlignment() const
{
    return Qt::AlignLeft;
}


/////////////////////////////////////////////////////////////////////
// Export methods
/////////////////////////////////////////////////////////////////////

QString AssignmentPreferenceCriterion::exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions */*dataOptions*/) const
{
    QString text = QObject::tr("Team Assignment Preferences: each team assigned a unique option based on members' ranked preferences.");
    if(penalizeNoOneRanked) {
        text += " " + QObject::tr("Penalty applied when no team member ranked the assigned option.");
    }
    if(penalizeAnyOneUnranked) {
        text += " " + QObject::tr("Penalty applied when any team member did not rank the assigned option.");
    }    return text;
}

QString AssignmentPreferenceCriterion::exportStudentText(const StudentRecord &student, const DataOptions */*dataOptions*/) const
{
    if(student.assignmentPreferences.isEmpty()) {
        return QObject::tr("  No preferences");
    }
    return "  " + student.assignmentPreferences.join(", ");
}
