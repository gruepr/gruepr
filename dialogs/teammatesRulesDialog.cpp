#include "teammatesRulesDialog.h"
#include "ui_teammatesRulesDialog.h"
#include "csvfile.h"
#include "gruepr.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "dialogs/findMatchingNameDialog.h"
#include <QCompleter>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QStringListModel>
#include <QTimer>

TeammatesRulesDialog::TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions,
                                           const QString &sectionname, const QStringList &currTeamSets, TypeOfTeammates typeOfTeammates,
                                           int initialNumberGiven, gruepr *parent) :
    QDialog(parent),
    ui(new Ui::TeammatesRulesDialog),
    m_type(typeOfTeammates),
    m_typeText(typeToString(typeOfTeammates)),
    numStudents(incomingStudents.size()),
    grueprParent(parent)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Select Students to ") + m_typeText);
    setSizeGripEnabled(true);
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);

    //copy data into local versions, including full database of students
    sectionName = sectionname;
    teamSets = currTeamSets;
    students = incomingStudents;
    requestsInSurvey = !dataOptions.prefTeammatesField.empty(); /// CHECK! WAS IS prefnonteammates FOR NEGATIVEREQUESTS???
    std::sort(students.begin(), students.end(), [](const StudentRecord &i, const StudentRecord &j)
                                                {return (i.lastname+i.firstname) < (j.lastname+j.firstname);});

    ui->scrollArea->setStyleSheet(QString("QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; border: 1px solid black;}") +
                                  SCROLLBARSTYLE);
    headerWidget = new QWidget(this);
    headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(0);
    headerWidget->setLayout(headerLayout);

    tableWidget = new QTableWidget(this);

    ui->scrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    auto *scrollAreaLayout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidget->layout());
    scrollAreaLayout->addWidget(headerWidget);
    tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tableWidget->horizontalHeader()->setVisible(false);
    scrollAreaLayout->addWidget(tableWidget);
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    scrollAreaLayout->setSpacing(0);
    ui->scrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->scrollArea->setWidgetResizable(true);

    ui->valuesFrame->setStyleSheet(BLUEFRAME);

    // UI elements to show only when type is groupTogether
    const bool isGroupTogether = (m_type == TypeOfTeammates::groupTogether);
    ui->numRequestsGrantedExplanation->setVisible(isGroupTogether);
    ui->numRequestsGrantedExplanation->setStyleSheet(LABEL12PTSTYLE);
    ui->numRequestsGrantedSpinBox->setVisible(isGroupTogether);
    if (isGroupTogether) {
        ui->numRequestsGrantedSpinBox->setStyleSheet(SPINBOXSTYLE);
        ui->numRequestsGrantedSpinBox->setValue(initialNumberGiven == REQUESTED_TEAMMATES_ALL? 0 : initialNumberGiven);
        connect(ui->numRequestsGrantedSpinBox, &QSpinBox::valueChanged, this, [this](int v) {
            if(v == 0) {
                numberGroupTogethersGiven = REQUESTED_TEAMMATES_ALL;
            }
            else {
                numberGroupTogethersGiven = v;
            }
        });
    }

    ui->loadButton->setStyleSheet(SMALLTOOLBUTTONSTYLEINVERTED);
    auto *loadMenu = new QMenu(this);
    const QFont font("DM Sans");
    if(requestsInSurvey) {
        auto *loadFromSurvey = new QAction("from student preferences", this);
        loadFromSurvey->setFont(font);
        loadFromSurvey->setIcon(QIcon(":/icons_new/list_file.png"));
        connect(loadFromSurvey, &QAction::triggered, this, [this](){loadStudentPrefs();});
        loadMenu->addAction(loadFromSurvey);
    }
    if(!teamSets.isEmpty()) {
        auto *loadFromTeamset = new QAction("from existing set of teams", this);
        loadFromTeamset->setFont(font);
        loadFromTeamset->setIcon(QIcon(":/icons_new/similar.png"));
        connect(loadFromTeamset, &QAction::triggered, this, [this](){loadExistingTeamset();});
        loadMenu->addAction(loadFromTeamset);
    }
    auto *loadFromCSV = new QAction("from CSV file", this);
    loadFromCSV->setFont(font);
    loadFromCSV->setIcon(QIcon(":/icons_new/upload_file.png"));
    connect(loadFromCSV, &QAction::triggered, this, [this](){loadCSVFile();});
    auto *loadFromgruepr = new QAction("from gruepr spreadsheet file", this);
    loadFromgruepr->setFont(font);
    loadFromgruepr->setIcon(QIcon(":/icons_new/icon.svg"));
    connect(loadFromgruepr, &QAction::triggered, this, [this](){loadSpreadsheetFile();});
    loadMenu->addAction(loadFromgruepr);
    loadMenu->addAction(loadFromCSV);
    ui->loadButton->setMenu(loadMenu);

    ui->clearButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(ui->clearButton, &QPushButton::clicked, this, [this](){clearValues();});

    tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tableWidget->setStyleSheet("QTableView{gridline-color: lightGray; background-color: " TRANSPARENT "; border: none; "
                               "font-size: 12pt; font-family: 'DM Sans';}"
                               "QTableWidget:item {border-right: 1px solid lightGray; color: black;}" + QString(SCROLLBARSTYLE));
    tableWidget->horizontalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                                                   "border-bottom: none; background-color:" DEEPWATERHEX "; "
                                                   "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}"
                                                   "QHeaderView::section{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                                                   "border-bottom: none; background-color:" DEEPWATERHEX "; "
                                                   "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}");
    tableWidget->verticalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                 "background-color:" DEEPWATERHEX "; "
                                                 "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}"
                                                 "QHeaderView::section{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                 "background-color:" DEEPWATERHEX "; "
                                                 "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
    //below is stupid way needed to get text in the top-left corner cell
    tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    topLeftTableHeaderButton = tableWidget->findChild<QAbstractButton *>();
    if (topLeftTableHeaderButton != nullptr) {
        topLeftTableHeaderButton->setStyleSheet("background-color: " DEEPWATERHEX "; color: white; border: none;");
        auto *lay = new QVBoxLayout(topLeftTableHeaderButton);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        lay->setAlignment(Qt::AlignCenter);
        auto *label = new QLabel(tr("Student"), this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("QLabel {font-size: 12pt; font-family: 'DM Sans'; color: white;}");
        //label->setContentsMargins(2, 2, 2, 2);
        lay->addWidget(label);
    }
    
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshDisplay(0, 0);
    initializeTableHeaders("", true);
}

