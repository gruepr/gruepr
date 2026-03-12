#include "URMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"

Criterion* URMIdentityCriterion::clone() const {
    return new URMIdentityCriterion(dataOptions, criteriaType, weight, penaltyStatus);
}

void URMIdentityCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *urmContentLayout = new QVBoxLayout();

    editRulesButton = new QPushButton(tr("Edit the race/ethnicity rules..."), parentCard);
    editRulesButton->setFixedHeight(40);
    editRulesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    editRulesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    urmContentLayout->addWidget(editRulesButton);

    ruleCountLabel = new QLabel(parentCard);
    urmContentLayout->addWidget(ruleCountLabel);

    parentCard->setContentAreaLayout(*urmContentLayout);


    // Helper to update the rule count label
    auto updateRuleCount = [this, teamingOptions]() {
        int count = 0;
        for (const auto [response, valMap] : teamingOptions->urmIdentityRules.asKeyValueRange()) {
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                count += values.size();
            }
        }
        ruleCountLabel->setText(count == 0 ? tr("No rules set")
                                           : QString::number(count) + (count == 1 ? tr(" rule set") : tr(" rules set")));
    };

    updateRuleCount();

    connect(editRulesButton, &QPushButton::clicked, this, [this, teamingOptions, updateRuleCount]() {
        auto *window = new IdentityRulesDialog(this->parentCard, IdentityRulesDialog::Mode::urm, teamingOptions, dataOptions);
        window->exec();
        delete window;
        updateRuleCount();
    });
}

void URMIdentityCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                          const TeamingOptions *const teamingOptions, const DataOptions *const /*dataOptions*/,
                                          std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const
{
    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        criteriaScores[team] = 1;

        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        bool penaltyApplied = false;

        // Count how many URM students on the team
        QMap<QString, int> urmResponseCounts;
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const QString &response = students[teammates[studentNum]].URMResponse;
            if (!response.isEmpty() && response != "--") {
                urmResponseCounts[response]++;
            }
            studentNum++;
        }

        // Apply per-response identity rules from urmIdentityRules
        // First check responses that are present on the team.
        auto applyRule = [&](const QString &response, int count) {
            const auto &unallowed = teamingOptions->urmIdentityRules.value(response).value("!=");
            for (const int val : unallowed) {
                if (count == val) {
                    penaltyApplied = true;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                    break;
                }
            }
        };

        for (auto it = urmResponseCounts.cbegin(); it != urmResponseCounts.cend(); ++it) {
            applyRule(it.key(), it.value());
        }

        // Next check rules for responses NOT present on the team (count == 0).
        // This handles rules like "latino != 0" (require at least one).
        for (const auto &response : teamingOptions->urmIdentityRules.keys()) {
            if (!urmResponseCounts.contains(response)) {
                applyRule(response, 0);
            }
        }

        if (penaltyApplied) {
            criteriaScores[team] = 0;
        }

        criteriaScores[team] *= weight;
    }
}

float URMIdentityCriterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team,
                                                     const TeamingOptions *teamingOptions, const DataOptions *dataOptions,
                                                     const QSet<long long> &/*allIDsBeingTeamed*/)
{
    // If there are no URM rules at all, nothing is relevant
    if (teamingOptions->urmIdentityRules.isEmpty()) {
        return Criterion::NO_SCORE;
    }

    // A rule with "!= 0" means every team is relevant (even teams with zero of that identity violate it).
    // Otherwise, relevance requires at least one team member whose response matches a rule key.
    bool anyRuleWithZero = false;
    for (const auto [response, valMap] : teamingOptions->urmIdentityRules.asKeyValueRange()) {
        if (valMap.value("!=").contains(0)) {
            anyRuleWithZero = true;
            break;
        }
    }

    if (!anyRuleWithZero) {
        bool anyRelevant = false;
        for (const auto studentID : team.studentIDs) {
            for (const auto &student : allStudents) {
                if (student.ID == studentID) {
                    if (teamingOptions->urmIdentityRules.contains(student.URMResponse)) {
                        anyRelevant = true;
                    }
                    break;
                }
            }
            if (anyRelevant) {
                break;
            }
        }

        if (!anyRelevant) {
            return Criterion::NO_SCORE;
        }
    }

    // Use the base class implementation to actually calculate the score
    return Criterion::scoreForOneTeamInDisplay(allStudents, team, teamingOptions, dataOptions);
}

QString URMIdentityCriterion::headerLabel(const DataOptions *) const {
    return tr("Race/Ethnicity");
}

Qt::TextElideMode URMIdentityCriterion::headerElideMode() const {
    return Qt::ElideNone;
}

QString URMIdentityCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *, float criterionScore) const {
    if (IS_NO_SCORE(criterionScore)) {
        return QString::fromUtf8(" ");
    }
    if (criterionScore > 0) {
        return QString::fromUtf8("✓");
    }
    return QString::fromUtf8("✗");
}

QVariant URMIdentityCriterion::teamSortValue(const TeamRecord &team, const DataOptions *, float criterionScore) const {
    if (IS_NO_SCORE(criterionScore)) {
        return 0;
    }
    if (criterionScore > 0) {
        return 1;
    }
    return -1;
}

QString URMIdentityCriterion::studentDisplayText(const StudentRecord &student, const DataOptions *) const {
    return student.URMResponse;
}

QString URMIdentityCriterion::exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *) const {
    QString text;
    for (const auto [response, valMap] : teamingOptions->urmIdentityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            for (const auto value : std::as_const(values)) {
                text += "\n" + tr("Race/ethnicity identity rule: ") + response + " " +
                        operation + " " + QString::number(value);
            }
        }
    }
    return text;
}

QString URMIdentityCriterion::exportStudentText(const StudentRecord &student, const DataOptions *) const {
    return " " + student.URMResponse + " ";
}
