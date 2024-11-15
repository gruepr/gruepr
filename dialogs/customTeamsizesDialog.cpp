#include "customTeamsizesDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team sizes
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamsizesDialog::customTeamsizesDialog(long long numStudents, int idealTeamsize, QWidget *parent)
    :listTableDialog(tr("Choose custom team sizes"), true, true, parent)
{
    this->numStudents = numStudents;
    //At most, there are as many teams as students
    teamsizes.resize(numStudents);

    setMinimumSize(XS_DLG_SIZE, XS_DLG_SIZE);

    //Rows 1&2 - the number of teams selector and a spacer
    auto *numTeamsLayout = new QHBoxLayout;
    auto *numTeamsLabel = new QLabel(tr("Number of teams: "), this);
    numTeamsLabel->setStyleSheet(QString(LABEL10PTSTYLE).replace("QLabel {", "QLabel {background-color: " TRANSPARENT ";"));
    numTeamsLayout->addWidget(numTeamsLabel);
    numTeamsBox = new QSpinBox(this);
    numTeamsBox->setStyleSheet(SPINBOXSTYLE);
    numTeamsBox->setRange(1, numStudents);
    numTeamsBox->setValue((numStudents%idealTeamsize == 0)? ((numStudents/idealTeamsize)-1) : (numStudents/idealTeamsize));
    numTeamsBox->installEventFilter(new MouseWheelBlocker(numTeamsBox));
    numTeamsBox->setFocusPolicy(Qt::StrongFocus);
    connect(numTeamsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &customTeamsizesDialog::refreshDisplay);
    numTeamsLayout->addWidget(numTeamsBox);
    theGrid->addLayout(numTeamsLayout, 0, 1, 1, 1);
    addSpacerRow(1);

    //Row 3 - table of the size choices
    theTable->setRowCount(numStudents);
    teamsizeBox.reserve(numStudents);
    int widthCol0 = 0, rowHeight = 0;
    for(int i = 0; i < numStudents; i++) {
        auto *label = new QLabel(tr("Team ") + QString::number(i+1) + " ", this);
        label->setStyleSheet(LABEL10PTSTYLE);
        theTable->setCellWidget(i, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamsizeBox << new QSpinBox(this);
        teamsizeBox.last()->setStyleSheet(SPINBOXSTYLE);
        teamsizeBox.last()->setRange(1, numStudents);
        teamsizeBox.last()->setValue(idealTeamsize);
        teamsizeBox.last()->installEventFilter(new MouseWheelBlocker(teamsizeBox.last()));
        teamsizeBox.last()->setFocusPolicy(Qt::StrongFocus);
        connect(teamsizeBox.last(), QOverload<int>::of(&QSpinBox::valueChanged), this, &customTeamsizesDialog::teamsizeChanged);
        theTable->setCellWidget(i, 1, teamsizeBox.last());
        rowHeight = std::max(rowHeight, std::max(label->height(), teamsizeBox.last()->height()));
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLEOVERSIZE));
    for(int i = 0; i < numStudents; i++) {
        theTable->verticalHeader()->resizeSection(i, int(float(rowHeight) * TABLEOVERSIZE));
    }
    theTable->adjustSize();

    //Rows 4&5 - a spacer and remaining students label
    addSpacerRow(3);
    remainingStudents = new QLabel(this);
    remainingStudents->setStyleSheet(LABEL10PTSTYLE);
    theGrid->addWidget(remainingStudents, 4, 0, 1, -1, Qt::AlignCenter);

    refreshDisplay();
    adjustSize();
}


void customTeamsizesDialog::refreshDisplay()
{
    numTeams = numTeamsBox->value();

    //show a label and a combobox for as many teams as chosen in the numTeams selection, and
    //display how many students remain to be placed on a team, using red font if that number is non-zero
    int studentsOnATeamCount = 0;
    for(int i = 0; i < numStudents; i++) {
        if(i < numTeams) {
            studentsOnATeamCount += teamsizeBox.at(i)->value();
            theTable->showRow(i);
        }
        else {
            theTable->hideRow(i);
        }
    }
    remainingStudents->setText(tr("Remaining students: ") + QString::number(numStudents-studentsOnATeamCount));
    if(numStudents != studentsOnATeamCount) {
        remainingStudents->setStyleSheet(QString(LABEL10PTSTYLE).replace("color: " DEEPWATERHEX ";", "color: red;"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
        remainingStudents->setStyleSheet(LABEL10PTSTYLE);
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }

    theTable->adjustSize();
}


void customTeamsizesDialog::teamsizeChanged(int /*unused*/)
{
    for(int i = 0; i < numStudents; i++) {
        teamsizes[i] = teamsizeBox.at(i)->value();
    }

    refreshDisplay();
}
