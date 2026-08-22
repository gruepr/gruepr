#include "URMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QJsonArray>
#include <algorithm>

Criterion* URMIdentityCriterion::clone() const {
    auto *copy = new URMIdentityCriterion(dataOptions, criteriaType, weight, penaltyStatus);
    copy->identityRules = identityRules;
    return copy;
}

QJsonObject URMIdentityCriterion::settingsToJson() const {
    QJsonObject json = Criterion::settingsToJson();
    QJsonArray rulesArray;
    for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            for (const auto value : values) {
                rulesArray.append(identityKey + "," + operation + "," + QString::number(value));
            }
        }
    }
    json["identityRules"] = rulesArray;
    return json;
}

void URMIdentityCriterion::settingsFromJson(const QJsonObject &json) {
    Criterion::settingsFromJson(json);
    identityRules.clear();
    const QJsonArray rulesArray = json["identityRules"].toArray();
    for (const auto &val : rulesArray) {
        const QStringList parts = val.toString().split(',');
        if (parts.size() == 3) {
            identityRules[parts.at(0)][parts.at(1)].append(parts.at(2).toInt());
        }
    }

    updateRuleCountLabel();
}

QStringList URMIdentityCriterion::identityOptions() const {
    QStringList options;
    for (const QString &resp : std::as_const(dataOptions->URMIdentityResponses)) {
        if (resp != "--") {
            options << resp;
        }
    }
    return options;
}

void URMIdentityCriterion::updateRuleCountLabel() const
{
    if (!ruleCountLabel) {
        return;
    }

    int count = 0;
    for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            count += values.size();
        }
    }
    ruleCountLabel->setText(count == 0 ? tr("No rules set")
                                       : QString::number(count) + (count == 1 ? tr(" rule set") : tr(" rules set")));
}

void URMIdentityCriterion::generateCriteriaCard(TeamingOptions *const /*teamingOptions*/)
{
    auto *urmContentLayout = new QVBoxLayout();

    editRulesButton = new QPushButton(tr("Edit the racial/ethnic identity rules..."), parentCard);
    editRulesButton->setFixedHeight(40);
    editRulesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    editRulesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    urmContentLayout->addWidget(editRulesButton);

    ruleCountLabel = new QLabel(parentCard);
    urmContentLayout->addWidget(ruleCountLabel);

    parentCard->setContentAreaLayout(*urmContentLayout);

    updateRuleCountLabel();

    connect(editRulesButton, &QPushButton::clicked, this, [this]() {
        auto *window = new IdentityRulesDialog(this->parentCard, &identityRules, identityOptions(), tr("Racial/Ethnic Identity Rules"));
        window->exec();
        delete window;
        updateRuleCountLabel();
    });
}

void URMIdentityCriterion::buildRuleTranslation(QHash<QString, int> &identityIndex, QList<int> &groupIdentities,
                                                QList<RuleGroup> &ruleGroups, QList<int> &ruleValues) const
{
    identityIndex.clear();
    groupIdentities.clear();
    ruleGroups.clear();
    ruleValues.clear();
    for(auto rule = identityRules.cbegin(); rule != identityRules.cend(); ++rule) {
        const qsizetype firstIdentity = groupIdentities.size();
        const QStringList identities = rule.key().split('|');
        for(const auto &identity : identities) {
            auto existing = identityIndex.constFind(identity);
            if(existing == identityIndex.cend()) {
                existing = identityIndex.insert(identity, int(identityIndex.size()));
            }
            groupIdentities.append(existing.value());
        }
        const qsizetype numIdentities = groupIdentities.size() - firstIdentity;

        const auto &valMap = rule.value();
        for(auto operation = valMap.cbegin(); operation != valMap.cend(); ++operation) {
            RuleOp op;
            if(operation.key() == "!=") {
                op = opNotEqual;
            }
            else if(operation.key() == "<") {
                op = opLessThan;
            }
            else if(operation.key() == ">") {
                op = opGreaterThan;
            }
            else {
                continue;
            }
            const qsizetype firstValue = ruleValues.size();
            ruleValues << operation.value();
            ruleGroups.append({firstIdentity, numIdentities, op, firstValue, ruleValues.size() - firstValue});
        }
    }
}

bool URMIdentityCriterion::anyRuleGroupTriggered(const QList<int> &responseCounts, const QList<int> &groupIdentities,
                                                 const QList<RuleGroup> &ruleGroups, const QList<int> &ruleValues,
                                                 bool applyPenalty, float &penaltyPointsForTeam)
{
    bool penaltyApplied = false;
    // each rule group may contribute one penalty point
    for (const auto &group : std::as_const(ruleGroups)) {
        int count = 0;
        for(qsizetype i = 0; i < group.numIdentities; i++) {
            count += responseCounts.at(groupIdentities.at(group.firstIdentity + i));
        }
        for(qsizetype i = 0; i < group.numValues; i++) {
            const int val = ruleValues.at(group.firstValue + i);
            if ((group.op == opNotEqual    && count == val) ||
                (group.op == opLessThan    && count >= val) ||
                (group.op == opGreaterThan && count <= val)) {
                penaltyApplied = true;
                if (applyPenalty) {
                    penaltyPointsForTeam += 1.0f;
                }
                break;
            }
        }
    }
    return penaltyApplied;
}

