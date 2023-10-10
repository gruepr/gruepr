#include "gruepr.h"
#include "ui_gruepr.h"
#include "dialogs/customTeamsizesDialog.h"
#include "dialogs/editOrAddStudentDialog.h"
#include "dialogs/findMatchingNameDialog.h"
#include "dialogs/attributeRulesDialog.h"
#include "dialogs/teammatesRulesDialog.h"
#include "dialogs/gatherURMResponsesDialog.h"
#include "widgets/pushButtonWithMouseEnter.h"
#include "widgets/sortableTableWidgetItem.h"
#include "widgets/teamsTabItem.h"
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QScreen>
#include <QTextBrowser>
#include <QtConcurrent>
#include <QtNetwork>
#include <random>


gruepr::gruepr(DataOptions &dataOptions, QList<StudentRecord> &students, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::gruepr)
{
    //Setup the main window
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowIcon(QIcon(":/icons_new/icon.svg"));
    setWindowTitle(tr("gruepr - Form teams"));
    qRegisterMetaType<QList<float> >("QList<float>");

    ui->dataSourceFrame->setStyleSheet(QString() + "QFrame {background-color: " + (QColor::fromString(QString(STARFISHHEX)).lighter(133).name()) + ";"
                                                           " color: " DEEPWATERHEX "; border: none;}"
                                                   "QFrame::disabled {background-color: lightGray; color: darkGray; border: none;}");
    ui->dataSourcePrelabel->setStyleSheet("QLabel {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 10pt;}"
                                       "QLabel::disabled {background-color: " TRANSPARENT "; color: darkGray; font-family:'DM Sans'; font-size: 10pt;}");
    ui->dataSourceLabel->setStyleSheet("QLabel {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 10pt;}"
                                       "QLabel::disabled {background-color: " TRANSPARENT "; color: darkGray; font-family:'DM Sans'; font-size: 10pt;}");
    ui->newDataSourceButton->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 10pt; "
                                                        "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; padding: 5px;}");
    QList<QFrame*> frames = {ui->sectionFrame, ui->teamSizeFrame, ui->genderFrame, ui->URMFrame, ui->attributesFrame, ui->scheduleFrame, ui->teammatesFrame};
    for(auto &frame : frames) {
        frame->setStyleSheet(QString() + "QFrame {background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; border: 1px solid; border-color: " AQUAHEX "}" +
                                         LABELSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    }
    ui->attributesStackedWidget->setStyleSheet(QString() + "QFrame {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; border: none;}");
    ui->scheduleWeight->setSuffix("  /  " + QString::number(TeamingOptions::MAXWEIGHT));
    ui->scheduleWeight->setToolTip(TeamingOptions::SCHEDULEWEIGHTTOOLTIP);
    ui->teamingOptionsScrollArea->setStyleSheet(SCROLLBARSTYLE);
    ui->letsDoItButton->setStyleSheet(GETSTARTEDBUTTONSTYLE);
    ui->addStudentPushButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->compareRosterPushButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->saveSurveyFilePushButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->dataDisplayTabWidget->setStyleSheet("QTabWidget {border: none; background-color: lightGray;}"
                                            "QTabWidget::pane {margin: 10px, 0px, 0px, 0px; border: 1px solid black;}");
    ui->dataDisplayTabWidget->tabBar()->setStyleSheet("QTabBar {alignment: center; margin: 6px; padding: 4px; border: none;}"
                                                       "QTabBar::tab {border: 1px solid " OPENWATERHEX "; border-radius: 5px; padding: 5px; margin-left: 2px; margin-right: 2px; "
                                                                      "font-family:'DM Sans'; font-size: 12pt;}"
                                                       "QTabBar::tab::selected {color: white; background: " OPENWATERHEX ";}"
                                                       "QTabBar::tab::!selected {color: " OPENWATERHEX "; background: white;}"
                                                       "QTabBar::close-button {image: url(:/icons_new/close.png); subcontrol-position: right; margin: 2px;}");
    ui->dataDisplayTabWidget->tabBar()->setDrawBase(false);
    QList<QPushButton *> buttons = {ui->letsDoItButton, ui->addStudentPushButton, ui->compareRosterPushButton, ui->saveSurveyFilePushButton};
    for(auto &button : buttons) {
        button->setIconSize(QSize(STD_ICON_SIZE, STD_ICON_SIZE));
    }
    ui->dataSourceIcon->setFixedSize(STD_ICON_SIZE, STD_ICON_SIZE);

    QList<QWidget *> selectors = {ui->sectionSelectionBox, ui->idealTeamSizeBox, ui->teamSizeBox,
                                  ui->minMeetingTimes, ui->desiredMeetingTimes, ui->meetingLengthSpinBox, ui->scheduleWeight};
    for(auto &selector : selectors) {
        selector->setFocusPolicy(Qt::StrongFocus);  // remove scrollwheel from affecting the value,
        selector->installEventFilter(new MouseWheelBlocker(selector)); // as it's too easy to mistake scrolling through the rows with changing the value
    }

    //Make the teams tabs double-clickable and closable (hide the close button on the students tab)
    connect(ui->dataDisplayTabWidget, &QTabWidget::tabBarDoubleClicked, this, &gruepr::editDataDisplayTabName);
    ui->dataDisplayTabWidget->setTabsClosable(true);
    ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);
    connect(ui->dataDisplayTabWidget, &QTabWidget::tabCloseRequested, this, &gruepr::dataDisplayTabClose);

    //Set alternate fonts on some UI features
    QFont altFont = this->font();
    altFont.setPointSize(altFont.pointSize() + 4);
    ui->letsDoItButton->setFont(altFont);
    ui->addStudentPushButton->setFont(altFont);
    ui->compareRosterPushButton->setFont(altFont);
    ui->saveSurveyFilePushButton->setFont(altFont);
    ui->dataDisplayTabWidget->setFont(altFont);

    this->dataOptions = new DataOptions(std::move(dataOptions));
    this->students = std::move(students);

    teamingOptions = nullptr;
    if(this->dataOptions->dataSource == DataOptions::fromPrevWork) {
        QFile savedFile(this->dataOptions->saveStateFileName, this);
        if(savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
            savedFile.close();
            QJsonObject content = doc.object();
            teamingOptions = new TeamingOptions(content["teamingoptions"].toObject());
            QJsonArray teamsetjsons = content["teamsets"].toArray();
            TeamsTabItem *teamTab = nullptr;
            for(const auto &teamsetjson : teamsetjsons) {
                teamTab = new TeamsTabItem(teamsetjson.toObject(), *teamingOptions, this->students, ui->letsDoItButton, this);
                ui->dataDisplayTabWidget->addTab(teamTab, teamTab->tabName);
                numTeams = int(teams.size());
            }
        }
        else {
            grueprGlobal::errorMessage(this, tr("Error"), tr("There was an error loading the previous data."));
        }
    }

    if(teamingOptions == nullptr) {             // either not from previous work, or loading from previous work failed
        teamingOptions = new TeamingOptions;
        loadDefaultSettings();
    }

    adjustSize();
    loadUI();

    //Connect the simple UI items to a single function that simply reads all of the items and updates the teamingOptions
    connect(ui->isolatedWomenCheckBox, &QCheckBox::stateChanged, this, [this](){simpleUIItemUpdate(ui->isolatedWomenCheckBox);});
    connect(ui->isolatedMenCheckBox, &QCheckBox::stateChanged, this, [this](){simpleUIItemUpdate(ui->isolatedMenCheckBox);});
    connect(ui->isolatedNonbinaryCheckBox, &QCheckBox::stateChanged, this, [this](){simpleUIItemUpdate(ui->isolatedNonbinaryCheckBox);});
    connect(ui->mixedGenderCheckBox, &QCheckBox::stateChanged, this, [this](){simpleUIItemUpdate(ui->mixedGenderCheckBox);});
    connect(ui->isolatedURMCheckBox, &QCheckBox::stateChanged, this, [this](){simpleUIItemUpdate(ui->isolatedURMCheckBox);});
    connect(ui->minMeetingTimes, &QSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(ui->minMeetingTimes);});
    connect(ui->desiredMeetingTimes, &QSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(ui->desiredMeetingTimes);});
    connect(ui->meetingLengthSpinBox, &QSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(ui->meetingLengthSpinBox);});
    connect(ui->scheduleWeight, &QDoubleSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(ui->scheduleWeight);});

    //Connect genetic algorithm progress signals to slots
    connect(this, &gruepr::generationComplete, this, &gruepr::updateOptimizationProgress, Qt::BlockingQueuedConnection);
    connect(&futureWatcher, &QFutureWatcher<void>::finished, this, &gruepr::optimizationComplete);

    saveState();
}

gruepr::~gruepr()
{
    delete dataOptions;
    delete teamingOptions;
    delete ui;
}


////////////////////
// A static public wrapper for the getGenomeScore function used internally
// The calculated scores are updated into the .scores members of the _teams array sent to the function
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
////////////////////
void gruepr::updateTeamScores(const StudentRecord *const _students, const int _numStudents, TeamRecord *const _teams,
                              const int _numTeams,const TeamingOptions *const _teamingOptions,
                              const DataOptions *const _dataOptions)
{
    auto *teamScores = new float[_numTeams];
    auto **attributeScore = new float*[_dataOptions->numAttributes];
    for(int attrib = 0; attrib < _dataOptions->numAttributes; attrib++)
    {
        attributeScore[attrib] = new float[_numTeams];
    }
    auto *schedScore = new float[_numTeams];
    auto **availabilityChart = new bool*[_dataOptions->dayNames.size()];
    for(int day = 0; day < _dataOptions->dayNames.size(); day++)
    {
        availabilityChart[day] = new bool[_dataOptions->timeNames.size()];
    }
    auto *penaltyPoints = new int[_numTeams];
    auto *teamSizes = new int[_numTeams];
    auto *genome = new int[_numStudents];
    int ID = 0;
    for(int teamnum = 0; teamnum < _numTeams; teamnum++)
    {
        teamSizes[teamnum] = _teams[teamnum].size;
        for(int teammate = 0; teammate < teamSizes[teamnum]; teammate++)
        {
            genome[ID] = _teams[teamnum].studentIndexes.at(teammate);
            ID++;
        }
    }
    getGenomeScore(_students, genome, _numTeams, teamSizes,
                   _teamingOptions, _dataOptions, teamScores,
                   attributeScore, schedScore, availabilityChart, penaltyPoints);
    for(int teamnum = 0; teamnum < _numTeams; teamnum++)
    {
        _teams[teamnum].score = teamScores[teamnum];
    }
    delete[] genome;
    delete[] teamSizes;
    delete[] penaltyPoints;
    for(int day = 0; day < _dataOptions->dayNames.size(); day++)
    {
        delete[] availabilityChart[day];
    }
    delete[] availabilityChart;
    delete[] schedScore;
    for(int attrib = 0; attrib < _dataOptions->numAttributes; attrib++)
    {
        delete[] attributeScore[attrib];
    }
    delete[] attributeScore;
    delete[] teamScores;
}


void gruepr::on_newDataSourceButton_clicked()
{
    restartRequested = true;
    close();
}


void gruepr::on_sectionSelectionBox_currentIndexChanged(int index)
{
    const QString desiredSection = ui->sectionSelectionBox->itemText(index);
    if(desiredSection == "")
    {
        numActiveStudents = 0;
        return;
    }

    if(!multipleSectionsInProgress)
    {
        teamingOptions->sectionName = desiredSection;

        if(!dataOptions->sectionIncluded) {
            teamingOptions->sectionType = TeamingOptions::SectionType::noSections;
        }
        else if(ui->sectionSelectionBox->currentIndex() == 1) {
            teamingOptions->sectionType = TeamingOptions::SectionType::allSeparately;
        }
        else if(ui->sectionSelectionBox->currentIndex() == 0) {
            teamingOptions->sectionType = TeamingOptions::SectionType::allTogether;
        }
        else {
            teamingOptions->sectionType = TeamingOptions::SectionType::oneSection;
        }
    }

    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();

    // update the response counts in the attribute tabs
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        const auto &attributeType = dataOptions->attributeType[attribute];

        // record a tally for each response, starting with a 0 count for each response found in all of the survey data
        std::map<QString, int> currentResponseCounts;
        for(const auto & responseCount : qAsConst(dataOptions->attributeQuestionResponseCounts[attribute]))
        {
            currentResponseCounts[responseCount.first] = 0;
        }
        for(const auto &student:students)
        {
            if((ui->sectionSelectionBox->currentIndex() == 0) || (ui->sectionSelectionBox->currentIndex() == 1) || (student.section == ui->sectionSelectionBox->currentText()))
            {
                const QString &currentStudentResponse = student.attributeResponse[attribute];

                if(!student.attributeResponse[attribute].isEmpty())
                {
                    if((attributeType == DataOptions::AttributeType::multicategorical) ||
                        (attributeType == DataOptions::AttributeType::multiordered))
                    {
                        //multivalued - tally each value
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent)
                        {
                            currentResponseCounts[responseFromStudent.trimmed()]++;
                        }
                    }
                    else
                    {
                        currentResponseCounts[currentStudentResponse]++;
                    }
                }
            }
        }

        // put this new tally in the responses textbox of the attribute tab
        attributeWidgets[attribute]->updateQuestionAndResponses(attribute, dataOptions, currentResponseCounts);
    }

    ui->idealTeamSizeBox->setMaximum(std::max(2,numActiveStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
}


void gruepr::editAStudent()
{
    int indexBeingEdited = sender()->property("StudentIndex").toInt();

    // remove this student's current attribute responses from the counts in dataOptions
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = students[indexBeingEdited].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
            {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
            else
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
        }
    }

    //Open window with the student record in it
    auto *win = new editOrAddStudentDialog(students[indexBeingEdited], dataOptions, this, false);

    //If user clicks OK, replace student in the database with edited copy
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        students[indexBeingEdited].createTooltip(dataOptions);
        students[indexBeingEdited].URM = teamingOptions->URMResponsesConsideredUR.contains(students[indexBeingEdited].URMResponse);

        rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    }

    // add back in this student's attribute responses from the counts in dataOptions and update the attribute tabs to show the counts
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = students[indexBeingEdited].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
            {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                }
            }
            else
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
            }
            attributeWidgets[attribute]->setValues(attribute, dataOptions, teamingOptions);
        }
    }

    delete win;
    saveState();
}


