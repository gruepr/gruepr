#include "teammatesRulesDialog.h"
#include "qcompleter.h"
#include "qheaderview.h"
#include "qlineedit.h"
#include "qscrollbar.h"
#include "qstringlistmodel.h"
#include "qtimer.h"
#include "ui_teammatesRulesDialog.h"
#include "csvfile.h"
#include "gruepr_globals.h"
#include "dialogs/findMatchingNameDialog.h"
#include "studentRecord.h"
#include <QMenu>
#include <QMessageBox>

TeammatesRulesDialog::TeammatesRulesDialog(const QList<StudentRecord> &incomingStudents, const DataOptions &dataOptions, const TeamingOptions &teamingOptions,
                                           const QString &sectionname, const QStringList &currTeamSets, QWidget *parent,
                                           bool autoLoadRequired, bool autoLoadPrevented, bool autoLoadRequested, int initialTabIndex) :
    QDialog(parent),
    ui(new Ui::TeammatesRulesDialog),
    numStudents(incomingStudents.size())
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Teammate rules"));
    setSizeGripEnabled(true);
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);

    ui->tabWidget->setCurrentIndex(initialTabIndex);
    //copy data into local versions, including full database of students
    sectionName = sectionname;
    teamSets = currTeamSets;
    students = incomingStudents;
    positiverequestsInSurvey = !dataOptions.prefTeammatesField.empty();
    negativerequestsInSurvey = !dataOptions.prefTeammatesField.empty();
    std::sort(students.begin(), students.end(), [](const StudentRecord &i, const StudentRecord &j)
                                                {return (i.lastname+i.firstname) < (j.lastname+j.firstname);});

    ui->tabWidget->tabBar()->setExpanding(true);
    ui->tabWidget->setStyleSheet(QString(TABWIDGETSTYLE).replace("QTabBar::tab {background-color: white;", "QTabBar::tab {background-color: " TRANSPARENT ";") +
                                 LABEL10PTSTYLE);

    auto scrollAreas = {ui->requiredScrollArea, ui->preventedScrollArea, ui->requestedScrollArea};
    for(auto &scrollArea : scrollAreas) {
        scrollArea->setStyleSheet(QString("QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; border: 1px solid black;}") +
                                  SCROLLBARSTYLE);
    }
    headerRequiredTeammates = new QWidget(this);
    headerLayoutRequiredTeammates = new QHBoxLayout(this);
    headerLayoutRequiredTeammates->setContentsMargins(0, 0, 0, 0);
    headerLayoutRequiredTeammates->setSpacing(0);
    headerRequiredTeammates->setLayout(headerLayoutRequiredTeammates);

    headerPreventedTeammates = new QWidget(this);
    headerLayoutPreventedTeammates = new QHBoxLayout(this);
    headerLayoutPreventedTeammates->setContentsMargins(0, 0, 0, 0);
    headerLayoutPreventedTeammates->setSpacing(0);
    headerPreventedTeammates->setLayout(headerLayoutPreventedTeammates);

    headerRequestedTeammates = new QWidget(this);
    headerLayoutRequestedTeammates = new QHBoxLayout(this);
    headerLayoutRequestedTeammates->setContentsMargins(0, 0, 0, 0);
    headerLayoutRequestedTeammates->setSpacing(0);
    headerRequestedTeammates->setLayout(headerLayoutRequestedTeammates);

    //initialize required_tableWidget
    //initialize prevented_tableWidget
    //initialize requested_tableWidget

    // auto scrollAreaWidgets = {ui->requiredScrollAreaWidget, ui->preventedScrollAreaWidget, ui->requestedScrollAreaWidget};
    // for(auto &scrollAreaWidget : scrollAreaWidgets) {
    //     scrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");

    //     auto *scrollAreaLayout = qobject_cast<QVBoxLayout*>(scrollAreaWidget->layout());
    //     if (scrollAreaLayout) {
    //         // Add the headerLayout to the now-empty layout
    //         scrollAreaLayout->addLayout(headerLayoutRequiredTeammates);
    //         auto *tableWidget = new QTableWidget();
    //         tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Make table expand to fill remaining space
    //         scrollAreaLayout->addWidget(tableWidget);

    //         // Ensure the layout fills the entire scroll area widget
    //         scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    //         scrollAreaLayout->setSpacing(0);
    //         scrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //     }
    // }
    // Initialize table widgets
    required_tableWidget = new QTableWidget(this);
    prevented_tableWidget = new QTableWidget(this);
    requested_tableWidget = new QTableWidget(this);

    // Set up requiredScrollAreaWidget
    ui->requiredScrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    auto *requiredScrollAreaLayout = qobject_cast<QVBoxLayout*>(ui->requiredScrollAreaWidget->layout());
    requiredScrollAreaLayout->addWidget(headerRequiredTeammates);
    required_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    required_tableWidget->horizontalHeader()->setVisible(false);
    requiredScrollAreaLayout->addWidget(required_tableWidget);
    requiredScrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    requiredScrollAreaLayout->setSpacing(0);
    ui->requiredScrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->requiredScrollArea->setWidgetResizable(true);

    // Set up preventedScrollAreaWidget
    ui->preventedScrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    auto *preventedScrollAreaLayout = qobject_cast<QVBoxLayout*>(ui->preventedScrollAreaWidget->layout());
    preventedScrollAreaLayout->addWidget(headerPreventedTeammates);
    prevented_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    prevented_tableWidget->horizontalHeader()->setVisible(false);
    preventedScrollAreaLayout->addWidget(prevented_tableWidget);
    preventedScrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    preventedScrollAreaLayout->setSpacing(0);
    ui->preventedScrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->preventedScrollArea->setWidgetResizable(true);

    // Set up requestedScrollAreaWidget
    ui->requestedScrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    auto *requestedScrollAreaLayout = qobject_cast<QVBoxLayout*>(ui->requestedScrollAreaWidget->layout());
    requestedScrollAreaLayout->addWidget(headerRequestedTeammates);
    requested_tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    requested_tableWidget->horizontalHeader()->setVisible(false);
    requestedScrollAreaLayout->addWidget(requested_tableWidget);
    requestedScrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    requestedScrollAreaLayout->setSpacing(0);
    ui->requestedScrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->requestedScrollArea->setWidgetResizable(true);


    auto frames = {ui->required_addFrame, ui->required_valuesFrame, ui->prevented_addFrame,
                   ui->prevented_valuesFrame, ui->requested_addFrame, ui->requested_valuesFrame};
    for(auto &frame : frames) {
        frame->setStyleSheet(BLUEFRAME);
    }
    ui->required_addFrame->setVisible(false);
    ui->requested_addFrame->setVisible(false);
    ui->prevented_addFrame->setVisible(false);

    auto addLabels = {ui->required_explanationLabel, ui->prevented_explanationLabel, ui->requested_explanationLabel};
    for(auto &addLabel : addLabels) {
        addLabel->setStyleSheet(LABEL10PTSTYLE);
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

    auto studentcomboboxes = {ui->required_studentSelectComboBox, ui->required_studentSelectComboBox2,
                              ui->prevented_studentSelectComboBox, ui->prevented_studentSelectComboBox2,
                              ui->requested_studentSelectComboBox, ui->requested_teammateSelectComboBox};
    for(auto &studentcombobox : studentcomboboxes) {
        studentcombobox->setStyleSheet(COMBOBOXSTYLE);
        for(const auto &student : qAsConst(students)) {
            if(((sectionName == "") || (sectionName == student.section)) && !student.deleted) {
                studentcombobox->addItem(student.lastname + ", " + student.firstname, student.ID);
            }
        }
    }
    possibleRequiredTeammates = {ui->required_studentSelectComboBox, ui->required_studentSelectComboBox2};
    possiblePreventedTeammates = {ui->prevented_studentSelectComboBox, ui->prevented_studentSelectComboBox2};
    possibleRequestedTeammates = {ui->requested_teammateSelectComboBox};

    auto loadButtons = {ui->required_loadButton, ui->prevented_loadButton, ui->requested_loadButton};
    const QFont font("DM Sans");
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

    auto clearButtons = {ui->required_clearButton, ui->prevented_clearButton, ui->requested_clearButton};
    for(auto clearButton : clearButtons) {
        clearButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        connect(clearButton, &QPushButton::clicked, this, [this](){clearValues(static_cast<TypeOfTeammates>(ui->tabWidget->currentIndex()));});
    }

    auto tableWidgets = {required_tableWidget, prevented_tableWidget, requested_tableWidget};
    for(auto &tableWidget : tableWidgets) {
        tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        //tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
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
    }

    ui->line->setStyleSheet("border-color: " AQUAHEX);
    ui->line->setFixedHeight(1);
    
    ui->requested_numRequestsExplanation->setStyleSheet(LABEL10PTSTYLE);
    ui->requested_numRequestsSpinBox->setStyleSheet(SPINBOXSTYLE);
    ui->requested_numRequestsSpinBox->setValue(teamingOptions.numberRequestedTeammatesGiven);
    connect(ui->requested_numRequestsSpinBox, &QSpinBox::valueChanged, this, [this](int newVal){numberRequestedTeammatesGiven = newVal;});
    connect(ui->requested_numRequestsSpinBox, &QSpinBox::valueChanged, this, [this](int newVal){if(newVal > 1) {
                                                                                                   ui->requested_numRequestsSpinBox->setSuffix(tr(" teammates"));}
                                                                                                else {
                                                                                                   ui->requested_numRequestsSpinBox->setSuffix(tr(" teammate "));}});
    ui->requested_lightbulb->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->requested_lightbulb->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    ui->requested_whatsThisLabel->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    const QString helpText = tr("<html><span style=\"color: black;\">The \"requested teammate\" feature is used when you want to ensure that "
                                "students will be placed on a team with a certain number of teammates from a list. "
                                "For example, you might allow students to make up to 5 requests, and ensure that they each get "
                                "at least 1 of them as a teammate."
                                "</span></html>");
    ui->requested_lightbulb->setToolTipText(helpText);
    ui->requested_whatsThisLabel->setToolTipText(helpText);

    clearAllValuesButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    clearAllValuesButton->setText(tr("Clear all rules"));
    clearAllValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    clearAllValuesButton->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(clearAllValuesButton, &QPushButton::clicked, this, &TeammatesRulesDialog::clearAllValues);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshDisplay(TypeOfTeammates::required, 0, 0, "");
    refreshDisplay(TypeOfTeammates::prevented, 0, 0, "");
    refreshDisplay(TypeOfTeammates::requested, 0, 0, "");

    initializeTableHeaders(TypeOfTeammates::required, "", true);
    initializeTableHeaders(TypeOfTeammates::prevented, "", true);
    initializeTableHeaders(TypeOfTeammates::requested, "", true);

    // the following options are for when this window was opened by gruepr immediately after starting, when the survey contained prefteammate or prefnonteammate questions
    if(autoLoadRequired) {
        ui->tabWidget->setCurrentIndex(0);
        loadStudentPrefs(TypeOfTeammates::required);
    }
    if(autoLoadPrevented) {
        ui->tabWidget->setCurrentIndex(1);
        loadStudentPrefs(TypeOfTeammates::prevented);
    }
    if(autoLoadRequested) {
        ui->tabWidget->setCurrentIndex(2);
        loadStudentPrefs(TypeOfTeammates::requested);
    }
    if(autoLoadRequired || autoLoadPrevented || autoLoadRequested) {
        accept();
    }
}

