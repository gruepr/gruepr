#include "teammatesRulesDialog.h"
#include "ui_teammatesRulesDialog.h"
#include "dataFile.h"
#include "gruepr.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "dialogs/findMatchingNameDialog.h"
#include <algorithm>
#include <QCompleter>
#include <QFileDialog>
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
    auto *loadFromCSV = new QAction("from CSV/text/Excel file", this);
    loadFromCSV->setFont(font);
    loadFromCSV->setIcon(QIcon(":/icons_new/upload_file.png"));
    connect(loadFromCSV, &QAction::triggered, this, [this](){loadTeammatesFile();});
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

//////////////////
// Resolve a list of names to student IDs, prompting via findMatchingNameDialog for any that don't
// exactly match an existing student's first+last name. Names the user chooses to ignore are simply
// left out of the result, so the returned list may be shorter than names.
//////////////////
QList<long long> TeammatesRulesDialog::resolveNamesToIDs(const QStringList &names, const QString &hintName)
{
    QList<long long> IDs;
    IDs.reserve(names.size());
    for(const auto &searchStudent : names) {   // searchStudent is the name we're looking for
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
            auto *choiceWindow = new findMatchingNameDialog(students, searchStudent, this, hintName);
            if(choiceWindow->exec() == QDialog::Accepted) {
                IDs << choiceWindow->currSurveyID;
            }
            delete choiceWindow;
        }
    }
    return IDs;
}


//////////////////
// Apply a groupTogether/splitApart pairing (per m_type) across IDs -- hub-and-spoke pairs IDs[0] with
// each other ID; all-pairs pairs every ID with every other ID.
//////////////////
void TeammatesRulesDialog::pairAllStudents(const QList<long long> &IDs, bool hubAndSpoke)
{
    if(IDs.isEmpty()) {
        return;
    }

    const auto findByID = [this](long long id) -> StudentRecord* {
        int index = 0;
        while((index < numStudents) && (students.at(index).ID != id)) {
            index++;
        }
        return (index < numStudents) ? &students[index] : nullptr;
    };

    for(int ID1 = 0; ID1 < (hubAndSpoke ? 1 : IDs.size()); ID1++) {
        StudentRecord *student1 = findByID(IDs[ID1]);
        if(student1 == nullptr) {
            continue;
        }
        for(int ID2 = ID1 + 1; ID2 < IDs.size(); ID2++) {
            if(IDs[ID1] == IDs[ID2]) {
                continue;
            }
            StudentRecord *student2 = findByID(IDs[ID2]);
            if(student2 == nullptr) {
                continue;
            }

            //we have at least one specified teammate pair!
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


bool TeammatesRulesDialog::loadTeammatesFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File of Teammates"), "",
                                                            tr("Teammates File") + " (*.csv *.txt *.xlsx);;All Files (*)");
    if(fileName.isEmpty()) {
        return false;
    }

    const std::unique_ptr<DataFile> teammatesFile = DataFile::createForFile(fileName);
    if(!teammatesFile->openExistingFile(fileName)) {
        return false;
    }

    // Two supported shapes:
    //  - "basename" format: header row is "basename, name1, name2, ...", and each subsequent row names
    //    one student (column 1) plus that student's own specific teammates (remaining columns) --
    //    a hub-and-spoke pairing between the basename and each named teammate.
    //  - headerless format: no header row at all; each row is simply a full team, and every name in
    //    the row is a mutual teammate of every other name in that row (all-pairs pairing).
    if(!teammatesFile->readHeader() || teammatesFile->headerValues.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        teammatesFile->close();
        return false;
    }
    const int numFields = int(teammatesFile->headerValues.size());
    const bool basenameFormat = (numFields >= 2) &&
                                 (teammatesFile->headerValues.at(0).toLower() == tr("basename")) &&
                                 (teammatesFile->headerValues.at(1).toLower().startsWith(tr("name")));

    // Each entry is one row's set of names: for basenameFormat, element 0 is the basename (the "hub")
    // and the rest are its specific teammates; for the headerless format, it's simply every name found
    // in that row (order doesn't matter -- all of them are paired with each other).
    QList<QStringList> teammateGroups;

    if(basenameFormat) {
        QStringList basenames;   // tracked separately just to detect a basename repeated across rows

        teammatesFile->readDataRow();
        if(teammatesFile->fieldValues.size() < numFields) {
            grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
            teammatesFile->close();
            return false;
        }
        teammatesFile->readHeader();   // reset the cursor back to the first data row
        while(teammatesFile->readDataRow()) {
            const QString basename = teammatesFile->fieldValues.at(0).trimmed();
            if(basenames.contains(basename)) {
                grueprGlobal::errorMessage(this, tr("File error."), tr("This file has an error in its format:\n"
                                                                 "The same name appears more than once in the first column."));
                teammatesFile->close();
                return false;
            }
            basenames << basename;
            teammateGroups.append(QStringList(basename));
            for(int i = 1; i < numFields; i++) {
                const QString teammate = teammatesFile->fieldValues.at(i).trimmed();
                if(!teammate.isEmpty()) {
                    teammateGroups.last() << teammate;
                }
            }
        }
    }
    else {
        teammatesFile->readDataRow(DataFile::ReadLocation::beginningOfFile);   // re-include the row readHeader() consumed above
        do {
            QStringList rowNames;
            for(const auto &field : std::as_const(teammatesFile->fieldValues)) {
                const QString name = field.trimmed();
                if(!name.isEmpty()) {
                    rowNames << name;
                }
            }
            if(rowNames.size() >= 2) {     // rows with 0 or 1 names have nobody to pair up
                teammateGroups << rowNames;
            }
        } while(teammatesFile->readDataRow());
    }
    teammatesFile->close();

    if(teammateGroups.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        return false;
    }

    // Now we have, per row, a list of names. Convert names to IDs and pair them up -- hub-and-spoke
    // for the basename format, all-pairs for the headerless format.
    for(const auto &group : std::as_const(teammateGroups)) {
        pairAllStudents(resolveNamesToIDs(group), basenameFormat);
    }

    refreshDisplay(0, 0);
    return true;
}

bool TeammatesRulesDialog::loadStudentPrefs()
{
    // Need to convert names to IDs and then add all to the preferences
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
            const QString baseStudentName = students[basestudent].firstname + " " + students[basestudent].lastname;
            prefs.prepend(baseStudentName);

            pairAllStudents(resolveNamesToIDs(prefs, baseStudentName), true);
        }
    }

    refreshDisplay(0, 0);
    return true;
}