void gruepr::removeAStudent(const QString &name, bool delayVisualUpdate)
{
    // use the name to find the index
    int index = 0;
    if(!name.isEmpty())
    {
        // don't have index, need to search and locate based on name
        while((index < dataOptions->numStudentsInSystem) &&
              (name.compare(students[index].firstname + " " + students[index].lastname), Qt::CaseInsensitive) != 0)
        {
            index++;
        }
    }
    removeAStudent(index, delayVisualUpdate);
}


void gruepr::removeAStudent(int index, bool delayVisualUpdate)
{
    if(teamingOptions->haveAnyRequiredTeammates || teamingOptions->haveAnyRequestedTeammates)
    {
        // remove this student from all other students who might have them as required/prevented/requested
        const int IDBeingRemoved = students[index].ID;
        for(int otherIndex = 0; otherIndex < dataOptions->numStudentsInSystem; otherIndex++)
        {
            students[otherIndex].requiredWith[IDBeingRemoved] = false;
            students[otherIndex].preventedWith[IDBeingRemoved] = false;
            students[otherIndex].requestedWith[IDBeingRemoved] = false;
        }
    }

    // update in dataOptions and then the attribute tab the count of each attribute response
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = students[index].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
            {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
            else
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
            attributeWidgets[attribute]->setValues(attribute, dataOptions, teamingOptions);
        }
    }

    //Remove the student
    dataOptions->numStudentsInSystem--;
    students.remove(index);

    if(delayVisualUpdate)
    {
        return;
    }

    rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    saveState();
}


void gruepr::on_addStudentPushButton_clicked()
{
    if(dataOptions->numStudentsInSystem < MAX_STUDENTS)
    {
        //Open window with a blank student record in it
        StudentRecord newStudent;
        auto *win = new editOrAddStudentDialog(newStudent, dataOptions, this, true);

        //If user clicks OK, add student to the database
        int reply = win->exec();
        if(reply == QDialog::Accepted)
        {
            newStudent.ID = students.size();
            newStudent.createTooltip(dataOptions);
            newStudent.URM = teamingOptions->URMResponsesConsideredUR.contains(newStudent.URMResponse);
            newStudent.ambiguousSchedule = (newStudent.availabilityChart.count("√") == 0 ||
                                           (newStudent.availabilityChart.count("√") == (dataOptions->dayNames.size() * dataOptions->timeNames.size())));
            students << newStudent;

            // update in dataOptions and then the attribute tab the count of each attribute response
            for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
            {
                const QString &currentStudentResponse = newStudent.attributeResponse[attribute];
                if(!currentStudentResponse.isEmpty())
                {
                    if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                        (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
                    {
                        //need to process each one
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                        {
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                    else
                    {
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    attributeWidgets[attribute]->setValues(attribute, dataOptions, teamingOptions);
                }
            }
            dataOptions->numStudentsInSystem++;
            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
        }
        delete win;
    }
    else
    {
        grueprGlobal::errorMessage(this, tr("Cannot add student."),
                                   tr("Sorry, we cannot add another student.\nThis version of gruepr does not allow more than ") +
                                   QString::number(MAX_STUDENTS) + ".");
    }
    saveState();
}


void gruepr::on_compareRosterPushButton_clicked()
{
    // Open the roster file
    QSettings savedSettings;
    CsvFile rosterFile;
    if(!rosterFile.open(this, CsvFile::read, tr("Open Student Roster File"), savedSettings.value("saveFileLocation").toString(), tr("Roster File")))
    {
        return;
    }

    QStringList names, emails;
    if(loadRosterData(rosterFile, names, emails))
    {
        bool dataHasChanged = false;

        // load all current names from the survey so we can later remove them as they're found in the roster and be left with problem cases
        QStringList namesNotFound;
        namesNotFound.reserve(dataOptions->numStudentsInSystem);
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            namesNotFound << students[index].firstname + " " + students[index].lastname;
        }

        // create a place to save info for names with mismatched emails
        QList <int> studentsWithDiffEmail;
        studentsWithDiffEmail.reserve(dataOptions->numStudentsInSystem);

        for(auto &name : names)
        {
            int index = 0;     // start at first student in database and look until we find a matching firstname + " " +last name
            while((index < dataOptions->numStudentsInSystem) &&
                   (name.compare(students[index].firstname + " " + students[index].lastname, Qt::CaseInsensitive) != 0))
            {
                index++;
            }

            if(index != dataOptions->numStudentsInSystem)
            {
                // Exact match found
                namesNotFound.removeAll(students[index].firstname + " " + students[index].lastname);
                if(students[index].email.compare(emails.at(names.indexOf(name)), Qt::CaseInsensitive) != 0)
                {
                    // Email in survey doesn't match roster
                    studentsWithDiffEmail << index;
                }
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance and allow user to pick a match, add as a new student, or ignore
                auto *choiceWindow = new findMatchingNameDialog(dataOptions->numStudentsInSystem, students, name, this, "", true, emails.at(names.indexOf(name)));
                if(choiceWindow->exec() == QDialog::Accepted)   // not ignoring this student
                {
                    if(choiceWindow->addStudent)    // add as a new student
                    {
                        dataHasChanged = true;

                        StudentRecord newStudent;
                        newStudent.surveyTimestamp = {};
                        newStudent.ID = students.size();
                        newStudent.firstname = name.split(" ").first();
                        newStudent.lastname = name.split(" ").mid(1).join(" ");
                        newStudent.email = emails.at(names.indexOf(name));
                        newStudent.URM = teamingOptions->URMResponsesConsideredUR.contains(newStudent.URMResponse);
                        newStudent.ambiguousSchedule = true;
                        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
                        {
                            newStudent.attributeVals[attribute] << -1;
                        }
                        newStudent.createTooltip(dataOptions);

                        students << newStudent;

                        dataOptions->numStudentsInSystem++;
                        numActiveStudents = dataOptions->numStudentsInSystem;
                    }
                    else   // selected an inexact match
                    {
                        QString surveyName = choiceWindow->currSurveyName;
                        namesNotFound.removeAll(surveyName);
                        index = 0;
                        while(surveyName != (students[index].firstname + " " + students[index].lastname))
                        {
                            index++;
                        }
                        if(choiceWindow->useRosterEmail)
                        {
                            dataHasChanged = true;
                            students[index].email = emails.at(names.indexOf(name));
                            students[index].createTooltip(dataOptions);
                        }
                        if(choiceWindow->useRosterName)
                        {
                            dataHasChanged = true;
                            students[index].firstname = name.split(" ").first();
                            students[index].lastname = name.split(" ").mid(1).join(" ");
                            students[index].createTooltip(dataOptions);
                        }
                    }
                }
                delete choiceWindow;
            }
        }

        // Now handle the times where the roster and survey have different email addresses
        bool keepAsking = true, makeTheChange = false;
        int i = 0;
        for(auto &studentNum : studentsWithDiffEmail)
        {
            QString surveyName = students[studentNum].firstname + " " + students[studentNum].lastname;
            QString surveyEmail = students[studentNum].email;
            if(keepAsking)
            {
                auto *whichEmailWindow = new QMessageBox(QMessageBox::Question, tr("Email addresses do not match"),
                                                         tr("This student on the roster:") +
                                                             "<br><b>" + surveyName + "</b><br>" +
                                                             tr("has a different email address in the survey.") + "<br><br>" +
                                                             tr("Select one of the following email addresses:") + "<br>" +
                                                             tr("Survey: ") + "<b>" + surveyEmail + "</b><br>" +
                                                             tr("Roster: ") + "<b>" +  emails.at(names.indexOf(surveyName))  + "</b><br>",
                                                         QMessageBox::Ok | QMessageBox::Cancel, this);
                whichEmailWindow->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
                whichEmailWindow->setStyleSheet(LABELSTYLE);
                whichEmailWindow->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
                whichEmailWindow->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
                auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(studentsWithDiffEmail.size() - i) + tr(" students)"));
                applyToAll->setStyleSheet(CHECKBOXSTYLE);
                whichEmailWindow->setCheckBox(applyToAll);
                connect(applyToAll, &QCheckBox::clicked, whichEmailWindow, [&keepAsking] (bool checked) {keepAsking = !checked;});
                whichEmailWindow->button(QMessageBox::Ok)->setText(tr("Use survey email address"));
                connect(whichEmailWindow->button(QMessageBox::Ok), &QPushButton::clicked, whichEmailWindow, &QDialog::accept);
                whichEmailWindow->button(QMessageBox::Cancel)->setText(tr("Use roster email address"));
                connect(whichEmailWindow->button(QMessageBox::Cancel), &QPushButton::clicked, whichEmailWindow, &QDialog::reject);

                if(whichEmailWindow->exec() == QDialog::Rejected)
                {
                    dataHasChanged = true;
                    makeTheChange = true;
                    students[studentNum].email = emails.at(names.indexOf(surveyName));
                    students[studentNum].createTooltip(dataOptions);
                }
                else
                {
                    makeTheChange = false;
                }
                delete whichEmailWindow;
            }
            else if(makeTheChange)
            {
                students[studentNum].email = emails.at(names.indexOf(surveyName));
                students[studentNum].createTooltip(dataOptions);
            }
            i++;
        }

        // Finally, handle the names on the survey that were not found in the roster
        keepAsking = true, makeTheChange = false;
        i = 0;
        for(auto &name : namesNotFound)
        {
            if(keepAsking)
            {
                auto *keepOrDeleteWindow = new QMessageBox(QMessageBox::Question, tr("Student not in roster file"),
                                                           tr("This student:") +
                                                               "<br><b>" + name + "</b><br>" +
                                                               tr("submitted a survey but was not found in the roster file.") + "<br><br>" +
                                                               tr("Should we keep this student or remove them?"),
                                                           QMessageBox::Ok | QMessageBox::Cancel, this);
                keepOrDeleteWindow->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                                      Qt::KeepAspectRatio, Qt::SmoothTransformation));
                keepOrDeleteWindow->setStyleSheet(LABELSTYLE);
                keepOrDeleteWindow->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
                keepOrDeleteWindow->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
                auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(namesNotFound.size() - i) + tr(" students)"));
                applyToAll->setStyleSheet(CHECKBOXSTYLE);
                keepOrDeleteWindow->setCheckBox(applyToAll);
                connect(applyToAll, &QCheckBox::clicked, keepOrDeleteWindow, [&keepAsking] (bool checked) {keepAsking = !checked;});
                keepOrDeleteWindow->button(QMessageBox::Ok)->setText(tr("Keep ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Ok), &QPushButton::clicked, keepOrDeleteWindow, &QDialog::accept);
                keepOrDeleteWindow->button(QMessageBox::Cancel)->setText(tr("Remove ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Cancel), &QPushButton::clicked, keepOrDeleteWindow, &QDialog::reject);

                if(keepOrDeleteWindow->exec() == QDialog::Rejected)
                {
                    dataHasChanged = true;
                    makeTheChange = true;
                    removeAStudent(name, true);
                }
                else
                {
                    makeTheChange = false;
                }

                delete keepOrDeleteWindow;
            }
            else if(makeTheChange)
            {
                removeAStudent(name, true);
            }
            i++;
        }

        if(dataHasChanged)
        {
            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
        }
    }
    rosterFile.close();
}


void gruepr::rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable()
{
    // go back through all records to see if any are duplicates; assume each isn't and then check
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        students[index].duplicateRecord = false;
        for(int index2 = 0; index2 < index; index2++)
        {
            if((students[index].firstname + students[index].lastname == students[index2].firstname + students[index2].lastname) ||
                    ((students[index].email == students[index2].email) && !students[index].email.isEmpty()))
            {
                students[index].duplicateRecord = true;
                students[index2].duplicateRecord = true;
                students[index2].createTooltip(dataOptions);
            }
        }
        students[index].createTooltip(dataOptions);
    }

    // Re-build the URM info
    if(dataOptions->URMIncluded)
    {
        dataOptions->URMResponses.clear();
        for(int URMindex = 0; URMindex < dataOptions->numStudentsInSystem; URMindex++)
        {
            if(!dataOptions->URMResponses.contains(students[URMindex].URMResponse, Qt::CaseInsensitive))
            {
                dataOptions->URMResponses << students[URMindex].URMResponse;
            }
        }
        QCollator sortAlphanumerically;
        sortAlphanumerically.setNumericMode(true);
        sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(dataOptions->URMResponses.begin(), dataOptions->URMResponses.end(), sortAlphanumerically);
        if(dataOptions->URMResponses.contains("--"))
        {
            // put the blank response option at the end of the list
            dataOptions->URMResponses.removeAll("--");
            dataOptions->URMResponses << "--";
        }
    }

    // Re-build the section options in the selection box
    if(dataOptions->sectionIncluded)
    {
        QString currentSection = ui->sectionSelectionBox->currentText();
        ui->sectionSelectionBox->clear();
        dataOptions->sectionNames.clear();
        for(int sectionIndex = 0; sectionIndex < dataOptions->numStudentsInSystem; sectionIndex++)
        {
            if(!dataOptions->sectionNames.contains(students[sectionIndex].section, Qt::CaseInsensitive))
            {
                dataOptions->sectionNames << students[sectionIndex].section;
            }
        }
        if(dataOptions->sectionNames.size() > 1)
        {
            QCollator sortAlphanumerically;
            sortAlphanumerically.setNumericMode(true);
            sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);
            ui->sectionSelectionBox->addItem(tr("Students in all sections together"));
            ui->sectionSelectionBox->addItem(tr("Students in all sections, each section separately"));
            ui->sectionSelectionBox->insertSeparator(2);
            ui->sectionSelectionBox->addItems(dataOptions->sectionNames);
        }
        else
        {
            ui->sectionSelectionBox->addItem(tr("Only one section in the data."));
        }

        if(ui->sectionSelectionBox->findText(currentSection) != -1)
        {
            ui->sectionSelectionBox->setCurrentText(currentSection);
        }
    }

    // Refresh student table data
    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();

    // Load new team sizes in selection box
    ui->idealTeamSizeBox->setMaximum(std::max(2,numActiveStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());
}


void gruepr::on_saveSurveyFilePushButton_clicked()
{
    QSettings savedSettings;
    CsvFile newSurveyFile;
    if(!newSurveyFile.open(this, CsvFile::write, tr("Save Survey Data File"), savedSettings.value("saveFileLocation", "").toString(), tr("Survey Data")))
    {
        return;
    }

    // write header
    if(dataOptions->timestampField != -1)
    {
        newSurveyFile.headerValues << "Timestamp";
    }
    if(dataOptions->LMSIDField != -1)
    {
        newSurveyFile.headerValues << "LMSID";
    }
    if(dataOptions->firstNameField != -1)
    {
     newSurveyFile.headerValues << "What is your first name (or the name you prefer to be called)?";
    }
    if(dataOptions->lastNameField != -1)
    {
        newSurveyFile.headerValues << "What is your last name?";
    }
    if(dataOptions->emailField != -1)
    {
        newSurveyFile.headerValues << "What is your email address?";
    }
    if(dataOptions->genderIncluded)
    {
        if(dataOptions->genderType == GenderType::pronoun)
        {
            newSurveyFile.headerValues << "What are your pronouns?";
        }
        else
        {
            newSurveyFile.headerValues << "With which gender do you identify most closely?";
        }
    }
    if(dataOptions->URMIncluded)
    {
        newSurveyFile.headerValues << "How do you identify your race, ethnicity, or cultural heritage?";
    }
    for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
    {
        newSurveyFile.headerValues << dataOptions->attributeQuestionText[attrib];
    }
    for(const auto &dayName : dataOptions->dayNames)
    {
        if(dataOptions->scheduleDataIsFreetime)
        {
            newSurveyFile.headerValues << "Check the times that you are FREE and will be AVAILABLE for group work. [" + dayName + "]";
        }
        else
        {
            newSurveyFile.headerValues << "Check the times that you are BUSY and will be UNAVAILABLE for group work. [" + dayName + "]";
        }
    }
    if(dataOptions->sectionIncluded)
    {
        newSurveyFile.headerValues << "In which section are you enrolled?";
    }
    if(dataOptions->prefTeammatesIncluded)
    {
        newSurveyFile.headerValues << "Please list name(s) of people you would like to have on your team";
    }
    if(dataOptions->prefNonTeammatesIncluded)
    {
        newSurveyFile.headerValues << "Please list name(s) of people you would like to not have on your team";
    }
    if(dataOptions->numNotes > 0)
    {
        newSurveyFile.headerValues << "Notes";
    }
    if(!newSurveyFile.writeHeader())
    {
        grueprGlobal::errorMessage(this, tr("No File Saved"), tr("No file was saved.\nThere was an issue writing the file."));
        return;
    }

    // Write each student's info
    for(const auto &student : students) {
        newSurveyFile.fieldValues.clear();
        newSurveyFile.fieldValues << student.surveyTimestamp.toString(Qt::ISODate);
        if(dataOptions->LMSIDField != -1)
        {
            newSurveyFile.fieldValues << QString::number(student.LMSID);
        }
        if(dataOptions->firstNameField != -1)
        {
            newSurveyFile.fieldValues << student.firstname;
        }
        if(dataOptions->lastNameField != -1)
        {
            newSurveyFile.fieldValues << student.lastname;
        }
        if(dataOptions->emailField != -1)
        {
            newSurveyFile.fieldValues << student.email;
        }
        if(dataOptions->genderIncluded)
        {
            QStringList genderOptions;
            if(dataOptions->genderType == GenderType::biol)
            {
                genderOptions = QString(BIOLGENDERS).split('/');
            }
            else if(dataOptions->genderType == GenderType::adult)
            {
                genderOptions = QString(ADULTGENDERS).split('/');
            }
            else if(dataOptions->genderType == GenderType::child)
            {
                genderOptions = QString(CHILDGENDERS).split('/');
            }
            else //if(dataOptions->genderResponses == GenderType::pronoun)
            {
                genderOptions = QString(PRONOUNS).split('/');
            }
            newSurveyFile.fieldValues << genderOptions.at(static_cast<int>(student.gender));
        }
        if(dataOptions->URMIncluded)
        {
            newSurveyFile.fieldValues << (student.URMResponse);
        }
        for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
        {
            newSurveyFile.fieldValues << student.attributeResponse[attrib];
        }
        for(int day = 0; day < dataOptions->dayNames.size(); day++)
        {
            QString times;
            bool first = true;
            for(int time = 0; time < dataOptions->timeNames.size(); time++)
            {
                if(dataOptions->scheduleDataIsFreetime)
                {
                    if(!student.unavailable[day][time])
                    {
                        if(!first)
                        {
                            times += ';';
                        }
                        first = false;
                        times += dataOptions->timeNames.at(time);
                    }
                }
                else
                {
                    if(student.unavailable[day][time])
                    {
                        if(!first)
                        {
                            times += ';';
                        }
                        first = false;
                        times += dataOptions->timeNames.at(time);
                    }
                }
            }
            newSurveyFile.fieldValues << times;
        }
        if(dataOptions->sectionIncluded)
        {
            newSurveyFile.fieldValues << student.section;
        }
        if(dataOptions->prefTeammatesIncluded)
        {
            QString list = student.prefTeammates;
            newSurveyFile.fieldValues << list.replace('\n',";");
        }
        if(dataOptions->prefNonTeammatesIncluded)
        {
            QString list = student.prefNonTeammates;
            newSurveyFile.fieldValues << list.replace('\n',";");
        }
        if(dataOptions->numNotes > 0)
        {
            newSurveyFile.fieldValues << student.notes;
        }
        newSurveyFile.writeDataRow();
    }

    newSurveyFile.close();
}


void gruepr::simpleUIItemUpdate(QObject *sender)
{
    teamingOptions->isolatedWomenPrevented = (ui->isolatedWomenCheckBox->isChecked());

    teamingOptions->isolatedMenPrevented = (ui->isolatedMenCheckBox->isChecked());

    teamingOptions->isolatedNonbinaryPrevented = (ui->isolatedNonbinaryCheckBox->isChecked());

    teamingOptions->singleGenderPrevented = (ui->mixedGenderCheckBox->isChecked());

    teamingOptions->isolatedURMPrevented = (ui->isolatedURMCheckBox->isChecked());
    ui->URMResponsesButton->setEnabled(teamingOptions->isolatedURMPrevented);
    if(sender == ui->isolatedURMCheckBox) {
        if(teamingOptions->isolatedURMPrevented && teamingOptions->URMResponsesConsideredUR.isEmpty()) {
            // if we are just now preventing isolated URM students, but have not selected yet which responses should be considered URM, let's ask user to enter those in
            on_URMResponsesButton_clicked();
        }
    }

    teamingOptions->minTimeBlocksOverlap = (ui->minMeetingTimes->value());
    if(sender == ui->minMeetingTimes) {
        if(ui->desiredMeetingTimes->value() < (ui->minMeetingTimes->value())) {
            ui->desiredMeetingTimes->setValue(ui->minMeetingTimes->value());
        }
    }

    teamingOptions->desiredTimeBlocksOverlap = (ui->desiredMeetingTimes->value());
    if(sender == ui->desiredMeetingTimes) {
        if(ui->minMeetingTimes->value() > (ui->desiredMeetingTimes->value())) {
            ui->minMeetingTimes->setValue(ui->desiredMeetingTimes->value());
        }
    }

    teamingOptions->meetingBlockSize = (ui->meetingLengthSpinBox->value());
    if(sender == ui->meetingLengthSpinBox) {
        ui->meetingLengthSpinBox->setSuffix(ui->meetingLengthSpinBox->value() > 1? tr(" hours") : tr(" hour"));
        if((dataOptions->timeNames.size() * dataOptions->dayNames.size() != 0)) {
            ui->minMeetingTimes->setMaximum(int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLengthSpinBox->value()));
            ui->desiredMeetingTimes->setMaximum(int(dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLengthSpinBox->value()));
        }
    }

    teamingOptions->scheduleWeight = float(ui->scheduleWeight->value());
}


void gruepr::on_URMResponsesButton_clicked()
{
    // open window to decide which values are to be considered underrepresented
    auto *win = new gatherURMResponsesDialog(dataOptions->URMResponses, teamingOptions->URMResponsesConsideredUR, this);

    //If user clicks OK, replace the responses considered underrepresented with the set from the window
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        teamingOptions->URMResponsesConsideredUR = win->URMResponsesConsideredUR;
        teamingOptions->URMResponsesConsideredUR.removeDuplicates();
        //(re)apply these values to the student database
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            students[index].URM = teamingOptions->URMResponsesConsideredUR.contains(students[index].URMResponse);
        }
    }

    delete win;
}


