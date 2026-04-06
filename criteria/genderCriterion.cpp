#include "genderCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QJsonArray>

Criterion* GenderCriterion::clone() const {
    auto *copy = new GenderCriterion(dataOptions, criteriaType, weight, penaltyStatus);
    copy->identityRules = identityRules;
    return copy;
}

QJsonObject GenderCriterion::settingsToJson() const {
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

void GenderCriterion::settingsFromJson(const QJsonObject &json) {
    Criterion::settingsFromJson(json);
    identityRules.clear();
    const QJsonArray rulesArray = json["identityRules"].toArray();
    for (const auto &val : rulesArray) {
        const QStringList parts = val.toString().split(',');
        if (parts.size() == 3) {
            identityRules[parts.at(0)][parts.at(1)].append(parts.at(2).toInt());
        }
    }

    // Backwards compat: if old save had singleGenderPrevented, convert to rules:
    if (json.contains("singleGenderPrevented") && json["singleGenderPrevented"].toBool()) {
        if (!identityRules[womanKey]["!="].contains(0)) {
            identityRules[womanKey]["!="].append(0);
        }
        if (!identityRules[manKey]["!="].contains(0)) {
            identityRules[manKey]["!="].append(0);
        }
    }

    // display settings in the card
    if (isolatedWomen) {
        isolatedWomen->setChecked(identityRules[womanKey]["!="].contains(1));
    }
    if (isolatedMen) {
        isolatedMen->setChecked(identityRules[manKey]["!="].contains(1));
    }
    if (isolatedNonbinary) {
        isolatedNonbinary->setChecked(identityRules[nonbinaryKey]["!="].contains(1));
    }
    if (mixedGender) {
        mixedGender->setChecked(identityRules[womanKey]["!="].contains(0) && identityRules[manKey]["!="].contains(0));
    }

    updateComplicatedRuleCountLabel();
}


void GenderCriterion::generateCriteriaCard(TeamingOptions *const /*teamingOptions*/)
{
    auto *genderContentLayout = new QVBoxLayout();
    isolatedWomen = new QCheckBox(tr("Prevent isolated women"), parentCard);
    isolatedWomen->setStyleSheet(CHECKBOXSTYLE);
    isolatedWomen->setChecked(identityRules[womanKey]["!="].contains(1));
    isolatedMen = new QCheckBox(tr("Prevent isolated men"));
    isolatedMen->setStyleSheet(CHECKBOXSTYLE);
    isolatedMen->setChecked(identityRules[manKey]["!="].contains(1));
    isolatedNonbinary = new QCheckBox(tr("Prevent isolated nonbinary students"), parentCard);
    isolatedNonbinary->setStyleSheet(CHECKBOXSTYLE);
    isolatedNonbinary->setChecked(identityRules[nonbinaryKey]["!="].contains(1));
    mixedGender = new QCheckBox(tr("Require mixed gender teams"), parentCard);
    mixedGender->setStyleSheet(CHECKBOXSTYLE);
    mixedGender->setChecked(identityRules[womanKey]["!="].contains(0) && identityRules[manKey]["!="].contains(0));
    complicatedGenderRule = new QPushButton(tr("Something more complicated..."), parentCard);
    complicatedGenderRule->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    complicatedGenderRule->setFixedHeight(40);
    complicatedGenderRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    complicatedRuleCountLabel = new QLabel(parentCard);

    genderContentLayout->addWidget(isolatedWomen);
    genderContentLayout->addWidget(isolatedMen);
    genderContentLayout->addWidget(isolatedNonbinary);
    genderContentLayout->addWidget(mixedGender);
    genderContentLayout->addWidget(complicatedGenderRule);
    genderContentLayout->addWidget(complicatedRuleCountLabel);
    parentCard->setContentAreaLayout(*genderContentLayout);

    updateComplicatedRuleCountLabel();

    connect(isolatedWomen, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!identityRules[womanKey]["!="].contains(1)){
                identityRules[womanKey]["!="].append(1);
            }
        } else {
            identityRules[womanKey]["!="].removeOne(1);
            if (identityRules[womanKey]["!="].isEmpty()){
                identityRules[womanKey].remove("!=");
            }
        }
    });
    connect(isolatedMen, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!identityRules[manKey]["!="].contains(1)){
                identityRules[manKey]["!="].append(1);
            }
        } else {
            identityRules[manKey]["!="].removeOne(1);
            if (identityRules[manKey]["!="].isEmpty()){
                identityRules[manKey].remove("!=");
            }
        }
    });
    connect(isolatedNonbinary, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!identityRules[nonbinaryKey]["!="].contains(1)){
                identityRules[nonbinaryKey]["!="].append(1);
            }
        } else {
            identityRules[nonbinaryKey]["!="].removeOne(1);
            if (identityRules[nonbinaryKey]["!="].isEmpty()){
                identityRules[nonbinaryKey].remove("!=");
            }
        }
    });
    connect(mixedGender, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!identityRules[womanKey]["!="].contains(0)) {
                identityRules[womanKey]["!="].append(0);
            }
            if (!identityRules[manKey]["!="].contains(0)) {
                identityRules[manKey]["!="].append(0);
            }
        } else {
            identityRules[womanKey]["!="].removeOne(0);
            if (identityRules[womanKey]["!="].isEmpty()) {
                identityRules[womanKey].remove("!=");
            }
            if (identityRules[womanKey].isEmpty()) {
                identityRules.remove(womanKey);
            }
            identityRules[manKey]["!="].removeOne(0);
            if (identityRules[manKey]["!="].isEmpty()) {
                identityRules[manKey].remove("!=");
            }
            if (identityRules[manKey].isEmpty()) {
                identityRules.remove(manKey);
            }
        }
    });
    connect(complicatedGenderRule, &QPushButton::clicked, this, [this]() {
         auto *window = new IdentityRulesDialog(this->parentCard, &identityRules, identityOptions(), tr("Gender Identity Rules"));
         window->exec();
         isolatedWomen->setChecked(identityRules[womanKey]["!="].contains(1));
         isolatedMen->setChecked(identityRules[manKey]["!="].contains(1));
         isolatedNonbinary->setChecked(identityRules[nonbinaryKey]["!="].contains(1));
         mixedGender->blockSignals(true);
         mixedGender->setChecked(identityRules[womanKey]["!="].contains(0) && identityRules[manKey]["!="].contains(0));
         mixedGender->blockSignals(false);
         delete window;
         updateComplicatedRuleCountLabel();
    });
}

void GenderCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                     const TeamingOptions *const /*teamingOptions*/, const DataOptions *const /*dataOptions*/,
                                     QList<float> &criteriaScores, QList<float> &penaltyPoints) const
{
    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        criteriaScores[team] = 1;
        bool penaltyApplied = false;

        if(teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // Count how many of each gender on the team
        QMap<QString, int> genderCounts;
        for (int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &student = students[teammates[studentNum]];
            if (student.gender.contains(Gender::woman)) {
                genderCounts[womanKey]++;
            }
            if (student.gender.contains(Gender::man)) {
                genderCounts[manKey]++;
            }
            if (student.gender.contains(Gender::nonbinary)) {
                genderCounts[nonbinaryKey]++;
            }
            studentNum++;
        }

        auto applyRule = [&](const QString &ruleKey) {
            const QStringList identities = ruleKey.split('|');
            int count = 0;
            for (const QString &identity : identities) {
                count += genderCounts.value(identity, 0);
            }
            const auto &valMap = identityRules.value(ruleKey);
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                for (const int val : values) {
                    if ((operation == "!=" && count == val) ||
                        (operation == "<"  && count >= val) ||
                        (operation == ">"  && count <= val)) {
                        penaltyApplied = true;
                        if (penaltyStatus) {
                            penaltyPoints[team] += 1.0f;
                        }
                        break;
                    }
                }
            }
        };

        for (const auto &ruleKey : identityRules.keys()) {
            applyRule(ruleKey);
        }

        if (penaltyApplied) {
            criteriaScores[team] = 0;
        }
        criteriaScores[team] *= weight;
        penaltyPoints[team] *= weight;
    }
}

QStringList GenderCriterion::identityOptions() const {
    // returning all gender values in the data, converted to standard values
    QStringList options;
    for (const Gender g : std::as_const(dataOptions->genderValues)) {
        if (g != Gender::unknown) {
            options << grueprGlobal::genderToString(g);
        }
    }
    return options;
}

void GenderCriterion::updateComplicatedRuleCountLabel() const
{
    if (!complicatedRuleCountLabel) {
        return;
    }

    // Total up all rules
    int totalCount = 0;
    for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            totalCount += values.size();
        }
    }

    // Subtract the ones represented by checkboxes
    int checkboxCount = 0;
    if (identityRules.value(womanKey).value("!=").contains(1)) {
        checkboxCount++;
    }
    if (identityRules.value(manKey).value("!=").contains(1)) {
        checkboxCount++;
    }
    if (identityRules.value(nonbinaryKey).value("!=").contains(1)) {
        checkboxCount++;
    }
    if (identityRules.value(womanKey).value("!=").contains(0)) {
        checkboxCount++;
    }
    if (identityRules.value(manKey).value("!=").contains(0)) {
        checkboxCount++;
    }

    const int complicatedCount = totalCount - checkboxCount;
    if (complicatedCount <= 0) {
        complicatedRuleCountLabel->clear();
    } else {
        complicatedRuleCountLabel->setText(QString::number(complicatedCount) +
                                           (complicatedCount == 1 ? tr(" additional rule set") : tr(" additional rules set")));
    }
}

