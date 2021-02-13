#include "customDialogs.h"
#include "Levenshtein.h"
#include <QCollator>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMovie>
#include <QStandardItemModel>
#include <QTextStream>
#include <QTimer>
#include <QToolTip>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog window to select sets of required, prevented, or requested teammates
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherTeammatesDialog::gatherTeammatesDialog(const typeOfTeammates whatTypeOfTeammate, const StudentRecord studentrecs[], int numStudentsComingIn,
                                             const DataOptions *const dataOptions, const QString &sectionname, QWidget *parent)
    : QDialog(parent)
{
    //copy data into local versions, including full database of students
    numStudents = numStudentsComingIn;
    sectionName = sectionname;
    student = new StudentRecord[numStudentsComingIn];
    for(int i = 0; i < numStudentsComingIn; i++)
    {
        student[i] = studentrecs[i];
    }
    whatType = whatTypeOfTeammate;

    //Set up window
    QString typeText;
    auto *explanation = new QLabel(this);
    if(whatType == required)
    {
        typeText = tr("Required");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) +
                             tr(" students that will be required to be on the same team, then click the \"Add set\" button."));
        requestsInSurvey = dataOptions->prefTeammatesIncluded;
    }
    else if (whatType == prevented)
    {
        typeText = tr("Prevented");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) +
                             tr(" students that will be prevented from bring on the same team, then click the \"Add set\" button."));
        requestsInSurvey = dataOptions->prefNonTeammatesIncluded;
    }
    else    // whatType == requested
    {
        typeText = tr("Requested");
        explanation->setText(tr("Select a student and up to ") + QString::number(possibleNumIDs) +
                             tr(" requested teammates, then click the \"Add set\" button."));
        requestsInSurvey = dataOptions->prefTeammatesIncluded;
    }
    setWindowTitle(tr("Select ") + typeText + tr(" Teammates"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setMinimumSize(600, 600);
    theGrid = new QGridLayout(this);

    //First row - the current data
    currentListOfTeammatesTable = new QTableWidget(this);
    currentListOfTeammatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    currentListOfTeammatesTable->setSelectionMode(QAbstractItemView::NoSelection);
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
                "font-weight:bold;}"
            "QTableCornerButton::section{"
                "border-top:0px solid #D8D8D8;"
                "border-left:0px solid #D8D8D8;"
                "border-right:1px solid black;"
                "border-bottom: 1px solid black;"
                "background-color:white;}");
    theGrid->addWidget(currentListOfTeammatesTable, 0, 0, 1, -1);

    //Second row - text explanation
    theGrid->addWidget(explanation, 1, 0, 1, -1);

    //Next rows - the selection of students
    QVector<StudentRecord> studentsInComboBoxes;
    //Add to combobox a list of all the student names (in this section)
    for(int ID = 0; ID < numStudentsComingIn; ID++)
    {
        if((sectionName == "") || (sectionName == student[ID].section))
        {
            studentsInComboBoxes << student[ID];
        }
    }
    std::sort(studentsInComboBoxes.begin(), studentsInComboBoxes.end(), [](const StudentRecord &i, const StudentRecord &j)
                                                                        {return (i.lastname+i.firstname) < (j.lastname+j.firstname);});

    //If this is requested teammates, add the 'base' student button in the third row
    int row = 2;
    if(whatType == requested)
    {
        possibleTeammates[possibleNumIDs].addItem("Select the student:");
        possibleTeammates[possibleNumIDs].setItemData(0, QBrush(Qt::gray), Qt::TextColorRole);
        possibleTeammates[possibleNumIDs].insertSeparator(1);
        for(int i = 0; i < studentsInComboBoxes.size(); i++)
        {
            possibleTeammates[possibleNumIDs].insertItem(i+2, studentsInComboBoxes[i].lastname + ", " + studentsInComboBoxes[i].firstname, studentsInComboBoxes[i].ID);
        }
        theGrid->addWidget(&possibleTeammates[possibleNumIDs], row, 0, 1, 2);
        row++;
    }

    //Rows 3&4 (or 4&5) - the teammate choice box(es), a spacer, and a load button
    for(int combobox = 0; combobox < possibleNumIDs; combobox++)
    {
        if(whatType != requested)
        {
            possibleTeammates[combobox].addItem("Select a student:");
        }
        else
        {
            possibleTeammates[combobox].addItem("Select a requested teammate:");
        }
        possibleTeammates[combobox].setItemData(0, QBrush(Qt::gray), Qt::TextColorRole);
        possibleTeammates[combobox].insertSeparator(1);
        for(int i = 0; i < studentsInComboBoxes.size(); i++)
        {
            possibleTeammates[combobox].insertItem(i+2, studentsInComboBoxes[i].lastname + ", " + studentsInComboBoxes[i].firstname,studentsInComboBoxes[i].ID);
        }
        theGrid->addWidget(&possibleTeammates[combobox], row+(combobox/4), combobox%4);
    }
    theGrid->setColumnMinimumWidth(4,15);
    loadTeammates = new QPushButton(this);
    loadTeammates->setText(tr("&Add set of\n") + typeText.toLower() + tr("\nteammates"));
    connect(loadTeammates, &QPushButton::clicked, this, &gatherTeammatesDialog::addOneTeammateSet);
    theGrid->addWidget(loadTeammates, row, 5, 2, 1);

    //Rows 5&6 (or 6&7) - a spacer then reset table/loadFile/ok/cancel buttons
    row += 2;
    theGrid->setRowMinimumHeight(row, 20);
    resetSaveOrLoad = new QComboBox(this);
    resetSaveOrLoad->setIconSize(QSize(15,15));
    resetSaveOrLoad->addItem(tr("Additional actions"));
    resetSaveOrLoad->insertSeparator(1);
    resetSaveOrLoad->addItem(QIcon(":/icons/delete.png"), tr("Clear all ") + typeText.toLower() + tr(" teammates..."));
    resetSaveOrLoad->setItemData(2, tr("Remove all currently listed data from the table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/save.png"), tr("Save the current set to a CSV file..."));
    resetSaveOrLoad->setItemData(3, tr("Save the current table to a csv file"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/openFile.png"), tr("Load a CSV file of teammates..."));
    resetSaveOrLoad->setItemData(4, tr("Add data from a csv file to the current table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/gruepr.png"), tr("Load a gruepr spreadsheet file..."));
    resetSaveOrLoad->setItemData(5, tr("Add names from a previous set of gruepr-created teams to the current table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/surveymaker.png"), tr("Import students' preferences from the survey"));
    if(whatType == required || whatType == requested)
    {
        if(requestsInSurvey)
        {
            resetSaveOrLoad->setItemData(6, tr("Add the names of the preferred teammate(s) submitted by students in the survey"), Qt::ToolTipRole);
        }
        else
        {
            resetSaveOrLoad->setItemData(6, tr("Preferred teammate information was not found in the survey"), Qt::ToolTipRole);
            auto model = qobject_cast< QStandardItemModel * >(resetSaveOrLoad->model());
            auto item = model->item(6);
            item->setEnabled(false);
        }
    }
    if(whatType == prevented)
    {
        if(requestsInSurvey)
        {
            resetSaveOrLoad->setItemData(6, tr("Add the names of the preferred non-teammate(s) submitted by students in the survey"), Qt::ToolTipRole);
        }
        else
        {
            resetSaveOrLoad->setItemData(6, tr("Preferred non-teammate information was not found in the survey"), Qt::ToolTipRole);
            auto model = qobject_cast< QStandardItemModel * >(resetSaveOrLoad->model());
            auto item = model->item(6);
            item->setEnabled(false);
        }
    }
    connect(resetSaveOrLoad, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {if(index == 2) {clearAllTeammateSets();}
                                                                                                           else if(index == 3) {saveCSVFile();}
                                                                                                           else if(index == 4) {loadCSVFile();}
                                                                                                           else if(index == 5) {loadSpreadsheetFile();}
                                                                                                           else if(index == 6) {loadStudentPrefs();}});
    theGrid->addWidget(resetSaveOrLoad, row+1, 0, 1, 3);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, row+1, 3, -1, -1);
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

    //IN THE FUTURE--SHOULD DECOUPLE THIS USE OF INDEX IN student ARRAY AS CORRESPONDING TO THE ID NUMBER
    if(whatType != requested)
    {
        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        for(int ID1 = 0; ID1 < count; ID1++)
        {
            for(int ID2 = ID1+1; ID2 < count; ID2++)
            {
                if(IDs[ID1] != IDs[ID2])
                {
                    //we have at least one required/prevented teammate pair!
                    if(whatType == required)
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
        int baseStudent = possibleTeammates[possibleNumIDs].itemData(possibleTeammates[possibleNumIDs].currentIndex()).toInt();
        //Reset combobox
        possibleTeammates[possibleNumIDs].setCurrentIndex(0);
        for(int ID1 = 0; ID1 < count; ID1++)
        {
            if(baseStudent != IDs[ID1])
            {
                //we have at least one requested teammate pair!
                student[baseStudent].requestedWith[IDs[ID1]] = true;
            }
        }
    }
    refreshDisplay();
}


void gatherTeammatesDialog::clearAllTeammateSets()
{
    int resp = QMessageBox::warning(this, tr("gruepr"),tr("This will remove all teammates data listed in the\ntable. Are you sure you want to continue?\n"),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(resp == QMessageBox::No)
    {
        resetSaveOrLoad->setCurrentIndex(0);
        return;
    }

    for(int ID1 = 0; ID1 < numStudents; ID1++)
    {
        if((sectionName == "") || (sectionName == student[ID1].section))
        {
            for(int ID2 = 0; ID2 < numStudents; ID2++)
            {
                if(whatType == required)
                {
                    student[ID1].requiredWith[ID2] = false;
                }
                else if(whatType == prevented)
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


bool gatherTeammatesDialog::saveCSVFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File of Teammates"), "", tr("Comma-Separated Value File (*.csv);;All Files (*)"));
    if(fileName.isEmpty())
    {
        resetSaveOrLoad->setCurrentIndex(0);
        return false;
    }

    QFile outputFile(fileName);
    QTextStream out(&outputFile);
    QString csvFileContents = "basename";

    for(int i = 1; i <= currentListOfTeammatesTable->columnCount(); i++)
    {
        csvFileContents += ",name" + QString::number(i);
    }
    csvFileContents += "\n";

    for(int basename = 0; basename < currentListOfTeammatesTable->rowCount(); basename++)
    {
        QStringList lastnameFirstname = currentListOfTeammatesTable->verticalHeaderItem(basename)->text().split(',');
        csvFileContents += lastnameFirstname.at(1).trimmed() + " " + lastnameFirstname.at(0).trimmed();
        for(int teammate = 0; teammate < currentListOfTeammatesTable->columnCount(); teammate++)
        {
            QStringList lastnameFirstname = {"",""};
            QWidget *teammateItem(currentListOfTeammatesTable->cellWidget(basename,teammate));
            if (teammateItem != nullptr)
            {
                lastnameFirstname = teammateItem->property("studentName").toString().split(',');
            }
            csvFileContents += "," + lastnameFirstname.at(1).trimmed() + " " + lastnameFirstname.at(0).trimmed();
        }
        csvFileContents += "\n";
    }

    if(outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&outputFile);
        out << csvFileContents;
        outputFile.close();
    }
    else
    {
        QMessageBox::critical(this, tr("No Files Saved"), tr("This data was not saved.\nThere was an issue writing the file to disk."));
    }

    refreshDisplay();
    return true;
}


bool gatherTeammatesDialog::loadCSVFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CSV File of Teammates"), "", tr("Comma-Separated Value File (*.csv);;All Files (*)"));
    if(fileName.isEmpty())
    {
        resetSaveOrLoad->setCurrentIndex(0);
        return false;
    }

    QFile inputFile(fileName);
    inputFile.open(QIODevice::ReadOnly);
    QTextStream in(&inputFile);

    // Read the header row and make sure file format is correct. If so, read next line to make sure it has data
    bool formattedCorrectly = true;
    QStringList fields = in.readLine().split(',');
    int numFields = fields.size();
    if(numFields < 2)       // should be basename, name1, name2, name3, ..., nameN
    {
        formattedCorrectly = false;
    }
    else
    {
        if((fields.at(0).toLower() != tr("basename")) || (!fields.at(1).toLower().startsWith(tr("name"))))
        {
            formattedCorrectly = false;
        }
        fields = in.readLine().split(',');
        if(fields.size() < numFields)
        {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        inputFile.close();
        return false;
    }

    // Process each row until there's an empty one. Load unique base names into basenames; load other names in the row into corresponding teammates list
    QStringList basenames;
    QVector<QStringList> teammates;
    while(!fields.isEmpty())
    {
        int pos = basenames.indexOf(fields.at(0)); // get index of this name

        if(pos == -1)   // basename is not yet found in basenames list
        {
            basenames << fields.at(0).trimmed();
            teammates.append(QStringList());
            for(int i = 1; i < numFields; i++)
            {
                QString teammate = fields.at(i).trimmed();
                if(!teammate.isEmpty())
                {
                    teammates.last() << teammate;
                }
            }
        }
        else
        {
            QMessageBox::critical(this, tr("File error."), tr("This file has an error in its format:\nThe same name appears more than once in the first column."), QMessageBox::Ok);
            inputFile.close();
            return false;
        }

        // read next row. If row is empty, then make fields empty
        fields = in.readLine().split(',');
        if(fields.size() == 1 && fields.at(0) == "")
        {
            fields.clear();
        }
    }

    // Now we have list of basenames and corresponding lists of teammates by name
    // Need to convert names to IDs and then add each teammate to the basename

    // First prepend the basenames to each list of teammates
    for(int basename = 0; basename < basenames.size(); basename++)
    {
        teammates[basename].prepend(basenames.at(basename));
    }

    for(int basename = 0; basename < basenames.size(); basename++)
    {
        QVector<int> IDs;
        for(int searchStudent = 0; searchStudent < teammates.at(basename).size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) &&
                  (teammates.at(basename).at(searchStudent).compare(student[knownStudent].firstname + " " + student[knownStudent].lastname, Qt::CaseInsensitive) != 0))
            {
                knownStudent++;
            }

            if(knownStudent != numStudents)
            {
                // Exact match found
                IDs << student[knownStudent].ID;
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(knownStudent, numStudents, student, teammates.at(basename).at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << (choiceWindow->namesList->currentData(Qt::UserRole)).toInt();
                }
                delete choiceWindow;
            }
        }

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
        for(int ID = 1; ID < IDs.size(); ID++)
        {
            if(IDs[0] != IDs[ID])
            {
                //we have at least one specified teammate pair!
                if(whatType == required)
                {
                    student[IDs[0]].requiredWith[IDs[ID]] = true;
                }
                else if(whatType == prevented)
                {
                    student[IDs[0]].preventedWith[IDs[ID]] = true;
                }
                else    //whatType == requested
                {
                    student[IDs[0]].requestedWith[IDs[ID]] = true;
                }
            }
        }
    }

    refreshDisplay();
    return true;
}

bool gatherTeammatesDialog::loadStudentPrefs()
{
    // Need to convert names to IDs and then add all to the preferences
    for(int basestudent = 0; basestudent < numStudents; basestudent++)
    {
        QStringList prefs;
        if(whatType == prevented)
        {
            prefs = student[basestudent].prefNonTeammates.split('\n');
        }
        else
        {
            prefs = student[basestudent].prefTeammates.split('\n');
        }
        prefs.removeAll("");
        prefs.prepend(student[basestudent].firstname + " " + student[basestudent].lastname);

        QVector<int> IDs;
        IDs.reserve(prefs.size());
        for(int searchStudent = 0; searchStudent < prefs.size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) && (prefs.at(searchStudent).compare((student[knownStudent].firstname + " " + student[knownStudent].lastname), Qt::CaseInsensitive) != 0))
            {
                knownStudent++;
            }

            if(knownStudent != numStudents)
            {
                // Exact match found
                IDs << student[knownStudent].ID;
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(knownStudent, numStudents, student, prefs.at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << (choiceWindow->namesList->currentData(Qt::UserRole)).toInt();
                }
                delete choiceWindow;
            }
        }

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
        for(int ID = 1; ID < IDs.size(); ID++)
        {
            if(IDs[0] != IDs[ID])
            {
                //we have at least one specified teammate pair!
                if(whatType == required)
                {
                    student[IDs[0]].requiredWith[IDs[ID]] = true;
                }
                else if(whatType == prevented)
                {
                    student[IDs[0]].preventedWith[IDs[ID]] = true;
                }
                else    //whatType == requested
                {
                    student[IDs[0]].requestedWith[IDs[ID]] = true;
                }
            }
        }
    }

    refreshDisplay();
    return true;
}

bool gatherTeammatesDialog::loadSpreadsheetFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Spreadsheet File of Previous Teammates"), "", tr("Spreadsheet File (*.txt);;All Files (*)"));
    if(fileName.isEmpty())
    {
        resetSaveOrLoad->setCurrentIndex(0);
        return false;
    }

    QFile inputFile(fileName);
    inputFile.open(QIODevice::ReadOnly);
    QTextStream in(&inputFile);

    // Read the header row and make sure file format is correct. If so, read next line to make sure it has data
    bool formattedCorrectly = true;
    QStringList fields = in.readLine().split('\t');
    if(fields.size() < 4)       // should be section, team, name, email
    {
        formattedCorrectly = false;
    }
    else
    {
        if((fields.at(0).toLower() != tr("section")) || (fields.at(1).toLower() != tr("team"))
                || (fields.at(2).toLower() != tr("name")) || (fields.at(3).toLower() != tr("email")))
        {
            formattedCorrectly = false;
        }
        fields = in.readLine().split('\t');
        if(fields.size() < 4)
        {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        inputFile.close();
        return false;
    }

    // Process each row until there's an empty one. Load unique team strings into teams; load new/matching names into corresponding teammates list
    QStringList teamnames;
    QVector<QStringList> teammates;
    while(!fields.isEmpty())
    {
        int pos = teamnames.indexOf(fields.at(1)); // get index of this team

        if(pos == -1)   // team is not yet found in teams list
        {
            teamnames << fields.at(1);
            teammates.append(QStringList(fields.at(2)));
        }
        else
        {
            teammates[pos].append(fields.at(2));
        }

        // read next row. If row is empty, then make fields empty
        fields = in.readLine().split('\t');
        if(fields.size() == 1 && fields.at(0) == "")
        {
            fields.clear();
        }
    }

    // Now we have list of teams and corresponding lists of teammates by name
    // Need to convert names to IDs and then work through all teammate pairings
    for(const auto &teammate : qAsConst(teammates))
    {
        QVector<int> IDs;
        IDs.reserve(teammate.size());
        for(int searchStudent = 0; searchStudent < teammate.size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) && (teammate.at(searchStudent).compare(student[knownStudent].firstname + " " + student[knownStudent].lastname, Qt::CaseInsensitive) != 0))
            {
                knownStudent++;
            }

            if(knownStudent != numStudents)
            {
                // Exact match found
                IDs << student[knownStudent].ID;
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(knownStudent, numStudents, student, teammate.at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << (choiceWindow->namesList->currentData(Qt::UserRole)).toInt();
                }
                delete choiceWindow;
            }
        }

        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        for(int ID1 = 0; ID1 < IDs.size(); ID1++)
        {
            for(int ID2 = ID1+1; ID2 < IDs.size(); ID2++)
            {
                if(IDs[ID1] != IDs[ID2])
                {
                    //we have at least one required/prevented teammate pair!
                    if(whatType == required)
                    {
                        student[IDs[ID1]].requiredWith[IDs[ID2]] = true;
                        student[IDs[ID2]].requiredWith[IDs[ID1]] = true;
                    }
                    else if(whatType == prevented)
                    {
                        student[IDs[ID1]].preventedWith[IDs[ID2]] = true;
                        student[IDs[ID2]].preventedWith[IDs[ID1]] = true;
                    }
                    else    //whatType == requested
                    {
                        student[IDs[ID1]].requestedWith[IDs[ID2]] = true;
                        student[IDs[ID2]].requestedWith[IDs[ID1]] = true;
                    }
                }
            }
        }
    }

    refreshDisplay();
    return true;
}


void gatherTeammatesDialog::refreshDisplay()
{
    QString typeText;
    if(whatType == required)
    {
        typeText = tr("Required");
    }
    else if (whatType == prevented)
    {
        typeText = tr("Prevented");
    }
    else
    {
        typeText = tr("Requested");
    }

    currentListOfTeammatesTable->clear();

    QFont font(this->font());
    font.setItalic(true);

    int column = 0;
    if(requestsInSurvey)
    {
        currentListOfTeammatesTable->setColumnCount(2);
        auto *prefHeaderItem = new QTableWidgetItem(tr("Preferences\nfrom Survey"));
        currentListOfTeammatesTable->setHorizontalHeaderItem(column, prefHeaderItem);
        currentListOfTeammatesTable->horizontalHeaderItem(column)->setFont(font);
        column++;
    }
    else
    {
        currentListOfTeammatesTable->setColumnCount(1);
    }
    currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + tr(" Teammate #1")));
    currentListOfTeammatesTable->setRowCount(0);
    teammatesSpecified = false;     // assume no teammates specified until we find one

    QVector<StudentRecord> studentAs;
    for(int ID = 0; ID < numStudents; ID++)
    {
        if((sectionName == "") || (sectionName == student[ID].section))
        {
            studentAs << student[ID];
        }
    }
    std::sort(studentAs.begin(), studentAs.end(), [](const StudentRecord &A, const StudentRecord &B) {return ((A.lastname+A.firstname) < (B.lastname+B.firstname));});

    int row = 0;
    bool atLeastOneTeammate;
    for(const auto &studentA : qAsConst(studentAs))
    {
        atLeastOneTeammate = false;
        column = 0;

        currentListOfTeammatesTable->setRowCount(row+1);
        currentListOfTeammatesTable->setVerticalHeaderItem(row, new QTableWidgetItem(studentA.lastname + ", " + studentA.firstname));

        if(requestsInSurvey)
        {
            QTableWidgetItem *stuPrefText = nullptr;
            if(whatType == prevented)
            {
                stuPrefText = new QTableWidgetItem(studentA.prefNonTeammates);
            }
            else
            {
                stuPrefText = new QTableWidgetItem(studentA.prefTeammates);
            }
            currentListOfTeammatesTable->setItem(row, column, stuPrefText);
            currentListOfTeammatesTable->item(row, column)->setFont(font);
            currentListOfTeammatesTable->item(row, column)->setBackground(Qt::gray);
            column++;
        }

        bool printStudent;
        for(int studentBID = 0; studentBID < numStudents; studentBID++)
        {
            if(whatType == required)
            {
                printStudent = studentA.requiredWith[studentBID];
            }
            else if(whatType == prevented)
            {
                printStudent = studentA.preventedWith[studentBID];
            }
            else
            {
                printStudent = studentA.requestedWith[studentBID];
            }
            if(printStudent)
            {
                atLeastOneTeammate = true;
                teammatesSpecified = true;
                if(currentListOfTeammatesTable->columnCount() < column+1)
                {
                    currentListOfTeammatesTable->setColumnCount(column+1);
                    currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + tr(" Teammate #")+QString::number(column+1)));
                }
                auto *box = new QHBoxLayout;
                auto *label = new QLabel(student[studentBID].lastname + ", " + student[studentBID].firstname);
                auto *remover = new QPushButton(QIcon(":/icons/delete.png"), "");
                remover->setFlat(true);
                remover->setIconSize(QSize(15, 15));
                remover->setProperty("studentAID", studentA.ID);
                remover->setProperty("studentBID", studentBID);
                if(whatType == required)
                {
                    connect(remover, &QPushButton::clicked, this, [this, remover]
                                                            {int studentAID = remover->property("studentAID").toInt();
                                                             int studentBID = remover->property("studentBID").toInt();
                                                             student[studentAID].requiredWith[studentBID] = false;
                                                             student[studentBID].requiredWith[studentAID] = false;
                                                             refreshDisplay();});
                }
                else if(whatType == prevented)
                {
                    connect(remover, &QPushButton::clicked, this, [this, remover]
                                                            {int studentAID = remover->property("studentAID").toInt();
                                                             int studentBID = remover->property("studentBID").toInt();
                                                             student[studentAID].preventedWith[studentBID] = false;
                                                             student[studentBID].preventedWith[studentAID] = false;
                                                             refreshDisplay();});
                }
                else
                {
                    connect(remover, &QPushButton::clicked, this, [this, remover]
                                                            {int studentAID = remover->property("studentAID").toInt();
                                                             int studentBID = remover->property("studentBID").toInt();
                                                             student[studentAID].requestedWith[studentBID] = false;
                                                             refreshDisplay();});
                }
                box->addWidget(label);
                box->addWidget(remover, 0, Qt::AlignLeft);
                box->setSpacing(0);
                auto *widg = new QWidget;
                widg->setLayout(box);
                widg->setProperty("studentName", label->text());
                currentListOfTeammatesTable->setCellWidget(row, column, widg);
                column++;
            }
        }
        if(!atLeastOneTeammate)
        {
            currentListOfTeammatesTable->setItem(row, column, new QTableWidgetItem("--"));
        }
        row++;
    }
    currentListOfTeammatesTable->resizeColumnsToContents();
    currentListOfTeammatesTable->resizeRowsToContents();

    resetSaveOrLoad->setCurrentIndex(0);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to select a name from a list when a perfect match is not found