void gruepr::responsesRulesButton_clicked()
{
    const int currAttribute = ui->attributesStackedWidget->currentIndex();
    //Open specialized dialog box to collect attribute values that are required on each team
    auto *win = new AttributeRulesDialog(currAttribute, *dataOptions, *teamingOptions, this);

    //If user clicks OK, replace with new sets of values
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        teamingOptions->haveAnyRequiredAttributes[currAttribute] = !(win->requiredValues.isEmpty());
        teamingOptions->requiredAttributeValues[currAttribute] = win->requiredValues;

        teamingOptions->haveAnyIncompatibleAttributes[currAttribute] = !(win->incompatibleValues.isEmpty());
        teamingOptions->incompatibleAttributeValues[currAttribute] = win->incompatibleValues;
    }

    delete win;
}


void gruepr::on_teammatesButton_clicked()
{
    QStringList teamTabNames;
    for(int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++)
    {
        teamTabNames << ui->dataDisplayTabWidget->tabText(tab);
    }

    auto *win = new TeammatesRulesDialog(students, *dataOptions, *teamingOptions,
                                         ((ui->sectionSelectionBox->currentIndex()==0) || (ui->sectionSelectionBox->currentIndex()==1))? "" : teamingOptions->sectionName,
                                         teamTabNames, this);
    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            this->students[index] = win->students[index];
        }
        teamingOptions->haveAnyRequiredTeammates = win->required_teammatesSpecified;
        teamingOptions->haveAnyPreventedTeammates = win->prevented_teammatesSpecified;
        teamingOptions->haveAnyRequestedTeammates = win->requested_teammatesSpecified;
        teamingOptions->numberRequestedTeammatesGiven = win->numberRequestedTeammatesGiven;
    }

    delete win;
}


void gruepr::on_idealTeamSizeBox_valueChanged(int arg1)
{
    if(teamingOptions->numberRequestedTeammatesGiven > arg1) {
        teamingOptions->numberRequestedTeammatesGiven = arg1;
    }

    // put suitable options in the team size selection box, depending on whether the number of students is evenly divisible by this desired team size
    ui->teamSizeBox->setUpdatesEnabled(false);

    // typically just figuring out team sizes for one section or for all students together, but need to re-calculate for each section if we will team all sections independently
    const int numSectionsToCalculate = (ui->sectionSelectionBox->currentIndex() == 1? int(dataOptions->sectionNames.size()) : 1);
    int numStudentsBeingTeamed = numActiveStudents;
    int smallerTeamsSizeA=0, smallerTeamsSizeB=0, numSmallerATeams=0, largerTeamsSizeA=0, largerTeamsSizeB=0, numLargerATeams=0;
    int cumNumSmallerATeams=0, cumNumSmallerBTeams = 0, cumNumLargerATeams=0, cumNumLargerBTeams = 0;
    for(int section = 0; section < numSectionsToCalculate; section++)
    {
        ui->teamSizeBox->clear();

        if(ui->sectionSelectionBox->currentIndex() == 1)
        {
            // if teaming all sections separately, figure out how many students in this section
            const QString sectionName = ui->sectionSelectionBox->itemText(section + 3);
            numStudentsBeingTeamed = 0;
            for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
            {
                if(students[index].section == sectionName)
                {
                    numStudentsBeingTeamed++;
                }
            }
        }
        else
        {
            numStudentsBeingTeamed = numActiveStudents;
        }

        teamingOptions->numTeamsDesired = std::max(1, numStudentsBeingTeamed/arg1);
        teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired;
        teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;

        if(numStudentsBeingTeamed%arg1 != 0)       //if teams can't be evenly divided into this size
        {

            // reset the potential team sizes
            for(int student = 0; student < MAX_STUDENTS; student++)
            {
                teamingOptions->smallerTeamsSizes[student] = 0;
                teamingOptions->largerTeamsSizes[student] = 0;
            }

            // What are the team sizes when desiredTeamSize represents a maximum size?
            teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired+1;
            for(int student = 0; student < numStudentsBeingTeamed; student++)      // run through every student
            {
                // add one student to each team (with 1 additional team relative to before) in turn until we run out of students
                (teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams])++;
                smallerTeamsSizeA = teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams];  // the larger of the two (uneven) team sizes
                numSmallerATeams = (student%teamingOptions->smallerTeamsNumTeams)+1;                                 // the number of larger teams
            }
            smallerTeamsSizeB = smallerTeamsSizeA - 1;                  // the smaller of the two (uneven) team sizes

            // And what are the team sizes when desiredTeamSize represents a minimum size?
            teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;
            for(int student = 0; student < numStudentsBeingTeamed; student++)	// run through every student
            {
                // add one student to each team in turn until we run out of students
                (teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams])++;
                largerTeamsSizeA = teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams];     // the larger of the two (uneven) team sizes
                numLargerATeams = (student%teamingOptions->largerTeamsNumTeams)+1;                                   // the number of larger teams
            }
            largerTeamsSizeB = largerTeamsSizeA - 1;					// the smaller of the two (uneven) team sizes

            // Add first option to selection box
            QString smallerTeamOption = writeTeamSizeOption(numSmallerATeams, smallerTeamsSizeA, teamingOptions->numTeamsDesired+1-numSmallerATeams, smallerTeamsSizeB);
            if(numSmallerATeams > 0)
            {
                cumNumSmallerATeams += numSmallerATeams;
            }
            if((teamingOptions->numTeamsDesired+1-numSmallerATeams) > 0)
            {
                cumNumSmallerBTeams += teamingOptions->numTeamsDesired+1-numSmallerATeams;
            }

            // Add second option to selection box
            QString largerTeamOption = writeTeamSizeOption(teamingOptions->numTeamsDesired-numLargerATeams, largerTeamsSizeB, numLargerATeams, largerTeamsSizeA);
            if((teamingOptions->numTeamsDesired-numLargerATeams) > 0)
            {
                cumNumLargerBTeams += teamingOptions->numTeamsDesired-numLargerATeams;
            }
            if(numLargerATeams > 0)
            {
                cumNumLargerATeams += numLargerATeams;
            }

            if(ui->sectionSelectionBox->currentIndex() != 1)
            {
                ui->teamSizeBox->addItem(smallerTeamOption);
                ui->teamSizeBox->addItem(largerTeamOption);
            }
        }
        else
        {
            cumNumSmallerATeams += teamingOptions->numTeamsDesired;
            smallerTeamsSizeA = arg1;
            cumNumLargerBTeams += teamingOptions->numTeamsDesired;
            largerTeamsSizeB = arg1;

            if(ui->sectionSelectionBox->currentIndex() != 1)
            {
                ui->teamSizeBox->addItem(QString::number(teamingOptions->numTeamsDesired) + tr(" teams (") + QString::number(arg1) + tr(" students each)"));
            }
        }
    }

    if(ui->sectionSelectionBox->currentIndex() == 1)
    {
        // load new team sizes in selection box by adding together the sizes from each section
        QString smallerTeamOption = writeTeamSizeOption(cumNumSmallerATeams, smallerTeamsSizeA, cumNumSmallerBTeams, smallerTeamsSizeB);
        QString largerTeamOption = writeTeamSizeOption(cumNumLargerBTeams, largerTeamsSizeB, cumNumLargerATeams, largerTeamsSizeA);

        ui->teamSizeBox->addItem(smallerTeamOption);
        if(smallerTeamOption != largerTeamOption)
        {
            ui->teamSizeBox->addItem(largerTeamOption);
        }
    }
    else
    {
        // allow custom team sizes (too complicated to allow this if teaming all sections separately
        ui->teamSizeBox->insertSeparator(ui->teamSizeBox->count());
        ui->teamSizeBox->addItem(tr("Custom team sizes"));
    }

    // if we have fewer than 4 students somehow, disable the form teams button
    ui->letsDoItButton->setEnabled(numStudentsBeingTeamed >= 4);

    ui->teamSizeBox->setUpdatesEnabled(true);
}