TeammatesRulesDialog::~TeammatesRulesDialog()
{
    delete ui;
}

void clearLayout(QHBoxLayout *layout)
{
    if (!layout) {
        return; // Safety check
    }

    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater(); // Delete the widget safely
        }
        delete item; // Remove the layout item
    }
}

void TeammatesRulesDialog::showToast(QWidget *parent, const QString &message, int duration) {
    // Create label for the toast message
    auto *toast = new QLabel(parent);
    toast->setText(message);
    toast->setStyleSheet("background-color: rgba(0, 0, 0, 180); color: white; padding: 10px; border-radius: 5px; font-size: 10pt;");
    toast->setAlignment(Qt::AlignCenter);

    // Position it at the bottom
    toast->adjustSize();
    const int x = (this->width()) / 2;  // Center horizontally
    const int y = parent->height() - toast->height() - 20; // Bottom with some margin
    toast->move(x, y);
    toast->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->show();

    // Auto-hide after 'duration' milliseconds
    QTimer::singleShot(duration, toast, &QLabel::close);
}


void TeammatesRulesDialog::refreshDisplay(int verticalScrollPos, int horizontalScrollPos, QString searchBarText)
{
    tableWidget->clear();

    int column = 0;
    if(requestsInSurvey) {
        tableWidget->setColumnCount(2);
        tableWidget->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("Preferences\nfrom Survey")));
        QFont italicized(tableWidget->font());
        italicized.setItalic(true);
        tableWidget->horizontalHeaderItem(column)->setFont(italicized);
        column++;
    }
    else {
        tableWidget->setColumnCount(1);
    }
    tableWidget->setHorizontalHeaderItem(column, new QTableWidgetItem(m_typeText + "\n" + tr("Student #1")));
    tableWidget->setRowCount(0);
    teammatesSpecified = false;     // assume no teammates specified until we find one

    //include a tool tip when pressed or etc that student they can add only when they have added prev student or include instructions in the beginning, which they can press X and hide)
    // the tradeoff is that they know they can add fields and etc, less decluttered
    QList<StudentRecord *> baseStudents, filteredStudents;
    QList<long long> allIDs;

    for (auto &student : students) {
        if (((sectionName == "") || (sectionName == student.section)) && !student.deleted) {
            allIDs << student.ID;
            if ((student.firstname + " " + student.lastname).contains(searchBarText, Qt::CaseInsensitive)) {
                filteredStudents << &student;
            }
            baseStudents << &student;
        }
    }
    std::sort(filteredStudents.begin(), filteredStudents.end(), [](const StudentRecord *A, const StudentRecord *B)
              { return (A->lastname + A->firstname) < (B->lastname + B->firstname); });

    int row = 0;
    for (auto *filteredStudent : std::as_const(filteredStudents)) {
        bool atLeastOneTeammate = false;
        column = requestsInSurvey ? 1 : 0;

        tableWidget->setRowCount(row + 1);
        tableWidget->setVerticalHeaderItem(row, new QTableWidgetItem(
                                                    filteredStudent->firstname + "  " + filteredStudent->lastname));

        if (requestsInSurvey) {
            auto *stuPrefText = new QLabel(this);
            stuPrefText->setStyleSheet("QLabel {font-size: 10pt; font-family: 'DM Sans'; font-style: italic; color: black;}");
            stuPrefText->setText(m_type == TypeOfTeammates::splitApart
                                     ? filteredStudent->prefNonTeammates
                                     : filteredStudent->prefTeammates);
            tableWidget->setCellWidget(row, 0, stuPrefText);
        }

        for (const auto studentBID : std::as_const(allIDs)) {
            bool printStudent = false;
            if (m_type == TypeOfTeammates::groupTogether) {
                printStudent = filteredStudent->groupTogether.contains(studentBID);
            }
            else {
                printStudent = filteredStudent->splitApart.contains(studentBID);
            }

            if (printStudent) {
                atLeastOneTeammate = true;
                teammatesSpecified = true;

                StudentRecord *studentB = nullptr;
                for (auto &student : students) {
                    if (student.ID == studentBID && !student.deleted) { studentB = &student; break; }
                }
                if (studentB == nullptr) {
                    continue;
                }

                if (tableWidget->columnCount() < column + 1) {
                    tableWidget->setColumnCount(column + 1);
                    tableWidget->setHorizontalHeaderItem(column, new QTableWidgetItem(
                                                                     m_typeText + "\n" + tr("Teammate #") +
                                                                     QString::number(column + (requestsInSurvey ? 0 : 1))));
                }

                auto *box = new QHBoxLayout;
                auto *label = new QLabel(studentB->firstname + "  " + studentB->lastname, this);
                label->setStyleSheet("QLabel {font-size: 10pt; font-family: 'DM Sans'; color: black;}");
                auto *remover = new QPushButton(QIcon(":/icons_new/trashButton.png"), "", this);
                remover->setFlat(true);
                remover->setIconSize(ICONSIZE);

                connect(remover, &QPushButton::clicked, this,
                        [this, filteredStudent, studentB, searchBarText] {
                            const int vPos = tableWidget->verticalScrollBar()->value();
                            const int hPos = tableWidget->horizontalScrollBar()->value();
                            if (m_type == TypeOfTeammates::groupTogether)  {
                                filteredStudent->groupTogether.remove(studentB->ID);
                                studentB->groupTogether.remove(filteredStudent->ID);
                            }
                            else {
                                filteredStudent->splitApart.remove(studentB->ID);
                                studentB->splitApart.remove(filteredStudent->ID);
                            }
                            refreshDisplay(vPos, hPos, searchBarText);
                            initializeTableHeaders(searchBarText);
                        });

                box->addWidget(label);
                box->addWidget(remover, 0, Qt::AlignLeft);
                box->setSpacing(0);
                auto *widg = new QWidget(this);
                widg->setLayout(box);
                widg->setProperty("studentName", label->text());
                tableWidget->setCellWidget(row, column, widg);
                column++;
            }
        }

        if (atLeastOneTeammate) {
            ui->clearButton->setEnabled(true);
        }

        // Final column: line edit for adding a new teammate
        if (tableWidget->columnCount() < column + 1) {
            tableWidget->setColumnCount(column + 1);
            tableWidget->setHorizontalHeaderItem(column, new QTableWidgetItem(
                                                             m_typeText + "\n" + tr("Teammate #") +
                                                             QString::number(column + (requestsInSurvey ? 0 : 1))));
        }

        auto *cellWidget = new QWidget(this);
        auto *box = new QHBoxLayout;
        auto *lineEdit = new QLineEdit(this);
        lineEdit->setPlaceholderText(tr("Enter a student.."));
        lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: black;}");

        QMap<QString, StudentRecord*> nameToRecord;
        QStringList studentNames;
        for (auto *const student : std::as_const(baseStudents)) {
            const QString fullName = student->firstname + " " + student->lastname;
            studentNames.append(fullName);
            nameToRecord[fullName] = student;
        }
        auto *model = new QStringListModel(studentNames, this);
        auto *completer = new QCompleter(model, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        lineEdit->setCompleter(completer);

        auto *confirmButton = new QPushButton(this);
        confirmButton->setIcon(QIcon(":/icons_new/Checkmark.png"));
        confirmButton->setVisible(false);
        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
                confirmButton, [confirmButton]() { confirmButton->setVisible(true); });

        connect(confirmButton, &QPushButton::clicked, this,
                [this, lineEdit, filteredStudent, nameToRecord, searchBarText]() {
                    const QString newText = lineEdit->text();
                    if (!nameToRecord.contains(newText)) {
                        showToast(this, tr("The student name does not exist, please double check your input."), 2000);
                        lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: darkred;}");
                        return;
                    }
                    StudentRecord *paired = nameToRecord[newText];
                    if (paired->ID == filteredStudent->ID) {
                        showToast(this, tr("Cannot pair a student with themselves."));
                        return;
                    }
                    if (m_type == TypeOfTeammates::groupTogether) {
                        filteredStudent->groupTogether.insert(paired->ID);
                        paired->groupTogether.insert(filteredStudent->ID);
                    }
                    else {
                        filteredStudent->splitApart.insert(paired->ID);
                        paired->splitApart.insert(filteredStudent->ID);
                    }
                    const int vPos = tableWidget->verticalScrollBar()->value();
                    const int hPos = tableWidget->horizontalScrollBar()->value();
                    refreshDisplay(vPos, hPos, searchBarText);
                    initializeTableHeaders(searchBarText);
                });

        box->addWidget(lineEdit);
        box->addWidget(confirmButton);
        cellWidget->setLayout(box);
        tableWidget->setCellWidget(row, column, cellWidget);
        row++;
    }

    tableWidget->resizeColumnsToContents();
    tableWidget->resizeRowsToContents();
    tableWidget->verticalScrollBar()->setValue(verticalScrollPos);
    tableWidget->horizontalScrollBar()->setValue(horizontalScrollPos);
}

