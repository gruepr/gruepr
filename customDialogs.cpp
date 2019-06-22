#include "customDialogs.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog window to select sets of required or prevented teammates
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherTeammatesDialog::gatherTeammatesDialog(const typeOfTeammates whatType, const studentRecord student[], int numStudentsInSystem, const QString &sectionName, QWidget *parent)
    : QDialog(parent)
{
    //copy data into local versions, including full database of students
    numStudents = numStudentsInSystem;
    this->sectionName = sectionName;
    this->student = new studentRecord[numStudentsInSystem];
    for(int i = 0; i < numStudentsInSystem; i++)
    {
        this->student[i] = student[i];
    }
    this->whatType = whatType;

    //start off with no teammate data changed
    teammatesSpecified = false;

    //Set up window
    QString typeText;
    QLabel *explanation = new QLabel(this);
    if(whatType == gatherTeammatesDialog::required)
    {
        typeText = tr("Required");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) + tr(" students that will be required to be on the same team, then click the \"Add set\" button."));
    }
    else if (whatType == gatherTeammatesDialog::prevented)
    {
        typeText = tr("Prevented");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) + tr(" students that will be prevented from bring on the same team, then click the \"Add set\" button."));
    }
    else
    {
        typeText = tr("Requested");
        explanation->setText(tr("Select a student and up to ") + QString::number(possibleNumIDs) + tr(" requested teammates, then click the \"Add set\" button."));
    }
    setWindowTitle(tr("Select ") + typeText + tr(" Teammates"));
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

    //Second row - text explanation
    theGrid->addWidget(explanation, 1, 0, 1, -1);

    //If this is requested teammates, add the 'base' student button in the third row
    int row = 2;
    if(whatType == gatherTeammatesDialog::requested)
    {
        possibleTeammates[8].addItem("Select the student:");
        possibleTeammates[8].setItemData(0, QBrush(Qt::gray), Qt::TextColorRole);
        possibleTeammates[8].insertSeparator(1);
        int indexInComboBox = 1;
        //Add to combobox a list of all the student names (in this section)
        for(int ID = 0; ID < numStudentsInSystem; ID++)
        {
            if((sectionName == "") || (sectionName == student[ID].section))
            {
                indexInComboBox++;
                possibleTeammates[8].insertItem(indexInComboBox, student[ID].firstname + " " + student[ID].lastname);
                possibleTeammates[8].setItemData(indexInComboBox, student[ID].ID);
            }
        }
        theGrid->addWidget(&possibleTeammates[8], row, 0, 1, 2);
        row++;
    }

    //Rows 3&4 (or 4&5) - the teammate choice box(es), a spacer, and a load button
    for(int i = 0; i < possibleNumIDs; i++)
    {
        if(whatType != gatherTeammatesDialog::requested)
        {
            possibleTeammates[i].addItem("Select a student:");
        }
        else
        {
            possibleTeammates[i].addItem("Select a requested teammate:");
        }
        possibleTeammates[i].setItemData(0, QBrush(Qt::gray), Qt::TextColorRole);
        possibleTeammates[i].insertSeparator(1);
        int indexInComboBox = 1;
        //Add to combobox a list of all the student names (in this section)
        for(int ID = 0; ID < numStudentsInSystem; ID++)
        {
            if((sectionName == "") || (sectionName == student[ID].section))
            {
                indexInComboBox++;
                possibleTeammates[i].insertItem(indexInComboBox, student[ID].firstname + " " + student[ID].lastname);
                possibleTeammates[i].setItemData(indexInComboBox, student[ID].ID);
            }
        }
        theGrid->addWidget(&possibleTeammates[i], row+(i/4), i%4);
    }
    theGrid->setColumnMinimumWidth(4,15);
    loadTeammates = new QPushButton(this);
    loadTeammates->setText(tr("&Add set of\n ") + typeText.toLower() + tr(" teammates "));
    connect(loadTeammates, &QPushButton::clicked, this, &gatherTeammatesDialog::addOneTeammateSet);
    theGrid->addWidget(loadTeammates, row, 5, 2, 1);

    //Rows 5&6 (or 6&7) - a spacer then reset table/ok/cancel buttons
    row += 2;
    theGrid->setRowMinimumHeight(row, 20);
    resetTableButton = new QPushButton(this);
    resetTableButton->setText(tr("&Reset All"));
    theGrid->addWidget(resetTableButton, row+1, 0, 1, 1);
    connect(resetTableButton, &QPushButton::clicked, this, &gatherTeammatesDialog::clearAllTeammateSets);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, row+1, 2, -1, -1);
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
    //Gather all selected IDs from the comboboxes
    int IDs[possibleNumIDs];
    int count = 0;
    for(int i = 0; i < possibleNumIDs; i++)
    {
        //If a student is selected in this combobox, load their ID into an array that holds all the selections
        if(possibleTeammates[i].currentIndex() >= 1)
        {
            IDs[count] = possibleTeammates[i].itemData(possibleTeammates[i].currentIndex()).toInt();
            count++;
        }

        //Reset combobox
        possibleTeammates[i].setCurrentIndex(0);
    }

    if(whatType != gatherTeammatesDialog::requested)
    {
        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        for(int ID1 = 0; ID1 < count; ID1++)
        {
            for(int ID2 = ID1+1; ID2 < count; ID2++)
            {
                if(IDs[ID1] != IDs[ID2])
                {
                    //we have at least one required/prevented teammate pair!
                    teammatesSpecified = true;

                    if(whatType == gatherTeammatesDialog::required)
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
    }
    else
    {
        int baseStudent = possibleTeammates[8].itemData(possibleTeammates[8].currentIndex()).toInt();
        //Reset combobox
        possibleTeammates[8].setCurrentIndex(0);
        for(int ID1 = 0; ID1 < count; ID1++)
        {
            //we have at least one requested teammate pair!
            teammatesSpecified = true;

            student[baseStudent].requestedWith[IDs[ID1]] = true;
        }
    }
    refreshDisplay();
}


void gatherTeammatesDialog::clearAllTeammateSets()
{
    teammatesSpecified = false;

    for(int ID1 = 0; ID1 < numStudents; ID1++)
    {
        if((sectionName == "") || (sectionName == student[ID1].section))
        {
            for(int ID2 = 0; ID2 < numStudents; ID2++)
            {
                if(whatType == gatherTeammatesDialog::required)
                {
                    student[ID1].requiredWith[ID2] = false;
                }
                else if(whatType == gatherTeammatesDialog::prevented)
                {
                    student[ID1].preventedWith[ID2] = false;
                }
                else
                {
                    student[ID1].requestedWith[ID2] = false;
                }
            }
        }
    }
    refreshDisplay();
}


void gatherTeammatesDialog::refreshDisplay()
{
    QString typeText;
    if(whatType == gatherTeammatesDialog::required)
    {
        typeText = tr("Required");
    }
    else if (whatType == gatherTeammatesDialog::prevented)
    {
        typeText = tr("Prevented");
    }
    else
    {
        typeText = tr("Requested");
    }

    currentListOfTeammatesTable->clear();
    currentListOfTeammatesTable->setColumnCount(1);
    currentListOfTeammatesTable->setHorizontalHeaderItem(0, new QTableWidgetItem(typeText + tr(" Teammate #1")));
    currentListOfTeammatesTable->setRowCount(0);

    int row=0, column;
    for(int ID1 = 0; ID1 < numStudents; ID1++)
    {
        if((sectionName == "") || (sectionName == student[ID1].section))
        {
            column = 0;
            currentListOfTeammatesTable->setRowCount(row+1);
            currentListOfTeammatesTable->setVerticalHeaderItem(row, new QTableWidgetItem(student[ID1].firstname + " " + student[ID1].lastname));
            currentListOfTeammatesTable->setItem(row, 0, new QTableWidgetItem("--"));
            bool printStudent;
            for(int ID2 = 0; ID2 < numStudents; ID2++)
            {
                if(whatType == gatherTeammatesDialog::required)
                {
                    printStudent = student[ID1].requiredWith[ID2];
                }
                else if(whatType == gatherTeammatesDialog::prevented)
                {
                    printStudent = student[ID1].preventedWith[ID2];
                }
                else
                {
                    printStudent = student[ID1].requestedWith[ID2];
                }
                if(printStudent)
                {
                    if(currentListOfTeammatesTable->columnCount() < column+1)
                    {
                        currentListOfTeammatesTable->setColumnCount(column+1);
                        currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + tr(" Teammate #") + QString::number(column+1)));
                    }
                    currentListOfTeammatesTable->setItem(row, column, new QTableWidgetItem(student[ID2].firstname + " " + student[ID2].lastname));
                    column++;
                }
            }
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
    int startingNumTeams = ((numStudents%idealTeamsize==0)? ((numStudents/idealTeamsize)-1) : (numStudents/idealTeamsize));
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
// A dialog to choose custom team names
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customTeamnamesDialog::customTeamnamesDialog(int numTeams, QStringList teamNames, QWidget *parent)
    :QDialog (parent)
{
    this->numTeams = numTeams;
    teamNumberLabel = new QLabel[numTeams];
    teamName = new QLineEdit[numTeams];

    //Set up window with a grid layout
    setWindowTitle(tr("Choose custom team names"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    //Row 1 through (numTeams/4) - the team names
    for(int i = 0; i < numTeams; i++)
    {
        teamNumberLabel[i].setText(tr("Team ") + QString::number(i+1));
        theGrid->addWidget(&teamNumberLabel[i], (i/4), 2*(i%4));
        theGrid->setColumnStretch(2*(i%4), 1);
        teamName[i].setPlaceholderText(tr("Custom name"));
        if(i < teamNames.size())
        {
            teamName[i].setText((teamNames.at(i) == QString::number(i+1))? "" : teamNames.at(i));
        }
        theGrid->addWidget(&teamName[i], (i/4), 2*(i%4)+1);
        theGrid->setColumnStretch(2*(i%4)+1, 1);
    }

    //Rows (numTeams/4)+1 and (numTeams/4)+2 - a spacer then reset table/ok/cancel buttons
    theGrid->setRowMinimumHeight((numTeams/4)+1, 20);
    resetNamesButton = new QPushButton(this);
    resetNamesButton->setText(tr("&Clear All Names"));
    theGrid->addWidget(resetNamesButton, (numTeams/4)+2, 0, 1, 1);
    connect(resetNamesButton, &QPushButton::clicked, this, &customTeamnamesDialog::clearAllNames);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, (numTeams/4)+2, 3, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


customTeamnamesDialog::~customTeamnamesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] teamNumberLabel;
    delete [] teamName;
}


void customTeamnamesDialog::clearAllNames()
{
    for(int i = 0; i < numTeams; i++)
    {
        teamName[i].clear();
    }
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
    explanation->setText(tr("\nThank you for registering your copy of gruepr.\n"
                            "Doing so enables me to best support\nthe community of educators that uses it.\n"
                            "\t-Josh\n"
                            "\t j.hertz@neu.edu\n"));
    theGrid->addWidget(explanation, 0, 0);

    name = new QLineEdit(this);
    name->setPlaceholderText(tr("full name [required]"));
    theGrid->addWidget(name, 1, 0);

    institution = new QLineEdit(this);
    institution->setPlaceholderText(tr("institution [required]"));
    theGrid->addWidget(institution, 2, 0);

    email = new QLineEdit(this);
    email->setPlaceholderText(tr("email address [required]"));
    theGrid->addWidget(email, 3, 0);
    //force an email address-like input
    //(one ore more letters, digit or special symbols, followed by @, followed by one ore more letters, digit or special symbols, followed by '.', followed by two, three or four letters)
    QRegularExpression emailAddressFormat("^[A-Z0-9.!#$%&*+_-~]+@[A-Z0-9.-]+\\.[A-Z]{2,64}$", QRegularExpression::CaseInsensitiveOption);
    email->setValidator(new QRegularExpressionValidator(emailAddressFormat, this));
    connect(email, &QLineEdit::textChanged, [=]() { QString stylecolor = (email->hasAcceptableInput())? "black" : "red"; email->setStyleSheet("QLineEdit {color: " + stylecolor + ";}"); });

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(4, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    theGrid->addWidget(buttonBox, 5, 0);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(name, &QLineEdit::textChanged, [=]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(institution, &QLineEdit::textChanged, [=]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(email, &QLineEdit::textChanged, [=]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});

    adjustSize();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose which item(s) to save or print
/////////////////////////////////////////////////////////////////////////////////////////////////////////

whichFilesDialog::whichFilesDialog(const action saveOrPrint, const QStringList previews, QWidget *parent)
    :QDialog (parent)
{
    QString saveOrPrintString = (saveOrPrint==whichFilesDialog::save?tr("save"):tr("print"));
    //Set up window with a grid layout
    setWindowTitle(tr("Choose files to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    explanation = new QLabel(this);
    explanation->setTextFormat(Qt::RichText);
    explanation->setText(tr("<br>You can ") + saveOrPrintString + tr(" the following files:<br><i>hover over title for a preview</i><br>"));
    theGrid->addWidget(explanation, 0, 0);

    studentFile = new QCheckBox(this);
    studentFile->resize(30,30);
    studentFile->setText(tr("Student's file:\nnames, email addresses, and team availability schedules."));
    if(!(previews.empty()))
    {
        studentFile->setToolTip(previews.at(0));
    }
    theGrid->addWidget(studentFile, 1, 0);

    instructorFile = new QCheckBox(this);
    instructorFile->resize(30,30);
    instructorFile->setText(tr("Instructor's file:\nnames, email addresses, demographic and attribute data, and team availability schedule."));
    if(!(previews.empty()))
    {
        instructorFile->setToolTip(previews.at(1));
    }
    theGrid->addWidget(instructorFile, 2, 0);

    spreadsheetFile = new QCheckBox(this);
    spreadsheetFile->resize(30,30);
    spreadsheetFile->setText(tr("Spreadsheet file:\nsections, teams, names, and email addresses in a tabular format."));
    if(!(previews.empty()))
    {
        spreadsheetFile->setToolTip(previews.at(2));
    }
    theGrid->addWidget(spreadsheetFile, 3, 0);

    if(saveOrPrint == whichFilesDialog::save)
    {
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::SaveAll | QDialogButtonBox::Cancel, this);
        buttonBox->button(QDialogButtonBox::Save)->setText("Save as &Text");
        buttonBox->button(QDialogButtonBox::SaveAll)->setText("Save as &PDF");
    }
    else
    {
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttonBox->button(QDialogButtonBox::Ok)->setText("&Print");
    }
    connect(buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button){QDialog::done(buttonBox->standardButton(button));});

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(4, 20);
    theGrid->addWidget(buttonBox, 5, 0);

    adjustSize();
}