inline QString gruepr::writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB)
{
    QString teamOption = QString::number(numTeamsA + numTeamsB) + ((numTeamsA + numTeamsB > 1)? tr(" teams") : tr(" team")) + " (";
    if(numTeamsA > 0)
    {
        teamOption += QString::number(numTeamsA) + tr(" of ") + QString::number(teamsizeA) + tr(" student");
        if(teamsizeA > 1)
        {
            teamOption += "s";
        }
    }
    if((numTeamsA > 0) && (numTeamsB > 0))
    {
        teamOption += ";  ";
    }
    if(numTeamsB > 0)
    {
        teamOption += QString::number(numTeamsB) + " of " + QString::number(teamsizeB) + tr(" student");
        if(teamsizeB > 1)
        {
            teamOption += "s";
        }
    }
    teamOption += ")";

    return teamOption;
}


void gruepr::on_teamSizeBox_currentIndexChanged(int index)
{
    if(ui->teamSizeBox->currentText() == QString::number(teamingOptions->numTeamsDesired) + tr(" teams (") + QString::number(ui->idealTeamSizeBox->value()) + tr(" students each)"))
    {
        // Evenly divisible teams, all same size
        setTeamSizes(ui->idealTeamSizeBox->value());
    }
    else if(ui->teamSizeBox->currentText() == tr("Custom team sizes"))
    {
        //Open specialized dialog box to collect teamsizes
        auto *win = new customTeamsizesDialog(numActiveStudents, ui->idealTeamSizeBox->value(), this);

        //If user clicks OK, use these team sizes, otherwise revert to option 1, smaller team sizes
        int reply = win->exec();
        if(reply == QDialog::Accepted)
        {
            teamingOptions->numTeamsDesired = win->numTeams;
            setTeamSizes(win->teamsizes.constData());
        }
        else
        {
            // Set to index 0 if cancelled
            ui->teamSizeBox->setCurrentIndex(0);
        }
        delete win;
        return;
    }
    else if(index == 0)
    {
        // Smaller teams desired
        teamingOptions->numTeamsDesired = teamingOptions->smallerTeamsNumTeams;
        setTeamSizes(teamingOptions->smallerTeamsSizes);
    }
    else if (index == 1)
    {
        // Larger teams desired
        teamingOptions->numTeamsDesired = teamingOptions->largerTeamsNumTeams;
        setTeamSizes(teamingOptions->largerTeamsSizes);
    }
}


void gruepr::on_letsDoItButton_clicked()
{
    // User wants to not isolate URM, but has not indicated any responses to be considered underrepresented
    if(dataOptions->URMIncluded && teamingOptions->isolatedURMPrevented && teamingOptions->URMResponsesConsideredUR.isEmpty())
    {
        bool okContinue = grueprGlobal::warningMessage(this, tr("gruepr"),
                                                        tr("You have selected to prevented isolated URM students, "
                                                           "however none of the race/ethnicity response values "
                                                           "have been selected to be considered as underrepresented.\n\n"
                                                           "Click Continue to form teams with no students considered URM, "
                                                           "or click Open Selection Window to select the URM responses."),
                                                        tr("Continue"), tr("Open Selection Window"));
        if(!okContinue)
        {
            ui->URMResponsesButton->animateClick();
            return;
        }
    }

    // Normalize all score factor weights using norm factor = number of factors / total weights of all factors
    teamingOptions->realNumScoringFactors = dataOptions->numAttributes + (dataOptions->dayNames.isEmpty()? 0 : 1);
    float normFactor = (float(teamingOptions->realNumScoringFactors)) /
            (std::accumulate(teamingOptions->attributeWeights, teamingOptions->attributeWeights + dataOptions->numAttributes, 0.0f) +
             (dataOptions->dayNames.isEmpty()? 0 : teamingOptions->scheduleWeight));
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        teamingOptions->realAttributeWeights[attribute] = teamingOptions->attributeWeights[attribute] * normFactor;
    }
    teamingOptions->realScheduleWeight = (dataOptions->dayNames.isEmpty()? 0 : teamingOptions->scheduleWeight) * normFactor;

    bestTeamSet.clear();
    finalTeams.clear();

    const bool teamingMultipleSections = (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately);
    multipleSectionsInProgress = teamingMultipleSections;
    const int numSectionsToTeam = (teamingMultipleSections? int(dataOptions->sectionNames.size()) : 1);
    const int teamSizeSelector = ui->teamSizeBox->currentIndex();
    for(int section = 0; section < numSectionsToTeam; section++)
    {
        if(teamingMultipleSections)
        {
            // team each section one at a time by changing the section and teamsize selection boxes
            ui->sectionSelectionBox->setCurrentIndex(section + 3);  // go to the next section (index: 0 = allTogether, 1 = allSeparately, 2 = separator line, 3 = first section)
            ui->teamSizeBox->setCurrentIndex(teamSizeSelector);     // pick the correct team sizes
        }

        // Create a new set of TeamRecords to hold the eventual results
        numTeams = teamingOptions->numTeamsDesired;
        teams.clear();
        teams.reserve(numTeams);
        for(int team = 0; team < numTeams; team++)	// run through every team to load dataOptions and size
        {
            teams << TeamRecord(dataOptions, teamingOptions->teamSizesDesired[team]);
        }

        // Create progress display plot
        progressChart = new BoxWhiskerPlot("", "Generation", "Scores");
        auto *chartView = new QChartView(progressChart);
        chartView->setRenderHint(QPainter::Antialiasing);

        // Create window to display progress, and connect the stop optimization button in the window to the actual stopping of the optimization thread
        QString sectionName = (teamingMultipleSections? (tr("section ") + QString::number(section + 1) + " / " + QString::number(numSectionsToTeam) + ": " + ui->sectionSelectionBox->currentText()) : "");
        progressWindow = new progressDialog(sectionName, chartView, this);
        progressWindow->show();
        connect(progressWindow, &progressDialog::letsStop, this, [this] {QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
                                                                         connect(this, &gruepr::turnOffBusyCursor, this, &QApplication::restoreOverrideCursor);
                                                                         optimizationStoppedmutex.lock();
                                                                         optimizationStopped = true;
                                                                         optimizationStoppedmutex.unlock();});

        // Get the IDs of students from desired section and change numStudents accordingly
        int numStudentsInSection = 0;
        studentIndexes = new int[dataOptions->numStudentsInSystem];
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            if(ui->sectionSelectionBox->currentIndex() == 0 || ui->sectionSelectionBox->currentText() == students[index].section)
            {
                studentIndexes[numStudentsInSection] = index;
                numStudentsInSection++;
            }
        }
        numActiveStudents = numStudentsInSection;

        // Set up the flag to allow a stoppage and set up futureWatcher to know when results are available
        optimizationStopped = false;
        future = QtConcurrent::run(&gruepr::optimizeTeams, this, studentIndexes);       // spin optimization off into a separate thread
        futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
        multipleSectionsInProgress = (section < (numSectionsToTeam - 1));

        // hold here until the optimization is done. This feels really hacky and probably can be improved with something simple!
        QEventLoop loop;
        connect(this, &gruepr::sectionOptimizationFullyComplete, this, [this, &loop, teamingMultipleSections, teamSizeSelector]
                                                                       { if(teamingMultipleSections && !multipleSectionsInProgress)
                                                                         {
                                                                            ui->sectionSelectionBox->setCurrentIndex(1);            // go back to each section separately
                                                                            ui->teamSizeBox->setCurrentIndex(teamSizeSelector);     // pick the correct team sizes
                                                                         }
                                                                         this->disconnect(SIGNAL(sectionOptimizationFullyComplete()));
                                                                         loop.quit();
                                                                       });
        loop.exec();
    }
}


void gruepr::updateOptimizationProgress(const QList<float> &allScores, const int *const orderedIndex, const int generation, const float scoreStability, const bool unpenalizedGenomePresent)
{
    if((generation % (BoxWhiskerPlot::PLOTFREQUENCY)) == 0)
    {
        progressChart->loadNextVals(allScores, orderedIndex, unpenalizedGenomePresent);
    }

    if(generation > GA::MAX_GENERATIONS)
    {
        progressWindow->setText(tr("We have reached ") + QString::number(GA::MAX_GENERATIONS) + tr(" generations."),
                                generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), true);
        progressWindow->highlightStopButton();
    }
    else if( (generation >= GA::MIN_GENERATIONS) && (scoreStability > GA::MIN_SCORE_STABILITY) )
    {
        progressWindow->setText(tr("Score appears to be stable!"), generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), true);
        progressWindow->highlightStopButton();
    }
    else
    {
        progressWindow->setText(tr("Please wait while your grueps are created!"), generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), false);
    }
}


void gruepr::optimizationComplete()
{
    // update UI
    delete progressChart;
    delete progressWindow;

    // free memory used to save array of IDs of students being teamed
    delete[] studentIndexes;

    // Get the results
    bestTeamSet << future.result();
    finalTeams << teams;

    emit sectionOptimizationFullyComplete();

    if(multipleSectionsInProgress)
    {
        return;
    }

    //alert
    QApplication::beep();
    QApplication::alert(this);

    // Load students into teams
    teams = finalTeams;
    int indexInTeamset = 0;
    for(auto &team : teams)
    {
        auto &IndexList = team.studentIndexes;
        IndexList.clear();
        for(int studentNum = 0, size = team.size; studentNum < size; studentNum++)
        {
            IndexList << bestTeamSet.at(indexInTeamset);
            indexInTeamset++;
        }
        //sort teammates within a team alphabetically by lastname,firstname
        std::sort(IndexList.begin(), IndexList.end(), [this] (const int a, const int b)
                                                        {return ((students[a].lastname + students[a].firstname) < (students[b].lastname + students[b].firstname));});
    }

    // Load scores and info into the teams
    updateTeamScores(students.constData(), numActiveStudents, teams.data(), int(teams.size()), teamingOptions, dataOptions);
    for(auto &team:teams)
    {
        team.refreshTeamInfo(students.constData());
    }

    // Sort teams by 1st student's name, then set default teamnames and create tooltips
    std::sort(teams.begin(), teams.end(), [this](const TeamRecord &a, const TeamRecord &b)
                                                 {return ((students[a.studentIndexes.at(0)].lastname + students[a.studentIndexes.at(0)].firstname) <
                                                          (students[b.studentIndexes.at(0)].lastname + students[b.studentIndexes.at(0)].firstname));});
    for(int team = 0; team < teams.size(); team++)
    {
        teams[team].name = QString::number(team+1);
        teams[team].createTooltip();
    }

    // Display the results in a new tab
    // Eventually maybe this should let the tab take ownership of the teams pointer, deleting when the tab is closed!
    QString teamSetName = tr("Team set ") + QString::number(teamingOptions->teamsetNumber);
    auto *teamTab = new TeamsTabItem(*teamingOptions, *dataOptions, teams, students, teamSetName, ui->letsDoItButton, this);
    ui->dataDisplayTabWidget->addTab(teamTab, teamSetName);
    numTeams = int(teams.size());
    teamingOptions->teamsetNumber++;

    ui->dataDisplayTabWidget->setCurrentWidget(teamTab);
}


void gruepr::dataDisplayTabClose(int closingTabIndex)
{
    // don't close the student tab!
    if(closingTabIndex < 1)
    {
        return;
    }

    auto *tab = ui->dataDisplayTabWidget->widget(closingTabIndex);
    ui->dataDisplayTabWidget->removeTab(closingTabIndex);
    tab->deleteLater();
}


void gruepr::editDataDisplayTabName(int tabIndex)
{
    // don't do anything if they double-clicked on the student tab
    if(tabIndex < 1)
    {
        return;
    }

    // pop up at the cursor location a little window to edit the tab title
    auto *win = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    win->setWindowTitle(tr("Rename this team set"));
    win->setSizeGripEnabled(true);
    win->move(QCursor::pos());
    auto *layout = new QVBoxLayout(win);
    auto *newNameEditor = new QLineEdit(win);
    newNameEditor->setStyleSheet(LINEEDITSTYLE);
    newNameEditor->setText(ui->dataDisplayTabWidget->tabText(tabIndex));
    newNameEditor->setPlaceholderText(ui->dataDisplayTabWidget->tabText(tabIndex));
    layout->addWidget(newNameEditor);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, win);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, win, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, win, &QDialog::reject);
    layout->addWidget(buttonBox);
    newNameEditor->selectAll();
    if(win->exec() == QDialog::Accepted && !newNameEditor->text().isEmpty())
    {
        ui->dataDisplayTabWidget->setTabText(tabIndex, newNameEditor->text());
        auto *tab = qobject_cast<TeamsTabItem *>(ui->dataDisplayTabWidget->widget(tabIndex));
        tab->tabName = newNameEditor->text();
    }
    win->deleteLater();
}


