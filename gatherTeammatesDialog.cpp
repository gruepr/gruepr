#include "gatherTeammatesDialog.h"
#include "csvfile.h"
#include "findMatchingNameDialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardItemModel>


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
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    theGrid = new QGridLayout(this);

    //First row - the current data
    currentListOfTeammatesTable = new QTableWidget(this);
    currentListOfTeammatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    currentListOfTeammatesTable->setSelectionMode(QAbstractItemView::NoSelection);
    currentListOfTeammatesTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    currentListOfTeammatesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
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
                "color:black;}"
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
    theGrid->setRowMinimumHeight(row, DIALOG_SPACER_ROWHEIGHT);
    resetSaveOrLoad = new QComboBox(this);
    resetSaveOrLoad->setIconSize(QSize(15,15));
    resetSaveOrLoad->addItem(tr("Additional actions"));
    int itemnum = 1;
    resetSaveOrLoad->insertSeparator(itemnum++);
    resetSaveOrLoad->addItem(QIcon(":/icons/delete.png"), tr("Clear all ") + typeText.toLower() + tr(" teammates..."));
    resetSaveOrLoad->setItemData(itemnum++, tr("Remove all currently listed data from the table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/save.png"), tr("Save the current set to a CSV file..."));
    resetSaveOrLoad->setItemData(itemnum++, tr("Save the current table to a csv file"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/openFile.png"), tr("Load a CSV file of teammates..."));
    resetSaveOrLoad->setItemData(itemnum++, tr("Add data from a csv file to the current table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/gruepr.png"), tr("Load a gruepr spreadsheet file..."));
    resetSaveOrLoad->setItemData(itemnum++, tr("Add names from a previous set of gruepr-created teams to the current table"), Qt::ToolTipRole);
    resetSaveOrLoad->addItem(QIcon(":/icons/surveymaker.png"), tr("Import students' preferences from the survey"));
    if(whatType == required || whatType == requested)
    {
        if(requestsInSurvey)
        {
            resetSaveOrLoad->setItemData(itemnum, tr("Add the names of the preferred teammate(s) submitted by students in the survey"), Qt::ToolTipRole);
        }
        else
        {
            resetSaveOrLoad->setItemData(itemnum, tr("Preferred teammate information was not found in the survey"), Qt::ToolTipRole);
            auto model = qobject_cast< QStandardItemModel * >(resetSaveOrLoad->model());
            auto item = model->item(itemnum);
            item->setEnabled(false);
        }
    }
    if(whatType == prevented)
    {
        if(requestsInSurvey)
        {
            resetSaveOrLoad->setItemData(itemnum, tr("Add the names of the preferred non-teammate(s) submitted by students in the survey"), Qt::ToolTipRole);
        }
        else
        {
            resetSaveOrLoad->setItemData(itemnum, tr("Preferred non-teammate information was not found in the survey"), Qt::ToolTipRole);
            auto model = (qobject_cast< QStandardItemModel * >(resetSaveOrLoad->model()));
            auto item = model->item(itemnum);
            item->setEnabled(false);
        }
    }
    connect(resetSaveOrLoad, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {int itemnum = 2;
                                                                                                           if(index == itemnum++) {clearAllTeammateSets();}
                                                                                                           else if(index == itemnum++) {saveCSVFile();}
                                                                                                           else if(index == itemnum++) {loadCSVFile();}
                                                                                                           else if(index == itemnum++) {loadSpreadsheetFile();}
                                                                                                           else if(index == itemnum++) {loadStudentPrefs();}});
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
    resetSaveOrLoad->setCurrentIndex(0);

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::write, tr("Save File of Teammates"), "", tr("Comma-Separated Value File (*.csv);;All Files (*)")))
    {
        return false;
    }

    // write header
    csvFile.headerValues << tr("basename");
    int firstDataCol = requestsInSurvey? 1 : 0;
    int lastDataCol = currentListOfTeammatesTable->columnCount() - firstDataCol;
    for(int i = 1; i <= lastDataCol; i++)
    {
        csvFile.headerValues << tr("name") + QString::number(i);
    }
    if(!csvFile.writeHeader())
    {
        QMessageBox::critical(this, tr("No Files Saved"), tr("This data was not saved.\nThere was an issue writing the file to disk."));
        return false;
    }

    // write data rows
    for(int basename = 0; basename < currentListOfTeammatesTable->rowCount(); basename++)
    {
        csvFile.fieldValues.clear();
        QStringList lastnameFirstname = currentListOfTeammatesTable->verticalHeaderItem(basename)->text().split(',');
        csvFile.fieldValues << lastnameFirstname.at(1).trimmed() + " " + lastnameFirstname.at(0).trimmed();
        for(int teammate = firstDataCol; teammate <= lastDataCol; teammate++)
        {
            QWidget *teammateItem(currentListOfTeammatesTable->cellWidget(basename,teammate));
            if (teammateItem != nullptr)
            {
                lastnameFirstname = teammateItem->property("studentName").toString().split(',');
                csvFile.fieldValues << lastnameFirstname.at(1).trimmed() + " " + lastnameFirstname.at(0).trimmed();
            }
        }
        csvFile.writeDataRow();
    }

    csvFile.close();
    return true;
}