void TeammatesRulesDialog::initializeTableHeaders(QString searchBarText, bool initializeStatus)
{
    clearLayout(headerLayout);

    const int vheaderWidth = tableWidget->verticalHeader()->sizeHint().width();
    if (initializeStatus) {
        initialWidthStudentHeader = vheaderWidth;
    }

    auto *topLeftWidget = new QWidget(this);
    topLeftWidget->setStyleSheet(
        "QWidget{border-right: 1px solid lightGray; background-color:" DEEPWATERHEX "; "
        "font-family: 'DM Sans'; font-size: 12pt; color: white; padding:2px;}");
    auto *topLeftLayout = new QVBoxLayout(topLeftWidget);
    topLeftLayout->setSpacing(2);

    auto *studentLabel = new QLabel(tr("Student"), this);
    studentLabel->setStyleSheet(
        "QLabel{border: none; background-color:" DEEPWATERHEX "; "
        "font-family: 'DM Sans'; font-size: 12pt; color: white;}");

    auto *searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText(tr("Filter by name"));
    searchBar->setText(searchBarText);
    searchBar->setStyleSheet(
        "QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: black; "
        "background-color: white; border: 1px solid lightGray; border-radius: 5px;}");
    connect(searchBar, &QLineEdit::textChanged, this,
            [this, searchBar]() { refreshDisplay(0, 0, searchBar->text()); });

    headerWidget->setFixedHeight(searchBar->sizeHint().height() + 35);
    topLeftWidget->setFixedWidth(initialWidthStudentHeader);
    topLeftWidget->setFixedHeight(searchBar->sizeHint().height() + 35);

    topLeftLayout->addWidget(studentLabel);
    topLeftLayout->addWidget(searchBar);
    headerLayout->addWidget(topLeftWidget, Qt::AlignCenter);

    for (int col = 0; col < tableWidget->columnCount(); ++col) {
        auto *colLabel = new QLabel(tableWidget->horizontalHeaderItem(col)->text(), this);
        colLabel->setStyleSheet(
            "QLabel{border-right: 1px solid lightGray; background-color:" DEEPWATERHEX "; "
            "font-family: 'DM Sans'; font-size: 12pt; color: white;}");
        colLabel->setFixedWidth(tableWidget->columnWidth(col));
        colLabel->setFixedHeight(searchBar->sizeHint().height() + 35);
        headerLayout->addWidget(colLabel, Qt::AlignCenter);
    }
    auto *spacer = new QLabel(this);
    spacer->setStyleSheet("QLabel {background-color: " DEEPWATERHEX ";}");
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    headerLayout->addWidget(spacer);
}