TeammatesRulesDialog::~TeammatesRulesDialog()
{
    delete ui;
}

void clearLayout(QHBoxLayout *layout) {
    if (!layout) return; // Safety check

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
    QLabel *toast = new QLabel(parent);
    toast->setText(message);
    toast->setStyleSheet("background-color: rgba(0, 0, 0, 180);"
                         "color: white;"
                         "padding: 10px;"
                         "border-radius: 5px;"
                         "font-size: 10pt;");
    toast->setAlignment(Qt::AlignCenter);

    // Position it at the bottom
    toast->adjustSize();
    int x = (this->width()) / 2;  // Center horizontally
    int y = parent->height() - toast->height() - 20; // Bottom with some margin
    toast->move(x, y);
    toast->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->show();

    // Auto-hide after 'duration' milliseconds
    QTimer::singleShot(duration, toast, &QLabel::close);
}


void TeammatesRulesDialog::refreshDisplay(TypeOfTeammates typeOfTeammates, int verticalScrollPos, int horizontalScrollPos, QString searchBarText)
{
    QString typeText;
    QTableWidget *table;
    QPushButton *clearButton;
    bool requestsInSurvey;
    bool *teammatesSpecified;
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("Required");
        table = required_tableWidget;
        clearButton = ui->required_clearButton;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &required_teammatesSpecified;
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("Prevented");
        table = prevented_tableWidget;
        clearButton = ui->prevented_clearButton;
        requestsInSurvey = negativerequestsInSurvey;
        teammatesSpecified = &prevented_teammatesSpecified;
    }
    else {
        typeText = tr("Requested");
        table = requested_tableWidget;
        clearButton = ui->requested_clearButton;
        requestsInSurvey = positiverequestsInSurvey;
        teammatesSpecified = &requested_teammatesSpecified;
    }

    table->clear();

    int column = 0;
    if(requestsInSurvey) {
        table->setColumnCount(2);
        table->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("Preferences\nfrom Survey")));
        required_tableWidget->ensurePolished();
        QFont italicized(required_tableWidget->font());
        italicized.setItalic(true);
        table->horizontalHeaderItem(column)->setFont(italicized);
        column++;
    }
    else {
        table->setColumnCount(1);
    }

    //pass in the maximum number of teammates, render all the columns properly.
    //for each row of baseStudent, render all existing students first, then afterwards render only 1 missing lineEdit
    //render all the lineEdits (with "Enter a teammate")

    //then render the existing teammates as labels and remove the lineEdits previously

    table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #1")));
    table->setRowCount(0);
    *teammatesSpecified = false;     // assume no teammates specified until we find one
    //include a tool tip when pressed or etc that student they can add only when they have added prev student or include instructions in the beginning, which they can press X and hide)
    // the tradeoff is that they know they can add fields and etc, less decluttered
    QList<StudentRecord *> baseStudents;
    QList<StudentRecord *> filteredStudents;
    QList<long long> allIDs;

    for(auto &student : students) {
        //only add students which are in the current section being grouped
        if(((sectionName == "") || (sectionName == student.section)) && !student.deleted) {
            allIDs << student.ID;
            QString fullName = student.firstname + " " + student.lastname;
            qDebug() << searchBarText;
            if (fullName.contains(searchBarText, Qt::CaseInsensitive)) { // Case-insensitive match
                filteredStudents << (&student);
            }
            baseStudents << &student;
        }
    }

    std::sort(filteredStudents.begin(), filteredStudents.end(), [](const StudentRecord *const A, const StudentRecord *const B)
              {return ((A->lastname+A->firstname) < (B->lastname+B->firstname));});

    int row = 0;
    for(auto *filteredStudent : qAsConst(filteredStudents)) {
        bool atLeastOneTeammate = false;
        column = 0;

        table->setRowCount(row+1);
        table->setVerticalHeaderItem(row, new QTableWidgetItem(filteredStudent->firstname + "  " + filteredStudent->lastname)); // using two spaces so that can split later

        if(requestsInSurvey) {
            auto *stuPrefText = new QLabel(this);
            stuPrefText->setStyleSheet("QLabel {font-size: 10pt; font-family: 'DM Sans'; font-style: italic; color: black;}");
            if(typeOfTeammates == TypeOfTeammates::prevented) {
                stuPrefText->setText(filteredStudent->prefNonTeammates);
            }
            else {
                stuPrefText->setText(filteredStudent->prefTeammates);
            }
            table->setCellWidget(row, column, stuPrefText);
            column++;
        }

        bool printStudent;
        for(const auto studentBID : qAsConst(allIDs)) {
            if(typeOfTeammates == TypeOfTeammates::required) {
                printStudent = filteredStudent->requiredWith.contains(studentBID);
            }
            else if(typeOfTeammates == TypeOfTeammates::prevented) {
                printStudent = filteredStudent->preventedWith.contains(studentBID);
            }
            else {
                printStudent = filteredStudent->requestedWith.contains(studentBID);
            }
            // add students who X (prevented/required/requested) teammates of baseStudent
            if(printStudent) {
                atLeastOneTeammate = true;
                *teammatesSpecified = true;

                // find studentB from their ID
                StudentRecord *studentB = nullptr;
                for(auto &stu : students) {
                    if((stu.ID == studentBID) && !stu.deleted) {
                        studentB = &stu;
                    }
                }
                if(studentB == nullptr) {
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
                //remover should only appear when the text is valid!
                auto *remover = new QPushButton(QIcon(":/icons_new/trashButton.png"), "", this);
                remover->setFlat(true);
                remover->setIconSize(ICONSIZE);
                if(typeOfTeammates == TypeOfTeammates::required) {
                    connect(remover, &QPushButton::clicked, this, [this, filteredStudent, studentB, table, searchBarText]
                                                            {
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                                                            filteredStudent->requiredWith.remove(studentB->ID);
                                                             studentB->requiredWith.remove(filteredStudent->ID);
                                                             refreshDisplay(TypeOfTeammates::required, verticalScrollPos, horizontalScrollPos, searchBarText);
                                                             initializeTableHeaders(TypeOfTeammates::required, searchBarText);
                    });
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    connect(remover, &QPushButton::clicked, this, [this, filteredStudent, studentB, table, searchBarText]
                                                            {
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                    filteredStudent->preventedWith.remove(studentB->ID);
                                                             studentB->preventedWith.remove(filteredStudent->ID);
                                                             refreshDisplay(TypeOfTeammates::prevented, verticalScrollPos, horizontalScrollPos, searchBarText);
                                                             initializeTableHeaders(TypeOfTeammates::prevented, searchBarText);
                    });
                }
                else {
                    connect(remover, &QPushButton::clicked, this, [this, filteredStudent, studentB, table, searchBarText]
                                                            {
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                        filteredStudent->requestedWith.remove(studentB->ID);
                                                             refreshDisplay(TypeOfTeammates::requested, verticalScrollPos, horizontalScrollPos, searchBarText);
                                                             initializeTableHeaders(TypeOfTeammates::required, searchBarText);

                    });
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

        //Add the LineEdit in Final Column
        if(table->columnCount() < column+1) {
            table->setColumnCount(column+1);
            table->setHorizontalHeaderItem(column, new QTableWidgetItem(typeText + "\n" + tr("Teammate #") +
                                                                        QString::number(column + (requestsInSurvey? 0:1))));
        }

        //Create Elements for Final Column
        auto *cellWidget = new QWidget(this);
        auto *box = new QHBoxLayout();
        //Set up lineEdit
        auto *lineEdit = new QLineEdit(this);
        lineEdit->setPlaceholderText("Enter a student.."); // Set initial value
        lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: black;}");

        // Set up the completer with all student names
        QMap<QString, StudentRecord*> studentNameToIdMap;
        QStringList studentNames;

        //get all names from baseStudents
        for (auto *student : baseStudents) {
            QString fullName = student->firstname + " " + student->lastname;
            studentNames.append(fullName);
            studentNameToIdMap[fullName] = student;  // Store name → ID mapping
        }
        //pressing enter also cancels the dialog

        auto *model = new QStringListModel(studentNames, this);
        auto *completer = new QCompleter(model, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        lineEdit->setCompleter(completer);

        QPushButton *confirmButton = new QPushButton(this);
        confirmButton->setIcon(QIcon(":/icons_new/Checkmark.png"));

        if(typeOfTeammates == TypeOfTeammates::required) {
            connect(confirmButton, &QPushButton::clicked, this, [this, table, lineEdit, filteredStudent, studentNameToIdMap, searchBarText](){
                QString newText = lineEdit->text(); //get the current text
                //check that the student name is valid and that user is not adding the student itself to the list.
                if (studentNameToIdMap.contains(newText)){
                    if (studentNameToIdMap[newText]->ID != filteredStudent->ID){
                        StudentRecord *pairedStudent = studentNameToIdMap[newText];
                        filteredStudent->requiredWith.insert(pairedStudent->ID);
                        pairedStudent->requiredWith.insert(filteredStudent->ID);
                        //keep the current scroll position for user
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                        refreshDisplay(TypeOfTeammates::required, verticalScrollPos, horizontalScrollPos, searchBarText);
                        initializeTableHeaders(TypeOfTeammates::required, searchBarText);
                    } else {
                        showToast(this, "Cannot prevent a student with themselves.");
                    }

                } else {
                    //turn the text red, print a toast message
                    showToast(this, "The student name does not exist, please double check your input.", 2000);
                    lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: darkred;}");
                }
            });
        } else if(typeOfTeammates == TypeOfTeammates::prevented){
            connect(confirmButton, &QPushButton::clicked, this, [this, table, lineEdit, filteredStudent, studentNameToIdMap, searchBarText](){
                QString newText = lineEdit->text(); //get the current text
                //check that the student name is valid and that user is not adding the student itself to the list.
                if (studentNameToIdMap.contains(newText)){
                    if (studentNameToIdMap[newText]->ID != filteredStudent->ID ){
                        StudentRecord *pairedStudent = studentNameToIdMap[newText];
                        filteredStudent->preventedWith.insert(pairedStudent->ID);
                        pairedStudent->preventedWith.insert(filteredStudent->ID);
                        //keep the current scroll position for user
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                        refreshDisplay(TypeOfTeammates::prevented, verticalScrollPos, horizontalScrollPos, searchBarText);
                        initializeTableHeaders(TypeOfTeammates::prevented, searchBarText);
                    } else {
                        showToast(this, "Cannot prevent a student with themselves.");
                    }
                } else {
                    //turn the text red, print a toast message
                    showToast(this, "The student name does not exist, please double check your input.", 2000);
                    lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: darkred;}");
                }
            });
        } else if(typeOfTeammates == TypeOfTeammates::requested){
            connect(confirmButton, &QPushButton::clicked, this, [this, table, lineEdit, filteredStudent, studentNameToIdMap, searchBarText](){
                QString newText = lineEdit->text(); //get the current text
                //check that the student name is valid and that user is not adding the student filteredStudent to the list.
                if (studentNameToIdMap.contains(newText) && studentNameToIdMap[newText]->ID != filteredStudent->ID ){
                    if (studentNameToIdMap[newText]->ID != filteredStudent->ID ){
                        StudentRecord *pairedStudent = studentNameToIdMap[newText];
                        filteredStudent->requestedWith.insert(pairedStudent->ID);
                        pairedStudent->requestedWith.insert(filteredStudent->ID);
                        //keep the current scroll position for user
                        int verticalScrollPos = table->verticalScrollBar()->value();
                        int horizontalScrollPos = table->horizontalScrollBar()->value();
                        refreshDisplay(TypeOfTeammates::requested, verticalScrollPos, horizontalScrollPos, searchBarText);
                        initializeTableHeaders(TypeOfTeammates::requested, searchBarText);
                    } else {
                        showToast(this, "Cannot prevent a student with themselves.");
                    }
                } else {
                    //turn the text red, print a toast message
                    showToast(this, "The student name does not exist, please double check your input.", 2000);
                    lineEdit->setStyleSheet("QLineEdit {font-size: 10pt; font-family: 'DM Sans'; color: darkred;}");
                }
            });
        }
        box->addWidget(lineEdit);
        box->addWidget(confirmButton);
        cellWidget->setLayout(box);
        table->setCellWidget(row, column, cellWidget);
        row++;
    }
    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    table->verticalScrollBar()->setValue(verticalScrollPos);
    table->horizontalScrollBar()->setValue(horizontalScrollPos);
}

void TeammatesRulesDialog::initializeTableHeaders(TypeOfTeammates typeOfTeammates, QString searchBarText, bool initializeStatus){
    //headerLayout
    //initialize TypeOfTeammates::required header
    //Initialize each table column as a label
    QString typeText;
    QHBoxLayout *headerLayout;
    QTableWidget *table;
    QWidget *headerWidget;
    if(typeOfTeammates == TypeOfTeammates::required) {
        typeText = tr("Required");
        table = required_tableWidget;
        headerLayout = headerLayoutRequiredTeammates;
        headerWidget = headerRequiredTeammates;
        clearLayout(headerLayoutRequiredTeammates);
    }
    else if (typeOfTeammates == TypeOfTeammates::prevented) {
        typeText = tr("Prevented");
        table = prevented_tableWidget;
        headerLayout = headerLayoutPreventedTeammates;
        headerWidget = headerPreventedTeammates;
        clearLayout(headerLayoutPreventedTeammates);
    }
    else {
        typeText = tr("Requested");
        table = requested_tableWidget;
        headerLayout = headerLayoutRequestedTeammates;
        headerWidget = headerRequestedTeammates;
        clearLayout(headerLayoutRequestedTeammates);
    }

    auto *topLeftHeaderLayout = new QVBoxLayout();
    topLeftHeaderLayout->setSpacing(0);
    auto *topLeftHeaderWidget = new QWidget(this);
    int width = table->verticalHeader()->sizeHint().width(); //width of abstract button
    if (initializeStatus == true){ //don't update the size of this apart from the first intialization (avoid weird layout issues)
        table->verticalHeader()->setFixedWidth(width);
        this->initialWidthStudentHeader = width;
    }
    //if not fixed width, the header may change according to the students filtered

    //createSearchBar
    auto *studentSearchBar = new QLineEdit(this);
    studentSearchBar->setFocusPolicy(Qt::StrongFocus);
    //for some reason the text does not update either
    studentSearchBar->setPlaceholderText("Filter by name");

    studentSearchBar->setText(searchBarText);
    studentSearchBar->setStyleSheet("QLineEdit { font-size: 10pt; font-family: 'DM Sans'; color: black; background-color: white; border: 1px solid lightGray; border-radius: 5px}");
    connect(studentSearchBar, &QLineEdit::textChanged, this, [this, typeOfTeammates, studentSearchBar](){
        refreshDisplay(typeOfTeammates, 0, 0, studentSearchBar->text());
    });
    headerWidget->setFixedHeight(studentSearchBar->sizeHint().height() + 35);

    auto *label = new QLabel(tr("Student"), this);
    label->setStyleSheet("QLabel{border-top: none; border-left: none; border-right: none; "
                         "border-bottom: none; background-color:" DEEPWATERHEX "; "
                         "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
    topLeftHeaderWidget->setStyleSheet("QWidget{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                                       "border-bottom: none; background-color:" DEEPWATERHEX "; "
                                       "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center; padding:2px;}");

    topLeftHeaderLayout->addWidget(label);
    topLeftHeaderLayout->addWidget(studentSearchBar);
    topLeftHeaderLayout->setSpacing(2);
    topLeftHeaderWidget->setLayout(topLeftHeaderLayout);
    topLeftHeaderWidget->setFixedWidth(initialWidthStudentHeader);
    topLeftHeaderWidget->setFixedHeight(studentSearchBar->sizeHint().height() + 35);
    headerLayout->addWidget(topLeftHeaderWidget, Qt::AlignCenter);

    for (int col = 0; col < table->columnCount(); ++col) { //get the size as well
        auto *label = new QLabel(table->horizontalHeaderItem(col)->text());
        label->setStyleSheet("QLabel{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                             "border-bottom: none; background-color:" DEEPWATERHEX "; "
                             "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
        int width = table->columnWidth(col);
        label->setFixedWidth(width);
        label->setFixedHeight(studentSearchBar->sizeHint().height() + 35);
        headerLayout->addWidget(label, Qt::AlignCenter);
    }
    QLabel *spacerLabel = new QLabel();
    spacerLabel->setStyleSheet("QLabel { background-color: " DEEPWATERHEX "; }");
    spacerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Add the spacer label and stretch to push all widgets to the left
    headerLayout->addWidget(spacerLabel);
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
    for(const auto &student : qAsConst(students)) {
        studentcombobox->setPlaceholderText(comboBoxes->first()->placeholderText());
        if(((sectionName == "") || (sectionName == student.section)) && !student.deleted) {
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
    for(const auto &comboBox : qAsConst(comboBoxes)) {
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
                    if(typeOfTeammates == TypeOfTeammates::required) {
                        student1->requiredWith << IDs[ID2];
                        student2->requiredWith << IDs[ID1];
                    }
                    else {
                        student1->preventedWith << IDs[ID2];
                        student2->preventedWith << IDs[ID1];
                    }
                }
            }
        }
    }
    else {
        const int baseStudentID = ui->requested_studentSelectComboBox->currentData().toInt();
        // find the student with this ID
        StudentRecord *baseStudent = nullptr;
        int index = 0;
        while((students.at(index).ID != baseStudentID) && (index < numStudents)) {
            index++;
        }
        if(index < numStudents) {
            baseStudent = &students[index];

            for(const int ID : qAsConst(IDs)) {
                if(baseStudentID != ID) {
                    //we have at least one requested teammate pair!
                    baseStudent->requestedWith << ID;
                }
            }
        }

        //Reset combobox
        ui->requested_studentSelectComboBox->setCurrentIndex(-1);
    }
    refreshDisplay(typeOfTeammates, 0, 0);
}

void TeammatesRulesDialog::clearAllValues()
{
    const bool okClear = grueprGlobal::warningMessage(this, "gruepr",
                                                       tr("This will remove all teammates rules listed in all of the tables.\n"
                                                          "Are you sure you want to continue?"),
                                                       tr("Yes"), tr("No"));
    if(!okClear) {
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
        const bool okClear = grueprGlobal::warningMessage(this, "gruepr",
                                                          tr("This will remove all rules listed in the ") + typeText + tr(" teammates table.\n"
                                                          "Are you sure you want to continue?"),
                                                          tr("Yes"), tr("No"));
        if(!okClear) {
            return;
        }
    }

    for(auto &student : students) {
        if((sectionName == "") || (sectionName == student.section)) {
            for(int index2 = 0; index2 < numStudents; index2++) {
                if(typeOfTeammates == TypeOfTeammates::required) {
                    student.requiredWith.remove(index2);
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    student.preventedWith.remove(index2);
                }
                else {
                    student.requestedWith.remove(index2);
                }
            }
        }
    }

    clearButton->setEnabled(false);
    if(!ui->required_clearButton->isEnabled() && !ui->requested_clearButton->isEnabled() && !ui->prevented_clearButton->isEnabled()) {
        clearAllValuesButton->setEnabled(false);
    }
    refreshDisplay(typeOfTeammates, 0, 0);
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

        //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
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
                if(typeOfTeammates == TypeOfTeammates::required) {
                    baseStudent->requiredWith << IDs[ID2];
                    student2->requiredWith << IDs[0];
                }
                else if(typeOfTeammates == TypeOfTeammates::prevented) {
                    baseStudent->preventedWith << IDs[ID2];
                    student2->preventedWith << IDs[0];
                }
                else {  //whatType == requested
                    baseStudent->requestedWith << IDs[ID2];
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates, 0, 0);
    return true;
}

bool TeammatesRulesDialog::loadStudentPrefs(TypeOfTeammates typeOfTeammates)
{
    // Need to convert names to IDs and then add all to the preferences
    QList<long long> IDs;
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

                //Add to the first ID (the basename) in each set all of the subsequent IDs in the set as a required / prevented / requested pairing
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
                        if(typeOfTeammates == TypeOfTeammates::required) {
                            baseStudent->requiredWith << IDs[ID2];
                            student2->requiredWith << IDs[0];
                        }
                        else if(typeOfTeammates == TypeOfTeammates::prevented) {
                            baseStudent->preventedWith << IDs[ID2];
                            student2->preventedWith << IDs[0];
                        }
                        else {  //whatType == requested
                            baseStudent->requestedWith << IDs[ID2];
                        }
                    }
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates, 0, 0);
    return true;
}

