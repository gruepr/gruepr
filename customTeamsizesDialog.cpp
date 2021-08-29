#include "customTeamsizesDialog.h"
#include "gruepr_consts.h"
#include <QHeaderView>
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team sizes
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamsizesDialog::customTeamsizesDialog(int numStudents, int idealTeamsize, QWidget *parent)
    :listTableDialog(tr("Choose custom team sizes"), true, true, parent)
{
    this->numStudents = numStudents;
    //At most, there are as many teams as students
    teamsizes = new int[numStudents];
    teamsizeBox = new QSpinBox[numStudents];

    setMinimumSize(XS_DLG_SIZE, XS_DLG_SIZE);

    //Rows 1&2 - the number of teams selector and a spacer
    numTeamsLabel.setText(tr("Number of teams: "));
    theGrid->addWidget(&numTeamsLabel, 0, 0, 1, 1, Qt::AlignRight);
    for(int i = 0; i < numStudents; i++)
    {
        numTeamsBox.addItem(QString::number(i+1));
    }
    int startingNumTeams = ((numStudents%idealTeamsize==0)? ((numStudents/idealTeamsize)-1) : (numStudents/idealTeamsize));
    numTeamsBox.setCurrentIndex(startingNumTeams);
    connect(&numTeamsBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &customTeamsizesDialog::refreshDisplay);
    theGrid->addWidget(&numTeamsBox, 0, 1, 1, -1, Qt::AlignLeft);
    addSpacerRow(1);

    //Row 3 - table of the size choices
    theTable->setRowCount(numStudents);
    int widthCol0 = 0;
    for(int i = 0; i < numStudents; i++)
    {
        auto label = new QLabel(tr("Team ") + QString::number(i+1) + " ");
        theTable->setCellWidget(i, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        teamsizeBox[i].setRange(1, numStudents);
        teamsizeBox[i].setValue(idealTeamsize);
        teamsizeBox[i].installEventFilter(this);    // remove scrollwheel from affecting the value, as box is in a table inside a scroll area; easy to mistakenly change value
        connect(&teamsizeBox[i], QOverload<int>::of(&QSpinBox::valueChanged), this, &customTeamsizesDialog::teamsizeChanged);
        theTable->setCellWidget(i, 1, &teamsizeBox[i]);
    }
    theTable->horizontalHeader()->resizeSection(0, widthCol0 * TABLECOLUMN0OVERWIDTH);
    theTable->adjustSize();

    //Rows 4&5 - a spacer and remaining students label
    addSpacerRow(3);
    theGrid->addWidget(&remainingStudents, 4, 0, 1, -1, Qt::AlignCenter);

    refreshDisplay(numTeamsBox.currentIndex());
    adjustSize();
}


customTeamsizesDialog::~customTeamsizesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] teamsizes;
    delete [] teamsizeBox;
}


void customTeamsizesDialog::refreshDisplay(int numTeamsBoxIndex)
{
    numTeams = numTeamsBoxIndex+1;

    //show a label and a combobox for as many teams as chosen in the numTeams selection, and
    //display how many students remain to be placed on a team, using red font if that number is non-zero
    int studentsOnATeamCount = 0;
    for(int i = 0; i < numStudents; i++)
    {
        if(i < numTeams)
        {
            studentsOnATeamCount += teamsizeBox[i].value();
            theTable->showRow(i);
        }
        else
        {
            theTable->hideRow(i);
        }
    }
    remainingStudents.setText(tr("Remaining students: ") + QString::number(numStudents-studentsOnATeamCount));
    if(numStudents != studentsOnATeamCount)
    {
        remainingStudents.setStyleSheet("QLabel { color : red; }");
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else
    {
        remainingStudents.setStyleSheet("QLabel { color : black; }");
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}


void customTeamsizesDialog::teamsizeChanged(int /*unused*/)
{
    for(int i = 0; i < numStudents; i++)
    {
        teamsizes[i] = teamsizeBox[i].value();
    }

    refreshDisplay(numTeamsBox.currentIndex());
}