void TeammatesRulesDialog::clearValues(bool verify)
{
    if (verify) {
        const bool ok = grueprGlobal::warningMessage(this, "gruepr",
                                                     tr("This will remove all values in the table.\nAre you sure you want to continue?"),
                                                     tr("Yes"), tr("No"));
        if (!ok) {
            return;
        }
    }
    for (auto &student : students) {
        if ((sectionName == "") || (sectionName == student.section)) {
            for (int i = 0; i < numStudents; i++) {
                if (m_type == TypeOfTeammates::groupTogether) {
                    student.groupTogether.remove(i);
                }
                else {
                    student.splitApart.remove(i);
                }
            }
        }
    }
    ui->clearButton->setEnabled(false);
    refreshDisplay(0, 0);
}

bool TeammatesRulesDialog::loadCSVFile()
{
    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::Operation::read, tr("Open CSV File of Teammates"), "", tr("Comma-Separated Value File"))) {
        return false;
    }

    // Read the header row and first data row to make sure file format is correct.
    bool formattedCorrectly = true;
    int numFields = 0;
    if(csvFile.readHeader()) {
        numFields = int(csvFile.headerValues.size());
    }
    if(numFields < 2) {     // should be basename, name1, name2, name3, ..., nameN
        formattedCorrectly = false;
    }
    else {
        if((csvFile.headerValues.at(0).toLower() != tr("basename")) || (!csvFile.headerValues.at(1).toLower().startsWith(tr("name")))) {
            formattedCorrectly = false;
        }
        csvFile.readDataRow();
        if(csvFile.fieldValues.size() < numFields) {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        csvFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique base names into basenames and other names in the row into corresponding teammates list
    QStringList basenames;
    QList<QStringList> teammates;
    csvFile.readHeader();
    while(csvFile.readDataRow()) {
        const int pos = int(basenames.indexOf(csvFile.fieldValues.at(0).trimmed())); // get index of this name

        if(pos == -1) { // basename is not yet found in basenames list
            basenames << csvFile.fieldValues.at(0).trimmed();
            teammates.append(QStringList());
            for(int i = 1; i < numFields; i++) {
                const QString teammate = csvFile.fieldValues.at(i).trimmed();
                if(!teammate.isEmpty()) {
                    teammates.last() << teammate;
                }
            }
        }
        else {
            grueprGlobal::errorMessage(this, tr("File error."), tr("This file has an error in its format:\n"
                                                             "The same name appears more than once in the first column."));
            csvFile.close();
            return false;
        }
    }
    csvFile.close();

    // Now we have list of basenames and corresponding lists of teammates by name
    // Need to convert names to IDs and then add each teammate to the basename

    // First prepend the basenames to each list of teammates
    for(int basestudent = 0; basestudent < basenames.size(); basestudent++) {
        teammates[basestudent].prepend(basenames.at(basestudent));
    }

    QList<long long> IDs;
    for(int basename = 0; basename < basenames.size(); basename++) {
        IDs.clear();
        for(const auto &searchStudent : teammates.at(basename)) {   // searchStudent is the name we're looking for
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) &&
                   (searchStudent.compare(students[knownStudent].firstname + " " + students[knownStudent].lastname, Qt::CaseInsensitive) != 0)) {
                knownStudent++;
            }

            if(knownStudent != numStudents) {
                // Exact match found
                IDs << students[knownStudent].ID;
            }
            else {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(students, searchStudent, this);
                if(choiceWindow->exec() == QDialog::Accepted) {
                    IDs << choiceWindow->currSurveyID;
                }
                delete choiceWindow;
            }
        }

        // find the baseStudent
        int index = 0;
        StudentRecord *baseStudent = nullptr, *student2 = nullptr;
        while((students.at(index).ID != IDs[0]) && (index < numStudents)) {
            index++;
        }
        if(index < numStudents) {
            baseStudent = &students[index];
        }
        else {
            continue;
        }

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a groupTogether / SplitApart pairing
        for(int ID2 = 1; ID2 < IDs.size(); ID2++) {
            if(IDs[0] != IDs[ID2]) {
                // find the student with ID2
                index = 0;
                while((students.at(index).ID != IDs[ID2]) && (index < numStudents)) {
                    index++;
                }
                if(index < numStudents) {
                    student2 = &students[index];
                }
                else {
                    continue;
                }

                //we have at least one specified teammate pair!
                if(m_type == TypeOfTeammates::groupTogether) {
                    baseStudent->groupTogether << IDs[ID2];
                    student2->groupTogether << IDs[0];
                }
                else {
                    baseStudent->splitApart << IDs[ID2];
                    student2->splitApart << IDs[0];
                }
            }
        }
    }

    refreshDisplay(0, 0);
    return true;
}