bool gatherTeammatesDialog::loadCSVFile()
{
    resetSaveOrLoad->setCurrentIndex(0);

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::read, tr("Open CSV File of Teammates"), "", tr("Comma-Separated Value File (*.csv);;All Files (*)")))
    {
        return false;
    }

    // Read the header row and first data row to make sure file format is correct.
    bool formattedCorrectly = true;
    int numFields = 0;
    if(csvFile.readHeader())
    {
        numFields = csvFile.headerValues.size();
    }
    if(numFields < 2)       // should be basename, name1, name2, name3, ..., nameN
    {
        formattedCorrectly = false;
    }
    else
    {
        if((csvFile.headerValues.at(0).toLower() != tr("basename")) || (!csvFile.headerValues.at(1).toLower().startsWith(tr("name"))))
        {
            formattedCorrectly = false;
        }
        csvFile.readDataRow();
        if(csvFile.fieldValues.size() < numFields)
        {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        csvFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique base names into basenames and other names in the row into corresponding teammates list
    QStringList basenames;
    QVector<QStringList> teammates;
    csvFile.readHeader();
    while(csvFile.readDataRow())
    {
        int pos = basenames.indexOf(csvFile.fieldValues.at(0).trimmed()); // get index of this name

        if(pos == -1)   // basename is not yet found in basenames list
        {
            basenames << csvFile.fieldValues.at(0).trimmed();
            teammates.append(QStringList());
            for(int i = 1; i < numFields; i++)
            {
                QString teammate = csvFile.fieldValues.at(i).trimmed();
                if(!teammate.isEmpty())
                {
                    teammates.last() << teammate;
                }
            }
        }
        else
        {
            QMessageBox::critical(this, tr("File error."), tr("This file has an error in its format:\nThe same name appears more than once in the first column."), QMessageBox::Ok);
            csvFile.close();
            return false;
        }
    }
    csvFile.close();

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
                auto *choiceWindow = new findMatchingNameDialog(numStudents, student, teammates.at(basename).at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << choiceWindow->currSurveyID;
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
                    student[IDs[ID]].requiredWith[IDs[0]] = true;
                }
                else if(whatType == prevented)
                {
                    student[IDs[0]].preventedWith[IDs[ID]] = true;
                    student[IDs[ID]].preventedWith[IDs[0]] = true;
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
                auto *choiceWindow = new findMatchingNameDialog(numStudents, student, prefs.at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << choiceWindow->currSurveyID;
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
                    student[IDs[ID]].requiredWith[IDs[0]] = true;
                }
                else if(whatType == prevented)
                {
                    student[IDs[0]].preventedWith[IDs[ID]] = true;
                    student[IDs[ID]].preventedWith[IDs[0]] = true;
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
    CsvFile spreadsheetFile(CsvFile::tab);
    if(!spreadsheetFile.open(this, CsvFile::read, tr("Open Spreadsheet File of Previous Teammates"), "", tr("Spreadsheet File (*.txt);;All Files (*)")))
    {
        resetSaveOrLoad->setCurrentIndex(0);
        return false;
    }

    // Read the header row and make sure file format is correct. If so, read next line to make sure it has data
    bool formattedCorrectly = true;
    int numFields = 0;
    if(spreadsheetFile.readHeader())
    {
        numFields = spreadsheetFile.headerValues.size();
    }
    if(numFields < 4)       // should be section, team, name, email
    {
        formattedCorrectly = false;
    }
    else
    {
        if((spreadsheetFile.headerValues.at(0).toLower() != tr("section")) || (spreadsheetFile.headerValues.at(1).toLower() != tr("team"))
                || (spreadsheetFile.headerValues.at(2).toLower() != tr("name")) || (spreadsheetFile.headerValues.at(3).toLower() != tr("email")))
        {
            formattedCorrectly = false;
        }
        spreadsheetFile.readDataRow();
        if(spreadsheetFile.fieldValues.size() < 4)
        {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        spreadsheetFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique team strings into teams and new/matching names into corresponding teammates list
    QStringList teamnames;
    QVector<QStringList> teammates;
    spreadsheetFile.readHeader();
    while(spreadsheetFile.readDataRow())
    {
        int pos = teamnames.indexOf(spreadsheetFile.fieldValues.at(1).trimmed()); // get index of this team

        if(pos == -1)   // team is not yet found in teams list
        {
            teamnames << spreadsheetFile.fieldValues.at(1).trimmed();
            teammates.append(QStringList(spreadsheetFile.fieldValues.at(2).trimmed()));
        }
        else
        {
            teammates[pos].append(spreadsheetFile.fieldValues.at(2).trimmed());
        }
    }
    spreadsheetFile.close();

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
                auto *choiceWindow = new findMatchingNameDialog(numStudents, student, teammate.at(searchStudent), this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << choiceWindow->currSurveyID;
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
    currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #1")));
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
                    currentListOfTeammatesTable->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #") +
                                                                                                      QString::number(column + (requestsInSurvey? 0:1))));
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