//////////////////
//Load window geometry and default teaming options saved from previous run. If non-existant, load app defaults.
//////////////////
void gruepr::loadDefaultSettings()
{
    QSettings savedSettings;

    //Restore teaming options
    ui->idealTeamSizeBox->setValue(savedSettings.value("idealTeamSize", 4).toInt());
    teamingOptions->isolatedWomenPrevented = savedSettings.value("isolatedWomenPrevented", false).toBool();
    teamingOptions->isolatedMenPrevented = savedSettings.value("isolatedMenPrevented", false).toBool();
    teamingOptions->isolatedNonbinaryPrevented = savedSettings.value("isolatedNonbinaryPrevented", false).toBool();
    teamingOptions->singleGenderPrevented = savedSettings.value("singleGenderPrevented", false).toBool();
    teamingOptions->isolatedURMPrevented = savedSettings.value("isolatedURMPrevented", false).toBool();
    teamingOptions->minTimeBlocksOverlap = savedSettings.value("minTimeBlocksOverlap", 4).toInt();
    teamingOptions->desiredTimeBlocksOverlap = savedSettings.value("desiredTimeBlocksOverlap", 8).toInt();
    teamingOptions->meetingBlockSize = savedSettings.value("meetingBlockSize", 1).toInt();
    teamingOptions->scheduleWeight = savedSettings.value("scheduleWeight", 4).toFloat();
    savedSettings.beginReadArray("Attributes");
    for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum) {
        savedSettings.setArrayIndex(attribNum);
        teamingOptions->desireHomogeneous[attribNum] = savedSettings.value("desireHomogeneous", false).toBool();
        teamingOptions->attributeWeights[attribNum] = savedSettings.value("Weight", 1).toFloat();
        // Shouldn't re-load the incompatible and required responses if working with new survey data
        if(dataOptions->dataSource == DataOptions::fromPrevWork) {
            int numIncompats = savedSettings.beginReadArray("incompatibleResponses");
            for(int incompResp = 0; incompResp < numIncompats; incompResp++) {
                savedSettings.setArrayIndex(incompResp);
                QStringList incompats = savedSettings.value("incompatibleResponses", "").toString().split(',');
                teamingOptions->incompatibleAttributeValues[attribNum] << QPair<int,int>(incompats.at(0).toInt(),incompats.at(1).toInt());
            }
            savedSettings.endArray();
            int numRequireds = savedSettings.beginReadArray("requiredResponses");
            for(int requiredResp = 0; requiredResp < numRequireds; requiredResp++) {
                savedSettings.setArrayIndex(requiredResp);
                int required = savedSettings.value("requiredResponse", "").toInt();
                teamingOptions->requiredAttributeValues[attribNum] << required;
            }
            savedSettings.endArray();
        }
    }
    savedSettings.endArray();
    teamingOptions->numberRequestedTeammatesGiven = savedSettings.value("requestedTeammateNumber", 1).toInt();
}


//////////////////
//Enable the appropriate UI settings when loading a set of students
//////////////////
void gruepr::loadUI()
{
    //Restore window geometry
    QSettings savedSettings;
    restoreGeometry(savedSettings.value("windowGeometry").toByteArray());

    ui->dataSourceLabel->setText(dataOptions->dataSourceName);
    if(dataOptions->dataSource == DataOptions::fromGoogle) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/google.png"));
    }
    else if(dataOptions->dataSource == DataOptions::fromCanvas) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/canvas.png"));
    }
    else if(dataOptions->dataSource == DataOptions::fromPrevWork) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/icon.svg"));
    }

    ui->sectionSelectionBox->blockSignals(true);
    if(dataOptions->sectionIncluded) {
        if(dataOptions->sectionNames.size() > 1) {
            ui->sectionSelectionBox->addItem(tr("Students in all sections together"));
            ui->sectionSelectionBox->addItem(tr("Students in all sections, each section separately"));
            ui->sectionSelectionBox->insertSeparator(2);
            ui->sectionSelectionBox->addItems(dataOptions->sectionNames);
            ui->sectionSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        else {
            ui->sectionSelectionBox->addItem(tr("Only one section in the data."));
            ui->sectionFrame->hide();
            ui->sectionSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
    }
    else {
        ui->sectionSelectionBox->addItem(tr("No section data."));
        ui->sectionFrame->hide();
        ui->sectionSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        teamingOptions->sectionType = TeamingOptions::SectionType::noSections;
    }
    teamingOptions->sectionName = ui->sectionSelectionBox->currentText();
    ui->sectionSelectionBox->blockSignals(false);

    refreshStudentDisplay();
    ui->studentTable->resetTable();

    ui->idealTeamSizeBox->setMaximum(std::max(2,numActiveStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box
    ui->teamsizeSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);

    if(dataOptions->genderIncluded) {
        ui->genderSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->isolatedWomenCheckBox->setChecked(teamingOptions->isolatedWomenPrevented);
        ui->isolatedMenCheckBox->setChecked(teamingOptions->isolatedMenPrevented);
        ui->isolatedNonbinaryCheckBox->setChecked(teamingOptions->isolatedNonbinaryPrevented);
        ui->mixedGenderCheckBox->setChecked(teamingOptions->singleGenderPrevented);
    }
    else {
        ui->genderFrame->hide();
        ui->genderSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    if(dataOptions->URMIncluded) {
        ui->URMSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->isolatedURMCheckBox->blockSignals(true);    // prevent select URM identities box from immediately opening
        ui->isolatedURMCheckBox->setChecked(teamingOptions->isolatedURMPrevented);
        ui->isolatedURMCheckBox->blockSignals(false);
    }
    else {
        ui->URMFrame->hide();
        ui->URMSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    if(dataOptions->genderIncluded && dataOptions->URMIncluded) {
        ui->genderSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->URMLabel->hide();
        ui->URMLabelSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->URMFrame->setStyleSheet(ui->URMFrame->styleSheet().replace("border: 1px solid;",
                                                                       "border-top: none; border-bottom: 1px solid; border-left: 1px solid; border-right: 1px solid;"));
    }

    if(dataOptions->numAttributes == 0) {
        ui->attributesFrame->hide();
        ui->attributeSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    else {
        ui->attributesFrame->setUpdatesEnabled(false);
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            attributeWidgets << new AttributeWidget;
            ui->attributesStackedWidget->addWidget(attributeWidgets.last());
            attributeWidgets.last()->setValues(attribute, dataOptions, teamingOptions);
            connect(attributeWidgets.last()->weight, &QDoubleSpinBox::valueChanged,
                        this, [this, attribute](double arg1){teamingOptions->attributeWeights[attribute] = float(arg1);});
            connect(attributeWidgets.last()->homogeneous, &SwitchButton::valueChanged,
                        this, [this, attribute](bool value){teamingOptions->desireHomogeneous[attribute] = value;});
            connect(attributeWidgets.last()->requiredIncompatsButton, &QPushButton::clicked, this, &gruepr::responsesRulesButton_clicked);

            if(dataOptions->numAttributes > 1) {
                const int rowSize = 5;  // number of buttons in each row
                attributeSelectorButtons << new QPushButton(tr("Q") + QString::number(attribute + 1));
                attributeSelectorButtons.last()->setFlat(true);
                QString stylesheet = attribute == 0? ATTRIBBUTTONONSTYLE : ATTRIBBUTTONOFFSTYLE;
                if(attribute == 0) {
                    stylesheet.replace("border-top-left-radius: 0px;", "border-top-left-radius: 5px;");
                }
                if( ((dataOptions->numAttributes < rowSize) && (attribute == (dataOptions->numAttributes - 1))) ||
                    ((dataOptions->numAttributes >= rowSize) && ((attribute / rowSize) == 0) && ((attribute % rowSize) == (rowSize - 1))) ) {
                    stylesheet.replace("border-top-right-radius: 0px;", "border-top-right-radius: 5px;");
                }
                if(attribute == (dataOptions->numAttributes-1)) {
                    stylesheet.replace("border-bottom-right-radius: 0px;", "border-bottom-right-radius: 5px;");
                }
                if( ((attribute / rowSize) == ((dataOptions->numAttributes - 1) / rowSize)) && ((attribute % rowSize) == 0) ) {
                    stylesheet.replace("border-bottom-left-radius: 0px;", "border-bottom-left-radius: 5px;");
                }
                attributeSelectorButtons.last()->setStyleSheet(stylesheet);
                ui->attributeSelectorGrid->addWidget(attributeSelectorButtons.last(), attribute/rowSize, attribute%rowSize);
                connect(attributeSelectorButtons.last(), &QPushButton::clicked, this, [this, attribute]
                                                                                        {ui->attributesStackedWidget->setCurrentIndex(attribute);
                                                                                          for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++) {
                                                                                            if( (attribute == attrib) ||
                                                                                                (attributeSelectorButtons[attrib]->styleSheet()
                                                                                                                           .contains("background-color: " OPENWATERHEX ";")) ) {
                                                                                               attributeSelectorButtons[attrib]->setStyleSheet(
                                                                                                                         attributeSelectorButtons[attrib]->styleSheet()
                                                                                                                         .replace("white", "black")
                                                                                                                         .replace(OPENWATERHEX, "white")
                                                                                                                         .replace("black", OPENWATERHEX));
                                                                                            }
                                                                                          }
                                                                                         });
                ui->attributeSelectorGrid->setColumnStretch(rowSize, 1);
            }

            //(re)set the weight to zero for any attributes with just one value in the data
            if(dataOptions->attributeVals[attribute].size() == 1) {
                teamingOptions->attributeWeights[attribute] = 0;
            }
        }

        ui->attributesStackedWidget->setCurrentIndex(0);
        ui->attributeSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->attributesFrame->setUpdatesEnabled(true);
    }

    if(!dataOptions->dayNames.isEmpty()) {
        ui->minMeetingTimes->setMaximum(int((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLengthSpinBox->value())));
        ui->minMeetingTimes->setValue(teamingOptions->minTimeBlocksOverlap);
        ui->desiredMeetingTimes->setMaximum(int((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLengthSpinBox->value())));
        ui->desiredMeetingTimes->setValue(teamingOptions->desiredTimeBlocksOverlap);
        ui->meetingLengthSpinBox->setValue(teamingOptions->meetingBlockSize);
        ui->scheduleWeight->setValue(teamingOptions->scheduleWeight);
        ui->scheduleSpacer->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    else {
        ui->scheduleFrame->hide();
        ui->scheduleSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
}


//////////////////
// Save everything for future re-opening
//////////////////
void gruepr::saveState()
{
    QSettings savedSettings;

    QFile saveFile(dataOptions->saveStateFileName, this);
    if(saveFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
        QJsonObject content;
        content["teamingoptions"] = teamingOptions->toJson();
        content["dataoptions"] = dataOptions->toJson();
        QJsonArray studentjsons;
        for(const auto &student : students) {
            studentjsons.append(student.toJson());
        }
        content["students"] = studentjsons;
        QJsonArray teamsetjsons;
        for(int tabIndex = 1; tabIndex < ui->dataDisplayTabWidget->count(); tabIndex++) {
            const auto *tab = qobject_cast<TeamsTabItem *>(ui->dataDisplayTabWidget->widget(tabIndex));
            teamsetjsons.append(tab->toJson());
        }
        content["teamsets"] = teamsetjsons;
        QJsonDocument doc(content);
        saveFile.write(doc.toJson(QJsonDocument::Compact));
        saveFile.close();

        //find which savestate this is in the settings
        int numIndexes = savedSettings.beginReadArray("prevWorks");
        int index = -1;
        for(int i = 0; i < numIndexes; i++) {
            savedSettings.setArrayIndex(i);
            if(savedSettings.value("prevWorkFile", "").toString().compare(saveFile.fileName(), Qt::CaseInsensitive) == 0) {
                index = i;
            }
        }
        savedSettings.endArray();
        savedSettings.beginWriteArray("prevWorks");
        if(index == -1) {
            savedSettings.setArrayIndex(numIndexes);
            savedSettings.setValue("prevWorkName", dataOptions->dataSourceName);
            savedSettings.setValue("prevWorkFile", saveFile.fileName());
        }
        else {
            savedSettings.setArrayIndex(index);
        }
        savedSettings.setValue("prevWorkDate", QDateTime::currentDateTime().toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)));
        savedSettings.endArray();
    }
}


//////////////////
// Set the "official" team sizes using an array of different sizes or a single, constant size
//////////////////
inline void gruepr::setTeamSizes(const int teamSizes[])
{
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++)	// run through every team
    {
        teamingOptions->teamSizesDesired[team] = teamSizes[team];
    }
}
inline void gruepr::setTeamSizes(const int singleSize)
{
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++)	// run through every team
    {
        teamingOptions->teamSizesDesired[team] = singleSize;
    }
}


//////////////////
// Read the roster datafile, returning true if successful and false if file is invalid
//////////////////
bool gruepr::loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails)
{
    // Read the header row
    if(!rosterFile.readHeader())
    {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        return false;
    }

    // Ask user what the columns mean
    // Preloading the selector boxes with "unused" except first time "email", "first name", "last name", and "name" are found
    QList<possFieldMeaning> rosterFieldOptions  = {{"First Name", "((first)|(given)|(preferred)).*(name)", 1},
                                                     {"Last Name", "((last)|(sur)|(family)).*(name)", 1},
                                                     {"Email Address", "(e).*(mail)", 1},
                                                     {"Full Name (First Last)", "(name)", 1},
                                                     {"Full Name (Last, First)", "(name)", 1}};;
    if(rosterFile.chooseFieldMeaningsDialog(rosterFieldOptions, this)->exec() == QDialog::Rejected)
    {
        return false;
    }

    // set field values now according to uer's selection of field meanings (defulting to -1 if not chosen)
    int emailField = int(rosterFile.fieldMeanings.indexOf("Email Address"));
    int firstNameField = int(rosterFile.fieldMeanings.indexOf("First Name"));
    int lastNameField = int(rosterFile.fieldMeanings.indexOf("Last Name"));
    int firstLastNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (First Last)"));
    int lastFirstNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (Last, First)"));

    // Process each row until there's an empty one. Load names and email addresses
    names.clear();
    emails.clear();
    if(rosterFile.hasHeaderRow)
    {
        rosterFile.readDataRow();
    }
    else
    {
        rosterFile.readDataRow(true);
    }
    do
    {
        if(firstLastNameField != -1)
        {
            names << rosterFile.fieldValues.at(firstLastNameField).trimmed();
        }
        else if(lastFirstNameField != -1)
        {
            QStringList lastandfirstname = rosterFile.fieldValues.at(lastFirstNameField).split(',');
            names << lastandfirstname.at(1).trimmed() + " " + lastandfirstname.at(0).trimmed();
        }
        else if(firstNameField != -1 && lastNameField != -1)
        {
            names << rosterFile.fieldValues.at(firstNameField).trimmed() + " " + rosterFile.fieldValues.at(lastNameField).trimmed();
        }
        else
        {
            grueprGlobal::errorMessage(this, tr("File error."), tr("This roster does not contain student names."));
            return false;
        }

        if(emailField != -1)
        {
            emails << rosterFile.fieldValues.at(emailField).trimmed();
        }
    }
    while(rosterFile.readDataRow());


    return true;
}


