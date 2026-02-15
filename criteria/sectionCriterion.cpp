#include "sectionCriterion.h"
#include "gruepr_globals.h"
#include "teamingOptions.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QComboBox>
#include <QPushButton>

void SectionCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto *sectionContentLayout = new QHBoxLayout();
    sectionContentLayout->setSpacing(1);

    editSectionNameButton = new QPushButton(parentCard);
    editSectionNameButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    editSectionNameButton->setIcon(QIcon(":/icons_new/edit.png"));
    editSectionNameButton->setToolTip(tr("Edit the section names"));
    editSectionNameButton->setMinimumHeight(30);
    editSectionNameButton->setMinimumWidth(34);
    editSectionNameButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sectionSelectionBox = new QComboBox(parentCard);
    sectionSelectionBox->installEventFilter(new MouseWheelBlocker(sectionSelectionBox));
    sectionSelectionBox->setFocusPolicy(Qt::StrongFocus);
    sectionSelectionBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    sectionContentLayout->addWidget(sectionSelectionBox);
    sectionContentLayout->addWidget(editSectionNameButton);
    parentCard->setContentAreaLayout(*sectionContentLayout);
}
