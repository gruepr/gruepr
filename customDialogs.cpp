#include "Levenshtein.h"
#include "customDialogs.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMovie>
#include <QTextStream>
#include <QTimer>
#include <QToolTip>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog window to select sets of required, prevented, or requested teammates
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherTeammatesDialog::gatherTeammatesDialog(const typeOfTeammates whatTypeOfTeammate, const studentRecord studentrecs[],
                                                int numStudentsComingIn, const QString &sectionname, QWidget *parent)
    : QDialog(parent)
{
    //copy data into local versions, including full database of students
    numStudents = numStudentsComingIn;
    sectionName = sectionname;
    student = new studentRecord[numStudentsComingIn];
    for(int i = 0; i < numStudentsComingIn; i++)
    {
        student[i] = studentrecs[i];
    }
    whatType = whatTypeOfTeammate;

    //start off with no teammate data changed
    teammatesSpecified = false;

    //Set up window
    QString typeText;
    auto *explanation = new QLabel(this);
    if(whatType == gatherTeammatesDialog::required)
    {
        typeText = tr("Required");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) +
                             tr(" students that will be required to be on the same team, then click the \"Add set\" button."));
    }
    else if (whatType == gatherTeammatesDialog::prevented)
    {
        typeText = tr("Prevented");
        explanation->setText(tr("Select up to ") + QString::number(possibleNumIDs) +
                             tr(" students that will be prevented from bring on the same team, then click the \"Add set\" button."));
    }
    else
    {
        typeText = tr("Requested");
        explanation->setText(tr("Select a student and up to ") + QString::number(possibleNumIDs) +
                             tr(" requested teammates, then click the \"Add set\" button."));
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

    //Next rows - the selection of students
    QList<studentRecord> studentsInComboBoxes;
    //Add to combobox a list of all the student names (in this section)
    for(int ID = 0; ID < numStudentsComingIn; ID++)
    {
        if((sectionName == "") || (sectionName == student[ID].section))
        {
            studentsInComboBoxes << student[ID];
        }
    }
    std::sort(studentsInComboBoxes.begin(), studentsInComboBoxes.end(), [](const studentRecord &i, const studentRecord &j)
                                                                        {return (i.lastname+i.firstname) < (j.lastname+j.firstname);});

    //If this is requested teammates, add the 'base' student button in the third row
    int row = 2;
    if(whatType == gatherTeammatesDialog::requested)
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
        if(whatType != gatherTeammatesDialog::requested)
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
    resetSaveOrLoad->addItem(QIcon(":/icons/open.png"), tr("Load a CSV file of teammates..."));
    resetSaveOrLoad->setItemData(4, tr("Add data from a csv file to the current table"), Qt::ToolTipRole);
    if(whatType != gatherTeammatesDialog::requested)
    {
        resetSaveOrLoad->addItem(QIcon(":/icons/gruepr.png"), tr("Load a gruepr spreadsheet file..."));
        resetSaveOrLoad->setItemData(5, tr("Add names from a previous set of gruepr-created teams to the current table"), Qt::ToolTipRole);
    }
    theGrid->addWidget(resetSaveOrLoad, row+1, 0, 1, 3);
    connect(resetSaveOrLoad, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {if(index == 2) clearAllTeammateSets();
                                                                                                           else if(index == 3) saveCSVFile();
                                                                                                           else if(index == 4) loadCSVFile();
                                                                                                           else if(index == 5) loadSpreadsheetFile();});
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
        int baseStudent = possibleTeammates[possibleNumIDs].itemData(possibleTeammates[possibleNumIDs].currentIndex()).toInt();
        //Reset combobox
        possibleTeammates[possibleNumIDs].setCurrentIndex(0);
        for(int ID1 = 0; ID1 < count; ID1++)
        {
            if(baseStudent != IDs[ID1])
            {
                //we have at least one requested teammate pair!
                teammatesSpecified = true;

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
    QList<QStringList> teammates;
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
        QList<int> IDs;
        for(int searchStudent = 0; searchStudent < teammates.at(basename).size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) && (teammates.at(basename).at(searchStudent)) != (student[knownStudent].firstname + " " + student[knownStudent].lastname))
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
                QMultiMap<int, QString> possibleStudents;
                for(knownStudent = 0; knownStudent < numStudents; knownStudent++)
                {
                    possibleStudents.insert(levenshtein::distance(teammates.at(basename).at(searchStudent),
                                            student[knownStudent].firstname + " " + student[knownStudent].lastname),
                                            student[knownStudent].firstname + " " + student[knownStudent].lastname + "&ID=" + QString::number(student[knownStudent].ID));
                }

                // Create student selection window
                auto *choiceWindow = new QDialog(this);
                choiceWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
                choiceWindow->setWindowTitle("Choose student");
                auto *grid = new QGridLayout(choiceWindow);
                auto *text = new QLabel(choiceWindow);
                text->setText(tr("An exact match for") + " <b>" + teammates.at(basename).at(searchStudent) + "</b> " +
                              tr("could not be found.<br>Please select this student from the list:"));
                grid->addWidget(text, 0, 0, 1, -1);
                auto *names = new QComboBox(choiceWindow);
                QMultiMap<int, QString>::const_iterator i = possibleStudents.constBegin();
                while (i != possibleStudents.constEnd())
                {
                    QStringList nameAndID = i.value().split("&ID=");    // split off the ID to use as the UserData role
                    names->addItem(nameAndID.at(0), nameAndID.at(1).toInt());
                    i++;
                }
                grid->addWidget(names, 1, 0, 1, -1);
                grid->setRowMinimumHeight(2, 20);
                auto *OKCancel = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, choiceWindow);
                OKCancel->button(QDialogButtonBox::Cancel)->setText("Ignore this student");
                connect(OKCancel, &QDialogButtonBox::accepted, choiceWindow, &QDialog::accept);
                connect(OKCancel, &QDialogButtonBox::rejected, choiceWindow, &QDialog::reject);
                grid->addWidget(OKCancel, 3, 0, 1, -1);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << (names->currentData(Qt::UserRole)).toInt();
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
                teammatesSpecified = true;

                if(whatType == gatherTeammatesDialog::required)
                {
                    student[IDs[0]].requiredWith[IDs[ID]] = true;
                }
                else if(whatType == gatherTeammatesDialog::prevented)
                {
                    student[IDs[0]].preventedWith[IDs[ID]] = true;
                }
                else    //whatType == gatherTeammatesDialog::requested
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
    QList<QStringList> teammates;
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
    for(int team = 0; team < teammates.size(); team++)
    {
        QList<int> IDs;
        for(int searchStudent = 0; searchStudent < teammates.at(team).size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) && (teammates.at(team).at(searchStudent)) != (student[knownStudent].firstname + " " + student[knownStudent].lastname))
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
                QMultiMap<int, QString> possibleStudents;
                for(knownStudent = 0; knownStudent < numStudents; knownStudent++)
                {
                    possibleStudents.insert(levenshtein::distance(teammates.at(team).at(searchStudent),
                                            student[knownStudent].firstname + " " + student[knownStudent].lastname),
                                            student[knownStudent].firstname + " " + student[knownStudent].lastname + "&ID=" + QString::number(student[knownStudent].ID));
                }

                // Create student selection window
                auto *choiceWindow = new QDialog(this);
                choiceWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
                choiceWindow->setWindowTitle("Choose student");
                auto *grid = new QGridLayout(choiceWindow);
                auto *text = new QLabel(choiceWindow);
                text->setText(tr("An exact match for") + " <b>" + teammates.at(team).at(searchStudent) + "</b> " +
                              tr("could not be found.<br>Please select this student from the list:"));
                grid->addWidget(text, 0, 0, 1, -1);
                auto *names = new QComboBox(choiceWindow);
                QMultiMap<int, QString>::const_iterator i = possibleStudents.constBegin();
                while (i != possibleStudents.constEnd())
                {
                    QStringList nameAndID = i.value().split("&ID=");    // split off the ID to use as the UserData role
                    names->addItem(nameAndID.at(0), nameAndID.at(1).toInt());
                    i++;
                }
                grid->addWidget(names, 1, 0, 1, -1);
                grid->setRowMinimumHeight(2, 20);
                auto *OKCancel = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, choiceWindow);
                OKCancel->button(QDialogButtonBox::Cancel)->setText("Ignore this student");
                connect(OKCancel, &QDialogButtonBox::accepted, choiceWindow, &QDialog::accept);
                connect(OKCancel, &QDialogButtonBox::rejected, choiceWindow, &QDialog::reject);
                grid->addWidget(OKCancel, 3, 0, 1, -1);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << (names->currentData(Qt::UserRole)).toInt();
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

    refreshDisplay();
    return true;
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

    QList<studentRecord> studentAs;
    for(int ID = 0; ID < numStudents; ID++)
    {
        if((sectionName == "") || (sectionName == student[ID].section))
        {
            studentAs << student[ID];
        }
    }
    std::sort(studentAs.begin(), studentAs.end(), [](const studentRecord &A, const studentRecord &B)
                                                    {return ((A.lastname+A.firstname) < (B.lastname+B.firstname));});

    int row=0, column;
    bool atLeastOneTeammate;
    for(QList<studentRecord>::iterator studentA = studentAs.begin(); studentA != studentAs.end(); studentA++)
    {
        atLeastOneTeammate = false;
        column = 0;
        currentListOfTeammatesTable->setRowCount(row+1);
        currentListOfTeammatesTable->setVerticalHeaderItem(row, new QTableWidgetItem(studentA->lastname + ", " + studentA->firstname));
        bool printStudent;
        for(int studentBID = 0; studentBID < numStudents; studentBID++)
        {
            if(whatType == gatherTeammatesDialog::required)
            {
                printStudent = studentA->requiredWith[studentBID];
            }
            else if(whatType == gatherTeammatesDialog::prevented)
            {
                printStudent = studentA->preventedWith[studentBID];
            }
            else
            {
                printStudent = studentA->requestedWith[studentBID];
            }
            if(printStudent)
            {
                atLeastOneTeammate = true;
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
                remover->setProperty("studentAID", studentA->ID);
                remover->setProperty("studentBID", studentBID);
                if(whatType == gatherTeammatesDialog::required)
                {
                    connect(remover, &QPushButton::clicked, this, [this, remover]
                                                            {int studentAID = remover->property("studentAID").toInt();
                                                             int studentBID = remover->property("studentBID").toInt();
                                                             student[studentAID].requiredWith[studentBID] = false;
                                                             student[studentBID].requiredWith[studentAID] = false;
                                                             refreshDisplay();});
                }
                else if(whatType == gatherTeammatesDialog::prevented)
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
            currentListOfTeammatesTable->setItem(row, 0, new QTableWidgetItem("--"));
        }
        row++;
    }
    currentListOfTeammatesTable->resizeColumnsToContents();
    currentListOfTeammatesTable->resizeRowsToContents();

    resetSaveOrLoad->setCurrentIndex(0);
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
    theGrid->addWidget(resetNamesButton, (numTeams/4)+2, 0, 1, 3);
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
    connect(studentFileLabel, &QPushButton::clicked, studentFiletxt, &QCheckBox::toggle);
    studentFileLabel->setText(tr("Student's file:\nnames, email addresses, and team availability schedules."));
    theGrid->addWidget(studentFileLabel, 2, 3);
    if(saveDialog)
    {
        studentFilepdf = new QCheckBox(this);
        studentFilepdf->resize(30,30);
        theGrid->addWidget(studentFilepdf, 2, 2);
        connect(studentFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(studentFileLabel, &QPushButton::clicked, studentFilepdf, &QCheckBox::toggle);
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
    connect(instructorFileLabel, &QPushButton::clicked, instructorFiletxt, &QCheckBox::toggle);
    instructorFileLabel->setText(tr("Instructor's file:\nFile data, teaming options, optimization data,\n"
                                    "names, email addresses, demographic and attribute data, and team availability schedule."));
    theGrid->addWidget(instructorFileLabel, 4, 3);
    if(saveDialog)
    {
        instructorFilepdf = new QCheckBox(this);
        instructorFilepdf->resize(30,30);
        theGrid->addWidget(instructorFilepdf, 4, 2);
        connect(instructorFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(instructorFileLabel, &QPushButton::clicked, instructorFilepdf, &QCheckBox::toggle);
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
    connect(spreadsheetFileLabel, &QPushButton::clicked, spreadsheetFiletxt, &QCheckBox::toggle);
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

editOrAddStudentDialog::editOrAddStudentDialog(const studentRecord &studentToBeEdited, const DataOptions &dataOptions,
                                               const QStringList &sectionNames, QWidget *parent)
    :QDialog (parent)
{
    student = studentToBeEdited;
    this->dataOptions = dataOptions;

    //Set up window with a grid layout
    if(studentToBeEdited.surveyTimestamp.secsTo(QDateTime::currentDateTime()) < 10)     // if timestamp is within the past 10 seconds, it is a new student
    {
        setWindowTitle(tr("Add new student record"));
    }
    else
    {
        setWindowTitle(tr("Edit student record"));
    }
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    int numFields = 4 + (dataOptions.genderIncluded?1:0) + (dataOptions.URMIncluded?1:0) + (dataOptions.sectionIncluded?1:0) +
                            dataOptions.numAttributes + (dataOptions.notesIncluded?1:0);
    explanation = new QLabel[numFields];
    datatext = new QLineEdit[numFields];
    databox = new QComboBox[numFields];
    datanumber = new QSpinBox[numFields];
    datacategorical = new CategoricalSpinBox[numFields];
    int field = 0;

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

    if(dataOptions.genderIncluded)
    {
        explanation[field].setText(tr("Gender identity"));
        databox[field].addItems(QStringList() << tr("woman") << tr("man") << tr("nonbinary/unknown"));
        databox[field].setCurrentText(student.gender==studentRecord::woman?tr("woman"):(student.gender==studentRecord::man?tr("man"):tr("nonbinary/unknown")));
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(dataOptions.URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[field].addItems(dataOptions.URMResponses);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.URMResponse);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(dataOptions.sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        databox[field].addItems(sectionNames);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.section);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    for(int attrib = 0; attrib < dataOptions.numAttributes; attrib++)
    {
        explanation[field].setText(tr("Attribute ") + QString::number(attrib + 1));
        QSpinBox *spinbox = (dataOptions.attributeIsOrdered[attrib] ? &datanumber[field] : &datacategorical[field]);
        datacategorical[field].setCategoricalValues(dataOptions.attributeQuestionResponses[attrib]);
        spinbox->setValue(student.attribute[attrib]);
        spinbox->setRange(0, dataOptions.attributeMax[attrib]);
        if(spinbox->value() == 0)
        {
            spinbox->setStyleSheet("QSpinBox { background-color: #DCDCDC;}");
        }
        connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int /*unused new value*/){ recordEdited(); });
        spinbox->setSpecialValueText(tr("not set/unknown"));
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(spinbox, field, 1);
        field++;
    }

    if(dataOptions.notesIncluded)
    {
        explanation[field].setText(tr("Notes"));
        datatext[field].setText(student.notes);
        connect(&datatext[field], &QLineEdit::editingFinished, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[field], field, 1);
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
    delete [] databox;
    delete [] datanumber;
    delete [] datacategorical;
}


void editOrAddStudentDialog::recordEdited()
{
    student.surveyTimestamp = QDateTime::fromString(datatext[0].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[1].text();
    student.lastname = datatext[2].text();
    student.email = datatext[3].text();
    int field = 4;
    if(dataOptions.genderIncluded)
    {
        student.gender = (databox[field].currentText() == tr("woman")? studentRecord::woman :
                                                                       (databox[field].currentText()==tr("man")? studentRecord::man : studentRecord::neither));
        field++;
    }
    if(dataOptions.URMIncluded)
    {
        student.URMResponse = databox[field].currentText();
        field++;
    }
    if(dataOptions.sectionIncluded)
    {
        student.section = databox[field].currentText();
        field++;
    }
    for(int attrib = 0; attrib < dataOptions.numAttributes; attrib++)
    {
        QSpinBox *spinbox = (dataOptions.attributeIsOrdered[attrib] ? &datanumber[field] : &datacategorical[field]);
        if(spinbox->value() != 0)
        {
            student.attribute[attrib] = spinbox->value();
            student.attributeResponse[attrib] = dataOptions.attributeQuestionResponses[attrib].at(spinbox->value() - 1);
            spinbox->setStyleSheet("QSpinBox { }");
        }
        else
        {
            student.attribute[attrib] = -1;
            student.attributeResponse[attrib] = "";
            spinbox->setStyleSheet("QSpinBox { background-color: #DCDCDC;}");
        }
        field++;
    }
    if(dataOptions.notesIncluded)
    {
        student.notes = datatext[field].text();
        field++;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which attribute values should be disallowed on the same team
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherIncompatibleResponsesDialog::gatherIncompatibleResponsesDialog(const int attribute, const DataOptions &dataOptions,
                                                                     const QList< QPair<int,int> > &currIncompats, QWidget *parent)
    :QDialog (parent)
{
    numPossibleValues = dataOptions.attributeQuestionResponses[attribute].size() + 1;
    incompatibleResponses = currIncompats;

    //Set up window with a grid layout
    setWindowTitle(tr("Select incompatible responses for Attribute ") + QString::number(attribute + 1));

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    attributeDescription = new QLabel(this);
    attributeDescription->setText("<html><br>" + dataOptions.attributeQuestionText.at(attribute) +
                         "<hr>" + tr("Prevent students with these responses from being placed on the same team:") + "<br></html>");
    attributeDescription->setWordWrap(true);
    theGrid->addWidget(attributeDescription, 0, 0, 1, -1);

    // a checkbox and a label for each response values
    enableValue = new QCheckBox[numPossibleValues];
    responses = new QPushButton[numPossibleValues];
    for(int response = 0; response < numPossibleValues; response++)
    {
        theGrid->addWidget(&enableValue[response], response + 1, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

        if(response == numPossibleValues - 1)
        {
            responses[response].setText(tr("-. value not set/unknown"));
        }
        else if(dataOptions.attributeIsOrdered[attribute])
        {
            // show reponse with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions.attributeQuestionResponses[attribute].at(response));
            responses[response].setText(match.captured(1) + match.captured(2));
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            responses[response].setText((response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))) +
                                         ". " + dataOptions.attributeQuestionResponses[attribute].at(response));
        }
        responses[response].setFlat(true);
        responses[response].setStyleSheet("Text-align:left");
        connect(&responses[response], &QPushButton::clicked, &enableValue[response], &QCheckBox::toggle);
        theGrid->addWidget(&responses[response], response + 1, 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
    }
    theGrid->setColumnStretch(1, 1);    // set second column as the one to grow

    //button to add the currently checked values as incompatible pairs
    addValuesButton = new QPushButton(this);
    addValuesButton->setText(tr("Add these\nincompatible\nvalue pairs"));
    connect(addValuesButton, &QPushButton::clicked, this, &gatherIncompatibleResponsesDialog::addValues);
    theGrid->addWidget(addValuesButton, numPossibleValues + 2, 0, 1, -1);

    //explanatory text of which response pairs will be considered incompatible
    explanation = new QLabel(this);
    explanation->clear();
    theGrid->addWidget(explanation, numPossibleValues + 3, 0, 1, -1);
    theGrid->setRowStretch(numPossibleValues + 3, 1);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numPossibleValues + 4, 20);
    resetValuesButton = new QPushButton(this);
    resetValuesButton->setText(tr("&Clear all\nincompatible\nvalues"));
    theGrid->addWidget(resetValuesButton, numPossibleValues + 5, 0, 1, 1);
    connect(resetValuesButton, &QPushButton::clicked, this, &gatherIncompatibleResponsesDialog::clearAllValues);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numPossibleValues + 5, 1, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateExplanation();
    adjustSize();
}


gatherIncompatibleResponsesDialog::~gatherIncompatibleResponsesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] enableValue;
    delete [] responses;
}


void gatherIncompatibleResponsesDialog::updateExplanation()
{
    if(incompatibleResponses.isEmpty())
    {
        explanation->setText("<html><hr><br><b>" + tr("No response values are incompatible.") + "<br></b></html>");
    }
    else
    {
        QString explanationText;
        for(const QPair<int, int> &pair : qAsConst(incompatibleResponses))
        {
            explanationText += tr("Students with response ") + responses[(pair.first)-1].text().split('.').at(0) +
                    tr(" will not be teamed with students with response ") + responses[(pair.second)-1].text().split('.').at(0) + ".<br>";
        }
        // remove all html tags, replace "-" with "not set/unknown"
        explanationText.remove("<html>").replace("-", tr("not set/unknown"));
        explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
    }
}


void gatherIncompatibleResponsesDialog::addValues()
{
    // create pairs for every combination of checked values
    for(int response1 = 0; response1 < numPossibleValues - 1; response1++)
    {
        for(int response2 = response1 + 1; response2 < numPossibleValues; response2++)
        {
            if( enableValue[response1].isChecked() && enableValue[response2].isChecked() &&
                !incompatibleResponses.contains(QPair<int,int>(response1+1, response2+1)) &&
                !incompatibleResponses.contains(QPair<int,int>(response2+1, response1+1)) )
            {
                incompatibleResponses << QPair<int,int>(response1+1, response2+1);
            }
        }
    }
    updateExplanation();

    // reset checkboxes
    for(int response = 0; response < numPossibleValues; response++)
    {
        enableValue[response].setChecked(false);
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

gatherURMResponsesDialog::gatherURMResponsesDialog(const DataOptions &dataOptions, const QStringList &currURMResponsesConsideredUR, QWidget *parent)
    :QDialog (parent)
{
    URMResponsesConsideredUR = currURMResponsesConsideredUR;

    //Set up window with a grid layout
    setWindowTitle(tr("Select underrepresented race/ethnicity responses"));

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    explanation = new QLabel(this);
    explanation->setText(tr("<html>Students gave the following responses when asked about their racial/ethnic/cultural identity."
                            "Which of these should be considered underrepresented?<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);

    // a checkbox and a label for each response values
    enableValue = new QCheckBox[dataOptions.URMResponses.size()];
    responses = new QPushButton[dataOptions.URMResponses.size()];
    for(int response = 0; response < dataOptions.URMResponses.size(); response++)
    {
        enableValue[response].setChecked(URMResponsesConsideredUR.contains(dataOptions.URMResponses.at(response)));
        theGrid->addWidget(&enableValue[response], (response/4) + 1, 2*(response%4), 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
        connect(&enableValue[response], &QCheckBox::stateChanged, this, [&, response](int state){
                                                                                 if(state == Qt::Checked)
                                                                                   {URMResponsesConsideredUR << dataOptions.URMResponses.at(response);}
                                                                                 else
                                                                                   {URMResponsesConsideredUR.removeAll(dataOptions.URMResponses.at(response));}
                                                                                 });
        responses[response].setText(dataOptions.URMResponses.at(response));
        responses[response].setFlat(true);
        responses[response].setStyleSheet("Text-align:left");
        connect(&responses[response], &QPushButton::clicked, &enableValue[response], &QCheckBox::toggle);
        theGrid->addWidget(&responses[response], (response/4) + 1, 2*(response%4) + 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
    }
    theGrid->setColumnStretch(1, 1);    // set second column as the one to grow

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(dataOptions.URMResponses.size() + 2, 20);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, dataOptions.URMResponses.size() + 3, 1, -1, -1);
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

progressDialog::progressDialog(const QString &text, QtCharts::QChartView *chart, QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Optimizing teams..."));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
    setSizeGripEnabled(true);
    theGrid = new QGridLayout(this);

    statusText = new QLabel(this);
    QFont defFont("Oxygen Mono");
    statusText->setFont(defFont);
    statusText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(statusText, 0, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);

    explanationIcon = new QLabel(this);
    explanationIcon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QMovie *movie = new QMovie(":/icons/loading.gif");
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

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(line, 2, 0, 1, -1);

    if(chart != nullptr)
    {
        theGrid->addWidget(chart, 5, 0, 1, -1);
        chart->hide();
        graphShown = false;

        showStatsButton = new QPushButton(QIcon(":/icons/down_arrow.png"), "Show progress", this);
        showStatsButton->setIconSize(QSize(20, 20));
        showStatsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(showStatsButton, &QPushButton::clicked, this, [this, chart] {statsButtonPushed(chart);});
        theGrid->addWidget(showStatsButton, 3, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
        theGrid->setColumnStretch(1,1);
    }

    onlyStopManually = new QCheckBox("Continue optimizing\nuntil I manually stop it.", this);
    onlyStopManually->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(onlyStopManually, 3, 2, 1, 1, Qt::AlignRight | Qt::AlignVCenter);

    stopHere = new QPushButton(QIcon(":/icons/stop.png"), "Stop", this);
    stopHere->setIconSize(QSize(30, 30));
    stopHere->setToolTip(tr("Stop the optimization process immediately and show the best set of teams found so far."));
    stopHere->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(stopHere, &QPushButton::clicked, this, [this] {emit letsStop();});
    theGrid->addWidget(stopHere, 3, 3, 1, -1, Qt::AlignRight | Qt::AlignVCenter);

    countdownToClose = new QTimer(this);
    setText(text);
    adjustSize();
}

void progressDialog::setText(const QString &text, int generation, float score, bool autostopInProgress)
{
    QString explanation = "<html>" + tr("Generation ") + QString::number(generation) + " - " + tr("Top Score = ") + QString::number(score) +
                              "<br><span style=\"color:" + (autostopInProgress? "green" : "black") + ";\">" + text + "<br>";
    if(autostopInProgress && !onlyStopManually->isChecked())
    {
        explanation += tr("Optimization will stop in ") + QString::number(secsLeftToClose) + tr(" seconds.");
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
        secsLeftToClose = 15;
        return;
    }

    secsLeftToClose--;
    explanationText->setText(explanationText->text().replace(QRegularExpression(tr("stop in ") + "\\d*"), tr("stop in ") +  QString::number(std::max(0, secsLeftToClose))));
    if(secsLeftToClose == 0)
    {
        stopHere->animateClick();
    }
}

void progressDialog::statsButtonPushed(QtCharts::QChartView *chart)
{
    graphShown = !graphShown;

    int height;
    QIcon icon;
    QString butText;
    if(graphShown)
    {
        chart->show();
        height = 400;
        icon = QIcon(":/icons/up_arrow.png");
        butText = "Hide progress";
    }
    else
    {
        chart->hide();
        height = 0;
        icon = QIcon(":/icons/down_arrow.png");
        butText = "Show progress";
    }
    int chartRow, x;
    theGrid->getItemPosition(theGrid->indexOf(chart), &chartRow, &x, &x, &x);
    theGrid->setRowMinimumHeight(chartRow, height);
    showStatsButton->setIcon(icon);
    showStatsButton->setText(butText);
    adjustSize();
}

progressDialog::~progressDialog()
{
    countdownToClose->stop();
}
