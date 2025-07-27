#include "genderCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
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
    complicatedGenderRule = new QPushButton(tr("Something more complicated"), parentCard);
    complicatedGenderRule->setFixedHeight(40);
    complicatedGenderRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    genderContentLayout->addWidget(isolatedWomen);
    genderContentLayout->addWidget(isolatedMen);
    genderContentLayout->addWidget(isolatedNonbinary);
    genderContentLayout->addWidget(mixedGender);
    genderContentLayout->addWidget(complicatedGenderRule);
    parentCard->setContentAreaLayout(*genderContentLayout);
}
