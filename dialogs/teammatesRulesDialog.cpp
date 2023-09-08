#include "teammatesRulesDialog.h"
#include "ui_TeammatesRulesDialog.h"
#include "csvfile.h"
#include "gruepr_globals.h"
#include "dialogs/findMatchingNameDialog.h"
#include <QMenu>
#include <QMessageBox>

TeammatesRulesDialog::TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions,
                                           const QString &sectionname, const QStringList &currTeamSets, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TeammatesRulesDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Teammate rules"));
    setSizeGripEnabled(true);
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);

    //copy data into local versions, including full database of students
    sectionName = sectionname;
    teamSets = currTeamSets;
    students = incomingStudents;
    positiverequestsInSurvey = dataOptions.prefTeammatesIncluded;
    negativerequestsInSurvey = dataOptions.prefTeammatesIncluded;
    std::sort(students.begin(), students.end(), [](const StudentRecord &i, const StudentRecord &j)
                                                {return (i.lastname+i.firstname) < (j.lastname+j.firstname);});

    ui->tabWidget->tabBar()->setExpanding(true);
    ui->tabWidget->setStyleSheet(QString() + TABWIDGETSTYLE + LABELSTYLE);

    auto scrollAreas = {ui->requiredScrollArea, ui->preventedScrollArea, ui->requestedScrollArea};
    for(auto &scrollArea : scrollAreas) {
        scrollArea->setStyleSheet(QString() + "QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX ";}" + SCROLLBARSTYLE);
    }

    auto scrollAreaWidgets = {ui->requiredScrollAreaWidget, ui->preventedScrollAreaWidget, ui->requestedScrollAreaWidget};
    for(auto &scrollAreaWidget : scrollAreaWidgets) {
        scrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    }

    auto addFrames = {ui->required_addFrame, ui->prevented_addFrame, ui->requested_addFrame};
    for(auto &addFrame : addFrames) {
        addFrame->setStyleSheet("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; border: 1px solid; border-color: " AQUAHEX ";}");
    }

    auto loadButtons = {ui->required_loadButton, ui->prevented_loadButton, ui->requested_loadButton};
    for(auto &loadButton : loadButtons) {
        loadButton->setStyleSheet(SMALLTOOLBUTTONSTYLEINVERTED);
        auto *loadFromCSV = new QAction("from CSV file", this);
        loadFromCSV->setIcon(QIcon(":/icons_new/upload_file.png"));
        auto *loadFromSurvey = new QAction("from student preferences", this);
        loadFromSurvey->setIcon(QIcon(":/icons_new/list_file.png"));
        auto *loadFromgruepr = new QAction("from gruepr spreadsheet file", this);
        loadFromgruepr->setIcon(QIcon(":/icons_new/icon.svg"));
        auto *loadMenu = new QMenu(this);
        loadMenu->addAction(loadFromSurvey);
        loadMenu->addAction(loadFromgruepr);
        loadMenu->addAction(loadFromCSV);
        loadButton->setMenu(loadMenu);
    }

    auto addLabels = {ui->required_explanationLabel, ui->prevented_explanationLabel, ui->requested_explanationLabel};
    for(auto &addLabel : addLabels) {
        addLabel->setStyleSheet(LABELSTYLE);
    }

    auto addTeammateButtons = {ui->required_addTeammatePushButton, ui->prevented_addTeammatePushButton, ui->requested_addTeammatePushButton};
    for(auto &addTeammateButton : addTeammateButtons) {
        addTeammateButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    }

    auto addSetButtons = {ui->required_addSetPushButton, ui->prevented_addSetPushButton, ui->requested_addSetPushButton};
    for(auto &addSetButton : addSetButtons) {
        addSetButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENT);
    }
    connect(ui->required_addSetPushButton, &QPushButton::clicked, this, [this](){addOneTeammateSet(TypeOfTeammates::required);});
    connect(ui->prevented_addSetPushButton, &QPushButton::clicked, this, [this](){addOneTeammateSet(TypeOfTeammates::prevented);});
    connect(ui->requested_addSetPushButton, &QPushButton::clicked, this, [this](){addOneTeammateSet(TypeOfTeammates::requested);});

    auto studentcomboboxes = {ui->required_studentSelectComboBox, ui->required_studentSelectComboBox2, ui->prevented_studentSelectComboBox, ui->prevented_studentSelectComboBox2,
                              ui->requested_studentSelectComboBox, ui->requested_teammateSelectComboBox};
    for(auto &studentcombobox : studentcomboboxes) {
        studentcombobox->setStyleSheet(COMBOBOXSTYLE);
        for(const auto &student : students) {
            if((sectionName == "") || (sectionName == student.section)) {
                studentcombobox->addItem(student.lastname + ", " + student.firstname, student.ID);
            }
        }
    }
    possibleRequiredTeammates = {ui->required_studentSelectComboBox, ui->required_studentSelectComboBox2};
    possiblePreventedTeammates = {ui->prevented_studentSelectComboBox, ui->prevented_studentSelectComboBox2};
    possibleRequestedTeammates = {ui->requested_teammateSelectComboBox};

    auto tableWidgets = {ui->required_tableWidget, ui->prevented_tableWidget, ui->requested_tableWidget};
    for(auto &tableWidget : tableWidgets) {
        tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: " BUBBLYHEX "; border-color: " OPENWATERHEX "; border: 1px solid black; "
                                               "font-size: 12pt; font-family: 'DM Sans'; color: black;}"
                                       "QTableView::item{border-left: 1px solid " AQUAHEX "; border-top: none; border-right: none; border-bottom: none; padding: 3px; "
                                                         "font-size: 12pt; font-family: 'DM Sans'; color: black;}"
                                       "QHeaderView::section{border-top: none; border-left: none; "
                                                            "border-right: 1px solid " AQUAHEX "; border-bottom: 1px solid " AQUAHEX "; "
                                                            "padding: 4px; font-size: 12pt; font-family: 'DM Sans'; color: black;}" +
                                       QString(SCROLLBARSTYLE));
    }

    ui->line->setStyleSheet("border-color: " AQUAHEX);
    ui->line->setFixedHeight(1);

    auto *resetValuesButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    resetValuesButton->setText(tr("Clear all rules"));
    resetValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    auto *restoreValuesButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    restoreValuesButton->setText(tr("Save to csv file"));
    restoreValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(resetValuesButton, &QPushButton::clicked, this, [this](){clearAllValues(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshDisplay(TypeOfTeammates::required);
    refreshDisplay(TypeOfTeammates::prevented);
    refreshDisplay(TypeOfTeammates::requested);
}

TeammatesRulesDialog::~TeammatesRulesDialog()
{
    delete ui;
}

void TeammatesRulesDialog::refreshDisplay(TypeOfTeammates typeOfTeammates)
{
    QString typeText;
    QTableWidget *table;
    bool requestsInSurvey;
    bool *teammatesSpecified;
    if(typeOfTeammates == TypeOfTeammates::required)
    {
        typeText = tr("Required");
        table = ui->required_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &required_teammatesSpecified;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented)
    {
        typeText = tr("Prevented");
        table = ui->prevented_tableWidget;
        requestsInSurvey = negativerequestsInSurvey;
        teammatesSpecified = &prevented_teammatesSpecified;
    }
    else
    {
        typeText = tr("Requested");
        table = ui->requested_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &requested_teammatesSpecified;
    }

    table->clear();

    ui->required_tableWidget->ensurePolished();
    QFont italicized(ui->required_tableWidget->font());
    italicized.setItalic(true);

    int column = 0;
    if(requestsInSurvey)
    {
        table->setColumnCount(2);
        auto *prefHeaderItem = new QTableWidgetItem(tr("Preferences\nfrom Survey"));
        table->setHorizontalHeaderItem(column, prefHeaderItem);
        table->horizontalHeaderItem(column)->setFont(italicized);
        column++;
    }
    else
    {
        table->setColumnCount(1);
    }
    table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #1")));
    table->setRowCount(0);
    *teammatesSpecified = false;     // assume no teammates specified until we find one

    QList<StudentRecord *> baseStudents;
    for(auto &student : students)
    {
        if((sectionName == "") || (sectionName == student.section))
        {
            baseStudents << &student;
        }
    }
    std::sort(baseStudents.begin(), baseStudents.end(), [](const StudentRecord *const A, const StudentRecord *const B)
              {return ((A->lastname+A->firstname) < (B->lastname+B->firstname));});

    int row = 0;
    for(auto *baseStudent : qAsConst(baseStudents)) {
        bool atLeastOneTeammate = false;
        column = 0;

        table->setRowCount(row+1);
        table->setVerticalHeaderItem(row, new QTableWidgetItem(baseStudent->lastname + ", " + baseStudent->firstname));

        if(requestsInSurvey)
        {
            QTableWidgetItem *stuPrefText = nullptr;
            if(typeOfTeammates == TypeOfTeammates::prevented)
            {
                stuPrefText = new QTableWidgetItem(baseStudent->prefNonTeammates);
            }
            else
            {
                stuPrefText = new QTableWidgetItem(baseStudent->prefTeammates);
            }
            table->setItem(row, column, stuPrefText);
            table->item(row, column)->setFont(italicized);
            column++;
        }

        bool printStudent;
        for(int studentBID = 0; studentBID < MAX_IDS; studentBID++)
        {
            if(typeOfTeammates == TypeOfTeammates::required)
            {
                printStudent = baseStudent->requiredWith[studentBID];
            }
            else if(typeOfTeammates == TypeOfTeammates::prevented)
            {
                printStudent = baseStudent->preventedWith[studentBID];
            }
            else
            {
                printStudent = baseStudent->requestedWith[studentBID];
            }
            if(printStudent)
            {
                atLeastOneTeammate = true;
                *teammatesSpecified = true;

                // find studentB from their ID
                StudentRecord *studentB = nullptr;
                int index = 0;
                while((students[index].ID != studentBID) && (index < students.size()))
                {
                    index++;
                }
                if(index < students.size())
                {
                    studentB = &students[index];
                }
                else
                {
                    continue;
                }
                if(table->columnCount() < column+1)
                {
                    table->setColumnCount(column+1);
                    table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #") +
                                                                                QString::number(column + (requestsInSurvey? 0:1))));
                }
                auto *box = new QHBoxLayout;
                auto *label = new QLabel(studentB->lastname + ", " + studentB->firstname);
                label->setStyleSheet("QLabel {font-size: 12pt; font-family: 'DM Sans'; color: black;}");
                auto *remover = new QPushButton(QIcon(":/icons_new/trashButton.png"), "");
                remover->setFlat(true);
                remover->setIconSize(ICONSIZE);
                if(typeOfTeammates == TypeOfTeammates::required)
                {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->requiredWith[studentB->ID] = false;
                                                             studentB->requiredWith[baseStudent->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::required);});
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented)
                {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->preventedWith[studentB->ID] = false;
                                                             studentB->preventedWith[baseStudent->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::prevented);});
                }
                else
                {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->requestedWith[studentB->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::requested);});
                }
                box->addWidget(label);
                box->addWidget(remover, 0, Qt::AlignLeft);
                box->setSpacing(0);
                auto *widg = new QWidget;
                widg->setLayout(box);
                widg->setProperty("studentName", label->text());
                table->setCellWidget(row, column, widg);
                column++;
            }
        }
        if(!atLeastOneTeammate)
        {
            table->setItem(row, column, new QTableWidgetItem("--"));
        }
        row++;
    }
    table->resizeColumnsToContents();
    table->resizeRowsToContents();

}