//////////////////
// Update current student info in table
//////////////////
void gruepr::refreshStudentDisplay()
{
    ui->studentTable->setUpdatesEnabled(false);
    ui->dataDisplayTabWidget->setCurrentIndex(0);
    ui->studentTable->clear();
    ui->studentTable->setSortingEnabled(false);

    ui->studentTable->setColumnCount(2 + (dataOptions->timestampField != -1? 1 : 0) + (dataOptions->firstNameField != -1? 1 : 0) +
                                     (dataOptions->lastNameField != -1? 1 : 0) + (dataOptions->sectionIncluded? 1 : 0));
    QIcon unsortedIcon(":/icons_new/upDownButton_white.png");
    int column = 0;
    if(dataOptions->timestampField != -1) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Survey  \n  Timestamp  ")));
    }
    if(dataOptions->firstNameField != -1) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  First  \n  Name  ")));
    }
    if(dataOptions->lastNameField != -1) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Last  \n  Name  ")));
    }
    if(dataOptions->sectionIncluded)
    {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Section  ")));
    }
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(tr("  Edit")));
    ui->studentTable->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("  Remove")));

    ui->studentTable->setRowCount(dataOptions->numStudentsInSystem);
    numActiveStudents = 0;
    QStringList rowNumbers;
    for(const auto &student : students)
    {
        column = 0;
        if((ui->sectionSelectionBox->currentIndex() == 0) || (ui->sectionSelectionBox->currentIndex() == 1) || (student.section == ui->sectionSelectionBox->currentText()))
        {
            rowNumbers << QString::number(numActiveStudents + 1);

            auto *timestamp = new SortableTableWidgetItem(SortableTableWidgetItem::datetime, QLocale::system().toString(student.surveyTimestamp, QLocale::ShortFormat));
            if(dataOptions->timestampField != -1) {
                ui->studentTable->setItem(numActiveStudents, column++, timestamp);
            }
            auto *firstName = new QTableWidgetItem(student.firstname);
            if(dataOptions->firstNameField != -1) {
                ui->studentTable->setItem(numActiveStudents, column++, firstName);
            }
            auto *lastName = new QTableWidgetItem(student.lastname);
            if(dataOptions->lastNameField != -1) {
                ui->studentTable->setItem(numActiveStudents, column++, lastName);
            }
            auto *section = new SortableTableWidgetItem(SortableTableWidgetItem::alphanumeric, student.section);
            if(dataOptions->sectionIncluded)
            {
                ui->studentTable->setItem(numActiveStudents, column++, section);
            }

            bool duplicate = student.duplicateRecord;

            QList<QTableWidgetItem*> items = {timestamp, firstName, lastName, section};
            for(auto &item : items) {
                item->setToolTip(student.tooltip);
                if(duplicate) {
                    item->setBackground(QBrush(QColor::fromString(STARFISHHEX)));
                }
            }

            auto *editButton = new PushButtonWithMouseEnter(QIcon(":/icons_new/edit.png"), "", this);
            editButton->setToolTip("<html>" + tr("Edit") + " " + student.firstname + " " + student.lastname + tr("'s data.") + "</html>");
            editButton->setProperty("StudentIndex", numActiveStudents);
            editButton->setProperty("duplicate", duplicate);
            if(duplicate)
            {
                editButton->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
            }
            connect(editButton, &PushButtonWithMouseEnter::clicked, this, &gruepr::editAStudent);
            // pass on mouse enter events onto cell in table
            connect(editButton, &PushButtonWithMouseEnter::mouseEntered, this, [this, editButton]
                                                                        {int row=0;
                                                                         while(editButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-2))
                                                                              {row++;}
                                                                         ui->studentTable->cellEntered(row);});
            connect(editButton, &PushButtonWithMouseEnter::mouseLeft, this, [this, editButton]
                                                                     {int row=0;
                                                                      while(editButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-2))
                                                                           {row++;}
                                                                      ui->studentTable->cellLeft(row);});
            ui->studentTable->setCellWidget(numActiveStudents, column++, editButton);

            auto *removerButton = new PushButtonWithMouseEnter(QIcon(":/icons_new/trashButton.png"), "", this);
            removerButton->setToolTip("<html>" + tr("Remove") + " " + student.firstname + " " + student.lastname + " " +
                                                 tr("from the list.") + "</html>");
            removerButton->setProperty("StudentIndex", numActiveStudents);
            removerButton->setProperty("duplicate", duplicate);
            if(duplicate)
            {
                removerButton->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
            }
            connect(removerButton, &PushButtonWithMouseEnter::clicked, this, [this, numStudents = numActiveStudents, removerButton] {
                                                                                removerButton->disconnect();
                                                                                removeAStudent(numStudents, false);});
            // pass on mouse enter events onto cell in table
            connect(removerButton, &PushButtonWithMouseEnter::mouseEntered, this, [this, removerButton]
                                                                           {int row=0;
                                                                            while(removerButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-1))
                                                                                 {row++;}
                                                                            ui->studentTable->cellEntered(row);});
            connect(removerButton, &PushButtonWithMouseEnter::mouseLeft, this, [this, removerButton]
                                                                        {int row=0;
                                                                         while(removerButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-1))
                                                                              {row++;}
                                                                         ui->studentTable->cellLeft(row);});
            ui->studentTable->setCellWidget(numActiveStudents, column, removerButton);

            numActiveStudents++;
        }
    }
    ui->studentTable->setRowCount(numActiveStudents);
    ui->studentTable->setVerticalHeaderLabels(rowNumbers);

    ui->studentTable->setUpdatesEnabled(true);
    ui->studentTable->resizeColumnsToContents();
    ui->studentTable->setSortingEnabled(true);
}


////////////////////////////////////////////
// Create and optimize teams using genetic algorithm
////////////////////////////////////////////
QList<int> gruepr::optimizeTeams(const int *const studentIndexes)
{
    // create and seed the pRNG (need to specifically do it here because this is happening in a new thread)
    std::random_device randDev;
    std::mt19937 pRNG(randDev());
    //std::mt19937 pRNG{static_cast<long unsigned int>(time(nullptr))};     //only for minGW, which does not play well with std::random_device

    // Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
    // Each genome in this generation stores (by permutation) which students are in which team.
    // Array has one entry per student and lists, in order, the "ID number" of the
    // student, referring to the order of the student in the students[] array.
    // For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
    // students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

    // allocate memory for a genepool for current generation and a next generation as it is being created
    int **genePool = new int*[GA::POPULATIONSIZE];
    int **nextGenGenePool = new int*[GA::POPULATIONSIZE];
    // allocate memory for current and next generation's ancestors
    int **ancestors = new int*[GA::POPULATIONSIZE];
    int **nextGenAncestors = new int*[GA::POPULATIONSIZE];
    int numAncestors = 2;           //always track mom & dad
    for(int generation = 0; generation < GA::NUMGENERATIONSOFANCESTORS; generation++)
    {
        numAncestors += (4<<generation);   //add an additional 2^(n+1) ancestors for the next level of (great)grandparents
    }
    for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
    {
        genePool[genome] = new int[numActiveStudents];
        nextGenGenePool[genome] = new int[numActiveStudents];
        ancestors[genome] = new int[numAncestors];
        nextGenAncestors[genome] = new int[numAncestors];
    }
    // allocate memory for array of indexes, to be sorted in order of score (so genePool[orderedIndex[0]] is the one with the top score)
    int *orderedIndex = new int[GA::POPULATIONSIZE];
    for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
    {
        orderedIndex[genome] = genome;
    }

    // create an initial population
    // start with an array of all the student IDs in order
    int *randPerm = new int[numActiveStudents];
    for(int i = 0; i < numActiveStudents; i++)
    {
        randPerm[i] = studentIndexes[i];
    }
    // then make "populationSize" number of random permutations for initial population, store in genePool
    for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
    {
        std::shuffle(randPerm, randPerm+numActiveStudents, pRNG);
        for(int ID = 0; ID < numActiveStudents; ID++)
        {
            genePool[genome][ID] = randPerm[ID];
        }
    }
    delete[] randPerm;

    // just use random values for the initial "ancestor" values
    std::uniform_int_distribution<unsigned int> randAncestor(0, GA::POPULATIONSIZE);
    for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
    {
        for(int ancestor = 0; ancestor < numAncestors; ancestor++)
        {
            ancestors[genome][ancestor] = int(randAncestor(pRNG));
        }
    }

    int *teamSizes = new int[MAX_TEAMS];
    for(int team = 0; team < numTeams; team++)
    {
        teamSizes[team] = teams[team].size;
    }

    // calculate this first generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
    QList<float> scores(GA::POPULATIONSIZE);
    float *unusedTeamScores = nullptr, *schedScore = nullptr;
    float **attributeScore = nullptr;
    int *penaltyPoints = nullptr;
    bool **availabilityChart = nullptr;
    bool unpenalizedGenomePresent = false;
    auto sharedStudents = students;
    auto sharedNumTeams = numTeams;
    auto *sharedTeamingOptions = teamingOptions;
    auto *sharedDataOptions = dataOptions;
#pragma omp parallel default(none) shared(scores, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions, unpenalizedGenomePresent) private(unusedTeamScores, attributeScore, schedScore, availabilityChart, penaltyPoints)
    {
        unusedTeamScores = new float[sharedNumTeams];
        attributeScore = new float*[sharedDataOptions->numAttributes];
        for(int attrib = 0; attrib < sharedDataOptions->numAttributes; attrib++)
        {
            attributeScore[attrib] = new float[sharedNumTeams];
        }
        schedScore = new float[sharedNumTeams];
        availabilityChart = new bool*[sharedDataOptions->dayNames.size()];
        for(int day = 0; day < sharedDataOptions->dayNames.size(); day++)
        {
            availabilityChart[day] = new bool[sharedDataOptions->timeNames.size()];
        }
        penaltyPoints = new int[sharedNumTeams];
#pragma omp for
        for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
        {
            scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes,
                                            sharedTeamingOptions, sharedDataOptions, unusedTeamScores,
                                            attributeScore, schedScore, availabilityChart, penaltyPoints);
            int totalPenaltyPoints = 0;
            for(int team = 0; team < sharedNumTeams; team++)
            {
                totalPenaltyPoints += penaltyPoints[team];
            }
            unpenalizedGenomePresent = unpenalizedGenomePresent || (totalPenaltyPoints == 0);
        }
        delete[] penaltyPoints;
        for(int day = 0; day < sharedDataOptions->dayNames.size(); day++)
        {
            delete[] availabilityChart[day];
        }
        delete[] availabilityChart;
        delete[] schedScore;
        for(int attrib = 0; attrib < sharedDataOptions->numAttributes; attrib++)
        {
            delete[] attributeScore[attrib];
        }
        delete[] attributeScore;
        delete[] unusedTeamScores;
    }

    // get genome indexes in order of score, largest to smallest
    std::sort(orderedIndex, orderedIndex+GA::POPULATIONSIZE, [&scores](const int i, const int j){return (scores.at(i) > scores.at(j));});
    emit generationComplete(scores, orderedIndex, 0, 0, unpenalizedGenomePresent);

    int child[MAX_STUDENTS];
    int *mom=nullptr, *dad=nullptr;                 // pointer to genome of mom and dad
    float bestScores[GA::GENERATIONS_OF_STABILITY]={0};	// historical record of best score in the genome, going back generationsOfStability generations
    float scoreStability = 0;
    int generation = 0;
    bool localOptimizationStopped = false;

    // set the working value of the genetic algorithm's tournament selection probability
    if(numActiveStudents > GA::GENOMESIZETHRESHOLD[1])
    {
        GA::topgenomelikelihood = GA::TOPGENOMELIKELIHOOD[2];
    }
    else if(numActiveStudents > GA::GENOMESIZETHRESHOLD[0])
    {
        GA::topgenomelikelihood = GA::TOPGENOMELIKELIHOOD[1];
    }
    else
    {
        GA::topgenomelikelihood = GA::TOPGENOMELIKELIHOOD[0];
    }

    // now optimize
    do						// allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
    {
        do					// keep optimizing until reach stability or maxGenerations
        {
            // clone the elites in genePool into nextGenGenePool, shifting their ancestor arrays as if "self-mating"
            for(int genome = 0; genome < GA::NUM_ELITES; genome++)
            {
                for(int ID = 0; ID < numActiveStudents; ID++)
                {
                    nextGenGenePool[genome][ID] = genePool[orderedIndex[genome]][ID];
                }

                nextGenAncestors[genome][0] = nextGenAncestors[genome][1] = orderedIndex[genome];   // both parents are this genome
                int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;  // parents are 0 & 1, so grandparents are 2, 3, 4, & 5
                for(int generation = 1; generation < GA::NUMGENERATIONSOFANCESTORS; generation++)
                {
                    //all four grandparents are this genome's parents, etc. for increasing generations
                    for(int ancestor = startAncestor; ancestor < (((endAncestor - startAncestor)/2) + startAncestor); ancestor++)
                    {
                        nextGenAncestors[genome][ancestor] = ancestors[orderedIndex[genome]][ancestor-startAncestor+prevStartAncestor];
                    }
                    for(int ancestor = (((endAncestor - startAncestor)/2) + startAncestor); ancestor < endAncestor; ancestor++)
                    {
                        nextGenAncestors[genome][ancestor] = ancestors[orderedIndex[genome]][ancestor-(((endAncestor - startAncestor)/2) + startAncestor)+prevStartAncestor];
                    }
                    prevStartAncestor = startAncestor;
                    startAncestor = endAncestor;
                    endAncestor += (4<<generation);     //add 2^(n+1)
                }
            }

            // create rest of population in nextGenGenePool by mating
            for(int genome = GA::NUM_ELITES; genome < GA::POPULATIONSIZE; genome++)
            {
                //get a couple of parents
                GA::tournamentSelectParents(genePool, orderedIndex, ancestors, mom, dad, nextGenAncestors[genome], pRNG);

                //mate them and put child in nextGenGenePool
                GA::mate(mom, dad, teamSizes, numTeams, child, numActiveStudents, pRNG);
                for(int ID = 0; ID < numActiveStudents; ID++)
                {
                    nextGenGenePool[genome][ID] = child[ID];
                }
            }

            // take all but the single top-scoring elite genome and mutate with some probability; if a mutation occurred, mutate same genome again with same probability
            std::uniform_int_distribution<unsigned int> randProbability(1, 100);
            for(int genome = 1; genome < GA::POPULATIONSIZE; genome++)
            {
                while(randProbability(pRNG) < GA::MUTATIONLIKELIHOOD)
                {
                    GA::mutate(&nextGenGenePool[genome][0], numActiveStudents, pRNG);
                }
            }

            // swap pointers to make nextGen's genePool and ancestors into this generation's
            std::swap(genePool, nextGenGenePool);
            std::swap(ancestors, nextGenAncestors);

            generation++;

            // calculate this generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
            unpenalizedGenomePresent = false;
            sharedStudents = students;
            sharedNumTeams = numTeams;
            sharedTeamingOptions = teamingOptions;
            sharedDataOptions = dataOptions;
#pragma omp parallel default(none) shared(scores, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions, unpenalizedGenomePresent) private(unusedTeamScores, attributeScore, schedScore, availabilityChart, penaltyPoints)
            {
                unusedTeamScores = new float[sharedNumTeams];
                attributeScore = new float*[sharedDataOptions->numAttributes];
                for(int attrib = 0; attrib < sharedDataOptions->numAttributes; attrib++)
                {
                    attributeScore[attrib] = new float[sharedNumTeams];
                }
                schedScore = new float[sharedNumTeams];
                availabilityChart = new bool*[sharedDataOptions->dayNames.size()];
                for(int day = 0; day < sharedDataOptions->dayNames.size(); day++)
                {
                    availabilityChart[day] = new bool[sharedDataOptions->timeNames.size()];
                }
                penaltyPoints = new int[sharedNumTeams];
#pragma omp for nowait
                for(int genome = 0; genome < GA::POPULATIONSIZE; genome++)
                {
                    scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes,
                                                    sharedTeamingOptions, sharedDataOptions, unusedTeamScores,
                                                    attributeScore, schedScore, availabilityChart, penaltyPoints);
                    int totalPenaltyPoints = 0;
                    for(int team = 0; team < sharedNumTeams; team++)
                    {
                        totalPenaltyPoints += penaltyPoints[team];
                    }
                    unpenalizedGenomePresent = unpenalizedGenomePresent || (totalPenaltyPoints == 0);
                }
                delete[] penaltyPoints;
                for(int day = 0; day < sharedDataOptions->dayNames.size(); day++)
                {
                    delete[] availabilityChart[day];
                }
                delete[] availabilityChart;
                delete[] schedScore;
                for(int attrib = 0; attrib < sharedDataOptions->numAttributes; attrib++)
                {
                    delete[] attributeScore[attrib];
                }
                delete[] attributeScore;
                delete[] unusedTeamScores;
            }

            // get genome indexes in order of score, largest to smallest
            std::sort(orderedIndex, orderedIndex+GA::POPULATIONSIZE, [&scores](const int i, const int j){return (scores.at(i) > scores.at(j));});

            // determine best score, save in historical record, and calculate score stability
            float maxScoreInThisGeneration = scores[orderedIndex[0]];
            float maxScoreFromGenerationsAgo = bestScores[(generation+1)%GA::GENERATIONS_OF_STABILITY];
            bestScores[generation%GA::GENERATIONS_OF_STABILITY] = maxScoreInThisGeneration;	//best scores from most recent generationsOfStability, wrapping storage location

            if(maxScoreInThisGeneration == maxScoreFromGenerationsAgo)
            {
                scoreStability = maxScoreInThisGeneration / 0.0001F;
            }
            else
            {
                scoreStability = maxScoreInThisGeneration / (maxScoreInThisGeneration - maxScoreFromGenerationsAgo);
            }

            emit generationComplete(scores, orderedIndex, generation, scoreStability, unpenalizedGenomePresent);

            optimizationStoppedmutex.lock();
            localOptimizationStopped = optimizationStopped;
            optimizationStoppedmutex.unlock();
        }
        while(!localOptimizationStopped && ((generation < GA::MIN_GENERATIONS) || ((generation < GA::MAX_GENERATIONS) && (scoreStability < GA::MIN_SCORE_STABILITY))));

        if(localOptimizationStopped)
        {
            keepOptimizing = false;
            emit turnOffBusyCursor();
        }
        else
        {
            keepOptimizing = true;
        }
    }
    while(keepOptimizing);

    finalGeneration = generation;
    teamSetScore = bestScores[generation%GA::GENERATIONS_OF_STABILITY];

    //copy best team set into a QList to return
    QList<int> bestTeamSet;
    bestTeamSet.reserve(numActiveStudents);
    for(int ID = 0; ID < numActiveStudents; ID++)
    {
        bestTeamSet << genePool[orderedIndex[0]][ID];
    }

    // deallocate memory
    for(int genome = 0; genome < GA::POPULATIONSIZE; ++genome)
    {
        delete[] nextGenGenePool[genome];
        delete[] genePool[genome];
        delete[] nextGenAncestors[genome];
        delete[] ancestors[genome];
    }
    delete[] nextGenGenePool;
    delete[] genePool;
    delete[] nextGenAncestors;
    delete[] ancestors;
    delete[] orderedIndex;
    delete[] teamSizes;

    return bestTeamSet;
}