/////////////////////////////////////////////////////////////////////////////////////////////////////////
findMatchingNameDialog::findMatchingNameDialog(int knownStudent, int numStudents, StudentRecord *student, const QString &searchName, QWidget *parent)
    :QDialog(parent)
{
    QMultiMap<int, QString> possibleStudents;
    for(knownStudent = 0; knownStudent < numStudents; knownStudent++)
    {
        possibleStudents.insert(levenshtein::distance(searchName, student[knownStudent].firstname + " " + student[knownStudent].lastname),
                                student[knownStudent].firstname + " " + student[knownStudent].lastname + "&ID=" + QString::number(student[knownStudent].ID));
    }

    // Create student selection window
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    setWindowTitle("Choose student");
    theGrid = new QGridLayout(this);
    explanation = new QLabel(this);
    explanation->setText(tr("An exact match for") + " <b>" + searchName + "</b> " + tr("could not be found.<br>Please select this student from the list:"));
    theGrid->addWidget(explanation, 0, 0, 1, -1);
    namesList = new QComboBox(this);
    QMultiMap<int, QString>::const_iterator i = possibleStudents.constBegin();
    while (i != possibleStudents.constEnd())
    {
        QStringList nameAndID = i.value().split("&ID=");    // split off the ID to use as the UserData role
        namesList->addItem(nameAndID.at(0), nameAndID.at(1).toInt());
        i++;
    }
    theGrid->addWidget(namesList, 1, 0, 1, -1);
    theGrid->setRowMinimumHeight(2, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Cancel)->setText("Ignore this student");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    theGrid->addWidget(buttonBox, 3, 0, 1, -1);

    adjustSize();
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
    teamsizeBox = new QSpinBox[numStudents];

    //Set up window with a grid layout
    setWindowTitle(tr("Choose custom team sizes"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setMinimumSize(200, 200);
    theGrid = new QGridLayout(this);

    //Rows 1-2 - the number of teams selector and a spacer
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
    theGrid->setRowMinimumHeight(1, 20);

    //Row 3 - table of the size choices
    teamSizesTable = new QTableWidget(this);
    teamSizesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    teamSizesTable->setSelectionMode(QAbstractItemView::NoSelection);
    teamSizesTable->setShowGrid(false);
    teamSizesTable->setAlternatingRowColors(true);
    teamSizesTable->setStyleSheet("QTableView::item{border-bottom: 1px solid black; padding: 10px;}");
    teamSizesTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    teamSizesTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    teamSizesTable->horizontalHeader()->setHidden(true);
    teamSizesTable->verticalHeader()->setHidden(true);
    teamSizesTable->setRowCount(numStudents);
    teamSizesTable->setColumnCount(2);
    teamSizesTable->horizontalHeader()->setStretchLastSection(true);;
    theGrid->addWidget(teamSizesTable, 2, 0, 1, -1);

    for(int i = 0; i < numStudents; i++)
    {
        teamSizesTable->setCellWidget(i, 0, new QLabel(tr("Team ") + QString::number(i+1) + " "));
        teamsizeBox[i].setRange(1, numStudents);
        teamsizeBox[i].setValue(idealTeamsize);
        QFontMetrics fm(teamsizeBox[i].font());
        teamsizeBox[i].setMaximumWidth(fm.horizontalAdvance(QString::number(numStudents*100)) + 20);
        connect(&teamsizeBox[i], QOverload<int>::of(&QSpinBox::valueChanged), this, &customTeamsizesDialog::teamsizeChanged);
        teamSizesTable->setCellWidget(i, 1, &teamsizeBox[i]);
    }
    teamSizesTable->resizeColumnsToContents();
    teamSizesTable->adjustSize();

    //Rows 4 - 7 - a spacer then remaining students label then a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(3, 20);
    theGrid->addWidget(&remainingStudents, 4, 0, 1, -1, Qt::AlignCenter);
    theGrid->setRowMinimumHeight(5, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 6, 0, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

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
            teamSizesTable->showRow(i);
        }
        else
        {
            teamSizesTable->hideRow(i);
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

    teamSizesTable->adjustSize();
    adjustSize();
}


void customTeamsizesDialog::teamsizeChanged(int /*unused*/)
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

customTeamnamesDialog::customTeamnamesDialog(int numTeams, const QStringList &teamNames, QWidget *parent)
    :QDialog (parent)
{
    this->numTeams = numTeams;
    teamName = new QLineEdit[numTeams];

    //Set up window with a grid layout
    setWindowTitle(tr("Choose custom team names"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setMinimumSize(300, 300);
    theGrid = new QGridLayout(this);

    //Row 1 - the table of team names
    teamNamesTable = new QTableWidget(this);
    teamNamesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    teamNamesTable->setSelectionMode(QAbstractItemView::NoSelection);
    teamNamesTable->setShowGrid(false);
    teamNamesTable->setAlternatingRowColors(true);
    teamNamesTable->setStyleSheet("QTableView::item{border-bottom: 1px solid black; padding: 10px;}");
    teamNamesTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    teamNamesTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    teamNamesTable->horizontalHeader()->setHidden(true);
    teamNamesTable->verticalHeader()->setHidden(true);
    teamNamesTable->setRowCount(numTeams);
    teamNamesTable->setColumnCount(2);
    teamNamesTable->horizontalHeader()->setStretchLastSection(true);
    theGrid->addWidget(teamNamesTable, 0, 0, 1, -1);

    for(int i = 0; i < numTeams; i++)
    {
        teamNamesTable->setCellWidget(i, 0, new QLabel(tr("Team ") + QString::number(i+1) + " "));
        teamName[i].setPlaceholderText(tr("Custom name"));
        if(i < teamNames.size())
        {
            teamName[i].setText((teamNames.at(i) == QString::number(i+1))? "" : teamNames.at(i));
        }
        teamNamesTable->setCellWidget(i, 1, &teamName[i]);
    }
    teamNamesTable->resizeColumnToContents(0);
    teamNamesTable->adjustSize();

    //Rows 2 and 3 - a spacer then reset table/ok/cancel buttons
    theGrid->setRowMinimumHeight(1, 20);
    resetNamesButton = new QPushButton(this);
    resetNamesButton->setText(tr("&Clear All Names"));
    theGrid->addWidget(resetNamesButton, 2, 0, 1, 1);
    connect(resetNamesButton, &QPushButton::clicked, this, &customTeamnamesDialog::clearAllNames);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 2, 1, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


customTeamnamesDialog::~customTeamnamesDialog()
{
    //delete dynamically allocated arrays created in class constructor
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
                            "\t gruepr@gmail.com\n"));
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
    //(one or more letters, digits, or special symbols, then '@', then one or more letters, digits, or special symbols, then '.', then 2, 3 or 4 letters)
    QRegularExpression emailAddressFormat("^[A-Z0-9.!#$%&*+_-~]+@[A-Z0-9.-]+\\.[A-Z]{2,64}$", QRegularExpression::CaseInsensitiveOption);
    email->setValidator(new QRegularExpressionValidator(emailAddressFormat, this));
    connect(email, &QLineEdit::textChanged, this, [this]()
                                             {QString stylecolor = (email->hasAcceptableInput())? "black" : "red";
                                              email->setStyleSheet("QLineEdit {color: " + stylecolor + ";}"); });

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(4, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    theGrid->addWidget(buttonBox, 5, 0);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(name, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(institution, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(email, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});

    adjustSize();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose which item(s) to save or print
/////////////////////////////////////////////////////////////////////////////////////////////////////////

whichFilesDialog::whichFilesDialog(const action saveOrPrint, const QStringList &previews, QWidget *parent)
    :QDialog (parent)
{
    saveDialog = (saveOrPrint == whichFilesDialog::save);
    QString saveOrPrintString = (saveDialog? tr("save") : tr("print"));

    //Set up window with a grid layout
    setWindowTitle(tr("Choose files to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);
    previousToolTipFont = QToolTip::font();
    QToolTip::setFont(QFont("Oxygen Mono", previousToolTipFont.pointSize()));

    explanation = new QLabel(this);
    explanation->setTextFormat(Qt::RichText);
    explanation->setText(tr("<br>You can ") + saveOrPrintString + tr(" the following files:<br>Preview a file by hovering over the title.<br>"));
    theGrid->addWidget(explanation, 0, 0, 1, -1);

    if(saveDialog)
    {
        textfile = new QLabel(this);
        textfile->setText(tr("text"));
        theGrid->addWidget(textfile, 1, 0);
        auto *pdftxtline = new QFrame(this);
        pdftxtline->setFrameShape(QFrame::VLine);
        pdftxtline->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(pdftxtline, 1, 1, 6, 1);
        pdffile = new QLabel(this);
        pdffile->setText("pdf");
        theGrid->addWidget(pdffile, 1, 2);
    }

    studentFiletxt = new QCheckBox(this);
    studentFiletxt->resize(30,30);
    theGrid->addWidget(studentFiletxt, 2, 0);
    connect(studentFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);

    studentFileLabel = new QPushButton(this);
    studentFileLabel->setFlat(true);
    studentFileLabel->setStyleSheet("Text-align:left");
    connect(studentFileLabel, &QPushButton::clicked, studentFiletxt, &QCheckBox::animateClick);
    studentFileLabel->setText(tr("Student's file:\nnames, email addresses, and team availability schedules."));
    theGrid->addWidget(studentFileLabel, 2, 3);
    if(saveDialog)
    {
        studentFilepdf = new QCheckBox(this);
        studentFilepdf->resize(30,30);
        theGrid->addWidget(studentFilepdf, 2, 2);
        connect(studentFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(studentFileLabel, &QPushButton::clicked, studentFilepdf, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty()))
    {
        studentFiletxt->setToolTip(previews.at(0));
        if(saveDialog)
        {
            studentFilepdf->setToolTip(previews.at(0));
        }
        studentFileLabel->setToolTip(previews.at(0));
    }
    auto *belowStudentLine = new QFrame(this);
    belowStudentLine->setFrameShape(QFrame::HLine);
    belowStudentLine->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(belowStudentLine, 3, 0, 1, -1);

    instructorFiletxt = new QCheckBox(this);
    instructorFiletxt->resize(30,30);
    theGrid->addWidget(instructorFiletxt, 4, 0);
    connect(instructorFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    instructorFileLabel = new QPushButton(this);
    instructorFileLabel->setFlat(true);
    instructorFileLabel->setStyleSheet("Text-align:left");
    connect(instructorFileLabel, &QPushButton::clicked, instructorFiletxt, &QCheckBox::animateClick);
    instructorFileLabel->setText(tr("Instructor's file:\nFile data, teaming options, optimization data,\n"
                                    "names, email addresses, demographic and attribute data, and team availability schedule."));
    theGrid->addWidget(instructorFileLabel, 4, 3);
    if(saveDialog)
    {
        instructorFilepdf = new QCheckBox(this);
        instructorFilepdf->resize(30,30);
        theGrid->addWidget(instructorFilepdf, 4, 2);
        connect(instructorFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(instructorFileLabel, &QPushButton::clicked, instructorFilepdf, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty()))
    {
        instructorFiletxt->setToolTip(previews.at(1));
        if(saveDialog)
        {
            instructorFilepdf->setToolTip(previews.at(1));
        }
        instructorFileLabel->setToolTip(previews.at(1));
    }
    auto *belowInstructorLine = new QFrame(this);
    belowInstructorLine->setFrameShape(QFrame::HLine);
    belowInstructorLine->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(belowInstructorLine, 5, 0, 1, -1);

    spreadsheetFiletxt = new QCheckBox(this);
    spreadsheetFiletxt->resize(30,30);
    theGrid->addWidget(spreadsheetFiletxt, 6, 0);
    connect(spreadsheetFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    spreadsheetFileLabel = new QPushButton(this);
    spreadsheetFileLabel->setFlat(true);
    spreadsheetFileLabel->setStyleSheet("Text-align:left");
    connect(spreadsheetFileLabel, &QPushButton::clicked, spreadsheetFiletxt, &QCheckBox::animateClick);
    spreadsheetFileLabel->setText(tr("Spreadsheet file:\nsections, teams, names, and email addresses in a tabular format."));
    theGrid->addWidget(spreadsheetFileLabel, 6, 3);
    if(!(previews.isEmpty()))
    {
        spreadsheetFiletxt->setToolTip(previews.at(2));
        spreadsheetFileLabel->setToolTip(previews.at(2));
    }

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    if(saveDialog)
    {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Save"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else
    {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Print"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    connect(buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button){QDialog::done(buttonBox->standardButton(button));});

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(7, 20);
    theGrid->addWidget(buttonBox, 8, 0, -1, -1);

    adjustSize();
}


whichFilesDialog::~whichFilesDialog()
{
    QToolTip::setFont(previousToolTipFont);
}


void whichFilesDialog::boxToggled()
{
    bool somethingClicked = studentFiletxt->isChecked() || instructorFiletxt->isChecked() || spreadsheetFiletxt->isChecked();
    if(saveDialog)
    {
        somethingClicked = somethingClicked || studentFilepdf->isChecked() || instructorFilepdf->isChecked();
    }
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(somethingClicked);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show/edit student data
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editOrAddStudentDialog::editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QStringList sectionNames, QWidget *parent)
    :QDialog (parent)
{
    student = studentToBeEdited;
    internalDataOptions = *dataOptions;

    //Set up window with a grid layout
    if(studentToBeEdited.surveyTimestamp.secsTo(QDateTime::currentDateTime()) < 10)     // if timestamp is within the past 10 seconds, it is a new student
    {
        setWindowTitle(tr("Add new student record"));
    }
    else
    {
        setWindowTitle(tr("Edit student record"));
    }
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    setSizeGripEnabled(true);
    theGrid = new QGridLayout(this);

    int numFields = 4 + (internalDataOptions.genderIncluded?1:0) + (internalDataOptions.URMIncluded?1:0) + (internalDataOptions.sectionIncluded?1:0) +
                         internalDataOptions.numAttributes + (internalDataOptions.prefTeammatesIncluded?1:0) +
                        (internalDataOptions.prefNonTeammatesIncluded?1:0) + (internalDataOptions.notesIncluded?1:0);
    explanation = new QLabel[numFields];
    datatext = new QLineEdit[numFields];
    datamultiline = new QPlainTextEdit[numFields];
    databox = new QComboBox[numFields];
    datacategorical = new CategoricalSpinBox[numFields];
    int field = 0;

    // calculate the height of 1 row of text in the multilines
    QFontMetrics fm(datamultiline[0].document()->defaultFont());
    QMargins margin = datamultiline[0].contentsMargins();
    const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline[0].document()->documentMargin()) + datamultiline[0].frameWidth() * 2 + margin.top() + margin.bottom();

    //Row 1 through 4--the required data
    QStringList fieldNames = {tr("Survey timestamp"), tr("First name"), tr("Last name"), tr("Email address")};
    QStringList fieldValues = {student.surveyTimestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)),
                               student.firstname, student.lastname, student.email};
    for(int i = 0; i < 4; i++)
    {
        explanation[field].setText(fieldNames.at(field));
        datatext[field].setText(fieldValues.at(field));
        connect(&datatext[field], &QLineEdit::editingFinished, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[field], field, 1);
        field++;
    }

    //Flag invalid timestamps
    connect(&(datatext[0]), &QLineEdit::textChanged, this, [this]()
                                                     {bool validTimeStamp = QDateTime::fromString(datatext[0].text(),
                                                            QLocale::system().dateTimeFormat(QLocale::ShortFormat)).isValid();
                                                      QString stylecolor = (validTimeStamp? "black" : "red");
                                                      datatext[0].setStyleSheet("QLineEdit {color: " + stylecolor + ";}");});

    if(internalDataOptions.genderIncluded)
    {
        explanation[field].setText(tr("Gender identity"));
        databox[field].addItems(QStringList() << tr("woman") << tr("man") << tr("nonbinary") << tr("unknown"));
        if(student.gender == StudentRecord::woman)
        {
            databox[field].setCurrentText(tr("woman"));
        }
        else if(student.gender == StudentRecord::man)
        {
            databox[field].setCurrentText(tr("man"));
        }
        else if(student.gender == StudentRecord::nonbinary)
        {
            databox[field].setCurrentText(tr("nonbinary"));
        }
        else
        {
           databox[field].setCurrentText(tr("unknown"));
        }
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(internalDataOptions.URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[field].addItems(internalDataOptions.URMResponses);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.URMResponse);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(internalDataOptions.sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        QCollator sortAlphanumerically;
        sortAlphanumerically.setNumericMode(true);
        sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(sectionNames.begin(), sectionNames.end(), sortAlphanumerically);
        databox[field].addItems(sectionNames);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.section);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    for(int attrib = 0; attrib < internalDataOptions.numAttributes; attrib++)
    {
        explanation[field].setText(tr("Attribute ") + QString::number(attrib + 1));
        QSpinBox *spinbox = &datacategorical[field];
        datacategorical[field].setWhatTypeOfValue(internalDataOptions.attributeIsOrdered[attrib] ? CategoricalSpinBox::numerical : CategoricalSpinBox::letter);
        datacategorical[field].setCategoricalValues(internalDataOptions.attributeQuestionResponses[attrib]);
        spinbox->setValue(student.attribute[attrib]);
        spinbox->setRange(0, internalDataOptions.attributeMax[attrib]);
        if(spinbox->value() == 0)
        {
            spinbox->setStyleSheet("QSpinBox {background-color: #DCDCDC;}");
        }
        connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int /*unused new value*/){ recordEdited(); });
        spinbox->setSpecialValueText(tr("not set/unknown"));
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(spinbox, field, 1);
        field++;
    }   

    if(internalDataOptions.prefTeammatesIncluded)
    {
        explanation[field].setText(tr("Preferred Teammates"));
        datamultiline[field].setPlainText(student.prefTeammates);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        explanation[field].setText(tr("Preferred Non-teammates"));
        datamultiline[field].setPlainText(student.prefNonTeammates);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    if(internalDataOptions.notesIncluded)
    {
        explanation[field].setText(tr("Notes"));
        datamultiline[field].setPlainText(student.notes);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numFields+1, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numFields+2, 0, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


editOrAddStudentDialog::~editOrAddStudentDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] explanation;
    delete [] datatext;
    delete [] datamultiline;
    delete [] databox;
    delete [] datacategorical;
}


void editOrAddStudentDialog::recordEdited()
{
    student.surveyTimestamp = QDateTime::fromString(datatext[0].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[1].text();
    student.lastname = datatext[2].text();
    student.email = datatext[3].text();
    int field = 4;
    if(internalDataOptions.genderIncluded)
    {
        if(databox[field].currentText() == tr("woman"))
        {
            student.gender = StudentRecord::woman;
        }
        else if(databox[field].currentText() == tr("man"))
        {
            student.gender = StudentRecord::man;
        }
        else if(databox[field].currentText() == tr("nonbinary"))
        {
            student.gender = StudentRecord::nonbinary;
        }
        else
        {
            student.gender = StudentRecord::unknown;
        }
        field++;
    }
    if(internalDataOptions.URMIncluded)
    {
        student.URMResponse = databox[field].currentText();
        field++;
    }
    if(internalDataOptions.sectionIncluded)
    {
        student.section = databox[field].currentText();
        field++;
    }
    for(int attrib = 0; attrib < internalDataOptions.numAttributes; attrib++)
    {
        if(datacategorical[field].value() == 0)
        {
            student.attribute[attrib] = -1;
            student.attributeResponse[attrib] = "";
            datacategorical[field].setStyleSheet("QSpinBox {background-color: #DCDCDC;}");
        }
        else
        {
            student.attribute[attrib] = datacategorical[field].value();
            student.attributeResponse[attrib] = internalDataOptions.attributeQuestionResponses[attrib].at(datacategorical[field].value() - 1);
            datacategorical[field].setStyleSheet("QSpinBox {}");
        }
        field++;
    }
    if(internalDataOptions.prefTeammatesIncluded)
    {
        student.prefTeammates = datamultiline[field].toPlainText();
        field++;
    }
    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        student.prefNonTeammates = datamultiline[field].toPlainText();
        field++;
    }
    if(internalDataOptions.notesIncluded)
    {
        student.notes = datatext[field].text();
        field++;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which attribute values should be disallowed on the same team
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherIncompatibleResponsesDialog::gatherIncompatibleResponsesDialog(const int attribute, const DataOptions *const dataOptions,
                                                                     const QVector< QPair<int,int> > &currIncompats, QWidget *parent)
    :QDialog (parent)
{
    numPossibleValues = dataOptions->attributeQuestionResponses[attribute].size() + 1;
    incompatibleResponses = currIncompats;

    //Set up window with a grid layout
    setWindowTitle(tr("Select incompatible responses for attribute ") + QString::number(attribute + 1));

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    QString attributeDescription = "<html><br><b>" + tr("Attribute") + " " + QString::number(attribute + 1) + ":</b><br>";
    attributeDescription += dataOptions->attributeQuestionText.at(attribute) +"<hr>";
    for(int response = 0; response < numPossibleValues; response++)
    {
        if(response == numPossibleValues - 1)
        {
            attributeDescription += tr("-. value not set/unknown");
        }
        else if(dataOptions->attributeIsOrdered[attribute])
        {
            // show reponse with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            attributeDescription += match.captured(1) + match.captured(2);
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            attributeDescription += (response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))) +
                                         ". " + dataOptions->attributeQuestionResponses[attribute].at(response);
        }
        attributeDescription += "<br>";
    }

    attributeQuestion = new QLabel(this);
    attributeQuestion->setText(attributeDescription + "</html>");
    theGrid->addWidget(attributeQuestion, 0, 0, 1, -1);

    auto *hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(hline, 1, 0, 1, -1);

    incompatAttributePart1 = new QLabel(this);
    incompatAttributePart1->setText("<html>" + tr("Prevent placing students with this response:") + "</html>");
    incompatAttributePart1->setWordWrap(true);
    theGrid->addWidget(incompatAttributePart1, 2, 0, 1, 2);

    // a checkbox and a label for each response value to set the primary
    primaryValues = new QRadioButton[numPossibleValues];
    primaryResponses = new QPushButton[numPossibleValues];
    primaryValuesGroup = new QButtonGroup(this);
    for(int response = 0; response < numPossibleValues; response++)
    {
        theGrid->addWidget(&primaryValues[response], response + 3, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
        primaryValuesGroup->addButton(&primaryValues[response]);

        if(response == numPossibleValues - 1)
        {
            primaryResponses[response].setText(tr("-"));
        }
        else if(dataOptions->attributeIsOrdered[attribute])
        {
            // show reponse with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            primaryResponses[response].setText(match.captured(1));
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            primaryResponses[response].setText((response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))));
        }
        primaryResponses[response].setFlat(true);
        primaryResponses[response].setStyleSheet("Text-align:left");
        connect(&primaryResponses[response], &QPushButton::clicked, &primaryValues[response], &QRadioButton::toggle);
        theGrid->addWidget(&primaryResponses[response], response + 3, 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
    }

    auto *vline = new QFrame(this);
    vline->setFrameShape(QFrame::VLine);
    vline->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(vline, 2, 2, numPossibleValues + 1, 1);

    incompatAttributePart2 = new QLabel(this);
    incompatAttributePart2->setText("<html>" + tr("on the same team as students with these responses:") + "</html>");
    incompatAttributePart2->setWordWrap(true);
    theGrid->addWidget(incompatAttributePart2, 2, 3, 1, -1);

    // a checkbox and a label for each response value to set the ones incompatible with the primary
    incompatValues = new QCheckBox[numPossibleValues];
    incompatResponses = new QPushButton[numPossibleValues];
    for(int response = 0; response < numPossibleValues; response++)
    {
        theGrid->addWidget(&incompatValues[response], response + 3, 3, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

        if(response == numPossibleValues - 1)
        {
            incompatResponses[response].setText(tr("-"));
        }
        else if(dataOptions->attributeIsOrdered[attribute])
        {
            // show reponse with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            incompatResponses[response].setText(match.captured(1));
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            incompatResponses[response].setText((response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))));
        }
        incompatResponses[response].setFlat(true);
        incompatResponses[response].setStyleSheet("Text-align:left");
        connect(&incompatResponses[response], &QPushButton::clicked, &incompatValues[response], &QCheckBox::toggle);
        theGrid->addWidget(&incompatResponses[response], response + 3, 4, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
    }

    // set second and fifth columns as the ones to grow
    theGrid->setColumnStretch(1, 1);
    theGrid->setColumnStretch(4, 1);

    //button to add the currently checked values as incompatible pairs
    addValuesButton = new QPushButton(this);
    addValuesButton->setText(tr("&Add these incompatible responses"));
    addValuesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(addValuesButton, &QPushButton::clicked, this, &gatherIncompatibleResponsesDialog::addValues);
    theGrid->addWidget(addValuesButton, numPossibleValues + 4, 0, 1, -1, Qt::AlignCenter);

    //explanatory text of which response pairs will be considered incompatible
    explanation = new QLabel(this);
    explanation->clear();
    theGrid->addWidget(explanation, numPossibleValues + 5, 0, 1, -1);
    theGrid->setRowStretch(numPossibleValues + 6, 1);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numPossibleValues + 7, 20);
    resetValuesButton = new QPushButton(this);
    resetValuesButton->setText(tr("&Clear all values"));
    resetValuesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    theGrid->addWidget(resetValuesButton, numPossibleValues + 8, 0, 1, 2);
    connect(resetValuesButton, &QPushButton::clicked, this, &gatherIncompatibleResponsesDialog::clearAllValues);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numPossibleValues + 8, 3, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    explanation->setText("<html><hr><br><b>" + tr("Students with these responses will not be placed on the same team:") + "<br><br><br></b></html>");
    adjustSize();
    updateExplanation();
}


gatherIncompatibleResponsesDialog::~gatherIncompatibleResponsesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] primaryValues;
    delete [] primaryResponses;
    delete [] incompatValues;
    delete [] incompatResponses;
}


void gatherIncompatibleResponsesDialog::updateExplanation()
{
    if(incompatibleResponses.isEmpty())
    {
        explanation->setText("<html><hr><br><b>" + tr("Currently all responses are compatible.") + "<br></b></html>");
    }
    else
    {
        QString explanationText = tr("Students with these responses will not be placed on the same team:<br>");
        for(const QPair<int, int> &pair : qAsConst(incompatibleResponses))
        {
            explanationText += "&nbsp;&nbsp;&nbsp;&nbsp;" + primaryResponses[(pair.first)-1].text().split('.').at(0) +
                               " " + QChar(0x27f7) + " " + primaryResponses[(pair.second)-1].text().split('.').at(0) + "<br>";
        }
        // remove all html tags, replace "-" with "not set/unknown"
        explanationText.remove("<html>").replace("-", tr("not set/unknown"));
        explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
    }
}


void gatherIncompatibleResponsesDialog::addValues()
{
    // create pairs for the primary value and each checked incompatible value
    for(int response1 = 0; response1 < numPossibleValues; response1++)
    {
        for(int response2 = 0; response2 < numPossibleValues; response2++)
        {
            if( primaryValues[response1].isChecked() && incompatValues[response2].isChecked() &&
                !incompatibleResponses.contains(QPair<int,int>(response1+1, response2+1)) &&
                !incompatibleResponses.contains(QPair<int,int>(response2+1, response1+1)) )
            {
                int smaller = std::min(response1+1, response2+1), larger = std::max(response1+1, response2+1);
                incompatibleResponses << QPair<int,int>(smaller, larger);
            }
        }
    }
    std::sort(incompatibleResponses.begin(), incompatibleResponses.end(), [](const QPair<int,int> a, const QPair<int,int> b){return (a.first*100+a.second) < (b.first*100+b.second);});
    updateExplanation();

    // reset checkboxes
    for(int response = 0; response < numPossibleValues; response++)
    {
        //primaryValues[response].setChecked(false);
        incompatValues[response].setChecked(false);
    }
}


void gatherIncompatibleResponsesDialog::clearAllValues()
{
    incompatibleResponses.clear();
    updateExplanation();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which racial/ethnic/cultural identities should be considered underrepresented
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherURMResponsesDialog::gatherURMResponsesDialog(const QStringList &URMResponses, const QStringList &currURMResponsesConsideredUR, QWidget *parent)
    :QDialog (parent)
{
    URMResponsesConsideredUR = currURMResponsesConsideredUR;

    //Set up window with a grid layout
    setWindowTitle(tr("Select underrepresented race/ethnicity responses"));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setMinimumSize(300, 300);

    theGrid = new QGridLayout(this);

    explanation = new QLabel(this);
    explanation->setText(tr("<html>Students gave the following responses when asked about their racial/ethnic/cultural identity. "
                            "Which of these should be considered underrepresented?<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);

    URMResponsesTable = new QTableWidget(this);
    URMResponsesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    URMResponsesTable->setSelectionMode(QAbstractItemView::NoSelection);
    URMResponsesTable->verticalHeader()->setHidden(true);
    URMResponsesTable->horizontalHeader()->setHidden(true);
    URMResponsesTable->setAlternatingRowColors(true);
    URMResponsesTable->setShowGrid(false);
    URMResponsesTable->setStyleSheet("QTableView::item{border-bottom: 1px solid black;}");
    URMResponsesTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    URMResponsesTable->horizontalHeader()->setStretchLastSection(true);
    theGrid->addWidget(URMResponsesTable, 1, 0, 1, -1);

    // a checkbox and a label for each response values
    const int numResponses = URMResponses.size();
    enableValue = new QCheckBox[numResponses];
    responses = new QPushButton[numResponses];
    URMResponsesTable->setRowCount(numResponses);
    URMResponsesTable->setColumnCount(2);
    for(int response = 0; response < numResponses; response++)
    {
        const QString &responseText = URMResponses.at(response);
        enableValue[response].setChecked(URMResponsesConsideredUR.contains(responseText));
        enableValue[response].setStyleSheet("Text-align:center; margin-left:10%; margin-right:10%;");
        URMResponsesTable->setCellWidget(response, 0, &enableValue[response]);
        responses[response].setText(responseText);
        responses[response].setFlat(true);
        responses[response].setStyleSheet("Text-align:left");
        URMResponsesTable->setCellWidget(response, 1, &responses[response]);
        connect(&responses[response], &QPushButton::clicked, &enableValue[response], &QCheckBox::toggle);
        connect(&enableValue[response], &QCheckBox::stateChanged, this, [&, response](int state){
                                                                                 if(state == Qt::Checked)
                                                                                   {URMResponsesConsideredUR << responses[response].text();
                                                                                    responses[response].setStyleSheet("Text-align:left;font-weight: bold;");}
                                                                                 else
                                                                                   {URMResponsesConsideredUR.removeAll(responses[response].text());
                                                                                    responses[response].setStyleSheet("Text-align:left;");}
                                                                                 });
    }
    URMResponsesTable->resizeColumnToContents(0);
    URMResponsesTable->adjustSize();

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(2, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 3, 1, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


gatherURMResponsesDialog::~gatherURMResponsesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] enableValue;
    delete [] responses;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show progress in optimization
/////////////////////////////////////////////////////////////////////////////////////////////////////////

progressDialog::progressDialog(QtCharts::QChartView *chart, QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Optimizing teams..."));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint);
    setSizeGripEnabled(true);
    setModal(true);
    theGrid = new QGridLayout(this);

    statusText = new QLabel(this);
    QFont defFont("Oxygen Mono");
    statusText->setFont(defFont);
    statusText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(statusText, 0, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);

    explanationIcon = new QLabel(this);
    explanationIcon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    auto *movie = new QMovie(":/icons/loading.gif");
    explanationIcon->setMovie(movie);
    movie->start();

    explanationText = new QLabel(this);
    explanationText->setFont(defFont);
    explanationText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto explanationBox = new QHBoxLayout;
    theGrid->addLayout(explanationBox, 1, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    explanationBox->addWidget(explanationIcon, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    explanationBox->addWidget(explanationText, 0, Qt::AlignLeft | Qt::AlignVCenter);
    explanationBox->addStretch(1);

    theGrid->setRowMinimumHeight(2, 20);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(line, 3, 0, 1, -1);

    if(chart != nullptr)
    {
        theGrid->addWidget(chart, 6, 0, 1, -1);
        chart->hide();
        graphShown = false;

        showStatsButton = new QPushButton(QIcon(":/icons/down_arrow.png"), "Show progress", this);
        showStatsButton->setIconSize(QSize(20, 20));
        showStatsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(showStatsButton, &QPushButton::clicked, this, [this, chart] {statsButtonPushed(chart);});
        theGrid->addWidget(showStatsButton, 4, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
        theGrid->setColumnStretch(1,1);
    }

    onlyStopManually = new QCheckBox("Continue optimizing\nuntil I manually stop it.", this);
    onlyStopManually->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(onlyStopManually, 4, 2, 1, 1, Qt::AlignRight | Qt::AlignVCenter);

    stopHere = new QPushButton(QIcon(":/icons/stop.png"), "Stop\nnow", this);
    stopHere->setIconSize(QSize(30, 30));
    stopHere->setToolTip(tr("Stop the optimization process immediately and show the best set of teams found so far."));
    stopHere->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(stopHere, &QPushButton::clicked, this, [this] {emit letsStop();});
    theGrid->addWidget(stopHere, 4, 3, 1, -1, Qt::AlignRight | Qt::AlignVCenter);

    countdownToClose = new QTimer(this);
    setText("");
    adjustSize();
}

void progressDialog::setText(const QString &text, int generation, float score, bool autostopInProgress)
{
    QString explanation = "<html>" + tr("Generation ") + QString::number(generation) + " - "
                          + tr("Top Score = ") + (score < 0? "<font face=\"serif\"> - </font>": "") + QString::number(std::abs(score)) +
                          "<br><span style=\"color:" + (autostopInProgress? "green" : "black") + ";\">" + text;
    if(autostopInProgress && !onlyStopManually->isChecked())
    {
        explanation += "<br>" + tr("Optimization will stop in ") + QString::number(secsLeftToClose) + tr(" seconds.");
    }
    explanation += "</span></html>";
    explanationText->setText(explanation);

    if(autostopInProgress)
    {
        explanationIcon->setPixmap(QIcon(":/icons/ok.png").pixmap(25, 25));
    }

    if(autostopInProgress && !onlyStopManually->isChecked())
    {
        statusText->setText(tr("Status: Finalizing..."));
    }
    else
    {
        statusText->setText(tr("Status: Optimizing..."));
    }
}

void progressDialog::highlightStopButton()
{
    stopHere->setFocus();

    if(countdownToClose->isActive() || onlyStopManually->isChecked())
    {
        return;
    }

    connect(countdownToClose, &QTimer::timeout, this, &progressDialog::updateCountdown);
    countdownToClose->start(1000);
}

void progressDialog::updateCountdown()
{
    if(onlyStopManually->isChecked())
    {
        secsLeftToClose = 5;
        return;
    }

    secsLeftToClose--;
    explanationText->setText(explanationText->text().replace(QRegularExpression(tr("stop in ") + "\\d*"), tr("stop in ") +  QString::number(std::max(0, secsLeftToClose))));
    if(secsLeftToClose == 0)
    {
        stopHere->animateClick();
    }
}

void progressDialog::reject()
{
    // If closing the window with click on close or hitting 'Esc', stop the optimization, too
    stopHere->animateClick();
    QDialog::reject();
}

void progressDialog::statsButtonPushed(QtCharts::QChartView *chart)
{
    graphShown = !graphShown;

    int height, width;
    QIcon icon;
    QString butText;
    if(graphShown)
    {
        chart->show();
        height = 400;
        width = QFontMetrics(QFont("Oxygen Mono", QFont("Oxygen Mono").pointSize() - 2)).width("10 15 20 25 30 35 40 45 50 55 60 65 70");
        icon = QIcon(":/icons/up_arrow.png");
        butText = "Hide progress";
    }
    else
    {
        chart->hide();
        height = 0;
        width = 0;
        icon = QIcon(":/icons/down_arrow.png");
        butText = "Show progress";
    }
    int chartRow, chartCol, x;
    theGrid->getItemPosition(theGrid->indexOf(chart), &chartRow, &chartCol, &x, &x);
    theGrid->setRowMinimumHeight(chartRow, height);
    theGrid->setColumnMinimumWidth(chartCol, width);
    showStatsButton->setIcon(icon);
    showStatsButton->setText(butText);
    adjustSize();
}

progressDialog::~progressDialog()
{
    countdownToClose->stop();
}