bool TeammatesRulesDialog::loadStudentPrefs()
{
    // Need to convert names to IDs and then add all to the preferences
    QList<long long> IDs;
    for(int basestudent = 0; basestudent < numStudents; basestudent++) {
        if((sectionName == "") || (sectionName == students[basestudent].section)) {
            QStringList prefs;
            if(m_type == TypeOfTeammates::splitApart) {
                prefs = students[basestudent].prefNonTeammates.split('\n');
            }
            else {
                prefs = students[basestudent].prefTeammates.split('\n');
            }
            prefs.removeAll("");
            prefs.prepend(students[basestudent].firstname + " " + students[basestudent].lastname);

            IDs.clear();
            IDs.reserve(prefs.size());
            for(int searchStudent = 0; searchStudent < prefs.size(); searchStudent++) {   // searchStudent is the name we're looking for
                int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
                while((knownStudent < numStudents) &&
                       (prefs.at(searchStudent).compare((students[knownStudent].firstname + " " + students[knownStudent].lastname), Qt::CaseInsensitive) != 0)) {
                    knownStudent++;
                }

                if(knownStudent != numStudents) {
                    // Exact match found
                    IDs << students[knownStudent].ID;
                }
                else {
                    // No exact match, so list possible matches sorted by Levenshtein distance
                    auto *choiceWindow = new findMatchingNameDialog(students, prefs.at(searchStudent), this, prefs.at(0));
                    if(choiceWindow->exec() == QDialog::Accepted) {
                        IDs << choiceWindow->currSurveyID;
                    }
                    delete choiceWindow;
                }

                // find the baseStudent
                int index = 0;
                StudentRecord *baseStudent = nullptr, *student2 = nullptr;
                while((students.at(index).ID != IDs[0]) && (index < numStudents)) {
                    index++;
                }
                if(index < numStudents) {
                    baseStudent = &students[index];
                }
                else {
                    continue;
                }

                //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a groupTogether / SplitApart pairing
                for(int ID2 = 1; ID2 < IDs.size(); ID2++) {
                    if(IDs[0] != IDs[ID2]) {
                        // find the student with ID2
                        index = 0;
                        while((students.at(index).ID != IDs[ID2]) && (index < numStudents)) {
                            index++;
                        }
                        if(index < numStudents) {
                            student2 = &students[index];
                        }
                        else {
                            continue;
                        }

                        //we have at least one specified teammate pair!
                        if(m_type == TypeOfTeammates::groupTogether) {
                            baseStudent->groupTogether << IDs[ID2];
                            student2->groupTogether << IDs[0];
                        }
                        else {
                            baseStudent->splitApart << IDs[ID2];
                            student2->splitApart << IDs[0];
                        }
                    }
                }
            }
        }
    }

    refreshDisplay(0, 0);
    return true;
}