QString GenderCriterion::headerLabel(const DataOptions *dataOptions) const {
    return (dataOptions->genderType == GenderType::pronoun) ? tr("Pronouns") : tr("Gender");
}

Qt::TextElideMode GenderCriterion::headerElideMode() const {
    return Qt::ElideNone;
}

QString GenderCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float /*criterionScore*/, const QList<StudentRecord> &/*students*/) const {
    QStringList genderInitials;
    if (dataOptions->genderType == GenderType::biol) {
        genderInitials = QString(BIOLGENDERSINITIALS).split('/');
    } else if (dataOptions->genderType == GenderType::adult) {
        genderInitials = QString(ADULTGENDERSINITIALS).split('/');
    } else if (dataOptions->genderType == GenderType::child) {
        genderInitials = QString(CHILDGENDERSINITIALS).split('/');
    } else {
        genderInitials = QString(PRONOUNSINITIALS).split('/');
    }

    QString genderText;
    if (team.numWomen > 0) {
        genderText += QString::number(team.numWomen) + genderInitials.at(static_cast<int>(Gender::woman));
        if (team.numMen > 0 || team.numNonbinary > 0 || team.numUnknown > 0) {
            genderText += ", ";
        }
    }
    if (team.numMen > 0) {
        genderText += QString::number(team.numMen) + genderInitials.at(static_cast<int>(Gender::man));
        if (team.numNonbinary > 0 || team.numUnknown > 0) {
            genderText += ", ";
        }
    }
    if (team.numNonbinary > 0) {
        genderText += QString::number(team.numNonbinary) + genderInitials.at(static_cast<int>(Gender::nonbinary));
        if (team.numUnknown > 0) {
            genderText += ", ";
        }
    }
    if (team.numUnknown > 0) {
        genderText += QString::number(team.numUnknown) + genderInitials.at(static_cast<int>(Gender::unknown));
    }
    return genderText;
}

QVariant GenderCriterion::teamSortValue(const TeamRecord &team, const DataOptions *, float /*criterionScore*/, const QList<StudentRecord> &/*students*/) const {
    return team.numMen - team.numWomen;
}

QString GenderCriterion::studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const {
    QStringList genderOptions;
    if (dataOptions->genderType == GenderType::biol) {
        genderOptions = QString(BIOLGENDERS).split('/');
    } else if (dataOptions->genderType == GenderType::adult) {
        genderOptions = QString(ADULTGENDERS).split('/');
    } else if (dataOptions->genderType == GenderType::child) {
        genderOptions = QString(CHILDGENDERS).split('/');
    } else {
        genderOptions = QString(PRONOUNS).split('/');
    }

    QString text;
    bool first = true;
    for (const auto gen : student.gender) {
        if (!first) {
            text += ", ";
        }
        text += genderOptions.at(static_cast<int>(gen));
        first = false;
    }
    return text;
}

QString GenderCriterion::exportTeamingOptionText(const TeamingOptions */*teamingOptions*/, const DataOptions *) const {
    QString text;
    for (const auto [identityKey, valMap] : identityRules.asKeyValueRange()) {
        for (const auto [operation, values] : valMap.asKeyValueRange()) {
            for (const auto value : std::as_const(values)) {
                const QString displayKey = QString(identityKey).replace('|', tr(" or "));
                text += "\n" + tr("Gender identity rule: ") + displayKey + " " +
                        operation + " " + QString::number(value);
            }
        }
    }
    return text;
}

QString GenderCriterion::exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const {
    QStringList genderOptions;
    if (dataOptions->genderType == GenderType::biol) {
        genderOptions = QString(BIOLGENDERS7CHAR).split('/');
    } else if (dataOptions->genderType == GenderType::adult) {
        genderOptions = QString(ADULTGENDERS7CHAR).split('/');
    } else if (dataOptions->genderType == GenderType::child) {
        genderOptions = QString(CHILDGENDERS7CHAR).split('/');
    } else {
        genderOptions = QString(PRONOUNS9CHAR).split('/');
    }
    QString text;
    bool first = true;
    for (const auto gen : student.gender) {
        if (!first) {
            text += ", ";
        }
        text += genderOptions.at(static_cast<int>(gen));
        first = false;
    }
    return " " + text + " ";
}
