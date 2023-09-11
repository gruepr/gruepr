#include "teammatesRulesDialog.h"
#include "ui_TeammatesRulesDialog.h"
#include "csvfile.h"
#include "gruepr_globals.h"
#include "dialogs/findMatchingNameDialog.h"
#include <QMenu>
#include <QMessageBox>

TeammatesRulesDialog::TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions, const TeamingOptions &teamingOptions,
                                           const QString &sectionname, const QStringList &currTeamSets, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TeammatesRulesDialog),
    numStudents(incomingStudents.size())
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
    ui->tabWidget->setStyleSheet(QString(TABWIDGETSTYLE).replace("QTabBar::tab {background-color: white;", "QTabBar::tab {background-color: " TRANSPARENT ";") + LABELSTYLE);

    auto scrollAreas = {ui->requiredScrollArea, ui->preventedScrollArea, ui->requestedScrollArea};
    for(auto &scrollArea : scrollAreas) {
        scrollArea->setStyleSheet(QString() + "QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; border: 1px solid black;}" + SCROLLBARSTYLE);
    }

    auto scrollAreaWidgets = {ui->requiredScrollAreaWidget, ui->preventedScrollAreaWidget, ui->requestedScrollAreaWidget};
    for(auto &scrollAreaWidget : scrollAreaWidgets) {
        scrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    }

    auto frames = {ui->required_addFrame, ui->required_valuesFrame, ui->prevented_addFrame, ui->prevented_valuesFrame, ui->requested_addFrame, ui->requested_valuesFrame};
    for(auto &frame : frames) {
        frame->setStyleSheet("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; border: 1px solid; border-color: " AQUAHEX ";}");
    }

    auto addLabels = {ui->required_explanationLabel, ui->prevented_explanationLabel, ui->requested_explanationLabel};
    for(auto &addLabel : addLabels) {
        addLabel->setStyleSheet(LABELSTYLE);
    }

    auto addTeammateButtons = {ui->required_addTeammatePushButton, ui->prevented_addTeammatePushButton, ui->requested_addTeammatePushButton};
    for(auto &addTeammateButton : addTeammateButtons) {
        addTeammateButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
        connect(addTeammateButton, &QPushButton::clicked, this, [this](){addTeammateSelector(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    }

    auto addSetButtons = {ui->required_addSetPushButton, ui->prevented_addSetPushButton, ui->requested_addSetPushButton};
    for(auto &addSetButton : addSetButtons) {
        addSetButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENT);
        connect(addSetButton, &QPushButton::clicked, this, [this](){addOneTeammateSet(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    }

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

    auto loadButtons = {ui->required_loadButton, ui->prevented_loadButton, ui->requested_loadButton};
    QFont font("DM Sans");
    for(auto &loadButton : loadButtons) {
        loadButton->setStyleSheet(SMALLTOOLBUTTONSTYLEINVERTED);
        auto *loadMenu = new QMenu(this);
        if( (((loadButton == ui->required_loadButton) || (loadButton == ui->requested_loadButton)) && positiverequestsInSurvey) ||
             ((loadButton == ui->prevented_loadButton) && negativerequestsInSurvey) ) {
            auto *loadFromSurvey = new QAction("from student preferences", this);
            loadFromSurvey->setFont(font);
            loadFromSurvey->setIcon(QIcon(":/icons_new/list_file.png"));
            connect(loadFromSurvey, &QAction::triggered, this, [this](){loadStudentPrefs(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
            loadMenu->addAction(loadFromSurvey);
        }
        auto *loadFromCSV = new QAction("from CSV file", this);
        loadFromCSV->setFont(font);
        loadFromCSV->setIcon(QIcon(":/icons_new/upload_file.png"));
        connect(loadFromCSV, &QAction::triggered, this, [this](){loadCSVFile(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
        auto *loadFromgruepr = new QAction("from gruepr spreadsheet file", this);
        loadFromgruepr->setFont(font);
        loadFromgruepr->setIcon(QIcon(":/icons_new/icon.svg"));
        connect(loadFromgruepr, &QAction::triggered, this, [this](){loadSpreadsheetFile(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
        loadMenu->addAction(loadFromgruepr);
        loadMenu->addAction(loadFromCSV);
        loadButton->setMenu(loadMenu);
    }

    auto saveButtons = {ui->required_saveButton, ui->prevented_saveButton, ui->requested_saveButton};
    for(auto saveButton : saveButtons) {
        saveButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        connect(saveButton, &QPushButton::clicked, this, [this](){saveCSVFile(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    }

    auto clearButtons = {ui->required_clearButton, ui->prevented_clearButton, ui->requested_clearButton};
    for(auto clearButton : clearButtons) {
        clearButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        connect(clearButton, &QPushButton::clicked, this, [this](){clearValues(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    }

    auto tableWidgets = {ui->required_tableWidget, ui->prevented_tableWidget, ui->requested_tableWidget};
    for(auto &tableWidget : tableWidgets) {
        tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->setStyleSheet("QTableView{gridline-color: lightGray; background-color: " TRANSPARENT "; border: none; font-size: 12pt; font-family: 'DM Sans';}"
                                        "QTableWidget:item {border-right: 1px solid lightGray; color: black;}" + QString(SCROLLBARSTYLE));
        tableWidget->horizontalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: 1px solid lightGray; border-bottom: none;"
                                                                    "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}"
                                          "QHeaderView::section{border-top: none; border-left: none; border-right: 1px solid lightGray; border-bottom: none;"
                                                                "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}");
        tableWidget->verticalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                                  "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}"
                                        "QHeaderView::section{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                              "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
        //below is stupid way needed to get text in the top-left corner cell
        auto *button = tableWidget->findChild<QAbstractButton *>();
        if (button) {
            button->setStyleSheet("background-color: " DEEPWATERHEX "; color: white; border: none;");
            auto *lay = new QVBoxLayout(button);
            lay->setContentsMargins(0, 0, 0, 0);
            auto *label = new QLabel(tr("Student"));
            label->setAlignment(Qt::AlignCenter);
            label->setStyleSheet("QLabel {font-size: 12pt; font-family: 'DM Sans'; color: white;}");
            label->setContentsMargins(2, 2, 2, 2);
            lay->addWidget(label);
        }
    }

    ui->line->setStyleSheet("border-color: " AQUAHEX);
    ui->line->setFixedHeight(1);

    ui->requested_numRequestsExplanation->setStyleSheet(LABELSTYLE);
    ui->requested_numRequestsSpinBox->setStyleSheet(SPINBOXSTYLE);
    ui->requested_numRequestsSpinBox->setValue(teamingOptions.numberRequestedTeammatesGiven);
    connect(ui->requested_numRequestsSpinBox, &QSpinBox::valueChanged, this, [this](int newVal){numberRequestedTeammatesGiven = newVal;});

    clearAllValuesButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    clearAllValuesButton->setText(tr("Clear all rules"));
    clearAllValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    clearAllValuesButton->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(clearAllValuesButton, &QPushButton::clicked, this, &TeammatesRulesDialog::clearAllValues);
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
    QPushButton *clearButton;
    bool requestsInSurvey;
    bool *teammatesSpecified;
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("Required");
        table = ui->required_tableWidget;
        clearButton = ui->required_clearButton;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &required_teammatesSpecified;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("Prevented");
        table = ui->prevented_tableWidget;
        clearButton = ui->prevented_clearButton;
        requestsInSurvey = negativerequestsInSurvey;
        teammatesSpecified = &prevented_teammatesSpecified;
    }
    else {
        typeText = tr("Requested");
        table = ui->requested_tableWidget;
        clearButton = ui->requested_clearButton;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &requested_teammatesSpecified;
    }

    table->clear();


    int column = 0;
    if(requestsInSurvey) {
        table->setColumnCount(2);
        table->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("Preferences\nfrom Survey")));
        ui->required_tableWidget->ensurePolished();
        QFont italicized(ui->required_tableWidget->font());
        italicized.setItalic(true);
        table->horizontalHeaderItem(column)->setFont(italicized);
        column++;
    }
    else {
        table->setColumnCount(1);
    }
    table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #1")));
    table->setRowCount(0);
    *teammatesSpecified = false;     // assume no teammates specified until we find one

    QList<StudentRecord *> baseStudents;
    for(auto &student : students) {
        if((sectionName == "") || (sectionName == student.section)) {
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
        table->setVerticalHeaderItem(row, new QTableWidgetItem(baseStudent->firstname + "  " + baseStudent->lastname)); // using two spaces so that can split later

        if(requestsInSurvey) {
            auto *stuPrefText = new QLabel(this);
            stuPrefText->setStyleSheet("QLabel {font-size: 10pt; font-family: 'DM Sans'; font-style: italic; color: black;}");
            if(typeOfTeammates == TypeOfTeammates::prevented) {
                stuPrefText->setText(baseStudent->prefNonTeammates);
            }
            else {
                stuPrefText->setText(baseStudent->prefTeammates);
            }
            table->setCellWidget(row, column, stuPrefText);
            column++;
        }

        bool printStudent;
        for(int studentBID = 0; studentBID < MAX_IDS; studentBID++) {
            if(typeOfTeammates == TypeOfTeammates::required) {
                printStudent = baseStudent->requiredWith[studentBID];
            }
            else if(typeOfTeammates == TypeOfTeammates::prevented) {
                printStudent = baseStudent->preventedWith[studentBID];
            }
            else {
                printStudent = baseStudent->requestedWith[studentBID];
            }
            if(printStudent) {
                atLeastOneTeammate = true;
                *teammatesSpecified = true;

                // find studentB from their ID
                StudentRecord *studentB = nullptr;
                int index = 0;
                while((students[index].ID != studentBID) && (index < numStudents)) {
                    index++;
                }
                if(index < numStudents) {
                    studentB = &students[index];
                }
                else {
                    continue;
                }
                if(table->columnCount() < column+1) {
                    table->setColumnCount(column+1);
                    table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #") +
                                                                                QString::number(column + (requestsInSurvey? 0:1))));
                }
                auto *box = new QHBoxLayout;
                auto *label = new QLabel(studentB->firstname + "  " + studentB->lastname, this);        // using two spaces so can split later
                label->setStyleSheet("QLabel {font-size: 10pt; font-family: 'DM Sans'; color: black;}");
                auto *remover = new QPushButton(QIcon(":/icons_new/trashButton.png"), "", this);
                remover->setFlat(true);
                remover->setIconSize(ICONSIZE);
                if(typeOfTeammates == TypeOfTeammates::required) {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->requiredWith[studentB->ID] = false;
                                                             studentB->requiredWith[baseStudent->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::required);});
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->preventedWith[studentB->ID] = false;
                                                             studentB->preventedWith[baseStudent->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::prevented);});
                }
                else {
                    connect(remover, &QPushButton::clicked, this, [this, baseStudent, studentB]
                                                            {baseStudent->requestedWith[studentB->ID] = false;
                                                             refreshDisplay(TypeOfTeammates::requested);});
                }
                box->addWidget(label);
                box->addWidget(remover, 0, Qt::AlignLeft);
                box->setSpacing(0);
                auto *widg = new QWidget(this);
                widg->setLayout(box);
                widg->setProperty("studentName", label->text());    // used when saving to csv file
                table->setCellWidget(row, column, widg);
                column++;
            }
        }
        if(atLeastOneTeammate) {
            clearButton->setEnabled(true);
            clearAllValuesButton->setEnabled(true);
        }
        else {
            table->setItem(row, column, new QTableWidgetItem("--"));
        }
        row++;
    }
    table->resizeColumnsToContents();
    table->resizeRowsToContents();

}

void TeammatesRulesDialog::addTeammateSelector(TypeOfTeammates typeOfTeammates)
{
    QList<QComboBox *> *comboBoxes;
    QVBoxLayout *addFrameLayout;
    if(typeOfTeammates == TypeOfTeammates::required) {
        comboBoxes = &possibleRequiredTeammates;
        addFrameLayout = qobject_cast<QVBoxLayout *>(ui->required_addFrame->layout());
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        comboBoxes = &possiblePreventedTeammates;
        addFrameLayout = qobject_cast<QVBoxLayout *>(ui->prevented_addFrame->layout());
    }
    else {
        comboBoxes = &possibleRequestedTeammates;
        addFrameLayout = qobject_cast<QVBoxLayout *>(ui->requested_addFrame->layout());
    }

    auto *studentcombobox = new QComboBox(this);
    studentcombobox->setStyleSheet(COMBOBOXSTYLE);
    for(const auto &student : students) {
        studentcombobox->setPlaceholderText(comboBoxes->first()->placeholderText());
        if((sectionName == "") || (sectionName == student.section)) {
            studentcombobox->addItem(student.lastname + ", " + student.firstname, student.ID);
        }
    }

    addFrameLayout->insertWidget(addFrameLayout->indexOf(comboBoxes->last()) + 1, studentcombobox);
    *comboBoxes << studentcombobox;
}

void TeammatesRulesDialog::addOneTeammateSet(TypeOfTeammates typeOfTeammates)
{
    //Gather all selected IDs from the comboboxes
    QList<QComboBox *> comboBoxes;
    if(typeOfTeammates == TypeOfTeammates::required) {
        comboBoxes = possibleRequiredTeammates;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        comboBoxes = possiblePreventedTeammates;
    }
    else {
        comboBoxes = possibleRequestedTeammates;
    }
    QList<int> IDs;
    for(const auto &comboBox : comboBoxes) {
        //If a student is selected in this combobox, load their ID into an array that holds all the selections
        if(comboBox->currentIndex() != -1) {
            IDs << comboBox->currentData().toInt();
        }

        //Reset combobox
        comboBox->setCurrentIndex(-1);
    }

    if(typeOfTeammates != TypeOfTeammates::requested) {
        StudentRecord *student1 = nullptr, *student2 = nullptr;
        //Work through all pairings in the set to enable as a required or prevented pairing in both studentRecords
        for(int ID1 = 0; ID1 < IDs.size(); ID1++) {
            // find the student with ID1
            int index = 0;
            while((students[index].ID != IDs[ID1]) && (index < numStudents)) {
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
                    while((students[index].ID != IDs[ID2]) && (index < numStudents)) {
                        index++;
                    }
                    if(index < numStudents) {
                        student2 = &students[index];
                    }
                    else {
                        continue;
                    }

                    //we have at least one required/prevented teammate pair!
                    if(typeOfTeammates == TypeOfTeammates::required) {
                        student1->requiredWith[IDs[ID2]] = true;
                        student2->requiredWith[IDs[ID1]] = true;
                    }
                    else {
                        student1->preventedWith[IDs[ID2]] = true;
                        student2->preventedWith[IDs[ID1]] = true;
                    }
                }
            }
        }
    }
    else {
        int baseStudentID = ui->requested_studentSelectComboBox->currentData().toInt();
        // find the student with this ID
        StudentRecord *baseStudent = nullptr;
        int index = 0;
        while((students[index].ID != baseStudentID) && (index < numStudents)) {
            index++;
        }
        if(index < numStudents) {
            baseStudent = &students[index];

            for(int ID1 = 0; ID1 < IDs.size(); ID1++) {
                if(baseStudentID != IDs[ID1]) {
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

void TeammatesRulesDialog::clearAllValues()
{
    auto *areYouSure = new QMessageBox(this);
    areYouSure->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    areYouSure->setStyleSheet(LABELSTYLE);
    areYouSure->setIcon(QMessageBox::Warning);
    areYouSure->setWindowTitle("gruepr");
    areYouSure->setText(tr("This will remove all teammates rules listed in all of the tables.\nAre you sure you want to continue?"));
    areYouSure->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    areYouSure->setDefaultButton(QMessageBox::No);
    areYouSure->button(QMessageBox::Yes)->setStyleSheet(SMALLBUTTONSTYLE);
    areYouSure->button(QMessageBox::No)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    int resp = areYouSure->exec();
    areYouSure->deleteLater();
    if(resp == QMessageBox::No) {
        return;
    }

    clearValues(TypeOfTeammates::required, false);
    clearValues(TypeOfTeammates::prevented, false);
    clearValues(TypeOfTeammates::requested, false);
}

void TeammatesRulesDialog::clearValues(TypeOfTeammates typeOfTeammates, bool verify)
{
    QString typeText;
    QPushButton *clearButton;
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("required");
        clearButton = ui->required_clearButton;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("prevented");
        clearButton = ui->prevented_clearButton;
    }
    else {
        typeText = tr("requested");
        clearButton = ui->requested_clearButton;
    }

    if(verify) {
        auto *areYouSure = new QMessageBox(this);
        areYouSure->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        areYouSure->setStyleSheet(LABELSTYLE);
        areYouSure->setIcon(QMessageBox::Warning);
        areYouSure->setWindowTitle("gruepr");
        areYouSure->setText(tr("This will remove all rules listed in the ") + typeText + tr(" teammates table.\nAre you sure you want to continue?"));
        areYouSure->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        areYouSure->setDefaultButton(QMessageBox::No);
        areYouSure->button(QMessageBox::Yes)->setStyleSheet(SMALLBUTTONSTYLE);
        areYouSure->button(QMessageBox::No)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        int resp = areYouSure->exec();
        areYouSure->deleteLater();
        if(resp == QMessageBox::No) {
            return;
        }
    }

    for(auto &student : students) {
        if((sectionName == "") || (sectionName == student.section)) {
            for(int index2 = 0; index2 < numStudents; index2++) {
                if(typeOfTeammates == TypeOfTeammates::required) {
                    student.requiredWith[index2] = false;
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    student.preventedWith[index2] = false;
                }
                else {
                    student.requestedWith[index2] = false;
                }
            }
        }
    }

    clearButton->setEnabled(false);
    if(!ui->required_clearButton->isEnabled() && !ui->requested_clearButton->isEnabled() && !ui->prevented_clearButton->isEnabled()) {
        clearAllValuesButton->setEnabled(false);
    }
    refreshDisplay(typeOfTeammates);
}

bool TeammatesRulesDialog::saveCSVFile(TypeOfTeammates typeOfTeammates)
{
    QString typeText;
    QTableWidget *table;
    bool requestsInSurvey;
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("Required");
        table = ui->required_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("Prevented");
        table = ui->prevented_tableWidget;
        requestsInSurvey = negativerequestsInSurvey;
    }
    else {
        typeText = tr("Requested");
        table = ui->requested_tableWidget;
        requestsInSurvey = positiverequestsInSurvey;
    }

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::write, tr("Save File of ") + typeText + tr(" Teammates"), "", tr("Comma-Separated Value File"))) {
        return false;
    }

    // write header
    csvFile.headerValues << tr("basename");
    int firstDataCol = requestsInSurvey? 1 : 0;
    int lastDataCol = table->columnCount() - firstDataCol;
    for(int i = 1; i <= lastDataCol; i++) {
        csvFile.headerValues << tr("name") + QString::number(i);
    }
    if(!csvFile.writeHeader()) {
        QMessageBox::critical(this, tr("No Files Saved"), tr("This data was not saved.\nThere was an issue writing the file to disk."));
        return false;
    }

    // write data rows
    QStringList firstnameLastname;
    for(int basename = 0; basename < table->rowCount(); basename++) {
        csvFile.fieldValues.clear();
        firstnameLastname = table->verticalHeaderItem(basename)->text().split("  ");
        csvFile.fieldValues << firstnameLastname.at(0).trimmed() + " " + firstnameLastname.at(1).trimmed();
        for(int teammate = firstDataCol; teammate <= lastDataCol; teammate++) {
            QWidget *teammateItem(table->cellWidget(basename,teammate));
            if (teammateItem != nullptr) {
                firstnameLastname = teammateItem->property("studentName").toString().split("  ");
                csvFile.fieldValues << firstnameLastname.at(0).trimmed() + " " + firstnameLastname.at(1).trimmed();
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
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("Required");
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("Prevented");
    }
    else {
        typeText = tr("Requested");
    }

    CsvFile csvFile;
    if(!csvFile.open(this, CsvFile::read, tr("Open CSV File of Teammates"), "", tr("Comma-Separated Value File"))) {
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
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        csvFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique base names into basenames and other names in the row into corresponding teammates list
    QStringList basenames;
    QList<QStringList> teammates;
    csvFile.readHeader();
    while(csvFile.readDataRow()) {
        int pos = int(basenames.indexOf(csvFile.fieldValues.at(0).trimmed())); // get index of this name

        if(pos == -1) { // basename is not yet found in basenames list
            basenames << csvFile.fieldValues.at(0).trimmed();
            teammates.append(QStringList());
            for(int i = 1; i < numFields; i++) {
                QString teammate = csvFile.fieldValues.at(i).trimmed();
                if(!teammate.isEmpty()) {
                    teammates.last() << teammate;
                }
            }
        }
        else {
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
    for(int basestudent = 0; basestudent < basenames.size(); basestudent++) {
        teammates[basestudent].prepend(basenames.at(basestudent));
    }

    QList<int> IDs;
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
                auto *choiceWindow = new findMatchingNameDialog(numStudents, students, searchStudent, this);
                if(choiceWindow->exec() == QDialog::Accepted) {
                    IDs << choiceWindow->currSurveyID;
                }
                delete choiceWindow;
            }
        }

        // find the baseStudent
        int index = 0;
        StudentRecord *baseStudent = nullptr, *student2 = nullptr;
        while((students[index].ID != IDs[0]) && (index < numStudents)) {
            index++;
        }
        if(index < numStudents) {
            baseStudent = &students[index];
        }
        else {
            continue;
        }

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
        for(int ID2 = 1; ID2 < IDs.size(); ID2++) {
            if(IDs[0] != IDs[ID2]) {
                // find the student with ID2
                index = 0;
                while((students[index].ID != IDs[ID2]) && (index < numStudents)) {
                    index++;
                }
                if(index < numStudents) {
                    student2 = &students[index];
                }
                else {
                    continue;
                }

                //we have at least one specified teammate pair!
                if(typeOfTeammates == TypeOfTeammates::required) {
                    baseStudent->requiredWith[IDs[ID2]] = true;
                    student2->requiredWith[IDs[0]] = true;
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    baseStudent->preventedWith[IDs[ID2]] = true;
                    student2->preventedWith[IDs[0]] = true;
                }
                else {  //whatType == requested
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
    // Need to convert names to IDs and then add all to the preferences
    QList<int> IDs;
    for(int basestudent = 0; basestudent < numStudents; basestudent++) {
        if((sectionName == "") || (sectionName == students[basestudent].section)) {
            QStringList prefs;
            if(typeOfTeammates == TypeOfTeammates::prevented) {
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
                    auto *choiceWindow = new findMatchingNameDialog(numStudents, students, prefs.at(searchStudent), this, prefs.at(0));
                    if(choiceWindow->exec() == QDialog::Accepted) {
                        IDs << choiceWindow->currSurveyID;
                    }
                    delete choiceWindow;
                }

                // find the baseStudent
                int index = 0;
                StudentRecord *baseStudent = nullptr, *student2 = nullptr;
                while((students[index].ID != IDs[0]) && (index < numStudents)) {
                    index++;
                }
                if(index < numStudents) {
                    baseStudent = &students[index];
                }
                else {
                    continue;
                }

                //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
                for(int ID2 = 1; ID2 < IDs.size(); ID2++) {
                    if(IDs[0] != IDs[ID2]) {
                        // find the student with ID2
                        index = 0;
                        while((students[index].ID != IDs[ID2]) && (index < numStudents)) {
                            index++;
                        }
                        if(index < numStudents) {
                            student2 = &students[index];
                        }
                        else {
                            continue;
                        }

                        //we have at least one specified teammate pair!
                        if(typeOfTeammates == TypeOfTeammates::required) {
                            baseStudent->requiredWith[IDs[ID2]] = true;
                            student2->requiredWith[IDs[0]] = true;
                        }
                        else if(typeOfTeammates == TypeOfTeammates::prevented) {
                            baseStudent->preventedWith[IDs[ID2]] = true;
                            student2->preventedWith[IDs[0]] = true;
                        }
                        else {  //whatType == requested
                            baseStudent->requestedWith[IDs[ID2]] = true;
                        }
                    }
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates);
    return true;
}

bool TeammatesRulesDialog::loadSpreadsheetFile(TypeOfTeammates typeOfTeammates)
{
    CsvFile spreadsheetFile(CsvFile::tab);
    if(!spreadsheetFile.open(this, CsvFile::read, tr("Open Spreadsheet File of Previous Teammates"), "", tr("Spreadsheet File"))) {
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
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        spreadsheetFile.close();
        return false;
    }

    // Having read the header row and determined that the file seems correctly formatted, read the remaining rows until there's an empty one
    // Process each row by loading unique team strings into teams and new/matching names into corresponding teammates list
    QStringList teamnames;
    QList<QStringList> teammateLists;
    spreadsheetFile.readHeader();
    while(spreadsheetFile.readDataRow()) {
        int pos = int(teamnames.indexOf(spreadsheetFile.fieldValues.at(1).trimmed())); // get index of this team

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
    QList<int> IDs;
    for(const auto &teammateList : qAsConst(teammateLists)) {
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
                auto *choiceWindow = new findMatchingNameDialog(numStudents, students, searchStudent, this);
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
            while((students[index].ID != IDs[ID1]) && (index < numStudents)) {
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
                    while((students[index].ID != IDs[ID2]) && (index < numStudents)) {
                        index++;
                    }
                    if(index < numStudents) {
                        student2 = &students[index];
                    }
                    else {
                        continue;
                    }

                    //we have at least one required/prevented teammate pair!
                    if(typeOfTeammates == TypeOfTeammates::required) {
                        student1->requiredWith[IDs[ID2]] = true;
                        student2->requiredWith[IDs[ID1]] = true;
                    }
                    else if(typeOfTeammates == TypeOfTeammates::prevented) {
                        student1->preventedWith[IDs[ID2]] = true;
                        student2->preventedWith[IDs[ID1]] = true;
                    }
                    else {  //whatType == requested
                        student1->requestedWith[IDs[ID2]] = true;
                        student2->requestedWith[IDs[ID1]] = true;
                    }
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates);
    return true;
}

bool TeammatesRulesDialog::loadExistingTeamset(TypeOfTeammates typeOfTeammates)
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
        auto *teamsetChooser = new QComboBox(win);
        teamsetChooser->setStyleSheet(COMBOBOXSTYLE);
        teamsetChooser->addItems(teamSets);
        layout->addWidget(teamsetChooser);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, win);
        buttons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
        connect(buttons, &QDialogButtonBox::accepted, win, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, win, &QDialog::reject);
        layout->addWidget(buttons);

        int result = win->exec();
        win->deleteLater();
        if(result == QDialog::Accepted) {
            teamSet = teamsetChooser->currentText();
        }
        else {
            return false;
        }
    }

    //INPROG - copied from other function--not functional
    /*
    // Process each row by loading unique base names into basenames and other names in the row into corresponding teammates list
    QStringList basenames;
    QList<QStringList> teammates;

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
        for(int searchStudent = 0; searchStudent < teammates.at(basename).size(); searchStudent++)  // searchStudent is the name we're looking for
        {
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) &&
                  (teammates.at(basename).at(searchStudent).compare(students[knownStudent].firstname + " " + students[knownStudent].lastname, Qt::CaseInsensitive) != 0))
            {
                knownStudent++;
            }

            if(knownStudent != numStudents)
            {
                // Exact match found
                IDs << students[knownStudent].ID;
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

        // find the baseStudent
        int index = 0;
        StudentRecord *baseStudent = nullptr, *student2 = nullptr;
        while((students[index].ID != IDs[0]) && (index < numStudents))
        {
            index++;
        }
        if(index < numStudents)
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
                while((students[index].ID != IDs[ID2]) && (index < numStudents))
                {
                    index++;
                }
                if(index < numStudents)
                {
                    student2 = &students[index];
                }
                else
                {
                    continue;
                }

                //we have at least one specified teammate pair!
                if(whatType == required)
                {
                    baseStudent->requiredWith[IDs[ID2]] = true;
                    student2->requiredWith[IDs[0]] = true;
                }
                else if(whatType == prevented)
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
*/
    refreshDisplay(typeOfTeammates);
    return true;
}