void URMIdentityCriterion::prepareForOptimization(const StudentRecord *students, const int studentIndexes[], int numStudents, const DataOptions * /*dataOptions*/)
{
    QHash<QString, int> identityIndex;
    buildRuleTranslation(identityIndex, cachedGroupIdentities, cachedRuleGroups, cachedRuleValues);
    cachedNumIdentitySlots = identityIndex.size();

    int maxPosition = -1;
    for(int i = 0; i < numStudents; i++) {
        maxPosition = std::max(maxPosition, studentIndexes[i]);
    }
    cachedIdentitySlot.assign(static_cast<size_t>(maxPosition + 1), -1);
    for(int i = 0; i < numStudents; i++) {
        const int position = studentIndexes[i];
        const QString &response = students[position].URMIdentityResponse;
        if (!response.isEmpty() && response != "--") {
            const auto identity = identityIndex.constFind(response);
            if (identity != identityIndex.cend()) {
                cachedIdentitySlot[static_cast<size_t>(position)] = identity.value();
            }
        }
    }
}

void URMIdentityCriterion::calculateScore(const StudentRecord *const /*students*/, const int teammates[], const int numTeams, const int teamSizes[],
                                          const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                          QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    QList<int> responseCounts(cachedNumIdentitySlots, 0);   // allocated once, refilled per team

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        criteriaScores[team] = 1;

        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // Count how many students on the team gave each of the responses named by a rule -- reads
        // the slot precomputed in prepareForOptimization instead of re-hashing URMIdentityResponse.
        responseCounts.fill(0);
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const int position = teammates[studentNum];
            const int slot = (position >= 0 && static_cast<size_t>(position) < cachedIdentitySlot.size())
                                 ? cachedIdentitySlot[static_cast<size_t>(position)] : -1;
            if (slot >= 0) {
                responseCounts[slot]++;
            }
            studentNum++;
        }

        float penalty = 0.0f;
        const bool penaltyApplied = anyRuleGroupTriggered(responseCounts, cachedGroupIdentities, cachedRuleGroups, cachedRuleValues,
                                                          penaltyStatus, penalty);
        penaltyPoints[team] += penalty;

        if (penaltyApplied) {
            criteriaScores[team] = 0;
        }

        criteriaScores[team] *= weight;
        penaltyPoints[team] *= weight;
    }
}

float URMIdentityCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team,
                                                     const TeamingOptions * /*teamingOptions*/, const DataOptions * /*dataOptions*/,
                                                     const QSet<long long> &/*allIDsBeingTeamed*/)
{
    if (identityRules.isEmpty()) {
        return Criterion::NO_SCORE;
    }

    if (team.size == 1) {
        return 1.0f;
    }

    // Self-contained -- deliberately does not touch the optimization-time cache above, since this
    // can be called at any time to redisplay a single team's score, independent of whether/when
    // prepareForOptimization last ran.
    QHash<QString, int> identityIndex;
    QList<int> groupIdentities;
    QList<RuleGroup> ruleGroups;
    QList<int> ruleValues;
    buildRuleTranslation(identityIndex, groupIdentities, ruleGroups, ruleValues);

    QList<int> responseCounts(identityIndex.size(), 0);
    for (const auto studentID : team.studentIDs) {
        const int i = grueprGlobal::findStudentIndex(allStudents, studentID);
        if (i >= allStudents.size()) {
            continue;
        }
        const QString &response = allStudents[i].URMIdentityResponse;
        if (!response.isEmpty() && response != "--") {
            const auto identity = identityIndex.constFind(response);
            if (identity != identityIndex.cend()) {
                responseCounts[identity.value()]++;
            }
        }
    }

    float unusedPenalty = 0.0f;
    const bool penaltyApplied = anyRuleGroupTriggered(responseCounts, groupIdentities, ruleGroups, ruleValues, false, unusedPenalty);
    return penaltyApplied ? 0.0f : 1.0f;
}

QString URMIdentityCriterion::headerLabel(const DataOptions *) const {
    return tr("Racial/Ethnic Identity");
}

Qt::TextElideMode URMIdentityCriterion::headerElideMode() const {
    return Qt::ElideNone;
}

QString URMIdentityCriterion::teamDisplayText(const TeamRecord &, const DataOptions *, float criterionScore, const QList<StudentRecord> &/*students*/) const {
    if (IS_NO_SCORE(criterionScore)) {
        return QString::fromUtf8(" ");
    }
    if (criterionScore > 0) {
        return QString::fromUtf8("✓");
    }
    return QString::fromUtf8("✗");
}

QVariant URMIdentityCriterion::teamSortValue(const TeamRecord &, const DataOptions *, float criterionScore, const QList<StudentRecord> &/*students*/) const {
    if (IS_NO_SCORE(criterionScore)) {
        return 0;
    }
    if (criterionScore > 0) {
        return 1;
    }
    return -1;
}

QString URMIdentityCriterion::studentDisplayText(const StudentRecord &student, const DataOptions *) const {
    return student.URMIdentityResponse;
}

QString URMIdentityCriterion::exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions *) const {
    QString text;
    for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            for (const auto value : std::as_const(values)) {
                const QString displayKey = QString(identityKey).replace('|', tr(" or "));
                text += "\n" + tr("Racial/ethnic identity rule: ") + displayKey + " " +
                        operation + " " + QString::number(value);
            }
        }
    }
    return text;
}

QString URMIdentityCriterion::exportStudentText(const StudentRecord &student) {
    return " " + student.URMIdentityResponse + " ";
}
