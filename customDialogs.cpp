#include "customDialogs.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog window to select sets of required or prevented teammates
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherTeammatesDialog::gatherTeammatesDialog(const typeOfTeammates requiredOrPrevented, const studentRecord student[], int numStudents, QWidget *parent)
    : QDialog(parent)
{
    //copy data into local versions, including full database of students
    this->numStudents = numStudents;
    this->student = new studentRecord[this->numStudents];
    for(int i = 0; i < this->numStudents; i++)
    {
        this->student[i] = student[i];
    }
    this->requiredOrPrevented = requiredOrPrevented;

    //Set up window
    setWindowTitle(tr("Select ") + QString(requiredOrPrevented==gatherTeammatesDialog::required?tr("Required"):tr("Prevented")) + tr(" Teammates"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setMinimumSize(600, 600);
    theGrid = new QGridLayout(this);

    //First row - the current data
    currentListOfTeammatesTable = new QTableWidget(this);
    currentListOfTeammatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    currentListOfTeammatesTable->setAlternatingRowColors(true);
    currentListOfTeammatesTable->setStyleSheet(
            "QTableView{gridline-color: black;}"
            "QHeaderView::section{"
                "border-top:0px solid #D8D8D8;"
                "border-left:0px solid #D8D8D8;"
                "border-right:1px solid black;"
                "border-bottom: 1px solid black;"
                "background-color:Gainsboro;"
                "padding:4px;"
                "font-weight:bold;"
            "}"
            "QTableCornerButton::section{"
                "border-top:0px solid #D8D8D8;"
                "border-left:0px solid #D8D8D8;"
                "border-right:1px solid black;"
                "border-bottom: 1px solid black;"
                "background-color:white;"
            "}");
    theGrid->addWidget(currentListOfTeammatesTable, 0, 0, 1, -1);

    //Second and third row - the teammate choice box(es), a spacer, and a load button
    for(int i = 0; i < 8; i++)
    {
        possibleTeammates[i].addItem("");
        //Create list of all the student names
        for(int j = 0; j < numStudents; j++)
        {
            possibleTeammates[i].addItem(student[j].firstname + " " + student[j].lastname);
        }
        theGrid->addWidget(&possibleTeammates[i], 1+(i/4), i%4);
    }
    theGrid->setColumnMinimumWidth(4,20);
    loadTeammates = new QPushButton(this);
    loadTeammates->setText(tr("&Add set of\n") + QString(requiredOrPrevented==gatherTeammatesDialog::required?tr(" required"):tr(" prevented")) + tr(" teammates "));
    connect(loadTeammates, &QPushButton::clicked, this, &gatherTeammatesDialog::addOneTeammateSet);
    theGrid->addWidget(loadTeammates, 1, 5, 2, 1);

    //Fourth and fifth row - a spacer then reset table/ok/cancel buttons
    theGrid->setRowMinimumHeight(3, 20);
    resetTableButton = new QPushButton(this);
    resetTableButton->setText(tr("&Reset"));
    theGrid->addWidget(resetTableButton, 4, 0, 1, 1);
    connect(resetTableButton, &QPushButton::clicked, this, &gatherTeammatesDialog::clearAllTeammateSets);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 4, 2, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
    refreshDisplay();
}


gatherTeammatesDialog::~gatherTeammatesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] student;
}


void gatherTeammatesDialog::addOneTeammateSet()
{
    const int possibleNumIDs = 8;   //number of comboboxes in the dialog box, i.e., possible choices of teammates

    //Gather all selected IDs from the comboboxes
    int IDs[possibleNumIDs];
    int count = 0;
    for(int i = 0; i < possibleNumIDs; i++)
    {
        //If a student is selected in this combobox, load the ID into an array of all selections
        if(possibleTeammates[i].currentIndex() >= 1)
        {
            IDs[count] = possibleTeammates[i].currentIndex()-1;
            count++;
        }

        //Reset combobox
        possibleTeammates[i].setCurrentIndex(0);
    }

    //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
    for(int ID1 = 0; ID1 < count; ID1++)
    {
        for(int ID2 = ID1+1; ID2 < count; ID2++)
        {
            if(IDs[ID1] != IDs[ID2])
            {
                if(requiredOrPrevented == gatherTeammatesDialog::required)
                {
                    student[IDs[ID1]].requiredWith[IDs[ID2]] = true;
                    student[IDs[ID2]].requiredWith[IDs[ID1]] = true;
                }
                else
                {
                    student[IDs[ID1]].preventedWith[IDs[ID2]] = true;
                    student[IDs[ID2]].preventedWith[IDs[ID1]] = true;
                }
            }
        }
    }
    refreshDisplay();
}