void TeammatesRulesDialog::addOneTeammateSet(TypeOfTeammates typeOfTeammates)
{
    //Gather all selected IDs from the comboboxes
    QList<QComboBox *> comboBoxes;
    if(typeOfTeammates == TypeOfTeammates::required)
    {
        comboBoxes = possibleRequiredTeammates;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented)
    {
        comboBoxes = possiblePreventedTeammates;
    }
    else
    {
        comboBoxes = possibleRequestedTeammates;
    }
    QList<int> IDs;
    for(const auto &comboBox : comboBoxes)
    {
        //If a student is selected in this combobox, load their ID into an array that holds all the selections
        if(comboBox->currentIndex() != -1)
        {
            IDs << comboBox->currentData().toInt();
        }

        //Reset combobox
        comboBox->setCurrentIndex(-1);
    }

    if(typeOfTeammates != TypeOfTeammates::requested)
    {
        StudentRecord *student1 = nullptr, *student2 = nullptr;
        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        for(int ID1 = 0; ID1 < IDs.size(); ID1++)
        {
            // find the student with ID1
            int index = 0;
            while((students[index].ID != IDs[ID1]) && (index < students.size()))
            {
                index++;
            }
            if(index < students.size())
            {
                student1 = &students[index];
            }
            else
            {
                continue;
            }

            for(int ID2 = ID1+1; ID2 < IDs.size(); ID2++)
            {
                if(IDs[ID1] != IDs[ID2])
                {
                    // find the student with ID2
                    index = 0;
                    while((students[index].ID != IDs[ID2]) && (index < students.size()))
                    {
                        index++;
                    }
                    if(index < students.size())
                    {
                        student2 = &students[index];
                    }
                    else
                    {
                        continue;
                    }

                    //we have at least one required/prevented teammate pair!
                    if(typeOfTeammates == TypeOfTeammates::required)
                    {
                        student1->requiredWith[IDs[ID2]] = true;
                        student2->requiredWith[IDs[ID1]] = true;
                    }
                    else
                    {
                        student1->preventedWith[IDs[ID2]] = true;
                        student2->preventedWith[IDs[ID1]] = true;
                    }
                }
            }
        }
    }
    else
    {
        int baseStudentID = ui->requested_studentSelectComboBox->currentData().toInt();
        // find the student with this ID
        StudentRecord *baseStudent = nullptr;
        int index = 0;
        while((students[index].ID != baseStudentID) && (index < students.size()))
        {
            index++;
        }
        if(index < students.size())
        {
            baseStudent = &students[index];

            for(int ID1 = 0; ID1 < IDs.size(); ID1++)
            {
                if(baseStudentID != IDs[ID1])
                {
                    //we have at least one requested teammate pair!
                    baseStudent->requestedWith[IDs[ID1]] = true;
                }
            }
        }

        //Reset combobox
        ui->requested_studentSelectComboBox->setCurrentIndex(-1);
    }

    refreshDisplay(typeOfTeammates);
}

