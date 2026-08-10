#include "URMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QJsonArray>

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

    // display the settings on the criteria card
    if (ruleCountLabel) {
        int count = 0;
        for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                count += values.size();
            }
        }
        ruleCountLabel->setText(count == 0 ? tr("No rules set")
                                           : QString::number(count) + (count == 1 ? tr(" rule set") : tr(" rules set")));
    }
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


    // Helper to update the rule count label
    auto updateRuleCount = [this]() {
        int count = 0;
        for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                count += values.size();
            }
        }
        ruleCountLabel->setText(count == 0 ? tr("No rules set")
                                           : QString::number(count) + (count == 1 ? tr(" rule set") : tr(" rules set")));
    };

    updateRuleCount();

    connect(editRulesButton, &QPushButton::clicked, this, [this, updateRuleCount]() {
        auto *window = new IdentityRulesDialog(this->parentCard, &identityRules, identityOptions(), tr("Racial/Ethnic Identity Rules"));
        window->exec();
        delete window;
        updateRuleCount();
    });
}

void URMIdentityCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                          const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                          QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    // Translate the rules into integer form once per call. Only the responses actually named by a
    // rule need counting (the rest were never read), so each is given an index and the per-team loop
    // below works on a small array of counts. Rules can be edited between calls, so this is derived
    // fresh each call.
    enum RuleOp {opNotEqual, opLessThan, opGreaterThan};
    struct RuleGroup {
        qsizetype firstIdentity;    // into groupIdentities
        qsizetype numIdentities;
        RuleOp op;
        qsizetype firstValue;       // into ruleValues
        qsizetype numValues;
    };
    QHash<QString, int> identityIndex;
    QList<int> groupIdentities;
    QList<RuleGroup> ruleGroups;
    QList<int> ruleValues;
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
    QList<int> responseCounts(identityIndex.size(), 0);   // allocated once, refilled per team

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        criteriaScores[team] = 1;

        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        bool penaltyApplied = false;

        // Count how many students on the team gave each of the responses named by a rule
        responseCounts.fill(0);
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const QString &response = students[teammates[studentNum]].URMIdentityResponse;
            if (!response.isEmpty() && response != "--") {
                const auto identity = identityIndex.constFind(response);
                if(identity != identityIndex.cend()) {
                    responseCounts[identity.value()]++;
                }
            }
            studentNum++;
        }

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
                    if (penaltyStatus) {
                        penaltyPoints[team] += 1.0f;
                    }
                    break;
                }
            }
        }

        if (penaltyApplied) {
            criteriaScores[team] = 0;
        }

        criteriaScores[team] *= weight;
        penaltyPoints[team] *= weight;
    }
}

float URMIdentityCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team,
                                                     const TeamingOptions *teamingOptions, const DataOptions *dataOptions,
                                                     const QSet<long long> &/*allIDsBeingTeamed*/)
{
    // If there are no URM rules at all, nothing is relevant
    if (identityRules.isEmpty()) {
        return Criterion::NO_SCORE;
    }

    // Use the base class implementation to actually calculate the score
    return Criterion::scoreForOneTeamInDisplay(allStudents, team, teamingOptions, dataOptions);
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