void gatherTeammatesDialog::clearAllTeammateSets()
{
    for(int ID1 = 0; ID1 < numStudents; ID1++)
    {
        for(int ID2 = 0; ID2 < numStudents; ID2++)
        {
            if(requiredOrPrevented == gatherTeammatesDialog::required)
            {
                student[ID1].requiredWith[ID2] = false;
            }
            else
            {
                student[ID1].preventedWith[ID2] = false;
            }
        }
    }
    refreshDisplay();
}


void gatherTeammatesDialog::refreshDisplay()
{
    currentListOfTeammatesTable->clear();
    currentListOfTeammatesTable->setColumnCount(0);
    currentListOfTeammatesTable->setRowCount(0);

    int row=0, column;
    for(int ID1 = 0; ID1 < numStudents; ID1++)
    {
        column = 0;
        for(int ID2 = 0; ID2 < numStudents; ID2++)
        {
            if(requiredOrPrevented == gatherTeammatesDialog::required)
            {
                if(student[ID1].requiredWith[ID2])
                {
                    if(currentListOfTeammatesTable->columnCount() < column+1)
                    {
                        currentListOfTeammatesTable->setColumnCount(column+1);
                        currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem("Required Teammate #" + QString::number(column+1)));
                    }
                    if(currentListOfTeammatesTable->rowCount() < row+1)
                    {
                        currentListOfTeammatesTable->setRowCount(row+1);
                        currentListOfTeammatesTable->setVerticalHeaderItem(row, new QTableWidgetItem(student[ID1].firstname + " " + student[ID1].lastname));
                    }
                    currentListOfTeammatesTable->setItem(row, column, new QTableWidgetItem(student[ID2].firstname + " " + student[ID2].lastname));
                    column++;
                }
            }
            else
            {
                if(student[ID1].preventedWith[ID2])
                {
                    if(currentListOfTeammatesTable->columnCount() < column+1)
                    {
                        currentListOfTeammatesTable->setColumnCount(column+1);
                        currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem("Prevented Teammate #" + QString::number(column+1)));
                    }
                    if(currentListOfTeammatesTable->rowCount() < row+1)
                    {
                        currentListOfTeammatesTable->setRowCount(row+1);
                        currentListOfTeammatesTable->setVerticalHeaderItem(row, new QTableWidgetItem(student[ID1].firstname + " " + student[ID1].lastname));
                    }
                    currentListOfTeammatesTable->setItem(row, column, new QTableWidgetItem(student[ID2].firstname + " " + student[ID2].lastname));
                    column++;
                }
            }
        }
        if(column != 0)
        {
            //there was at least one required/prevented teammate listed
            row++;
        }
    }

    currentListOfTeammatesTable->resizeColumnsToContents();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom team sizes
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamsizesDialog::customTeamsizesDialog(int numStudents, int idealTeamsize, QWidget *parent)
    :QDialog (parent)
{
    this->numStudents = numStudents;
    //At most, there are as many teams as students
    teamsizes = new int[numStudents];
    teamNumberLabel = new QLabel[numStudents];
    teamsizeBox = new QSpinBox[numStudents];

    //Set up window with a grid layout
    setWindowTitle(tr("Choose custom team sizes"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    //Rows 1-3 - the number of teams selector and a spacer
    numTeamsLabel.setText(tr("Number of teams: "));
    theGrid->addWidget(&numTeamsLabel, 0, 0, 1, -1, Qt::AlignHCenter);
    for(int i = 0; i < numStudents; i++)
    {
        numTeamsBox.addItem(QString::number(i+1));
    }
    int startingNumTeams = ((numStudents%idealTeamsize==0)?((numStudents/idealTeamsize)-1):(numStudents/idealTeamsize));
    numTeamsBox.setCurrentIndex(startingNumTeams);
    connect(&numTeamsBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &customTeamsizesDialog::refreshDisplay);
    theGrid->addWidget(&numTeamsBox, 1, 0, 1, -1, Qt::AlignHCenter);
    theGrid->setRowMinimumHeight(2, 20);

    //Row 4 through (maxTeams/4+4) - the size choices
    for(int i = 0; i < numStudents; i++)
    {
        teamNumberLabel[i].setText(tr("Team ") + QString::number(i+1));
        theGrid->addWidget(&teamNumberLabel[i], 3 + (i/4), 2*(i%4));
        theGrid->setColumnStretch(2*(i%4), 1);
        teamNumberLabel[i].hide();
        teamsizeBox[i].setRange(1, numStudents);
        teamsizeBox[i].setValue(idealTeamsize);
        theGrid->addWidget(&teamsizeBox[i], 3 + (i/4), 2*(i%4)+1);
        theGrid->setColumnStretch(2*(i%4)+1, 1);
        teamsizeBox[i].hide();
        connect(&teamsizeBox[i], QOverload<int>::of(&QSpinBox::valueChanged), this, &customTeamsizesDialog::teamsizeChanged);
    }

    //Rows (maxTeams/4+5) and (maxTeams/4+6) - a spacer then remaining students label and ok/cancel buttons
    theGrid->setRowMinimumHeight((numStudents/4)+5, 20);
    theGrid->addWidget(&remainingStudents, (numStudents/4)+6, 0, 1, 3);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, (numStudents/4)+6, 3, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshDisplay(numTeamsBox.currentIndex());
    adjustSize();
}


customTeamsizesDialog::~customTeamsizesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] teamsizes;
    delete [] teamNumberLabel;
    delete [] teamsizeBox;
}