void TeammatesRulesDialog::clearAllValues(TypeOfTeammates typeOfTeammates)
{
    QString typeText;
    if(typeOfTeammates == TypeOfTeammates::required)
    {
        typeText = tr("Required");
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented)
    {
        typeText = tr("Prevented");
    }
    else
    {
        typeText = tr("Requested");
    }

    auto *areYouSure = new QMessageBox(this);
    areYouSure->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    areYouSure->setStyleSheet(LABELSTYLE);
    areYouSure->setIcon(QMessageBox::Warning);
    areYouSure->setWindowTitle("gruepr");
    areYouSure->setText(tr("This will remove all teammates data listed in the") + "\n" + typeText + tr(" teammates table. Are you sure you want to continue?\n"));
    areYouSure->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    areYouSure->setDefaultButton(QMessageBox::No);
    areYouSure->button(QMessageBox::Yes)->setStyleSheet(SMALLBUTTONSTYLE);
    areYouSure->button(QMessageBox::No)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    int resp = areYouSure->exec();
    areYouSure->deleteLater();
    if(resp == QMessageBox::No)
    {
        return;
    }

    for(auto &student : students)
    {
        if((sectionName == "") || (sectionName == student.section))
        {
            for(int index2 = 0; index2 < students.size(); index2++)
            {
                if(typeOfTeammates == TypeOfTeammates::required)
                {
                    student.requiredWith[index2] = false;
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented)
                {
                    student.preventedWith[index2] = false;
                }
                else
                {
                    student.requestedWith[index2] = false;
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates);
}

bool TeammatesRulesDialog::saveCSVFile(TypeOfTeammates typeOfTeammates)
{
    QString typeText;
    QTableWidget *table;
    bool requestsInSurvey;
    bool *teammatesSpecified;
    if(typeOfTeammates == TypeOfTeammates::required)
    {
        typeText = tr("Required");
        table = ui->required_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &required_teammatesSpecified;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented)
    {
        typeText = tr("Prevented");
        table = ui->prevented_tableWidget;
        requestsInSurvey = negativerequestsInSurvey;
        teammatesSpecified = &prevented_teammatesSpecified;
    }
    else
    {
        typeText = tr("Requested");
        table = ui->requested_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &requested_teammatesSpecified;
    }

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::write, tr("Save File of ") + typeText + tr(" Teammates"), "", tr("Comma-Separated Value File")))
    {
        return false;
    }

    // write header
    csvFile.headerValues << tr("basename");
    int firstDataCol = requestsInSurvey? 1 : 0;
    int lastDataCol = table->columnCount() - firstDataCol;
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
    for(int basename = 0; basename < table->rowCount(); basename++)
    {
        csvFile.fieldValues.clear();
        QStringList lastnameFirstname = table->verticalHeaderItem(basename)->text().split(',');
        csvFile.fieldValues << lastnameFirstname.at(1).trimmed() + " " + lastnameFirstname.at(0).trimmed();
        for(int teammate = firstDataCol; teammate <= lastDataCol; teammate++)
        {
            QWidget *teammateItem(table->cellWidget(basename,teammate));
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

bool TeammatesRulesDialog::loadCSVFile(TypeOfTeammates typeOfTeammates)
{
    QString typeText;
    QTableWidget *table;
    bool requestsInSurvey;
    bool *teammatesSpecified;
    if(typeOfTeammates == TypeOfTeammates::required)
    {
        typeText = tr("Required");
        table = ui->required_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &required_teammatesSpecified;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented)
    {
        typeText = tr("Prevented");
        table = ui->prevented_tableWidget;
        requestsInSurvey = negativerequestsInSurvey;
        teammatesSpecified = &prevented_teammatesSpecified;
    }
    else
    {
        typeText = tr("Requested");
        table = ui->requested_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &requested_teammatesSpecified;
    }

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::read, tr("Open CSV File of Teammates"), "", tr("Comma-Separated Value File")))
    {
        return false;
    }

    // Read the header row and first data row to make sure file format is correct.
    bool formattedCorrectly = true;
    int numFields = 0;
    if(csvFile.readHeader())
    {
        numFields = int(csvFile.headerValues.size());
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
    QList<QStringList> teammates;
    csvFile.readHeader();
    while(csvFile.readDataRow())
    {
        int pos = int(basenames.indexOf(csvFile.fieldValues.at(0).trimmed())); // get index of this name

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
            QMessageBox::critical(this, tr("File error."), tr("This file has an error in its format:\n"
                                                              "The same name appears more than once in the first column."), QMessageBox::Ok);
            csvFile.close();
            return false;
        }
    }
    csvFile.close();

    // Now we have list of basenames and corresponding lists of teammates by name
    // Need to convert names to IDs and then add each teammate to the basename

    // First prepend the basenames to each list of teammates
    for(int basestudent = 0; basestudent < basenames.size(); basestudent++)
    {
        teammates[basestudent].prepend(basenames.at(basestudent));
    }

    QList<int> IDs;
    for(int basename = 0; basename < basenames.size(); basename++)
    {
        IDs.clear();
        for(const auto &searchStudent : teammates.at(basename))  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < students.size()) &&
                   (searchStudent.compare(students[knownStudent].firstname + " " + students[knownStudent].lastname, Qt::CaseInsensitive) != 0))
            {
                knownStudent++;
            }

            if(knownStudent != students.size())
            {
                // Exact match found
                IDs << students[knownStudent].ID;
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(students.size(), students, searchStudent, this);
                if(choiceWindow->exec() == QDialog::Accepted)
                {
                    IDs << choiceWindow->currSurveyID;
                }
                delete choiceWindow;
            }
        }

        // find the baseStudent
        int index = 0;
        StudentRecord *baseStudent = nullptr, *student2 = nullptr;
        while((students[index].ID != IDs[0]) && (index < students.size()))
        {
            index++;
        }
        if(index < students.size())
        {
            baseStudent = &students[index];
        }
        else
        {
            continue;
        }

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
        for(int ID2 = 1; ID2 < IDs.size(); ID2++)
        {
            if(IDs[0] != IDs[ID2])
            {
                // find the student with ID2
                index = 0;
                while((students[index].ID != IDs[ID2]) && (index < students.size()))
                {
                    index++;
                }
                if(index < students.size())
                {
                    student2 = &students[index];
                }
                else
                {
                    continue;
                }

                //we have at least one specified teammate pair!
                if(typeOfTeammates == TypeOfTeammates::required)
                {
                    baseStudent->requiredWith[IDs[ID2]] = true;
                    student2->requiredWith[IDs[0]] = true;
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented)
                {
                    baseStudent->preventedWith[IDs[ID2]] = true;
                    student2->preventedWith[IDs[0]] = true;
                }
                else    //whatType == requested
                {
                    baseStudent->requestedWith[IDs[ID2]] = true;
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates);
    return true;
}

bool TeammatesRulesDialog::loadStudentPrefs(TypeOfTeammates typeOfTeammates)
{

    return true;
}

bool TeammatesRulesDialog::loadSpreadsheetFile(TypeOfTeammates typeOfTeammates)
{

    return true;
}

bool TeammatesRulesDialog::loadExistingTeamset(TypeOfTeammates typeOfTeammates)
{

    return true;
}
