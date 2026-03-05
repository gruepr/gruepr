#include "genderCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"

void GenderCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *genderContentLayout = new QVBoxLayout();
    isolatedWomen = new QCheckBox(tr("Prevent isolated women"), parentCard);
    isolatedWomen->setChecked(teamingOptions->genderIdentityRules[Gender::woman]["!="].contains(1));
    isolatedMen = new QCheckBox(tr("Prevent isolated men"));
    isolatedMen->setChecked(teamingOptions->genderIdentityRules[Gender::man]["!="].contains(1));
    isolatedNonbinary = new QCheckBox(tr("Prevent isolated nonbinary students"), parentCard);
    isolatedNonbinary->setChecked(teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].contains(1));
    mixedGender = new QCheckBox(tr("Require mixed gender teams"), parentCard);
    mixedGender->setChecked(teamingOptions->singleGenderPrevented);
    // complicatedGenderRule = new QPushButton(tr("Something more complicated"), parentCard);
    // complicatedGenderRule->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    // complicatedGenderRule->setFixedHeight(40);
    // complicatedGenderRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    genderContentLayout->addWidget(isolatedWomen);
    genderContentLayout->addWidget(isolatedMen);
    genderContentLayout->addWidget(isolatedNonbinary);
    genderContentLayout->addWidget(mixedGender);
    // genderContentLayout->addWidget(complicatedGenderRule);
    parentCard->setContentAreaLayout(*genderContentLayout);   

    connect(isolatedWomen, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!teamingOptions->genderIdentityRules[Gender::woman]["!="].contains(1)){
                teamingOptions->genderIdentityRules[Gender::woman]["!="].append(1);
            }
        } else {
            teamingOptions->genderIdentityRules[Gender::woman]["!="].removeOne(1);
            if (teamingOptions->genderIdentityRules[Gender::woman]["!="].isEmpty()){
                teamingOptions->genderIdentityRules[Gender::woman].remove("!=");
            }
        }
    });
    connect(isolatedMen, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!teamingOptions->genderIdentityRules[Gender::man]["!="].contains(1)){
                teamingOptions->genderIdentityRules[Gender::man]["!="].append(1);
            }
        } else {
            teamingOptions->genderIdentityRules[Gender::man]["!="].removeOne(1);
            if (teamingOptions->genderIdentityRules[Gender::man]["!="].isEmpty()){
                teamingOptions->genderIdentityRules[Gender::man].remove("!=");
            }
        }
    });
    connect(isolatedNonbinary, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        if (state == Qt::Checked) {
            if (!teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].contains(1)){
                teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].append(1);
            }
        } else {
            teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].removeOne(1);
            if (teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].isEmpty()){
                teamingOptions->genderIdentityRules[Gender::nonbinary].remove("!=");
            }
        }
    });
    connect(mixedGender, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        teamingOptions->singleGenderPrevented = (state == Qt::Checked);
    });
    // connect(complicatedGenderRule, &QPushButton::clicked, this, [this, teamingOptions]() {
    //     //FROMDEV: Need to generalize so that not only one gender identity at a time
    //     auto *window = new IdentityRulesDialog(this->parentCard, Gender::woman, teamingOptions, dataOptions);
    //     window->exec();
    //     isolatedWomen->setChecked(teamingOptions->genderIdentityRules[Gender::woman]["!="].contains(1));
    //     isolatedMen->setChecked(teamingOptions->genderIdentityRules[Gender::man]["!="].contains(1));
    //     isolatedNonbinary->setChecked(teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].contains(1));
    // });
}

void GenderCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                     const TeamingOptions *const teamingOptions, const DataOptions *const /*dataOptions*/,
                                     std::vector<float> &criteriaScores, std::vector<int> &penaltyPoints) const
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
        int numWomen = 0;
        int numMen = 0;
        int numNonbinary = 0;
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            if(students[teammates[studentNum]].gender.contains(Gender::man)) {
                numMen++;
            }
            else if(students[teammates[studentNum]].gender.contains(Gender::woman)) {
                numWomen++;
            }
            else if(students[teammates[studentNum]].gender.contains(Gender::nonbinary)) {
                numNonbinary++;
            }
            studentNum++;
        }

        auto applyRule = [&](Gender g, int count) {
            const auto& unallowed = teamingOptions->genderIdentityRules.value(g).value("!=");
            for (int val : unallowed) {
                if (count == val) {
                    penaltyApplied = true;
                    if (penaltyStatus) {
                        penaltyPoints[team]++;
                    }
                    break;
                }
            }
        };

        applyRule(Gender::woman, numWomen);
        applyRule(Gender::man, numMen);
        applyRule(Gender::nonbinary, numNonbinary);

        if (teamingOptions->singleGenderPrevented && (numMen == 0 || numWomen == 0)) {
            penaltyApplied = true;
            if (penaltyStatus) {
                penaltyPoints[team]++;
            }
        }

        if (penaltyApplied) {
            criteriaScores[team] = 0;
        }
        criteriaScores[team] *= weight;
    }
}

QString GenderCriterion::headerLabel(const DataOptions *dataOptions) const {
    return (dataOptions->genderType == GenderType::pronoun) ? tr("Pronouns") : tr("Gender");
}

Qt::TextElideMode GenderCriterion::headerElideMode() const {
    return Qt::ElideNone;
}

QString GenderCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float /*criterionScore*/) const {
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
        if (team.numMen > 0 || team.numNonbinary > 0 || team.numUnknown > 0) genderText += ", ";
    }
    if (team.numMen > 0) {
        genderText += QString::number(team.numMen) + genderInitials.at(static_cast<int>(Gender::man));
        if (team.numNonbinary > 0 || team.numUnknown > 0) genderText += ", ";
    }
    if (team.numNonbinary > 0) {
        genderText += QString::number(team.numNonbinary) + genderInitials.at(static_cast<int>(Gender::nonbinary));
        if (team.numUnknown > 0) genderText += ", ";
    }
    if (team.numUnknown > 0) {
        genderText += QString::number(team.numUnknown) + genderInitials.at(static_cast<int>(Gender::unknown));
    }
    return genderText;
}

QVariant GenderCriterion::teamSortValue(const TeamRecord &team, const DataOptions *, float /*criterionScore*/) const {
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
        if (!first) text += ", ";
        text += genderOptions.at(static_cast<int>(gen));
        first = false;
    }
    return text;
}