//////////////////
// Calculate score for one teamset (one genome)
// Returns the total net score (which is, typically, the harmonic mean of all team scores)
// Modifys the teamScores[] to give scores for each individual team in the genome, too
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
//////////////////
float gruepr::getGenomeScore(const StudentRecord _students[], const int _teammates[], const int _numTeams, const int _teamSizes[],
                            const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, float _teamScores[],
                             float **_attributeScore, float *_schedScore, bool **_availabilityChart, int *_penaltyPoints)
{
    // Initialize each component score
    for(int team = 0; team < _numTeams; team++)
    {
        for(int attribute = 0; attribute < _dataOptions->numAttributes; attribute++)
        {
            _attributeScore[attribute][team] = 0;
        }
        _schedScore[team] = 0;
        _penaltyPoints[team] = 0;
    }

    int studentNum = 0;

    // Calculate each component score:

    // Calculate attribute scores and penalties for each attribute for each team:
    std::multiset<int> attributeLevelsInTeam;
    std::multiset<float> timezoneLevelsInTeam;
    for(int attribute = 0; attribute < _dataOptions->numAttributes; attribute++)
    {
        if((_teamingOptions->realAttributeWeights[attribute] > 0) ||
           (_teamingOptions->haveAnyIncompatibleAttributes[attribute]) ||
           (_teamingOptions->haveAnyRequiredAttributes[attribute]))
        {
            const bool thisIsTimezone = (_dataOptions->attributeField[attribute] == _dataOptions->timezoneField);
            studentNum = 0;
            for(int team = 0; team < _numTeams; team++)
            {
                // gather all attribute values
                attributeLevelsInTeam.clear();
                timezoneLevelsInTeam.clear();
                for(int teammate = 0; teammate < _teamSizes[team]; teammate++)
                {
                    attributeLevelsInTeam.insert(_students[_teammates[studentNum]].attributeVals[attribute].constBegin(),
                                                 _students[_teammates[studentNum]].attributeVals[attribute].constEnd());
                    if(thisIsTimezone)
                    {
                        timezoneLevelsInTeam.insert(_students[_teammates[studentNum]].timezone);
                    }
                    studentNum++;
                }

                // Add a penalty per pair of incompatible attribute responses found
                if(_teamingOptions->haveAnyIncompatibleAttributes[attribute])
                {
                    // go through each pair found in teamingOptions->incompatibleAttributeValues[attribute] list and see if both are found in attributeLevelsInTeam
                    for(const auto &pair : qAsConst(_teamingOptions->incompatibleAttributeValues[attribute]))
                    {
                        int n = int(attributeLevelsInTeam.count(pair.first));
                        if(pair.first == pair.second)
                        {
                            _penaltyPoints[team] += (n * (n-1))/ 2;  // number of incompatible pairings is the sum 1 -> n-1 (calculation = 0 if n == 0 or n == 1)
                        }
                        else
                        {
                            int m = int(attributeLevelsInTeam.count(pair.second));
                            _penaltyPoints[team] += n * m;           // number of incompatible pairings is the number of n -> m interactions (calculation = 0 if n == 0 or m == 0)
                        }
                    }
                }

                // Add a penalty per required attribute response not found
                if(_teamingOptions->haveAnyRequiredAttributes[attribute])
                {
                    // go through each value found in teamingOptions->requiredAttributeValues[attrib] list and see whether it's found in attributeLevelsInTeam
                    for(const auto value : qAsConst(_teamingOptions->requiredAttributeValues[attribute]))
                    {
                        if(attributeLevelsInTeam.count(value) == 0)
                        {
                            _penaltyPoints[team]++;
                        }
                    }
                }

                // Remove attribute values of -1 (unknown/not set) and then determine attribute scores assuming we have any
                attributeLevelsInTeam.erase(-1);
                if((_teamingOptions->realAttributeWeights[attribute] > 0) && (!attributeLevelsInTeam.empty()))
                {
                    float attributeRangeInTeam;
                    if(thisIsTimezone)
                    {
                        // "attribute" is timezone, so use timezone values
                        attributeRangeInTeam = *timezoneLevelsInTeam.crbegin() - *timezoneLevelsInTeam.cbegin();
                    }
                    else if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                            (_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
                    {
                        // attribute has meaningful ordering/numerical values--heterogeneous means create maximum spread between max and min values
                        attributeRangeInTeam = *attributeLevelsInTeam.crbegin() - *attributeLevelsInTeam.cbegin();  // crbegin is last = largest val; cbegin is 1st = smallest
                    }
                    else
                    {
                        // attribute is categorical or multicategorical--heterogeneous means create maximum number of unique values
                        attributeRangeInTeam = -1;
                        int prevVal = -1;
                        for(const auto currVal : attributeLevelsInTeam)
                        {
                            if(currVal != prevVal)
                            {
                                attributeRangeInTeam += 1;
                            }
                            prevVal = currVal;
                        }
                    }

                    _attributeScore[attribute][team] = attributeRangeInTeam /
                                                      (*(_dataOptions->attributeVals[attribute].crbegin()) - *(_dataOptions->attributeVals[attribute].cbegin()));
                    if(_teamingOptions->desireHomogeneous[attribute])	//attribute scores are 0 if homogeneous and +1 if full range of values are in a team, so flip if want homogeneous
                    {
                        _attributeScore[attribute][team] = 1 - _attributeScore[attribute][team];
                    }
                }

                _attributeScore[attribute][team] *= _teamingOptions->realAttributeWeights[attribute];
            }
        }
    }

    // Calculate schedule scores for each team:
    if(_teamingOptions->realScheduleWeight > 0)
    {
        const int numDays = int(_dataOptions->dayNames.size());
        const int numTimes = int(_dataOptions->timeNames.size());

        // combine each student's schedule array into a team schedule array
        studentNum = 0;
        for(int team = 0; team < _numTeams; team++)
        {            
            if(_teamSizes[team] == 1)
            {
                studentNum++;
                continue;
            }

            // start compiling a team availability chart; begin with that of the first student on team (unless they have ambiguous schedule)
            int numStudentsWithAmbiguousSchedules = 0;
            const auto &firstStudentOnTeam = _students[_teammates[studentNum]];
            if(!firstStudentOnTeam.ambiguousSchedule)
            {
                const auto &firstStudentUnavailability = firstStudentOnTeam.unavailable;
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes; time++)
                    {
                        _availabilityChart[day][time] = !firstStudentUnavailability[day][time];
                    }
                }
            }
            else
            {
                // ambiguous schedule, so note it and start with all timeslots available
                numStudentsWithAmbiguousSchedules++;
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes; time++)
                    {
                        _availabilityChart[day][time] = true;
                    }
                }
            }
            studentNum++;

            // now move on to each subsequent student and, unless they have ambiguous schedule, merge their availability into the team's
            for(int teammate = 1; teammate < _teamSizes[team]; teammate++)
            {
                const auto &currStudent = _students[_teammates[studentNum]];
                if(currStudent.ambiguousSchedule)
                {
                    numStudentsWithAmbiguousSchedules++;
                    studentNum++;
                    continue;
                }
                const auto &currStudentUnavailability = currStudent.unavailable;
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes; time++)
                    {
                        // "and" each student's not-unavailability
                        _availabilityChart[day][time] = _availabilityChart[day][time] && !currStudentUnavailability[day][time];
                    }
                }
                studentNum++;
            }

            // keep schedule score at 0 unless 2+ students have unambiguous sched (avoid runaway score by grouping students w/ambiguous scheds)
            if((_teamSizes[team] - numStudentsWithAmbiguousSchedules) < 2)
            {
                continue;
            }

            // count how many free time blocks there are
            if(_teamingOptions->meetingBlockSize == 1)
            {
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes; time++)
                    {
                        if(_availabilityChart[day][time])
                        {
                            _schedScore[team]++;
                        }
                    }
                }
            }
            else if(_teamingOptions->meetingBlockSize == 2)   //user wants to count only 2-hr time blocks, but don't count wrap-around past end of 1 day!
            {
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes-1; time++)
                    {
                        if(_availabilityChart[day][time])
                        {
                            time++;
                            if(_availabilityChart[day][time])
                            {
                                _schedScore[team]++;
                            }
                        }
                    }
                }
            }
            else if(_teamingOptions->meetingBlockSize == 3)   //user wants to count only 3-hr time blocks, but don't count wrap-around past end of 1 day!
            {
                for(int day = 0; day < numDays; day++)
                {
                    for(int time = 0; time < numTimes-2; time++)
                    {
                        if(_availabilityChart[day][time])
                        {
                            time++;
                            if(_availabilityChart[day][time])
                            {
                                time++;
                                if(_availabilityChart[day][time])
                                {
                                    _schedScore[team]++;
                                }
                            }
                        }
                    }
                }
            }

            // convert counts to a schedule score
            // normal schedule score is number of overlaps / desired number of overlaps
            if(_schedScore[team] > _teamingOptions->desiredTimeBlocksOverlap)		// if team has > desiredTimeBlocksOverlap, additional overlaps count less
            {
                int numAdditionalOverlaps = int(_schedScore[team]) - _teamingOptions->desiredTimeBlocksOverlap;
                _schedScore[team] = _teamingOptions->desiredTimeBlocksOverlap;
                float factor = 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
                for(int n = 1 ; n <= numAdditionalOverlaps; n++)
                {
                    _schedScore[team] += factor;
                    factor *= 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
                }
            }
            else if(_schedScore[team] < _teamingOptions->minTimeBlocksOverlap)	// if team has fewer than minTimeBlocksOverlap, zero out the score and apply penalty
            {
                _schedScore[team] = 0;
                _penaltyPoints[team]++;
            }
            _schedScore[team] /= _teamingOptions->desiredTimeBlocksOverlap;
            _schedScore[team] *= _teamingOptions->realScheduleWeight;
        }
    }

    // Determine gender penalties
    if(_dataOptions->genderIncluded && (_teamingOptions->isolatedWomenPrevented || _teamingOptions->isolatedMenPrevented ||
                                       _teamingOptions->isolatedNonbinaryPrevented || _teamingOptions->singleGenderPrevented))
    {
        studentNum = 0;
        for(int team = 0; team < _numTeams; team++)
        {
            if(_teamSizes[team] == 1)
            {
                studentNum++;
                continue;
            }

            // Count how many of each gender on the team
            int numWomen = 0;
            int numMen = 0;
            int numNonbinary = 0;
            for(int teammate = 0; teammate < _teamSizes[team]; teammate++)
            {
                if(_students[_teammates[studentNum]].gender == Gender::man)
                {
                    numMen++;
                }
                else if(_students[_teammates[studentNum]].gender == Gender::woman)
                {
                    numWomen++;
                }
                else if(_students[_teammates[studentNum]].gender == Gender::nonbinary)
                {
                    numNonbinary++;
                }
                studentNum++;
            }

            // Apply penalties as appropriate
            if(_teamingOptions->isolatedWomenPrevented && numWomen == 1)
            {
                _penaltyPoints[team]++;
            }
            if(_teamingOptions->isolatedMenPrevented && numMen == 1)
            {
                _penaltyPoints[team]++;
            }
            if(_teamingOptions->isolatedNonbinaryPrevented && numNonbinary == 1)
            {
                _penaltyPoints[team]++;
            }
            if(_teamingOptions->singleGenderPrevented && (numMen == 0 || numWomen == 0))
            {
                _penaltyPoints[team]++;
            }
        }
    }

    // Determine URM penalties
    if(_dataOptions->URMIncluded && _teamingOptions->isolatedURMPrevented)
    {
        studentNum = 0;
        for(int team = 0; team < _numTeams; team++)
        {
            if(_teamSizes[team] == 1)
            {
                studentNum++;
                continue;
            }

            // Count how many URM on the team
            int numURM = 0;
            for(int teammate = 0; teammate < _teamSizes[team]; teammate++)
            {
                if(_students[_teammates[studentNum]].URM)
                {
                    numURM++;
                }
                studentNum++;
            }

            // Apply penalties as appropriate
            if(numURM == 1)
            {
                _penaltyPoints[team]++;
            }
        }
    }

    // Determine penalties for required teammates NOT on team, prevented teammates on team, and insufficient number of requested teammates on team
    if(_teamingOptions->haveAnyRequiredTeammates || _teamingOptions->haveAnyPreventedTeammates || _teamingOptions->haveAnyRequestedTeammates)
    {
        std::set<int> IDsBeingTeamed, IDsOnTeam, requestedIDsByStudent;
        std::multiset<int> requiredIDsOnTeam, preventedIDsOnTeam;   //multiset so that penalties are in proportion to number of missed requirements
        std::vector< std::set<int> > requestedIDs;  // each set is the requests of one student; vector is all the students on the team

        // Get all IDs being teamed (so that we can make sure we only check the requireds/prevented/requesteds that are actually within this teamset)
        studentNum = 0;
        for(int team = 0; team < _numTeams; team++)
        {
            for(int teammate = 0; teammate < _teamSizes[team]; teammate++)
            {
                IDsBeingTeamed.insert(_students[_teammates[studentNum]].ID);
                studentNum++;
            }
        }

        // Loop through each team
        studentNum = 0;
        const StudentRecord *currStudent = nullptr;
        for(int team = 0; team < _numTeams; team++)
        {
            IDsOnTeam.clear();
            requiredIDsOnTeam.clear();
            preventedIDsOnTeam.clear();
            requestedIDs.clear();
            //loop through each student on team and collect their ID and their required/prevented/requested IDs
            for(int teammate = 0; teammate < _teamSizes[team]; teammate++)
            {
                currStudent = &_students[_teammates[studentNum]];
                IDsOnTeam.insert(currStudent->ID);
                requestedIDsByStudent.clear();
                for(const auto ID : IDsBeingTeamed)
                {
                    if(currStudent->requiredWith[ID])
                    {
                        requiredIDsOnTeam.insert(ID);
                    }
                    if(currStudent->preventedWith[ID])
                    {
                        preventedIDsOnTeam.insert(ID);
                    }
                    if(currStudent->requestedWith[ID])
                    {
                        requestedIDsByStudent.insert(ID);
                    }
                }
                requestedIDs.push_back(requestedIDsByStudent);
                studentNum++;
            }

            if(_teamingOptions->haveAnyRequiredTeammates)
            {
                //loop through all the required IDs to see if each is present on the team--if not, increment penalty
                for(const auto requiredIDOnTeam : requiredIDsOnTeam)
                {
                    if(IDsOnTeam.count(requiredIDOnTeam) == 0)
                    {
                        _penaltyPoints[team]++;
                    }
                }
            }

            if(_teamingOptions->haveAnyPreventedTeammates)
            {
                //loop through all the prevented IDs to see if each is missing on the team--if not, increment penalty
                for(const auto preventedIDOnTeam : preventedIDsOnTeam)
                {
                    if(IDsOnTeam.count(preventedIDOnTeam) != 0)
                    {
                        _penaltyPoints[team]++;
                    }
                }
            }

            if(_teamingOptions->haveAnyRequestedTeammates)
            {
                for(const auto &requestedIDSet : requestedIDs)
                {
                    int numRequestedTeammates = 0, numRequestedTeammatesFound = 0;
                    for(const auto requestedIDOnTeam : requestedIDSet)
                    {
                        numRequestedTeammates++;
                        if(IDsOnTeam.count(requestedIDOnTeam) != 0)
                        {
                            numRequestedTeammatesFound++;
                        }
                    }
                    //apply penalty if student has unfulfilled requests that exceed the number allowed
                    if(numRequestedTeammatesFound < std::min(numRequestedTeammates, _teamingOptions->numberRequestedTeammatesGiven))
                    {
                        _penaltyPoints[team]++;
                    }
                }
            }
        }
    }

    //Bring component scores together for final team scores and, ultimately, a net score:
    //final team scores are normalized to be out of 100 (but with possible "extra credit" for more than desiredTimeBlocksOverlap hours w/ 100% team availability)
    for(int team = 0; team < _numTeams; team++)
    {
        // remove the schedule extra credit if any penalties are being applied, so that a very high schedule overlap doesn't cancel out the penalty
        if((_schedScore[team] > _teamingOptions->realScheduleWeight) && (_penaltyPoints[team] > 0))
        {
            _schedScore[team] = _teamingOptions->realScheduleWeight;
        }

        _teamScores[team] = _schedScore[team];
        for(int attribute = 0; attribute < _dataOptions->numAttributes; attribute++)
        {
            _teamScores[team] += _attributeScore[attribute][team];
        }
        _teamScores[team] = 100 * ((_teamScores[team] / float(_teamingOptions->realNumScoringFactors)) - _penaltyPoints[team]);
    }

    //Use the harmonic mean for the "total score"
    //This value, the inverse of the average of the inverses, is skewed towards the smaller members so that we optimize for better values of the worse teams
    //very poor teams have 0 or negative scores, and this makes the harmonic mean meaningless
    //if any teamScore is <= 0, return the arithmetic mean punished by reducing towards negative infinity by half the arithmetic mean
    float harmonicSum = 0, regularSum = 0;
    int numTeamsScored = 0;
    bool allTeamsPositive = true;
    for(int team = 0; team < _numTeams; team++)
    {
        //ignore unpenalized teams of one since their score of 0 is not meaningful
        if(_teamSizes[team] == 1 && _teamScores[team] == 0)
        {
            continue;
        }
        numTeamsScored++;
        regularSum += _teamScores[team];

        if(_teamScores[team] <= 0)
        {
            allTeamsPositive = false;
        }
        else
        {
            harmonicSum += 1/_teamScores[team];
        }
    }

    if(allTeamsPositive)
    {
        return(float(numTeamsScored)/harmonicSum);      //harmonic mean
    }

    float mean = regularSum / float(numTeamsScored);    //"punished" arithmetic mean
    return(mean - (std::abs(mean)/2));
}