bool TeammatesRulesDialog::loadSpreadsheetFile(TypeOfTeammates typeOfTeammates)
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
                    if(typeOfTeammates == TypeOfTeammates::required) {
                        student1->requiredWith << IDs[ID2];
                        student2->requiredWith << IDs[ID1];
                    }
                    else if(typeOfTeammates == TypeOfTeammates::prevented) {
                        student1->preventedWith << IDs[ID2];
                        student2->preventedWith << IDs[ID1];
                    }
                    else {  //whatType == requested
                        student1->requestedWith << IDs[ID2];
                        student2->requestedWith << IDs[ID1];
                    }
                }
            }
        }
    }

    refreshDisplay(typeOfTeammates, 0, 0);
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

        const int result = win->exec();
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
    for(int basestudent = 0; basestudent < basenames.size(); basestudent++) {
        teammates[basestudent].prepend(basenames.at(basestudent));
    }

    QList<int> IDs;
    for(int basename = 0; basename < basenames.size(); basename++) {
        IDs.clear();
        for(int searchStudent = 0; searchStudent < teammates.at(basename).size(); searchStudent++) { // searchStudent is the name we're looking for
            int knownStudent = 0;     // start at first student in database and look until we find a matching first+last name
            while((knownStudent < numStudents) &&
                  (teammates.at(basename).at(searchStudent).compare(students[knownStudent].firstname +
                    " " + students[knownStudent].lastname, Qt::CaseInsensitive) != 0)) {
                knownStudent++;
            }

            if(knownStudent != numStudents) {
                // Exact match found
                IDs << students[knownStudent].ID;
            }
            else {
                // No exact match, so list possible matches sorted by Levenshtein distance
                auto *choiceWindow = new findMatchingNameDialog(numStudents, student, teammates.at(basename).at(searchStudent), this);
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
                if(whatType == required) {
                    baseStudent->requiredWith[IDs[ID2]] = true;
                    student2->requiredWith[IDs[0]] = true;
                }
                else if(whatType == prevented) {
                    baseStudent->preventedWith[IDs[ID2]] = true;
                    student2->preventedWith[IDs[0]] = true;
                }
                else {   //whatType == requested
                    baseStudent->requestedWith[IDs[ID2]] = true;
                }
            }
        }
    }
*/
    refreshDisplay(typeOfTeammates, 0, 0);
    return true;
}
