#include "dayNamesDialog.h"
#include "gruepr_globals.h"
#include <QDialogButtonBox>
#include <QGridLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to select / name days for the schedule in the survey
/////////////////////////////////////////////////////////////////////////////////////////////////////////

dayNamesDialog::dayNamesDialog(QList<QCheckBox *> dayselectors, QList<QLineEdit *> daynames, QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Schedule days"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *theGrid = new QGridLayout(this);

    for(int day = 0; day < MAX_DAYS; day++) {
        dayselectors[day]->setStyleSheet(CHECKBOXSTYLE);
        daynames[day]->setStyleSheet(LINEEDITSTYLE);
        theGrid->addWidget(dayselectors[day], day, 0);
        theGrid->addWidget(daynames[day], day, 1);
    }

    //a spacer then ok button
    theGrid->setRowMinimumHeight(MAX_DAYS, DIALOG_SPACER_ROWHEIGHT);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    buttonBox->setStyleSheet(SMALLBUTTONSTYLE);
    theGrid->addWidget(buttonBox, MAX_DAYS + 1, 0, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    adjustSize();
}