bool TeammatesRulesDialog::loadSpreadsheetFile()
{
    CsvFile spreadsheetFile(CsvFile::Delimiter::tab);
    if(!spreadsheetFile.open(this, CsvFile::Operation::read, tr("Open Spreadsheet File of Previous Teammates"), "", tr("Spreadsheet File"))) {
        return false;
    }

    // Read the header row and make sure file format is correct. If so, read next line to make sure it has data
    bool formattedCorrectly = true;
    int numFields = 0;
    if(spreadsheetFile.readHeader()) {
        numFields = int(spreadsheetFile.headerValues.size());
    }
    if(numFields < 4) {     // should be section, team, name, email
        formattedCorrectly = false;
    }
    else {
        if((spreadsheetFile.headerValues.at(0).toLower() != tr("section")) || (spreadsheetFile.headerValues.at(1).toLower() != tr("team"))
            || (spreadsheetFile.headerValues.at(2).toLower() != tr("name")) || (spreadsheetFile.headerValues.at(3).toLower() != tr("email"))) {
            formattedCorrectly = false;
        }
        spreadsheetFile.readDataRow();
        if(spreadsheetFile.fieldValues.size() < 4) {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        spreadsheetFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique team strings into teams and new/matching names into corresponding teammates list
    QStringList teamnames;
    QList<QStringList> teammateLists;
    spreadsheetFile.readHeader();
    while(spreadsheetFile.readDataRow()) {
        const int pos = int(teamnames.indexOf(spreadsheetFile.fieldValues.at(1).trimmed())); // get index of this team

        if(pos == -1) {     // team is not yet found in teams list
            teamnames << spreadsheetFile.fieldValues.at(1).trimmed();
            teammateLists.append(QStringList(spreadsheetFile.fieldValues.at(2).trimmed()));
        }
        else {
            teammateLists[pos].append(spreadsheetFile.fieldValues.at(2).trimmed());
        }
    }
    spreadsheetFile.close();

    // Now we have list of teams and corresponding lists of teammates by name
    // Need to convert names to IDs and then work through all teammate pairings
    QList<long long> IDs;
    for(const auto &teammateList : std::as_const(teammateLists)) {
        IDs.clear();
        IDs.reserve(teammateList.size());
        for(const auto &searchStudent : teammateList) {     // searchStudent is the name we're looking for
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) &&
                   (searchStudent.compare(students[knownStudent].firstname + " " + students[knownStudent].lastname, Qt::CaseInsensitive) != 0)) {
                knownStudent++;
            }

            if(knownStudent != numStudents) {
                // Exact match found
                IDs << students[knownStudent].ID;
            }
            else {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(students, searchStudent, this);
                if(choiceWindow->exec() == QDialog::Accepted) {
                    IDs << choiceWindow->currSurveyID;
                }
                delete choiceWindow;
            }
        }

        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        StudentRecord *student1 = nullptr, *student2 = nullptr;
        for(int ID1 = 0; ID1 < IDs.size(); ID1++) {
            // find the student with ID1
            int index = 0;
            while((students.at(index).ID != IDs[ID1]) && (index < numStudents)) {
                index++;
            }
            if(index < numStudents) {
                student1 = &students[index];
            }
            else {
                continue;
            }

            for(int ID2 = ID1+1; ID2 < IDs.size(); ID2++) {
                if(IDs[ID1] != IDs[ID2]) {
                    // find the student with ID2
                    index = 0;
                    while((students.at(index).ID != IDs[ID2]) && (index < numStudents)) {
                        index++;
                    }
                    if(index < numStudents) {
                        student2 = &students[index];
                    }
                    else {
                        continue;
                    }

                    //we have at least one required/prevented teammate pair!
                    if(m_type == TypeOfTeammates::groupTogether) {
                        student1->groupTogether << IDs[ID2];
                        student2->groupTogether << IDs[ID1];
                    }
                    else {
                        student1->splitApart << IDs[ID2];
                        student2->splitApart << IDs[ID1];
                    }
                }
            }
        }
    }

    refreshDisplay(0, 0);
    return true;
}