TeammatesRulesDialog::ParsedSpreadsheetTeams TeammatesRulesDialog::parseTeamsFromSpreadsheetFile(const QString &fileName)
{
    ParsedSpreadsheetTeams result;

    // Concrete format (csv/txt/xlsx) picked from the file's extension, matching what gruepr's own
    // Spreadsheet export can produce.
    const std::unique_ptr<DataFile> spreadsheetFile = DataFile::createForFile(fileName);
    if(!spreadsheetFile->openExistingFile(fileName)) {
        return result;
    }

    // Read the header row and find the columns we need by name, rather than fixed position -- tolerant
    // of a combined "Name" column (older exports) or separate "First Name"/"Last Name" columns (current
    // Spreadsheet export), and of "Section"/"Email"/other columns being absent, reordered, or additional.
    int teamCol = -1, nameCol = -1, firstNameCol = -1, lastNameCol = -1;
    if(spreadsheetFile->readHeader()) {
        for(int field = 0; field < spreadsheetFile->headerValues.size(); field++) {
            const QString header = spreadsheetFile->headerValues.at(field).trimmed().toLower();
            if(header == tr("team").toLower()) {
                teamCol = field;
            }
            else if(header == tr("name").toLower()) {
                nameCol = field;
            }
            else if(header == tr("first name").toLower()) {
                firstNameCol = field;
            }
            else if(header == tr("last name").toLower()) {
                lastNameCol = field;
            }
        }
    }

    bool formattedCorrectly = (teamCol != -1) && ((nameCol != -1) || ((firstNameCol != -1) && (lastNameCol != -1)));
    const int lastNeededCol = std::max({teamCol, nameCol, firstNameCol, lastNameCol});
    if(formattedCorrectly) {
        spreadsheetFile->readDataRow();
        if(spreadsheetFile->fieldValues.size() <= lastNeededCol) {
            formattedCorrectly = false;
        }
    }
    if(!formattedCorrectly) {
        spreadsheetFile->close();
        return result;
    }

    const auto nameFromRow = [nameCol, firstNameCol, lastNameCol](const QStringList &fields) -> QString {
        if(nameCol != -1) {
            return fields.at(nameCol).trimmed();
        }
        return (fields.at(firstNameCol).trimmed() + " " + fields.at(lastNameCol).trimmed()).trimmed();
    };

    // Having found the header columns and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique team strings into teams and new/matching names into corresponding teammates list
    QStringList &teamnames = result.teamNames;
    QList<QStringList> &teammateLists = result.teammateNames;
    spreadsheetFile->readHeader();
    while(spreadsheetFile->readDataRow()) {
        if(spreadsheetFile->fieldValues.size() <= lastNeededCol) {
            continue;   // skip any short/malformed row
        }
        const QString name = nameFromRow(spreadsheetFile->fieldValues);
        const int pos = int(teamnames.indexOf(spreadsheetFile->fieldValues.at(teamCol).trimmed())); // get index of this team

        if(pos == -1) {     // team is not yet found in teams list
            teamnames << spreadsheetFile->fieldValues.at(teamCol).trimmed();
            teammateLists.append(QStringList(name));
        }
        else {
            teammateLists[pos].append(name);
        }
    }
    spreadsheetFile->close();

    result.success = true;
    return result;
}

bool TeammatesRulesDialog::loadSpreadsheetFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Spreadsheet File of Previous Teammates"), "",
                                                          tr("Spreadsheet File (*.csv *.txt *.xlsx);;All Files (*)"));
    if(fileName.isEmpty()) {
        return false;
    }

    const ParsedSpreadsheetTeams parsed = parseTeamsFromSpreadsheetFile(fileName);
    if(!parsed.success) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        return false;
    }
    //const QStringList &teamnames = parsed.teamNames;
    const QList<QStringList> &teammateLists = parsed.teammateNames;

    // Now we have list of teams and corresponding lists of teammates by name
    // Need to convert names to IDs and then work through all teammate pairings
    for(const auto &teammateList : std::as_const(teammateLists)) {
        pairAllStudents(resolveNamesToIDs(teammateList), false);
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