void customTeamsizesDialog::refreshDisplay(int numTeamsBoxIndex)
{
    numTeams = numTeamsBoxIndex+1;

    //show a label and a combobox for as many teams as chosen in the numTeams selection
    int studentsOnATeamCount = 0;
    for(int i = 0; i < numStudents; i++)
    {
        if(i < numTeams)
        {
            teamNumberLabel[i].show();
            teamsizeBox[i].show();
            studentsOnATeamCount += teamsizeBox[i].value();
        }
        else
        {
            teamNumberLabel[i].hide();
            teamsizeBox[i].hide();
        }
    }

    //display how many students remain to be placed on a team, using red font if that number is non-zero
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

    adjustSize();
}


void customTeamsizesDialog::teamsizeChanged(int)
{
    for(int i = 0; i < numStudents; i++)
    {
        teamsizes[i] = teamsizeBox[i].value();
    }

    refreshDisplay(numTeamsBox.currentIndex());
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to swap the place (i.e., team assignment) of two selected students
/////////////////////////////////////////////////////////////////////////////////////////////////////////

swapTeammatesDialog::swapTeammatesDialog(const studentRecord student[], int numStudents, QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Choose two students to swap teams"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    //First row and second row - the teammate choice boxes
    studentA = new QComboBox(this);
    studentB = new QComboBox(this);
    studentA->addItem("");
    studentB->addItem("");
    //Create list of all the student names
    for(int j = 0; j < numStudents; j++)
    {
        studentA->addItem(student[j].firstname + " " + student[j].lastname);
        studentB->addItem(student[j].firstname + " " + student[j].lastname);
    }
    theGrid->addWidget(studentA, 0, 0);
    theGrid->addWidget(studentB, 1, 0);

    //Third and fourth row - a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(2, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 3, 0);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


void swapTeammatesDialog::refreshDisplay()
{
    //function held for later use
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to register the software
/////////////////////////////////////////////////////////////////////////////////////////////////////////

registerDialog::registerDialog(QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Register your copy of gruepr"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    explanation = new QLabel(this);
    explanation->setText(tr("\n\nThank you for registering your copy of gruepr.\nDoing so enables me to best support\nthe community of educators that uses it.\n\tThanks!\n\t  --Josh\n\t    j.hertz@neu.edu"));
    theGrid->addWidget(explanation, 0, 0);

    name = new QLineEdit(this);
    name->setPlaceholderText(tr("full name"));
    theGrid->addWidget(name, 1, 0);

    institution = new QLineEdit(this);
    institution->setPlaceholderText(tr("institution"));
    theGrid->addWidget(institution, 2, 0);

    email = new QLineEdit(this);
    email->setPlaceholderText(tr("email address"));
    theGrid->addWidget(email, 3, 0);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(4, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 5, 0);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}
