#include "URMIdentityCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "dialogs/gatherURMResponsesDialog.h"
#include "dialogs/identityRulesDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"

void URMIdentityCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *urmContentLayout = new QVBoxLayout();
    isolatedURM = new QCheckBox(tr("Prevent Isolated URM"), parentCard);
    isolatedURM->setChecked(teamingOptions->isolatedURMPrevented);
    chooseURMResponses = new QPushButton(tr("Set Responses Considered Underrepresented"), parentCard);
    chooseURMResponses->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    // complicatedURMRule = new QPushButton(tr("Something more complicated"), parentCard);
    // complicatedURMRule->setFixedHeight(40);
    // complicatedURMRule->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    urmContentLayout->addWidget(isolatedURM);
    urmContentLayout->addWidget(chooseURMResponses);
    // urmContentLayout->addWidget(complicatedURMRule);
    parentCard->setContentAreaLayout(*urmContentLayout);

    connect(isolatedURM, &QCheckBox::checkStateChanged, this, [teamingOptions](Qt::CheckState state) {
        teamingOptions->isolatedURMPrevented = (state == Qt::Checked);
    });
    connect(chooseURMResponses, &QPushButton::clicked, this, [this, teamingOptions]() {
        auto *win = new gatherURMResponsesDialog(dataOptions->URMResponses, teamingOptions->URMResponsesConsideredUR, this->parentCard);

        //If user clicks OK, replace the responses considered underrepresented with the set from the window
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            teamingOptions->URMResponsesConsideredUR = win->URMResponsesConsideredUR;
            teamingOptions->URMResponsesConsideredUR.removeDuplicates();
        }

        delete win;
    });

    // connect(complicatedURMRule, &QPushButton::clicked, this, [this, teamingOptions]() {
    //     //FROMDEV: Need to generalize so that not only one identity at a time
    //     auto *window = new IdentityRulesDialog(this->parentCard, /*Gender::woman*/, teamingOptions, dataOptions);
    //     window->exec();
    //     isolatedURM->setChecked(teamingOptions->URMIdentityRules[/*Gender::woman*/]["!="].contains(1));
    // });
}