//////////////////
// Before closing the main application window, see if we want to save the current settings as defaults
//////////////////
void gruepr::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    savedSettings.setValue("windowGeometry", saveGeometry());
    saveState();    // save current work for possible future use

    if(restartRequested) {
        event->accept();
        emit closed();
        return;
    }

    bool dontActuallyExit = false;
    bool saveSettings = savedSettings.value("saveDefaultsOnExit", false).toBool();

    if(savedSettings.value("askToSaveDefaultsOnExit", true).toBool())
    {
        QApplication::beep();
        auto *saveOptionsOnClose = new QMessageBox(this);
        saveOptionsOnClose->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        saveOptionsOnClose->setStyleSheet(LABELSTYLE);
        auto *neverShowAgain = new QCheckBox(tr("Don't ask me this again"), saveOptionsOnClose);
        neverShowAgain->setStyleSheet(CHECKBOXSTYLE);

        saveOptionsOnClose->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                              Qt::KeepAspectRatio, Qt::SmoothTransformation));
        saveOptionsOnClose->setWindowTitle(tr("Save Options?"));
        saveOptionsOnClose->setText(tr("Before exiting, should we save the\ncurrent teaming options as defaults?"));
        saveOptionsOnClose->setCheckBox(neverShowAgain);
        saveOptionsOnClose->setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        auto *dontSave = saveOptionsOnClose->addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
        saveOptionsOnClose->button(QMessageBox::Save)->setStyleSheet(SMALLBUTTONSTYLE);
        saveOptionsOnClose->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        dontSave->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        saveOptionsOnClose->exec();

        if(saveOptionsOnClose->result() == QMessageBox::Save)
        {
            saveSettings = true;
        }
        else if(saveOptionsOnClose->result() == QMessageBox::Cancel)
        {
            dontActuallyExit = true;
        }

        if(neverShowAgain->checkState() == Qt::Checked)
        {
            savedSettings.setValue("askToSaveDefaultsOnExit", false);
            savedSettings.setValue("saveDefaultsOnExit", saveSettings);
        }
        saveOptionsOnClose->deleteLater();
    }

    if(dontActuallyExit) {
        event->ignore();
    }
    else {
        if(saveSettings) {
            savedSettings.setValue("idealTeamSize", ui->idealTeamSizeBox->value());
            savedSettings.setValue("isolatedWomenPrevented", teamingOptions->isolatedWomenPrevented);
            savedSettings.setValue("isolatedMenPrevented", teamingOptions->isolatedMenPrevented);
            savedSettings.setValue("isolatedNonbinaryPrevented", teamingOptions->isolatedNonbinaryPrevented);
            savedSettings.setValue("singleGenderPrevented", teamingOptions->singleGenderPrevented);
            savedSettings.setValue("isolatedURMPrevented", teamingOptions->isolatedURMPrevented);
            savedSettings.setValue("minTimeBlocksOverlap", teamingOptions->minTimeBlocksOverlap);
            savedSettings.setValue("desiredTimeBlocksOverlap", teamingOptions->desiredTimeBlocksOverlap);
            savedSettings.setValue("meetingBlockSize", teamingOptions->meetingBlockSize);
            savedSettings.setValue("scheduleWeight", teamingOptions->scheduleWeight);
            savedSettings.beginWriteArray("Attributes");
            for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum) {
                savedSettings.setArrayIndex(attribNum);
                savedSettings.setValue("desireHomogeneous", teamingOptions->desireHomogeneous[attribNum]);
                savedSettings.setValue("weight", teamingOptions->attributeWeights[attribNum]);
                savedSettings.remove("incompatibleResponses");  //clear any existing values
                savedSettings.beginWriteArray("incompatibleResponses");
                for(int incompResp = 0; incompResp < teamingOptions->incompatibleAttributeValues[attribNum].size(); incompResp++) {
                    savedSettings.setArrayIndex(incompResp);
                    savedSettings.setValue("incompatibleResponses",
                                           (QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).first) + "," +
                                            QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).second)));
                }
                savedSettings.endArray();
                savedSettings.beginWriteArray("requiredResponses");
                for(int requiredResp = 0; requiredResp < teamingOptions->requiredAttributeValues[attribNum].size(); requiredResp++) {
                    savedSettings.setArrayIndex(requiredResp);
                    savedSettings.setValue("requiredResponse",
                                           (QString::number(teamingOptions->requiredAttributeValues[attribNum].at(requiredResp))));
                }
                savedSettings.endArray();
            }
            savedSettings.endArray();
            savedSettings.setValue("requestedTeammateNumber", teamingOptions->numberRequestedTeammatesGiven);
        }

        event->accept();
        emit closed();
    }
}