bool TeammatesRulesDialog::loadExistingTeamset()
{
    // choose which existing teamset
    QString teamSet;
    if(teamSets.count() == 1) {
        teamSet = teamSets.constFirst();
    }
    else {
        auto *win = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        win->setWindowTitle(tr("Which teamset to load?"));
        win->setSizeGripEnabled(true);
        auto *layout = new QVBoxLayout(win);
        auto *teamsetChooser = new StyledComboBox(win);
        teamsetChooser->addItems(teamSets);
        layout->addWidget(teamsetChooser);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, win);
        buttons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
        connect(buttons, &QDialogButtonBox::accepted, win, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, win, &QDialog::reject);
        layout->addWidget(buttons);

        const int result = win->exec();
        win->deleteLater();
        if(result == QDialog::Accepted) {
            teamSet = teamsetChooser->currentText();
        }
        else {
            return false;
        }
    }

    const auto teamIDLists = grueprParent->getTeamSetData(teamSet);
    if(teamIDLists.isEmpty()) {
        return false;
    }

    for(const auto &teamIDs : teamIDLists) {
        for(int i = 0; i < teamIDs.size(); i++) {
            // find student with this ID
            int index1 = 0;
            while((index1 < numStudents) && (students.at(index1).ID != teamIDs[i])) {
                index1++;
            }
            if(index1 == numStudents) {
                continue;
            }

            for(int j = i + 1; j < teamIDs.size(); j++) {
                if(teamIDs[i] != teamIDs[j]) {
                    int index2 = 0;
                    while((index2 < numStudents) && (students.at(index2).ID != teamIDs[j])) {
                        index2++;
                    }
                    if(index2 == numStudents) {
                        continue;
                    }

                    if(m_type == TypeOfTeammates::groupTogether) {
                        students[index1].groupTogether << teamIDs[j];
                        students[index2].groupTogether << teamIDs[i];
                    }
                    else {
                        students[index1].splitApart << teamIDs[j];
                        students[index2].splitApart << teamIDs[i];
                    }
                }
            }
        }
    }

    refreshDisplay(0, 0);
    return true;
}
