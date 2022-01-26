#include "ui_gruepr.h"
#include "gruepr.h"
#include "dialogs/baseTimeZoneDialog.h"
#include "dialogs/customTeamsizesDialog.h"
#include "dialogs/editOrAddStudentDialog.h"
#include "dialogs/findMatchingNameDialog.h"
#include "dialogs/gatherAttributeValuesDialog.h"
#include "dialogs/gatherTeammatesDialog.h"
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


gruepr::gruepr(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::gruepr)
{
    //Setup the main window
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowIcon(QIcon(":/icons/gruepr.png"));
    statusBarLabel = new QLabel("", this);
    ui->statusBar->addWidget(statusBarLabel);
    qRegisterMetaType<QVector<float> >("QVector<float>");

    //Put attribute label next to that tab widget
    auto attlab = new QLabel(tr("Attribute") + ": ", ui->attributesTabWidget);
    ui->attributesTabWidget->setCornerWidget(attlab, Qt::TopLeftCorner);

    //Setup the main window menu items
    connect(ui->actionLoad_Survey_File, &QAction::triggered, this, &gruepr::on_loadSurveyFileButton_clicked);
    connect(ui->actionLoad_Student_Roster, &QAction::triggered, this, &gruepr::loadStudentRoster);
    connect(ui->actionSave_Survey_File, &QAction::triggered, this, &gruepr::on_saveSurveyFilePushButton_clicked);
    connect(ui->actionLoad_Teaming_Options_File, &QAction::triggered, this, &gruepr::loadOptionsFile);
    connect(ui->actionSave_Teaming_Options_File, &QAction::triggered, this, &gruepr::saveOptionsFile);
    connect(ui->actionCreate_Teams, &QAction::triggered, this, &gruepr::on_letsDoItButton_clicked);
    ui->actionExit->setMenuRole(QAction::QuitRole);
    connect(ui->actionExit, &QAction::triggered, this, &gruepr::close);
    //ui->actionSettings->setMenuRole(QAction::PreferencesRole);
    //connect(ui->actionSettings, &QAction::triggered, this, &gruepr::settingsWindow);
    connect(ui->actionHelp, &QAction::triggered, this, &gruepr::helpWindow);
    ui->actionAbout->setMenuRole(QAction::AboutRole);
    connect(ui->actionAbout, &QAction::triggered, this, &gruepr::aboutWindow);
    connect(ui->actiongruepr_Homepage, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl("https://bit.ly/grueprFromApp"));});
    connect(ui->actionBugReport, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl("http://bit.ly/grueprBugReportFromApp"));});

    //Set alternate fonts on some UI features
    QFont altFont = this->font();
    altFont.setPointSize(altFont.pointSize() + 4);
    ui->loadSurveyFileButton->setFont(altFont);
    ui->letsDoItButton->setFont(altFont);
    ui->addStudentPushButton->setFont(altFont);
    ui->saveSurveyFilePushButton->setFont(altFont);
    ui->dataDisplayTabWidget->setFont(altFont);
    ui->teamingOptionsGroupBox->setFont(altFont);

    //Reduce size of the options icons if the screen is small
#ifdef Q_OS_WIN32
    if(QGuiApplication::primaryScreen()->availableSize().height() < SMALL_SCREENSIZE_WIN)
#endif
#ifdef Q_OS_MACOS
    if(QGuiApplication::primaryScreen()->availableSize().height() < SMALL_SCREENSIZE_MAC)
#endif
    {
        ui->label_15->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_16->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_18->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_20->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_21->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_22->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
        ui->label_24->setMaximumSize(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE);
    }
    adjustSize();

    //create options
    teamingOptions = new TeamingOptions;
    dataOptions = new DataOptions;

    //Connect genetic algorithm progress signals to slots
    connect(this, &gruepr::generationComplete, this, &gruepr::updateOptimizationProgress, Qt::BlockingQueuedConnection);
    connect(&futureWatcher, &QFutureWatcher<void>::finished, this, &gruepr::optimizationComplete);

    // load all of the default values
    loadDefaultSettings();
    ui->letsDoItButton->setEnabled(false);
}

gruepr::~gruepr()
{
    delete[] student;
    delete dataOptions;
    delete teamingOptions;
    delete[] attributeTab;
    delete ui;
}


////////////////////
// A static public wrapper for the getGenomeScore function used internally
// The calculated scores are updated into the .scores members of the _teams array sent to the function
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
////////////////////
void gruepr::getTeamScores(const StudentRecord _student[], const int _numStudents, TeamRecord _teams[], const int _numTeams,
                           const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions)
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
    getGenomeScore(_student, genome, _numTeams, teamSizes,
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


void gruepr::on_loadSurveyFileButton_clicked()
{
    CsvFile surveyFile;
    /*  Used if the open file dialog will display auto-read field meanings - idea abandoned for now because non-native file dialog is ugly
    surveyFile.defaultFieldMeanings = {{"Timestamp", "(timestamp)", 1}, {"First Name", "((first)|(given)|(preferred))(?!.*last).*(name)", 1},
                                       {"Last Name", "^(?!.*first).*((last)|(sur)|(family)).*(name)", 1}, {"Email Address", "(e).*(mail)", 1},
                                       {"Gender", "(gender)", 1}, {"Racial/ethnic identity", "((minority)|(ethnic))", 1},
                                       {"Schedule", "(check).+(times)", MAX_DAYS}, {"Section", "in which section are you enrolled", 1},
                                       {"Timezone","(time zone)", 1}, {"Preferred Teammates", "(name).*(like to have on your team)", 1},
                                       {"Preferred Non-teammates", "(name).*(like to not have on your team)", 1},
                                       {"Attribute", ".*", MAX_ATTRIBUTES}, {"Notes", "", MAX_NOTES_FIELDS}};
    */
    if(!surveyFile.open(this, CsvFile::read, tr("Open Survey Data File"), dataOptions->dataFile.canonicalPath(), tr("Survey Data")))
    {
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    resetUI();

    //reset the data
    delete[] student;
    student = new StudentRecord[MAX_STUDENTS];
    teamingOptions->reset();
    delete dataOptions;
    dataOptions = new DataOptions;
    dataOptions->dataFile = surveyFile.fileInfo();

    if(!loadSurveyData(surveyFile))
    {
        QApplication::restoreOverrideCursor();
        return;
    }

    // Check for duplicate students; warn if found
    bool duplicatesExist = false;
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        if(student[index].duplicateRecord)
        {
            duplicatesExist = true;
        }
    }
    if(duplicatesExist)
    {
        QMessageBox::warning(this, tr("Possible duplicate submissions"), tr("There appears to be at least one student with multiple survey submissions. "
                                                                            "Possible duplicates are marked with a yellow background in the table."));
    }

    if(dataOptions->prefTeammatesIncluded || dataOptions->prefNonTeammatesIncluded)
    {
        QString message = tr("This survey included") + " ";
        if(dataOptions->prefTeammatesIncluded && dataOptions->prefNonTeammatesIncluded)
        {
            message += tr("questions");
        }
        else
        {
            message += tr("a question");
        }
        message += " " + tr("about students' preferences for") + " ";
        if(dataOptions->prefTeammatesIncluded)
        {
            message += tr("people to be on their team");
        }
        if(dataOptions->prefTeammatesIncluded && dataOptions->prefNonTeammatesIncluded)
        {
            message += " " + tr("and") + " ";
        }
        if(dataOptions->prefNonTeammatesIncluded)
        {
            message += tr("people to not be on their team");
        }
        message += ". " + tr("To import those preferences, use the \"import from the survey\" action in the Required, Prevented, and/or Preferred Teammate options.");
        QMessageBox::information(this, tr("Teammate preferences found in survey"),message);
    }

    loadUI();

    QApplication::restoreOverrideCursor();
}


void gruepr::loadStudentRoster()
{
    // Open the roster file
    CsvFile rosterFile;
    if(!rosterFile.open(this, CsvFile::read, tr("Open Student Roster File"), dataOptions->dataFile.canonicalPath(), tr("Roster File")))
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
            namesNotFound << student[index].firstname + " " + student[index].lastname;
        }

        // create a place to save info for names with mismatched emails
        QVector <int> studentsWithDiffEmail;
        studentsWithDiffEmail.reserve(dataOptions->numStudentsInSystem);

        for(auto &name : names)
        {
            int index = 0;     // start at first student in database and look until we find a matching firstname + " " +last name
            while((index < dataOptions->numStudentsInSystem) &&
                  (name.compare(student[index].firstname + " " + student[index].lastname, Qt::CaseInsensitive) != 0))
            {
                index++;
            }

            if(index != dataOptions->numStudentsInSystem)
            {
                // Exact match found
                namesNotFound.removeAll(student[index].firstname + " " + student[index].lastname);
                if(student[index].email.compare(emails.at(names.indexOf(name)), Qt::CaseInsensitive) != 0)
                {
                    // Email in survey doesn't match roster
                    studentsWithDiffEmail << index;
                }
            }
            else
            {
                // No exact match, so list possible matches sorted by Levenshtein distance and allow user to pick a match, add as a new student, or ignore
                auto *choiceWindow = new findMatchingNameDialog(dataOptions->numStudentsInSystem, student, name, this, true, emails.at(names.indexOf(name)));
                if(choiceWindow->exec() == QDialog::Accepted)   // not ignoring this student
                {
                    if(choiceWindow->addStudent)    // add as a new student
                    {
                        dataHasChanged = true;
                        student[dataOptions->numStudentsInSystem].ID = dataOptions->latestStudentID;
                        dataOptions->latestStudentID++;
                        student[dataOptions->numStudentsInSystem].firstname = name.split(" ").first();
                        student[dataOptions->numStudentsInSystem].lastname = name.split(" ").mid(1).join(" ");
                        student[dataOptions->numStudentsInSystem].email = emails.at(names.indexOf(name));
                        student[dataOptions->numStudentsInSystem].ambiguousSchedule = true;
                        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
                        {
                            student[dataOptions->numStudentsInSystem].attributeVals[attribute] << -1;
                        }
                        student[dataOptions->numStudentsInSystem].createTooltip(dataOptions);
                        dataOptions->numStudentsInSystem++;
                        numStudents = dataOptions->numStudentsInSystem;
                    }
                    else   // selected an inexact match
                    {
                        QString surveyName = choiceWindow->currSurveyName;   // need to split off and remove email address
                        namesNotFound.removeAll(surveyName);
                        index = 0;
                        while(surveyName != (student[index].firstname + " " + student[index].lastname))
                        {
                            index++;
                        }
                        if(choiceWindow->useRosterEmail)
                        {
                            dataHasChanged = true;
                            student[index].email = emails.at(names.indexOf(name));
                            student[index].createTooltip(dataOptions);
                        }
                        if(choiceWindow->useRosterName)
                        {
                            dataHasChanged = true;
                            student[index].firstname = name.split(" ").first();
                            student[index].lastname = name.split(" ").mid(1).join(" ");
                            student[index].createTooltip(dataOptions);
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
            QString surveyName = student[studentNum].firstname + " " + student[studentNum].lastname;
            QString surveyEmail = student[studentNum].email;
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
                auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(studentsWithDiffEmail.size() - i) + tr(" students)"));
                whichEmailWindow->setCheckBox(applyToAll);
                connect(applyToAll, &QCheckBox::clicked, this, [&keepAsking] (bool checked) {keepAsking = !checked;});
                whichEmailWindow->button(QMessageBox::Ok)->setText(tr("Use survey email address"));
                connect(whichEmailWindow->button(QMessageBox::Ok), &QPushButton::clicked, whichEmailWindow, &QDialog::accept);
                whichEmailWindow->button(QMessageBox::Cancel)->setText(tr("Use roster email address"));
                connect(whichEmailWindow->button(QMessageBox::Cancel), &QPushButton::clicked, whichEmailWindow, &QDialog::reject);

                if(whichEmailWindow->exec() == QDialog::Rejected)
                {
                    dataHasChanged = true;
                    makeTheChange = true;
                    student[studentNum].email = emails.at(names.indexOf(surveyName));
                    student[studentNum].createTooltip(dataOptions);
                }
                else
                {
                    makeTheChange = false;
                }
                delete whichEmailWindow;
            }
            else if(makeTheChange)
            {
                student[studentNum].email = emails.at(names.indexOf(surveyName));
                student[studentNum].createTooltip(dataOptions);
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
                auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(namesNotFound.size() - i) + tr(" students)"));
                keepOrDeleteWindow->setCheckBox(applyToAll);
                connect(applyToAll, &QCheckBox::clicked, this, [&keepAsking] (bool checked) {keepAsking = !checked;});
                keepOrDeleteWindow->button(QMessageBox::Ok)->setText(tr("Keep ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Ok), &QPushButton::clicked, keepOrDeleteWindow, &QDialog::accept);
                keepOrDeleteWindow->button(QMessageBox::Cancel)->setText(tr("Remove ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Cancel), &QPushButton::clicked, keepOrDeleteWindow, &QDialog::reject);

                if(keepOrDeleteWindow->exec() == QDialog::Rejected)
                {
                    dataHasChanged = true;
                    makeTheChange = true;
                    removeAStudent(0, name, true);
                }
                else
                {
                    makeTheChange = false;
                }

                delete keepOrDeleteWindow;
            }
            else if(makeTheChange)
            {
                removeAStudent(0, name, true);
            }
            i++;
        }

        /* This code is useful if allowing to load a roster without already loading survey
         * If using, put the code above (up to the if(loadRosterData()) inside the else{} below this if{}
            if(dataOptions->numStudentsInSystem == 0)       // no student records already; treat this as if survey file with only names and emails
            {

                if(names.size() < 4)
                {
                    QMessageBox::critical(this, tr("Insufficient number of students."),
                                          tr("There are not enough survey responses in the file."
                                         " There must be at least 4 students for gruepr to work properly."), QMessageBox::Ok);
                    return;
                }

                //reset the UI and data
                resetUI();
                delete[] student;
                student = new StudentRecord[MAX_STUDENTS];
                teamingOptions->reset();
                delete dataOptions;
                dataOptions = new DataOptions;
                dataOptions->dataFile = rosterFile.fileInfo();

                // load students
                for(auto &name : names)
                {
                    student[dataOptions->numStudentsInSystem].ID = dataOptions->latestStudentID;
                    dataOptions->latestStudentID++;
                    student[dataOptions->numStudentsInSystem].firstname = name.split(" ").first();
                    student[dataOptions->numStudentsInSystem].lastname = name.split(" ").mid(1).join(" ");
                    student[dataOptions->numStudentsInSystem].email = emails.at(names.indexOf(name));
                    student[dataOptions->numStudentsInSystem].ambiguousSchedule = true;
                    student[dataOptions->numStudentsInSystem].createTooltip(dataOptions);
                    dataOptions->numStudentsInSystem++;
                }
                numStudents = dataOptions->numStudentsInSystem;

                loadUI();
            }
            else
            {
            }
        */

        if(dataHasChanged)
        {
            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
        }
    }
    rosterFile.close();
}


void gruepr::loadOptionsFile()
{
    //read all options from a text file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), dataOptions->dataFile.canonicalFilePath(),
                                                    tr("gruepr Settings File (*.json);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile loadFile(fileName);
        if(loadFile.open(QIODevice::ReadOnly))
        {
            QJsonDocument loadDoc(QJsonDocument::fromJson(loadFile.readAll()));
            QJsonObject loadObject = loadDoc.object();

            if(loadObject.contains("idealTeamSize") && loadObject["idealTeamSize"].isDouble())
            {
                ui->idealTeamSizeBox->setValue(loadObject["idealTeamSize"].toInt());
                on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
            }
            if(loadObject.contains("isolatedWomenPrevented") && loadObject["isolatedWomenPrevented"].isBool())
            {
                teamingOptions->isolatedWomenPrevented = loadObject["isolatedWomenPrevented"].toBool();
                ui->isolatedWomenCheckBox->setChecked(teamingOptions->isolatedWomenPrevented);
            }
            if(loadObject.contains("isolatedMenPrevented") && loadObject["isolatedMenPrevented"].isBool())
            {
                teamingOptions->isolatedMenPrevented = loadObject["isolatedMenPrevented"].toBool();
                ui->isolatedMenCheckBox->setChecked(teamingOptions->isolatedMenPrevented);
            }
            if(loadObject.contains("isolatedNonbinaryPrevented") && loadObject["isolatedNonbinaryPrevented"].isBool())
            {
                teamingOptions->isolatedNonbinaryPrevented = loadObject["isolatedNonbinaryPrevented"].toBool();
                ui->isolatedNonbinaryCheckBox->setChecked(teamingOptions->isolatedNonbinaryPrevented);
            }
            if(loadObject.contains("singleGenderPrevented") && loadObject["singleGenderPrevented"].isBool())
            {
                teamingOptions->singleGenderPrevented = loadObject["singleGenderPrevented"].toBool();
                ui->mixedGenderCheckBox->setChecked(teamingOptions->singleGenderPrevented);
            }
            if(loadObject.contains("isolatedURMPrevented") && loadObject["isolatedURMPrevented"].isBool())
            {
                teamingOptions->isolatedURMPrevented = loadObject["isolatedURMPrevented"].toBool();
                ui->isolatedURMCheckBox->blockSignals(true);    // prevent select URM identities box from immediately opening
                ui->isolatedURMCheckBox->setChecked(teamingOptions->isolatedURMPrevented);
                ui->isolatedURMCheckBox->blockSignals(false);
            }
            if(loadObject.contains("URMResponsesConsideredUR") && loadObject["URMResponsesConsideredUR"].isString())
            {
                teamingOptions->URMResponsesConsideredUR = loadObject["URMResponsesConsideredUR"].toString().split(';');
            }
            if(loadObject.contains("minTimeBlocksOverlap") && loadObject["minTimeBlocksOverlap"].isDouble())
            {
                teamingOptions->minTimeBlocksOverlap = loadObject["minTimeBlocksOverlap"].toInt();
                ui->minMeetingTimes->setValue(teamingOptions->minTimeBlocksOverlap);
            }
            if(loadObject.contains("desiredTimeBlocksOverlap") && loadObject["desiredTimeBlocksOverlap"].isDouble())
            {
                teamingOptions->desiredTimeBlocksOverlap = loadObject["desiredTimeBlocksOverlap"].toInt();
                ui->desiredMeetingTimes->setValue(teamingOptions->desiredTimeBlocksOverlap);
            }
            if(loadObject.contains("meetingBlockSize") && loadObject["meetingBlockSize"].isDouble())
            {
                teamingOptions->meetingBlockSize = loadObject["meetingBlockSize"].toInt();
                ui->meetingLength->setCurrentIndex(teamingOptions->meetingBlockSize - 1);
            }
            if(loadObject.contains("scheduleWeight") && loadObject["scheduleWeight"].isDouble())
            {
                teamingOptions->scheduleWeight = float(loadObject["scheduleWeight"].toDouble());
                ui->scheduleWeight->setValue(teamingOptions->scheduleWeight);
            }

            for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
            {
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"desireHomogeneous") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"desireHomogeneous"].isBool())
                {
                    teamingOptions->desireHomogeneous[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"desireHomogeneous"].toBool();
                }
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Weight") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Weight"].isDouble())
                {
                    teamingOptions->attributeWeights[attribute] = float(loadObject["Attribute" + QString::number(attribute+1)+"Weight"].toDouble());
                    //reset the weight to zero for any attributes with just one value in the data
                    if(dataOptions->attributeVals[attribute].size() == 1)
                    {
                        teamingOptions->attributeWeights[attribute] = 0;
                    }
                }
                int requiredResponseNum = 0;
                QVector<int> setOfRequiredResponses;
                while(loadObject.contains("Attribute" + QString::number(attribute+1) + "requiredResponse" + QString::number(requiredResponseNum+1)) &&
                      loadObject["Attribute" + QString::number(attribute+1) + "requiredResponse" + QString::number(requiredResponseNum+1)].isDouble())
                {
                    setOfRequiredResponses << loadObject["Attribute" + QString::number(attribute+1) +
                                              "requiredResponse" + QString::number(requiredResponseNum+1)].toInt();
                    requiredResponseNum++;
                }
                teamingOptions->requiredAttributeValues[attribute] = setOfRequiredResponses;
                int incompatibleResponseNum = 0;
                QVector< QPair<int,int> > setOfIncompatibleResponses;
                while(loadObject.contains("Attribute" + QString::number(attribute+1) + "incompatibleResponse" + QString::number(incompatibleResponseNum+1)) &&
                      loadObject["Attribute" + QString::number(attribute+1) + "incompatibleResponse" + QString::number(incompatibleResponseNum+1)].isString())
                {
                    QStringList incoRes = loadObject["Attribute" + QString::number(attribute+1) + "incompatibleResponse" +
                                            QString::number(incompatibleResponseNum+1)].toString().split(',');
                    setOfIncompatibleResponses << QPair<int,int>(incoRes.at(0).toInt(),incoRes.at(1).toInt());
                    incompatibleResponseNum++;
                }
                teamingOptions->incompatibleAttributeValues[attribute] = setOfIncompatibleResponses;
                if(attribute < dataOptions->numAttributes)
                {
                    attributeTab[attribute].setValues(attribute, dataOptions, teamingOptions);
                }
            }

            if(loadObject.contains("numberRequestedTeammatesGiven") && loadObject["numberRequestedTeammatesGiven"].isDouble())
            {
                teamingOptions->numberRequestedTeammatesGiven = loadObject["numberRequestedTeammatesGiven"].toInt();
                ui->requestedTeammateNumberBox->setValue(teamingOptions->numberRequestedTeammatesGiven);
            }

            loadFile.close();
        }
        else
        {
            QMessageBox::critical(this, tr("File Error"), tr("This file cannot be read."));
        }
    }
}


void gruepr::saveOptionsFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), dataOptions->dataFile.canonicalPath(),
                                                    tr("gruepr Settings File (*.json);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile saveFile(fileName);
        if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QJsonObject saveObject;
            saveObject["idealTeamSize"] = ui->idealTeamSizeBox->value();
            saveObject["isolatedWomenPrevented"] = teamingOptions->isolatedWomenPrevented;
            saveObject["isolatedMenPrevented"] = teamingOptions->isolatedMenPrevented;
            saveObject["isolatedNonbinaryPrevented"] = teamingOptions->isolatedNonbinaryPrevented;
            saveObject["singleGenderPrevented"] = teamingOptions->singleGenderPrevented;
            saveObject["isolatedURMPrevented"] = teamingOptions->isolatedURMPrevented;
            saveObject["URMResponsesConsideredUR"] = teamingOptions->URMResponsesConsideredUR.join(';');
            saveObject["minTimeBlocksOverlap"] = teamingOptions->minTimeBlocksOverlap;
            saveObject["desiredTimeBlocksOverlap"] = teamingOptions->desiredTimeBlocksOverlap;
            saveObject["meetingBlockSize"] = teamingOptions->meetingBlockSize;
            saveObject["scheduleWeight"] = teamingOptions->scheduleWeight;
            for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
            {
                saveObject["Attribute" + QString::number(attribute+1)+"desireHomogeneous"] = teamingOptions->desireHomogeneous[attribute];
                saveObject["Attribute" + QString::number(attribute+1)+"Weight"] = teamingOptions->attributeWeights[attribute];
                for(int requiredResp = 0; requiredResp < teamingOptions->requiredAttributeValues[attribute].size(); requiredResp++)
                {
                    saveObject["Attribute" + QString::number(attribute+1)+"requiredResponse" + QString::number(requiredResp+1)] =
                            teamingOptions->requiredAttributeValues[attribute].at(requiredResp);
                }
                for(int incompResp = 0; incompResp < teamingOptions->incompatibleAttributeValues[attribute].size(); incompResp++)
                {
                    saveObject["Attribute" + QString::number(attribute+1)+"incompatibleResponse" + QString::number(incompResp+1)] =
                            (QString::number(teamingOptions->incompatibleAttributeValues[attribute].at(incompResp).first) + "," +
                             QString::number(teamingOptions->incompatibleAttributeValues[attribute].at(incompResp).second));
                }
            }
            saveObject["numberRequestedTeammatesGiven"] = teamingOptions->numberRequestedTeammatesGiven;

            QJsonDocument saveDoc(saveObject);
            saveFile.write(saveDoc.toJson());
            saveFile.close();
        }
        else
        {
            QMessageBox::critical(this, tr("No Files Saved"), tr("This settings file was not saved.\nThere was an issue writing the file to disk."));
        }
    }
}


void gruepr::on_sectionSelectionBox_currentIndexChanged(const QString &desiredSection)
{
    if(desiredSection == "")
    {
        numStudents = 0;
        return;
    }

    sectionName = desiredSection;

    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();
    ui->idealTeamSizeBox->setMaximum(std::max(2,numStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
}


void gruepr::editAStudent()
{
    int indexBeingEdited = sender()->property("StudentIndex").toInt();

    // remove this student's current attribute responses from the counts in dataOptions
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = student[indexBeingEdited].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if(dataOptions->attributeType[attribute] == DataOptions::ordered)
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
            else if((dataOptions->attributeType[attribute] == DataOptions::categorical) || (dataOptions->attributeType[attribute] == DataOptions::timezone))
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
            else
            {
                //multicategorical
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
        }
    }

    //Open window with the student record in it
    auto *win = new editOrAddStudentDialog(student[indexBeingEdited], dataOptions, this, false);

    //If user clicks OK, replace student in the database with edited copy
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        student[indexBeingEdited].createTooltip(dataOptions);
        student[indexBeingEdited].URM = teamingOptions->URMResponsesConsideredUR.contains(student[indexBeingEdited].URMResponse);

        rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    }

    // add back in this student's attribute responses from the counts in dataOptions and update the attribute tabs to show the counts
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = student[indexBeingEdited].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if(dataOptions->attributeType[attribute] == DataOptions::ordered)
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
            }
            else if((dataOptions->attributeType[attribute] == DataOptions::categorical) || (dataOptions->attributeType[attribute] == DataOptions::timezone))
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
            }
            else
            {
                //multicategorical
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                }
            }
            attributeTab[attribute].setValues(attribute, dataOptions, teamingOptions);
        }
    }


    delete win;
}


void gruepr::removeAStudent(int index, const QString &name, bool delayVisualUpdate)
{
    // can be called by index or by name. name defaults to empty, so if name is not empty must use the name to find the index
    int indexBeingRemoved = index;
    if(!name.isEmpty())
    {
        // don't have index, need to search and locate based on name
        while((indexBeingRemoved < dataOptions->numStudentsInSystem) &&
              (name.compare(student[indexBeingRemoved].firstname + " " + student[indexBeingRemoved].lastname), Qt::CaseInsensitive) != 0)
        {
            indexBeingRemoved++;
        }
    }

    if(teamingOptions->haveAnyRequiredTeammates || teamingOptions->haveAnyRequestedTeammates)
    {
        // remove this student from all other students who might have them as required/prevented/requested
        const int IDBeingRemoved = student[indexBeingRemoved].ID;
        for(int otherIndex = 0; otherIndex < dataOptions->numStudentsInSystem; otherIndex++)
        {
            student[otherIndex].requiredWith[IDBeingRemoved] = false;
            student[otherIndex].preventedWith[IDBeingRemoved] = false;
            student[otherIndex].requestedWith[IDBeingRemoved] = false;
        }
    }

    // update in dataOptions and then the attribute tab the count of each attribute response
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        const QString &currentStudentResponse = student[indexBeingRemoved].attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty())
        {
            if(dataOptions->attributeType[attribute] == DataOptions::ordered)
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
            else if((dataOptions->attributeType[attribute] == DataOptions::categorical) || (dataOptions->attributeType[attribute] == DataOptions::timezone))
            {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
            else
            {
                //multicategorical
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
            attributeTab[attribute].setValues(attribute, dataOptions, teamingOptions);
        }
    }

    //Remove the student by moving all subsequent ones in the array ahead by one
    dataOptions->numStudentsInSystem--;
    for(int index = indexBeingRemoved; index < dataOptions->numStudentsInSystem; index++)
    {
        student[index] = student[index+1];
    }

    if(delayVisualUpdate)
    {
        return;
    }

    rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
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
            const int newIndex = dataOptions->numStudentsInSystem;
            student[newIndex] = newStudent;
            student[newIndex].ID = dataOptions->latestStudentID;
            student[newIndex].createTooltip(dataOptions);
            student[newIndex].URM = teamingOptions->URMResponsesConsideredUR.contains(student[newIndex].URMResponse);

            // update in dataOptions and then the attribute tab the count of each attribute response
            for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
            {
                const QString &currentStudentResponse = student[newIndex].attributeResponse[attribute];
                if(!currentStudentResponse.isEmpty())
                {
                    if(dataOptions->attributeType[attribute] == DataOptions::ordered)
                    {
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if((dataOptions->attributeType[attribute] == DataOptions::categorical) || (dataOptions->attributeType[attribute] == DataOptions::timezone))
                    {
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else
                    {
                        //multicategorical
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : qAsConst(setOfResponsesFromStudent))
                        {
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                    attributeTab[attribute].setValues(attribute, dataOptions, teamingOptions);
                }
            }

            dataOptions->latestStudentID++;
            dataOptions->numStudentsInSystem++;

            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
        }

        delete win;
    }
    else
    {
        QMessageBox::warning(this, tr("Cannot add student."),
                             tr("Sorry, we cannot add another student.\nThis version of gruepr does not allow more than ") +
                             QString(MAX_STUDENTS) + tr("."), QMessageBox::Ok);
    }
}


void gruepr::rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable()
{
    // go back through all records to see if any are duplicates; assume each isn't and then check
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        student[index].duplicateRecord = false;
        for(int index2 = 0; index2 < index; index2++)
        {
            if((student[index].firstname + student[index].lastname == student[index2].firstname + student[index2].lastname) ||
                    ((student[index].email == student[index2].email) && !student[index].email.isEmpty()))
            {
                student[index].duplicateRecord = true;
                student[index2].duplicateRecord = true;
                student[index2].createTooltip(dataOptions);
            }
        }
        student[index].createTooltip(dataOptions);
    }

    // Re-build the URM info
    if(dataOptions->URMIncluded)
    {
        dataOptions->URMResponses.clear();
        for(int URMindex = 0; URMindex < dataOptions->numStudentsInSystem; URMindex++)
        {
            if(!dataOptions->URMResponses.contains(student[URMindex].URMResponse, Qt::CaseInsensitive))
            {
                dataOptions->URMResponses << student[URMindex].URMResponse;
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
            if(!dataOptions->sectionNames.contains(student[sectionIndex].section, Qt::CaseInsensitive))
            {
                dataOptions->sectionNames << student[sectionIndex].section;
            }
        }
        if(dataOptions->sectionNames.size() > 1)
        {
            QCollator sortAlphanumerically;
            sortAlphanumerically.setNumericMode(true);
            sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);
            ui->sectionSelectionBox->setEnabled(true);
            ui->label_2->setEnabled(true);
            ui->label_22->setEnabled(true);
            ui->sectionSelectionBox->addItem(tr("Students in all sections together"));
            ui->sectionSelectionBox->insertSeparator(1);
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

    // Enable save data file option, since data set is now edited
    ui->saveSurveyFilePushButton->setEnabled(true);
    ui->actionSave_Survey_File->setEnabled(true);

    // Refresh student table data
    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();

    // Load new team sizes in selection box
    ui->idealTeamSizeBox->setMaximum(std::max(2,numStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());

}


void gruepr::on_saveSurveyFilePushButton_clicked()
{
    CsvFile newSurveyFile;
    if(!newSurveyFile.open(this, CsvFile::write, tr("Save Survey Data File"), dataOptions->dataFile.canonicalPath(), tr("Survey Data")))
    {
        return;
    }

    // write header
    if(dataOptions->timestampField != -1)
    {
        newSurveyFile.headerValues << "Timestamp";
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
        newSurveyFile.headerValues << "With which gender do you identify?";
    }
    if(dataOptions->URMIncluded)
    {
        newSurveyFile.headerValues << "How do you identify your race, ethnicity, or cultural heritage?";
    }
    for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
    {
        newSurveyFile.headerValues << dataOptions->attributeQuestionText[attrib];
    }
    for(int day = 0; day < dataOptions->dayNames.size(); day++)
    {
        if(dataOptions->scheduleDataIsFreetime)
        {
            newSurveyFile.headerValues << "Check the times that you are FREE and will be AVAILABLE for group work. [" + dataOptions->dayNames[day] + "]";
        }
        else
        {
            newSurveyFile.headerValues << "Check the times that you are BUSY and will be UNAVAILABLE for group work. [" + dataOptions->dayNames[day] + "]";
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
        QMessageBox::critical(this, tr("No File Saved"), tr("No file was saved.\nThere was an issue writing the file."));
        return;
    }

    // Write each student's info
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        newSurveyFile.fieldValues.clear();
        newSurveyFile.fieldValues << student[index].surveyTimestamp.toString(Qt::ISODate) <<
                                     student[index].firstname << student[index].lastname << student[index].email;
        if(dataOptions->genderIncluded)
        {
            if(student[index].gender == StudentRecord::woman)
            {
                newSurveyFile.fieldValues << tr("woman");
            }
            else if(student[index].gender == StudentRecord::man)
            {
                newSurveyFile.fieldValues << tr("man");
            }
            else if(student[index].gender == StudentRecord::nonbinary)
            {
                newSurveyFile.fieldValues << tr("nonbinary");
            }
            else
            {
                newSurveyFile.fieldValues << tr("unknown");
            }
        }
        if(dataOptions->URMIncluded)
        {
            newSurveyFile.fieldValues << (student[index].URMResponse);
        }
        for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
        {
            newSurveyFile.fieldValues << student[index].attributeResponse[attrib];
        }
        for(int day = 0; day < dataOptions->dayNames.size(); day++)
        {
            QString times;
            bool first = true;
            for(int time = 0; time < dataOptions->timeNames.size(); time++)
            {
                if(dataOptions->scheduleDataIsFreetime)
                {
                    if(!student[index].unavailable[day][time])
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
                    if(student[index].unavailable[day][time])
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
            newSurveyFile.fieldValues << student[index].section;
        }
        if(dataOptions->prefTeammatesIncluded)
        {
            newSurveyFile.fieldValues << student[index].prefTeammates.replace('\n',";");
        }
        if(dataOptions->prefNonTeammatesIncluded)
        {
            newSurveyFile.fieldValues << student[index].prefNonTeammates.replace('\n',";");
        }
        if(dataOptions->numNotes > 0)
        {
            newSurveyFile.fieldValues << student[index].notes;
        }
        newSurveyFile.writeDataRow();
    }

    newSurveyFile.close();

    ui->saveSurveyFilePushButton->setEnabled(false);
    ui->actionSave_Survey_File->setEnabled(false);
}


void gruepr::on_isolatedWomenCheckBox_stateChanged(int arg1)
{
    teamingOptions->isolatedWomenPrevented = (arg1 != 0);
}


void gruepr::on_isolatedMenCheckBox_stateChanged(int arg1)
{
    teamingOptions->isolatedMenPrevented = (arg1 != 0);
}


void gruepr::on_isolatedNonbinaryCheckBox_stateChanged(int arg1)
{
    teamingOptions->isolatedNonbinaryPrevented = (arg1 != 0);
}


void gruepr::on_mixedGenderCheckBox_stateChanged(int arg1)
{
    teamingOptions->singleGenderPrevented = (arg1 != 0);
}


void gruepr::on_isolatedURMCheckBox_stateChanged(int arg1)
{
    teamingOptions->isolatedURMPrevented = (arg1 != 0);
    if(teamingOptions->isolatedURMPrevented && teamingOptions->URMResponsesConsideredUR.isEmpty())
    {
        // if we are preventing isolated URM students, but have not selected yet which responses should be considered URM, let's ask user to enter those in
        on_URMResponsesButton_clicked();
    }
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
            student[index].URM = teamingOptions->URMResponsesConsideredUR.contains(student[index].URMResponse);
        }
    }

    delete win;
}


void gruepr::requiredResponsesButton_clicked()
{
    int currAttribute = ui->attributesTabWidget->currentIndex();
    //Open specialized dialog box to collect attribute values that are required on each team
    auto *win = new gatherAttributeValuesDialog(currAttribute, dataOptions, teamingOptions, gatherAttributeValuesDialog::required, this);

    //If user clicks OK, replace student database with copy that has had values added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        teamingOptions->haveAnyRequiredAttributes[currAttribute] = !(win->requiredValues.isEmpty());
        teamingOptions->requiredAttributeValues[currAttribute] = win->requiredValues;
    }

    delete win;
}


void gruepr::incompatibleResponsesButton_clicked()
{
    int currAttribute = ui->attributesTabWidget->currentIndex();
    //Open specialized dialog box to collect attribute pairings that should prevent students from being on the same team
    auto *win = new gatherAttributeValuesDialog(currAttribute, dataOptions, teamingOptions, gatherAttributeValuesDialog::incompatible, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        teamingOptions->haveAnyIncompatibleAttributes[currAttribute] = !(win->incompatibleValues.isEmpty());
        teamingOptions->incompatibleAttributeValues[currAttribute] = win->incompatibleValues;
    }

    delete win;
}


void gruepr::on_scheduleWeight_valueChanged(double arg1)
{
    teamingOptions->scheduleWeight = float(arg1);
}


void gruepr::on_minMeetingTimes_valueChanged(int arg1)
{
    teamingOptions->minTimeBlocksOverlap = arg1;
    if(ui->desiredMeetingTimes->value() < (arg1))
    {
        ui->desiredMeetingTimes->setValue(arg1);
    }
}


void gruepr::on_desiredMeetingTimes_valueChanged(int arg1)
{
    teamingOptions->desiredTimeBlocksOverlap = arg1;
    if(ui->minMeetingTimes->value() > (arg1))
    {
        ui->minMeetingTimes->setValue(arg1);
    }
}


void gruepr::on_meetingLength_currentIndexChanged(int index)
{
    teamingOptions->meetingBlockSize = (index + 1);
    ui->minMeetingTimes->setMaximum((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (index + 1));
    ui->desiredMeetingTimes->setMaximum((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (index + 1));
}


void gruepr::on_requiredTeammatesButton_clicked()
{
    QStringList teamTabNames;
    for(int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++)
    {
        teamTabNames << ui->dataDisplayTabWidget->tabText(tab);
    }
    //Open specialized dialog box to collect pairings that are required
    auto *win = new gatherTeammatesDialog(gatherTeammatesDialog::required, student, dataOptions->numStudentsInSystem,
                                          dataOptions, (ui->sectionSelectionBox->currentIndex()==0)? "" : sectionName, &teamTabNames, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            this->student[index] = win->student[index];
        }
        teamingOptions->haveAnyRequiredTeammates = win->teammatesSpecified;
    }

    delete win;
}


void gruepr::on_preventedTeammatesButton_clicked()
{
    QStringList teamTabNames;
    for(int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++)
    {
        teamTabNames << ui->dataDisplayTabWidget->tabText(tab);
    }
    //Open specialized dialog box to collect pairings that are prevented
    auto *win = new gatherTeammatesDialog(gatherTeammatesDialog::prevented, student, dataOptions->numStudentsInSystem,
                                          dataOptions, (ui->sectionSelectionBox->currentIndex()==0)? "" : sectionName, &teamTabNames, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            this->student[index] = win->student[index];
        }
        teamingOptions->haveAnyPreventedTeammates = win->teammatesSpecified;
    }

    delete win;
}


void gruepr::on_requestedTeammatesButton_clicked()
{
    QStringList teamTabNames;
    for(int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++)
    {
        teamTabNames << ui->dataDisplayTabWidget->tabText(tab);
    }
    //Open specialized dialog box to collect pairings that are requested
    auto *win = new gatherTeammatesDialog(gatherTeammatesDialog::requested, student, dataOptions->numStudentsInSystem,
                                          dataOptions, (ui->sectionSelectionBox->currentIndex()==0)? "" : sectionName, &teamTabNames, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = win->exec();
    if(reply == QDialog::Accepted)
    {
        for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
        {
            this->student[index] = win->student[index];
        }
        teamingOptions->haveAnyRequestedTeammates = win->teammatesSpecified;
    }

    delete win;
}


void gruepr::on_requestedTeammateNumberBox_valueChanged(int arg1)
{
    teamingOptions->numberRequestedTeammatesGiven = arg1;
}


void gruepr::on_idealTeamSizeBox_valueChanged(int arg1)
{
    ui->requestedTeammateNumberBox->setMaximum(arg1);

    //put suitable options in the team size selection box, depending on whether the number of students is evenly divisible by this desired team size
    ui->teamSizeBox->clear();

    teamingOptions->numTeamsDesired = std::max(1, numStudents/arg1);
    teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired;
    teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;

    if(numStudents%arg1 != 0)       //if teams can't be evenly divided into this size
    {
        int smallerTeamsSizeA=0, smallerTeamsSizeB=0, numSmallerATeams=0, largerTeamsSizeA=0, largerTeamsSizeB=0, numLargerATeams=0;

        // reset the potential team sizes
        for(int student = 0; student < MAX_STUDENTS; student++)
        {
            teamingOptions->smallerTeamsSizes[student] = 0;
            teamingOptions->largerTeamsSizes[student] = 0;
        }

        // What are the team sizes when desiredTeamSize represents a maximum size?
        teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired+1;
        for(int student = 0; student < numStudents; student++)      // run through every student
        {
            // add one student to each team (with 1 additional team relative to before) in turn until we run out of students
            (teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams])++;
            smallerTeamsSizeA = teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams];  // the larger of the two (uneven) team sizes
            numSmallerATeams = (student%teamingOptions->smallerTeamsNumTeams)+1;                                 // the number of larger teams
        }
        smallerTeamsSizeB = smallerTeamsSizeA - 1;                  // the smaller of the two (uneven) team sizes

        // And what are the team sizes when desiredTeamSize represents a minimum size?
        teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;
        for(int student = 0; student < numStudents; student++)	// run through every student
        {
            // add one student to each team in turn until we run out of students
            (teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams])++;
            largerTeamsSizeA = teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams];     // the larger of the two (uneven) team sizes
            numLargerATeams = (student%teamingOptions->largerTeamsNumTeams)+1;                                   // the number of larger teams
        }
        largerTeamsSizeB = largerTeamsSizeA - 1;					// the smaller of the two (uneven) team sizes

        // Add first option to selection box
        QString smallerTeamOption;
        if(numSmallerATeams > 0)
        {
            smallerTeamOption += QString::number(numSmallerATeams) + tr(" team");
            if(numSmallerATeams > 1)
            {
                smallerTeamOption += "s";
            }
            smallerTeamOption += " of " + QString::number(smallerTeamsSizeA) + tr(" student");
            if(smallerTeamsSizeA > 1)
            {
                smallerTeamOption += "s";
            }
        }
        if((numSmallerATeams > 0) && ((teamingOptions->numTeamsDesired+1-numSmallerATeams) > 0))
        {
            smallerTeamOption += " + ";
        }
        if((teamingOptions->numTeamsDesired+1-numSmallerATeams) > 0)
        {
            smallerTeamOption += QString::number(teamingOptions->numTeamsDesired+1-numSmallerATeams) + tr(" team");
            if((teamingOptions->numTeamsDesired+1-numSmallerATeams) > 1)
            {
                smallerTeamOption += "s";
            }
            smallerTeamOption += " of " + QString::number(smallerTeamsSizeB) + tr(" student");
            if(smallerTeamsSizeB > 1)
            {
                smallerTeamOption += "s";
            }
        }

        // Add second option to selection box
        QString largerTeamOption;
        if((teamingOptions->numTeamsDesired-numLargerATeams) > 0)
        {
            largerTeamOption += QString::number(teamingOptions->numTeamsDesired-numLargerATeams) + tr(" team");
            if((teamingOptions->numTeamsDesired-numLargerATeams) > 1)
            {
                largerTeamOption += "s";
            }
            largerTeamOption += " of " + QString::number(largerTeamsSizeB) + tr(" student");
            if(largerTeamsSizeB > 1)
            {
                largerTeamOption += "s";
            }
        }
        if(((teamingOptions->numTeamsDesired-numLargerATeams) > 0) && (numLargerATeams > 0))
        {
            largerTeamOption += " + ";
        }
        if(numLargerATeams > 0)
        {
            largerTeamOption += QString::number(numLargerATeams) + tr(" team");
            if(numLargerATeams > 1)
            {
                largerTeamOption += "s";
            }
            largerTeamOption += " of " + QString::number(largerTeamsSizeA) + tr(" student");
            if(largerTeamsSizeA > 1)
            {
                largerTeamOption += "s";
            }
        }

        ui->teamSizeBox->addItem(smallerTeamOption);
        ui->teamSizeBox->addItem(largerTeamOption);
    }
    else
    {
        ui->teamSizeBox->addItem(QString::number(teamingOptions->numTeamsDesired) + tr(" teams of ") + QString::number(arg1) + tr(" students"));
    }
    ui->teamSizeBox->insertSeparator(ui->teamSizeBox->count());
    ui->teamSizeBox->addItem(tr("Custom team sizes"));

    // if we have fewer than 4 students somehow, disable the form teams button
    ui->letsDoItButton->setEnabled(numStudents >= 4);
}


void gruepr::on_teamSizeBox_currentIndexChanged(int index)
{
    if(ui->teamSizeBox->currentText() == (QString::number(teamingOptions->numTeamsDesired) + tr(" teams of ") + QString::number(ui->idealTeamSizeBox->value()) + tr(" students")))
    {
        // Evenly divisible teams, all same size
        setTeamSizes(ui->idealTeamSizeBox->value());
    }
    else if(ui->teamSizeBox->currentText() == tr("Custom team sizes"))
    {
        //Open specialized dialog box to collect teamsizes
        auto *win = new customTeamsizesDialog(numStudents, ui->idealTeamSizeBox->value(), this);

        //If user clicks OK, use these team sizes, otherwise revert to option 1, smaller team sizes
        int reply = win->exec();
        if(reply == QDialog::Accepted)
        {
            teamingOptions->numTeamsDesired = win->numTeams;
            setTeamSizes(win->teamsizes);
        }
        else
        {
            // Set to index 0 if cancelled
            bool oldState = ui->teamSizeBox->blockSignals(true);
            ui->teamSizeBox->setCurrentIndex(0);
            if(ui->teamSizeBox->currentText() == (QString::number(teamingOptions->numTeamsDesired) + tr(" teams of ") +
                                                  QString::number(ui->idealTeamSizeBox->value()) + tr(" students")))
            {
                // Evenly divisible teams, all same size
                setTeamSizes(ui->idealTeamSizeBox->value());
            }
            else
            {
                teamingOptions->numTeamsDesired = teamingOptions->smallerTeamsNumTeams;
                setTeamSizes(teamingOptions->smallerTeamsSizes);
            }
            ui->teamSizeBox->blockSignals(oldState);
        }

        delete win;
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
        int buttonClicked = QMessageBox::warning(this, tr("gruepr"),
                                                 tr("You have selected to prevented isolated URM students,\n"
                                                    "however none of the race/ethnicity response values\n"
                                                    "have been selected to be considered as underrepresented.\n\n"
                                                    "Click OK to continue with no students considered URM,\n"
                                                    "or click Cancel to go back and select URM responses."),
                                                 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if(buttonClicked == QMessageBox::Cancel)
        {
            ui->URMResponsesButton->setFocus();
            return;
        }
    }

    // Create a new array of TeamRecords to hold the eventual results
    numTeams = teamingOptions->numTeamsDesired;
    teams = new TeamRecord[numTeams];
    for(int team = 0; team < numTeams; team++)	// run through every team to load dataOptions and size
    {
        teams[team] = TeamRecord(*dataOptions, teamingOptions->teamSizesDesired[team]);
    }

    // Normalize all score factor weights using norm factor = number of factors / total weights of all factors
    teamingOptions->realNumScoringFactors = dataOptions->numAttributes + (dataOptions->dayNames.isEmpty()? 0 : 1);
    float normFactor = (float(teamingOptions->realNumScoringFactors)) /
            (std::accumulate(teamingOptions->attributeWeights, teamingOptions->attributeWeights + dataOptions->numAttributes, float(0.0)) +
             (dataOptions->dayNames.isEmpty()? 0 : teamingOptions->scheduleWeight));
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        teamingOptions->realAttributeWeights[attribute] = teamingOptions->attributeWeights[attribute] * normFactor;
    }
    teamingOptions->realScheduleWeight = (dataOptions->dayNames.isEmpty()? 0 : teamingOptions->scheduleWeight) * normFactor;

#ifdef Q_OS_WIN32
    // Set up to show progess on windows taskbar
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(windowHandle());
    taskbarProgress = taskbarButton->progress();
    taskbarProgress->show();
    taskbarProgress->setMaximum(0);
#endif

    // Create progress display plot
    progressChart = new BoxWhiskerPlot("", "Generation", "Scores");
    auto *chartView = new QtCharts::QChartView(progressChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Create window to display progress, and connect the stop optimization button in the window to the actual stopping of the optimization thread
    progressWindow = new progressDialog(chartView, this);
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
        if(ui->sectionSelectionBox->currentIndex() == 0 || ui->sectionSelectionBox->currentText() == student[index].section)
        {
            studentIndexes[numStudentsInSection] = index;
            numStudentsInSection++;
        }
    }
    numStudents = numStudentsInSection;

    // Set up the flag to allow a stoppage and set up futureWatcher to know when results are available
    optimizationStopped = false;
    future = QtConcurrent::run(this, &gruepr::optimizeTeams, studentIndexes);       // spin optimization off into a separate thread
    futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
}


void gruepr::updateOptimizationProgress(const QVector<float> &allScores, const int *const orderedIndex, const int generation, const float scoreStability)
{
    if((generation % (progressChart->plotFrequency)) == 0)
    {
        progressChart->loadNextVals(allScores, orderedIndex);
    }

    if(generation > MAX_GENERATIONS)
    {
        progressWindow->setText(tr("We have reached ") + QString::number(MAX_GENERATIONS) + tr(" generations."),
                                generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), true);
        progressWindow->highlightStopButton();
    }
    else if( (generation >= MIN_GENERATIONS) && (scoreStability > MIN_SCORE_STABILITY) )
    {
        progressWindow->setText(tr("Score appears to be stable."), generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), true);
        progressWindow->highlightStopButton();
    }
    else
    {
        progressWindow->setText(tr("Optimization in progress."), generation, *std::max_element(allScores.constBegin(), allScores.constEnd()), false);
    }

#ifdef Q_OS_WIN32
    if(generation >= GENERATIONS_OF_STABILITY)
    {
        taskbarProgress->setMaximum(100);
        taskbarProgress->setValue((scoreStability<100)? static_cast<int>(scoreStability) : 100);
    }
#endif
}


void gruepr::optimizationComplete()
{
    //alert
    QApplication::beep();
    QApplication::alert(this);

    // update UI
#ifdef Q_OS_WIN32
    taskbarProgress->hide();
#endif
    delete progressChart;
    delete progressWindow;

    // free memory used to save array of IDs of students being teamed
    delete[] studentIndexes;

    // Get the results
    QVector<int> bestTeamSet = future.result();

    // Load students into teams
    int indexInTeamset = 0;
    for(int team = 0; team < numTeams; team++)
    {
        auto &IndexList = teams[team].studentIndexes;
        IndexList.clear();
        for(int studentNum = 0, size = teams[team].size; studentNum < size; studentNum++)
        {
            IndexList << bestTeamSet.at(indexInTeamset);
            indexInTeamset++;
        }
        //sort teammates within a team alphabetically by lastname,firstname
        std::sort(IndexList.begin(), IndexList.end(), [this] (const int a, const int b)
                                                        {return ((student[a].lastname + student[a].firstname) < (student[b].lastname + student[b].firstname));});
    }

    // Load scores and info into the teams
    getTeamScores(student, numStudents, teams, numTeams, teamingOptions, dataOptions);
    for(int team = 0; team < numTeams; team++)
    {
        teams[team].refreshTeamInfo(student);
    }

    // Sort teams by 1st student's name, then set default teamnames and create tooltips
    std::sort(teams, teams+numTeams, [this](const TeamRecord &a, const TeamRecord &b)
                                            {return ((student[a.studentIndexes.at(0)].lastname + student[a.studentIndexes.at(0)].firstname) <
                                                    (student[b.studentIndexes.at(0)].lastname + student[b.studentIndexes.at(0)].firstname));});
    for(int team = 0; team < numTeams; team++)
    {
        teams[team].name = QString::number(team+1);
        teams[team].createTooltip();
    }

    // Display the results in a new tab
    // Eventually maybe this should let the tab take ownership of the teams pointer, deleting when the tab is closed!
    auto teamTab = new TeamsTabItem(teamingOptions, sectionName, dataOptions, teams, numTeams, student, this);
    static int teamsetNum = 1;
    ui->dataDisplayTabWidget->addTab(teamTab, tr("Team set ") + QString::number(teamsetNum));

    // if this is the first teams tab, make the tabs closable, hide the close button on the students tab, & engage signals for tabs closing, switching, & double-click
    if(teamsetNum == 1)
    {
        ui->dataDisplayTabWidget->setTabsClosable(true);
        ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
        ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);
        connect(ui->dataDisplayTabWidget, &QTabWidget::tabCloseRequested, this, &gruepr::dataDisplayTabClose);
        connect(ui->dataDisplayTabWidget, &QTabWidget::currentChanged, this, &gruepr::dataDisplayTabSwitch);
        connect(ui->dataDisplayTabWidget, &QTabWidget::tabBarDoubleClicked, this, &gruepr::editDataDisplayTabName);
    }
    teamsetNum++;

    ui->actionSave_Teams->setEnabled(true);
    ui->actionPrint_Teams->setEnabled(true);

    ui->dataDisplayTabWidget->setCurrentWidget(teamTab);
}


void gruepr::dataDisplayTabSwitch(int newTabIndex)
{
    // don't do anything if they selected the student tab
    if(newTabIndex < 1)
    {
        return;
    }

    // update the save and print teams menu items to this new tab
    ui->actionSave_Teams->disconnect();
    ui->actionPrint_Teams->disconnect();
    auto tab = qobject_cast<TeamsTabItem *>(ui->dataDisplayTabWidget->widget(newTabIndex));
    ui->actionSave_Teams->setText(tr("Save Teams") + " (" + ui->dataDisplayTabWidget->tabText(newTabIndex) + ")...");
    connect(ui->actionSave_Teams, &QAction::triggered, tab, &TeamsTabItem::saveTeams);
    ui->actionPrint_Teams->setText(tr("Print Teams") + " (" + ui->dataDisplayTabWidget->tabText(newTabIndex) + ")...");
    connect(ui->actionPrint_Teams, &QAction::triggered, tab, &TeamsTabItem::printTeams);
}


void gruepr::dataDisplayTabClose(int closingTabIndex)
{
    // don't close the student tab!
    if(closingTabIndex < 1)
    {
        return;
    }

    if(ui->dataDisplayTabWidget->count() == 2)
    {
        // we're going to be down to just the student tab, so disable the save and print teams menu items
        ui->actionSave_Teams->disconnect();
        ui->actionPrint_Teams->disconnect();
        ui->actionSave_Teams->setText(tr("Save Teams..."));
        ui->actionSave_Teams->setEnabled(false);
        ui->actionPrint_Teams->setText(tr("Print Teams..."));
        ui->actionPrint_Teams->setEnabled(false);
    }
    else if(ui->dataDisplayTabWidget->currentIndex() == 0 && ui->actionSave_Teams->text().contains(ui->dataDisplayTabWidget->tabText(closingTabIndex)))
    {
        // we're viewing the student tab and the tab we're closing is the one currently pointed to by the save and print teams menu items
        // redirect these menu items to point to the next tab down (or next one up if next one down is the students tab
        ui->actionSave_Teams->disconnect();
        ui->actionPrint_Teams->disconnect();
        int nextLogicalIndex = ((closingTabIndex == 1) ? (2) : (closingTabIndex - 1));
        auto tab = qobject_cast<TeamsTabItem *>(ui->dataDisplayTabWidget->widget(nextLogicalIndex));
        ui->actionSave_Teams->setText(tr("Save Teams") + " (" + ui->dataDisplayTabWidget->tabText(nextLogicalIndex) + ")...");
        connect(ui->actionSave_Teams, &QAction::triggered, tab, &TeamsTabItem::saveTeams);
        ui->actionPrint_Teams->setText(tr("Print Teams") + " (" + ui->dataDisplayTabWidget->tabText(nextLogicalIndex) + ")...");
        connect(ui->actionPrint_Teams, &QAction::triggered, tab, &TeamsTabItem::printTeams);
    }

    auto tab = ui->dataDisplayTabWidget->widget(closingTabIndex);
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
    auto win = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    win->setWindowTitle(tr("Rename this team set"));
    win->setSizeGripEnabled(true);
    win->move(QCursor::pos());
    auto layout = new QVBoxLayout(win);
    auto newNameEditor = new QLineEdit(win);
    newNameEditor->setText(ui->dataDisplayTabWidget->tabText(tabIndex));
    newNameEditor->setPlaceholderText(ui->dataDisplayTabWidget->tabText(tabIndex));
    layout->addWidget(newNameEditor);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, win);
    connect(buttons, &QDialogButtonBox::accepted, win, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, win, &QDialog::reject);
    layout->addWidget(buttons);
    if(win->exec() == QDialog::Accepted && !newNameEditor->text().isEmpty())
    {
        ui->dataDisplayTabWidget->setTabText(tabIndex, newNameEditor->text());
    }
    win->deleteLater();

    // update the text in the save and print teams menu items to this new tab name
    ui->actionSave_Teams->setText(tr("Save Teams") + " (" + ui->dataDisplayTabWidget->tabText(tabIndex) + ")...");
    ui->actionPrint_Teams->setText(tr("Print Teams") + " (" + ui->dataDisplayTabWidget->tabText(tabIndex) + ")...");
}


void gruepr::settingsWindow()
{
}


void gruepr::helpWindow()
{
    QFile helpFile(":/help.html");
    if (!helpFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QDialog helpWindow(this);
    helpWindow.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    helpWindow.setSizeGripEnabled(true);
    helpWindow.setWindowTitle("Help");
    QGridLayout theGrid(&helpWindow);
    QTextBrowser helpContents(&helpWindow);
    helpContents.setHtml(tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr " GRUEPR_VERSION_NUMBER "</h1>"
                            "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                            "<p>Joshua Hertz <a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                            "<p>Project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>"));
    helpContents.append(helpFile.readAll());
    helpFile.close();
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(LG_DLG_SIZE, LG_DLG_SIZE);
    helpWindow.exec();
}


void gruepr::aboutWindow()
{
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString user = registeredUser.isEmpty()? tr("UNREGISTERED") : (tr("registered to ") + registeredUser);
    QMessageBox::about(this, tr("About gruepr"),
                       tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr " GRUEPR_VERSION_NUMBER "</h1>"
                          "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                          "<br>Joshua Hertz<br><a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                          "<p>This copy of gruepr is ") + user + tr("."
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>."
                          "<p>gruepr incorporates:"
                             "<ul><li>Code libraries from <a href = http://qt.io>Qt, v 5.15</a>, released under the GNU Lesser General Public License version 3</li>"
                             "<li>Icons from <a href = https://icons8.com>Icons8</a>, released under Creative Commons license \"Attribution-NoDerivs 3.0 Unported\"</li>"
                             "<li><span style=\"font-family:'Oxygen Mono';\">The font <a href = https://www.fontsquirrel.com/fonts/oxygen-mono>"
                                                                   "Oxygen Mono</a>, Copyright &copy; 2012, Vernon Adams (vern@newtypography.co.uk),"
                                                                   " released under SIL OPEN FONT LICENSE V1.1.</span></li>"
                             "<li>A photo of a grouper, courtesy Rich Whalen</li></ul>"
                          "<h3>Disclaimer</h3>"
                          "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of "
                          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                          "<p>This program is free software: you can redistribute it and/or modify it under the terms of the "
                          "<a href = https://www.gnu.org/licenses/gpl.html>GNU General Public License</a> "
                          "as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."));
}


//////////////////
//Load window geometry and default teaming options saved from previous run. If non-existant, load app defaults.
//////////////////
void gruepr::loadDefaultSettings()
{
    QSettings savedSettings;

    //Restore window geometry
    restoreGeometry(savedSettings.value("windowGeometry").toByteArray());

    //Restore last data file folder location
    dataOptions->dataFile.setFile(savedSettings.value("dataFileLocation", "").toString());

    //Restore teaming options
    ui->idealTeamSizeBox->setValue(savedSettings.value("idealTeamSize", 4).toInt());
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());        // load new team sizes in teamingOptions and in ui selection box
    teamingOptions->isolatedWomenPrevented = savedSettings.value("isolatedWomenPrevented", false).toBool();
    ui->isolatedWomenCheckBox->setChecked(teamingOptions->isolatedWomenPrevented);
    teamingOptions->isolatedMenPrevented = savedSettings.value("isolatedMenPrevented", false).toBool();
    ui->isolatedMenCheckBox->setChecked(teamingOptions->isolatedMenPrevented);
    teamingOptions->isolatedNonbinaryPrevented = savedSettings.value("isolatedNonbinaryPrevented", false).toBool();
    ui->isolatedNonbinaryCheckBox->setChecked(teamingOptions->isolatedNonbinaryPrevented);
    teamingOptions->singleGenderPrevented = savedSettings.value("singleGenderPrevented", false).toBool();
    ui->mixedGenderCheckBox->setChecked(teamingOptions->singleGenderPrevented);
    teamingOptions->isolatedURMPrevented = savedSettings.value("isolatedURMPrevented", false).toBool();
    ui->isolatedURMCheckBox->blockSignals(true);    // prevent select URM identities box from immediately opening
    ui->isolatedURMCheckBox->setChecked(teamingOptions->isolatedURMPrevented);
    ui->isolatedURMCheckBox->blockSignals(false);
    teamingOptions->minTimeBlocksOverlap = savedSettings.value("minTimeBlocksOverlap", 4).toInt();
    ui->minMeetingTimes->setValue(teamingOptions->minTimeBlocksOverlap);
    teamingOptions->desiredTimeBlocksOverlap = savedSettings.value("desiredTimeBlocksOverlap", 8).toInt();
    ui->desiredMeetingTimes->setValue(teamingOptions->desiredTimeBlocksOverlap);
    teamingOptions->meetingBlockSize = savedSettings.value("meetingBlockSize", 1).toInt();
    ui->meetingLength->setCurrentIndex(teamingOptions->meetingBlockSize-1);
    teamingOptions->scheduleWeight = savedSettings.value("scheduleWeight", 4).toFloat();
    ui->scheduleWeight->setValue(double(teamingOptions->scheduleWeight));
    savedSettings.beginReadArray("Attributes");
    for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum)
    {
        savedSettings.setArrayIndex(attribNum);
        teamingOptions->desireHomogeneous[attribNum] = savedSettings.value("desireHomogeneous", false).toBool();
        teamingOptions->attributeWeights[attribNum] = savedSettings.value("Weight", 1).toFloat();
        int numIncompats = savedSettings.beginReadArray("incompatibleResponses");
        for(int incompResp = 0; incompResp < numIncompats; incompResp++)
        {
            savedSettings.setArrayIndex(incompResp);
            QStringList incompats = savedSettings.value("incompatibleResponses", "").toString().split(',');
            teamingOptions->incompatibleAttributeValues[attribNum] << QPair<int,int>(incompats.at(0).toInt(),incompats.at(1).toInt());
        }
        savedSettings.endArray();
    }
    savedSettings.endArray();
    teamingOptions->numberRequestedTeammatesGiven = savedSettings.value("requestedTeammateNumber", 1).toInt();
    ui->requestedTeammateNumberBox->setValue(teamingOptions->numberRequestedTeammatesGiven);
}


//////////////////
//Reset all of the UI settings when loading a set of students
//////////////////
void gruepr::resetUI()
{
    ui->minMeetingTimes->setEnabled(false);
    ui->desiredMeetingTimes->setEnabled(false);
    ui->meetingLength->setEnabled(false);
    ui->scheduleWeight->setEnabled(false);
    ui->label_16->setEnabled(false);
    ui->label_0->setEnabled(false);
    ui->label_6->setEnabled(false);
    ui->label_7->setEnabled(false);
    ui->label_8->setEnabled(false);
    ui->label_9->setEnabled(false);
    ui->requiredTeammatesButton->setEnabled(false);
    ui->label_18->setEnabled(false);
    ui->preventedTeammatesButton->setEnabled(false);
    ui->requestedTeammatesButton->setEnabled(false);
    ui->label_11->setEnabled(false);
    ui->requestedTeammateNumberBox->setEnabled(false);
    ui->sectionSelectionBox->clear();
    ui->sectionSelectionBox->setEnabled(false);
    ui->label_2->setEnabled(false);
    ui->label_22->setEnabled(false);
    ui->attributesTabWidget->setEnabled(false);
    ui->label_21->setEnabled(false);
    ui->studentTable->clear();
    ui->studentTable->setRowCount(0);
    ui->studentTable->setColumnCount(0);
    ui->studentTable->setEnabled(false);
    ui->addStudentPushButton->setEnabled(false);
    ui->saveSurveyFilePushButton->setEnabled(false);
    ui->actionSave_Survey_File->setEnabled(false);
    ui->dataDisplayTabWidget->setCurrentIndex(0);
    ui->isolatedWomenCheckBox->setEnabled(false);
    ui->isolatedMenCheckBox->setEnabled(false);
    ui->isolatedNonbinaryCheckBox->setEnabled(false);
    ui->mixedGenderCheckBox->setEnabled(false);
    ui->label_15->setEnabled(false);
    ui->isolatedURMCheckBox->setEnabled(false);
    ui->URMResponsesButton->setEnabled(false);
    ui->label_24->setEnabled(false);
    ui->teamSizeBox->clear();
    ui->teamSizeBox->setEnabled(false);
    ui->label_20->setEnabled(false);
    ui->label_10->setEnabled(false);
    ui->idealTeamSizeBox->setEnabled(false);
    ui->letsDoItButton->setEnabled(false);
    ui->actionCreate_Teams->setEnabled(false);
    ui->actionLoad_Student_Roster->setEnabled(false);
    ui->actionSave_Teams->setEnabled(false);
    ui->actionSave_Teams->disconnect();
    ui->actionSave_Teams->setText(tr("Save Teams..."));
    ui->actionPrint_Teams->setEnabled(false);
    ui->actionPrint_Teams->disconnect();
    ui->actionPrint_Teams->setText(tr("Print Teams..."));
    // remove any teams tabs (since we delete each one, move from last down to first; don't delete index 0 which is the student tab)
    for(int tabIndex = ui->dataDisplayTabWidget->count() - 1; tabIndex > 0; tabIndex--)
    {
        auto tab = ui->dataDisplayTabWidget->widget(tabIndex);
        ui->dataDisplayTabWidget->removeTab(tabIndex);
        tab->deleteLater();
    }
}


//////////////////
//Enable the appropriate UI settings when loading a set of students
//////////////////
void gruepr::loadUI()
{
    statusBarLabel->setText("File: " + dataOptions->dataFile.fileName());
    ui->actionLoad_Student_Roster->setEnabled(true);
    ui->studentTable->setEnabled(true);
    ui->addStudentPushButton->setEnabled(true);
    ui->requiredTeammatesButton->setEnabled(true);
    ui->label_18->setEnabled(true);
    ui->preventedTeammatesButton->setEnabled(true);
    ui->requestedTeammatesButton->setEnabled(true);
    ui->label_11->setEnabled(true);
    ui->requestedTeammateNumberBox->setEnabled(true);

    ui->sectionSelectionBox->blockSignals(true);
    if(dataOptions->sectionIncluded)
    {
        if(dataOptions->sectionNames.size() > 1)
        {
            ui->sectionSelectionBox->setEnabled(true);
            ui->label_2->setEnabled(true);
            ui->label_22->setEnabled(true);
            ui->sectionSelectionBox->addItem(tr("Students in all sections together"));
            ui->sectionSelectionBox->insertSeparator(1);
            ui->sectionSelectionBox->addItems(dataOptions->sectionNames);
        }
        else
        {
            ui->sectionSelectionBox->addItem(tr("Only one section in the data."));
        }
    }
    else
    {
        ui->sectionSelectionBox->addItem(tr("No section data."));
    }
    sectionName = ui->sectionSelectionBox->currentText();
    ui->sectionSelectionBox->blockSignals(false);

    refreshStudentDisplay();

    ui->studentTable->resetTable();

    ui->idealTeamSizeBox->setMaximum(std::max(2,numStudents/2));
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box

    ui->attributesTabWidget->setUpdatesEnabled(false);
    ui->attributesTabWidget->clear();
    delete[] attributeTab;
    attributeTab = new attributeTabItem[MAX_ATTRIBUTES];
    if(dataOptions->numAttributes > 0)
    {
        //(re)set the weight to zero for any attributes with just one value in the data
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
        {
            if(dataOptions->attributeVals[attribute].size() == 1)
            {
                teamingOptions->attributeWeights[attribute] = 0;
            }
            ui->attributesTabWidget->addTab(&attributeTab[attribute], QString::number(attribute + 1));
            attributeTab[attribute].setValues(attribute, dataOptions, teamingOptions);
            connect(attributeTab[attribute].weight, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                        this, [this](double arg1){teamingOptions->attributeWeights[ui->attributesTabWidget->currentIndex()] = float(arg1);});
            connect(attributeTab[attribute].homogeneous, &QCheckBox::stateChanged,
                        this, [this](int arg1){teamingOptions->desireHomogeneous[ui->attributesTabWidget->currentIndex()] = (arg1 != 0);});
            connect(attributeTab[attribute].requiredButton, &QPushButton::clicked, this, &gruepr::requiredResponsesButton_clicked);
            connect(attributeTab[attribute].incompatsButton, &QPushButton::clicked, this, &gruepr::incompatibleResponsesButton_clicked);
        }
        ui->label_21->setEnabled(true);
        ui->attributesTabWidget->setEnabled(true);
        ui->attributesTabWidget->setCurrentIndex(0);
    }
    else
    {
        ui->attributesTabWidget->addTab(&attributeTab[0], "1");
    }
    ui->attributesTabWidget->setUpdatesEnabled(true);

    if(dataOptions->genderIncluded)
    {
        ui->isolatedWomenCheckBox->setEnabled(true);
        ui->isolatedMenCheckBox->setEnabled(true);
        ui->isolatedNonbinaryCheckBox->setEnabled(true);
        ui->mixedGenderCheckBox->setEnabled(true);
        ui->label_15->setEnabled(true);
    }

    if(dataOptions->URMIncluded)
    {
        ui->isolatedURMCheckBox->setEnabled(true);
        ui->URMResponsesButton->setEnabled(true);
        ui->label_24->setEnabled(true);
    }

    if(!dataOptions->dayNames.isEmpty())
    {
        ui->minMeetingTimes->setEnabled(true);
        ui->desiredMeetingTimes->setEnabled(true);
        ui->meetingLength->setEnabled(true);
        ui->scheduleWeight->setEnabled(true);
        ui->label_16->setEnabled(true);
        ui->label_0->setEnabled(true);
        ui->label_6->setEnabled(true);
        ui->label_7->setEnabled(true);
        ui->label_8->setEnabled(true);
        ui->label_9->setEnabled(true);
        ui->minMeetingTimes->setMaximum((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLength->currentIndex() + 1));
        ui->desiredMeetingTimes->setMaximum((dataOptions->timeNames.size() * dataOptions->dayNames.size()) / (ui->meetingLength->currentIndex() + 1));
    }

    ui->idealTeamSizeBox->setEnabled(true);
    ui->teamSizeBox->setEnabled(true);
    ui->label_20->setEnabled(true);
    ui->label_10->setEnabled(true);
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary

    ui->actionLoad_Teaming_Options_File->setEnabled(true);
    ui->actionSave_Teaming_Options_File->setEnabled(true);
    ui->letsDoItButton->setEnabled(true);
    ui->actionCreate_Teams->setEnabled(true);
}


//////////////////
// Set the "official" team sizes using an array of different sizes or a single, constant size
//////////////////
void gruepr::setTeamSizes(const int teamSizes[])
{
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++)	// run through every team
    {
        teamingOptions->teamSizesDesired[team] = teamSizes[team];
    }
}
void gruepr::setTeamSizes(const int singleSize)
{
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++)	// run through every team
    {
        teamingOptions->teamSizesDesired[team] = singleSize;
    }
}


//////////////////
// Read the survey datafile, setting the data options and loading all of the student records, returning true if successful and false if file is invalid
//////////////////
bool gruepr::loadSurveyData(CsvFile &surveyFile)
{
    if(!surveyFile.readHeader())
    {
        // header row could not be read as valid data
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        surveyFile.close();
        return false;
    }

    if(surveyFile.headerValues.size() < 4)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        surveyFile.close();
        return false;
    }

    // See if there are header fields after any of (preferred teammates / non-teammates, section, or schedule) since those are probably notes fields
    auto lastKnownMeaningfulField = QRegularExpression("(.*(name).*(like to not have on your team).*)|(.*(name).*(like to have on your team).*)|"
                                                       ".*(in which section are you enrolled).*|(.*(check).+(times).*)", QRegularExpression::CaseInsensitiveOption);
    int notesFieldsProbBeginAt = 1 + surveyFile.headerValues.lastIndexOf(lastKnownMeaningfulField);
    if((notesFieldsProbBeginAt != 0) && (notesFieldsProbBeginAt != surveyFile.headerValues.size()))
    {
        //if notesFieldsProbBeginAt == 0 then none of these questions exist, so assume no notes because list ends with attributes
        //and if notesFieldsProbBeginAt == headervalues size, also assume no notes because list ends with one of these questions
        for(int field = notesFieldsProbBeginAt; field < surveyFile.fieldMeanings.size(); field++)
        {
            surveyFile.fieldMeanings[field] = "Notes";
        }
    }

    // Ask user what the columns mean
    QVector<possFieldMeaning> surveyFieldOptions = {{"Timestamp", "(timestamp)", 1}, {"First Name", "((first)|(given)|(preferred))(?!.*last).*(name)", 1},
                                                    {"Last Name", "^(?!.*first).*((last)|(sur)|(family)).*(name)", 1}, {"Email Address", "(e).*(mail)", 1},
                                                    {"Gender", "(gender)", 1}, {"Racial/ethnic identity", "((minority)|(ethnic))", 1},
                                                    {"Schedule", "(check).+(times)", MAX_DAYS}, {"Section", "in which section are you enrolled", 1},
                                                    {"Timezone","(time zone)", 1}, {"Preferred Teammates", "(name).*(like to have on your team)", 1},
                                                    {"Preferred Non-teammates", "(name).*(like to not have on your team)", 1},
                                                    {"Attribute", ".*", MAX_ATTRIBUTES}, {"Notes", "", MAX_NOTES_FIELDS}};
    QApplication::restoreOverrideCursor();
    if(surveyFile.chooseFieldMeaningsDialog(surveyFieldOptions, this)->exec() == QDialog::Rejected)
    {
        return false;
    }
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    // set field values now according to user's selection of field meanings (defaulting to -1 if not chosen)
    dataOptions->timestampField = surveyFile.fieldMeanings.indexOf("Timestamp");
    dataOptions->firstNameField = surveyFile.fieldMeanings.indexOf("First Name");
    dataOptions->lastNameField = surveyFile.fieldMeanings.indexOf("Last Name");
    dataOptions->emailField = surveyFile.fieldMeanings.indexOf("Email Address");
    dataOptions->genderField = surveyFile.fieldMeanings.indexOf("Gender");
    dataOptions->genderIncluded = (dataOptions->genderField != -1);
    dataOptions->URMField = surveyFile.fieldMeanings.indexOf("Racial/ethnic identity");
    dataOptions->URMIncluded = (dataOptions->URMField != -1);
    dataOptions->sectionField = surveyFile.fieldMeanings.indexOf("Section");
    dataOptions->sectionIncluded = (dataOptions->sectionField != -1);
    dataOptions->timezoneField = surveyFile.fieldMeanings.indexOf("Timezone");
    dataOptions->timezoneIncluded = (dataOptions->timezoneField != -1);
    dataOptions->prefTeammatesField = surveyFile.fieldMeanings.indexOf("Preferred Teammates");
    dataOptions->prefTeammatesIncluded = (dataOptions->prefTeammatesField != -1);
    dataOptions->prefNonTeammatesField = surveyFile.fieldMeanings.indexOf("Preferred Non-teammates");
    dataOptions->prefNonTeammatesIncluded = (dataOptions->prefNonTeammatesField != -1);
    // notes fields
    int lastFoundIndex = 0;
    dataOptions->numNotes = surveyFile.fieldMeanings.count("Notes");
    for(int note = 0; note < dataOptions->numNotes; note++)
    {
        dataOptions->notesField[note] = surveyFile.fieldMeanings.indexOf("Notes", lastFoundIndex);
        lastFoundIndex = std::max(lastFoundIndex, 1 + surveyFile.fieldMeanings.indexOf("Notes", lastFoundIndex));
    }
    // attribute fields, adding timezone field as an attribute if it exists
    lastFoundIndex = 0;
    dataOptions->numAttributes = surveyFile.fieldMeanings.count("Attribute");
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        dataOptions->attributeField[attribute] = surveyFile.fieldMeanings.indexOf("Attribute", lastFoundIndex);
        dataOptions->attributeQuestionText << surveyFile.headerValues.at(dataOptions->attributeField[attribute]);
        lastFoundIndex = std::max(lastFoundIndex, 1 + surveyFile.fieldMeanings.indexOf("Attribute", lastFoundIndex));
    }
    if(dataOptions->timezoneIncluded)
    {
        dataOptions->attributeField[dataOptions->numAttributes] = dataOptions->timezoneField;
        dataOptions->attributeQuestionText << surveyFile.headerValues.at(dataOptions->timezoneField);
        dataOptions->numAttributes++;
    }
    // schedule fields
    lastFoundIndex = 0;
    for(int scheduleQuestion = 0, numScheduleFields = surveyFile.fieldMeanings.count("Schedule"); scheduleQuestion < numScheduleFields; scheduleQuestion++)
    {
        dataOptions->scheduleField[scheduleQuestion] = surveyFile.fieldMeanings.indexOf("Schedule", lastFoundIndex);
        QString scheduleQuestionText = surveyFile.headerValues.at(dataOptions->scheduleField[scheduleQuestion]);
        if(scheduleQuestionText.contains(QRegularExpression(".+\\b(free|available)\\b.+", QRegularExpression::CaseInsensitiveOption)))
        {
            // if >=1 field has this language, all interpreted as free time
            dataOptions->scheduleDataIsFreetime = true;
        }
        if(scheduleQuestionText.contains(QRegularExpression(".+\\b(your home)\\b.+", QRegularExpression::CaseInsensitiveOption)))
        {
            // if >=1 field has this language, all interpreted as referring to each student's home timezone
            dataOptions->homeTimezoneUsed = true;
        }
        QRegularExpression dayNameFinder("\\[([^[]*)\\]");   // Day name is in brackets at end of field (where Google Forms puts column titles in matrix questions)
        QRegularExpressionMatch dayName = dayNameFinder.match(scheduleQuestionText);
        if(dayName.hasMatch())
        {
            dataOptions->dayNames << dayName.captured(1);
        }
        else
        {
            dataOptions->dayNames << " " + QString::number(scheduleQuestion+1) + " ";
        }
        lastFoundIndex = std::max(lastFoundIndex, 1 + surveyFile.fieldMeanings.indexOf("Schedule", lastFoundIndex));
    }

    if(!surveyFile.fieldMeanings.contains("Schedule") && !surveyFile.fieldMeanings.contains("Attribute"))
    {
        QMessageBox::critical(this, tr("File error."), tr("A survey file must contain at least one schedule question or one attribute question."), QMessageBox::Ok);
        surveyFile.close();
        return false;
    }

    // read one line of data; if no data after header row then file is invalid
    if(!surveyFile.readDataRow())
    {
        QMessageBox::critical(this, tr("Insufficient number of students."),
                              tr("There are no survey responses in this file."), QMessageBox::Ok);
        surveyFile.close();
        return false;
    }

    // If there is schedule info, read through the schedule fields in all of the responses to compile a list of time names, save as dataOptions->TimeNames
    if(!dataOptions->dayNames.isEmpty())
    {
        QStringList allTimeNames;
        do
        {
            for(int scheduleQuestion = 0, numScheduleQuestions = surveyFile.fieldMeanings.count("Schedule"); scheduleQuestion < numScheduleQuestions; scheduleQuestion++)
            {
                QString scheduleFieldText = QString(surveyFile.fieldValues.at(dataOptions->scheduleField[scheduleQuestion]).toUtf8()).toLower().split(';').join(',');
                QTextStream scheduleFieldStream(&scheduleFieldText);
                allTimeNames << CsvFile::getLine(scheduleFieldStream);
            }
        }
        while(surveyFile.readDataRow());
        allTimeNames.removeDuplicates();
        allTimeNames.removeOne("");
        //sort allTimeNames smartly, using mapped string -> hour of day integer; any timeName not found is put at the beginning of the list
        QStringList timeNamesStrings = QString(TIME_NAMES).split(",");
        std::sort(allTimeNames.begin(), allTimeNames.end(), [&timeNamesStrings] (const QString &a, const QString &b)
                                        {return TIME_MEANINGS[std::max(0,timeNamesStrings.indexOf(a))] < TIME_MEANINGS[std::max(0,timeNamesStrings.indexOf(b))];});
        dataOptions->timeNames = allTimeNames;
        //pad the timeNames to include all 24 hours if we will be time-shifting student responses based on their home timezones later
        if(dataOptions->homeTimezoneUsed)
        {
            dataOptions->earlyHourAsked = TIME_MEANINGS[std::max(0,timeNamesStrings.indexOf(dataOptions->timeNames.constFirst()))];
            dataOptions->lateHourAsked = TIME_MEANINGS[std::max(0,timeNamesStrings.indexOf(dataOptions->timeNames.constLast()))];
            for(int hour = 0; hour < dataOptions->earlyHourAsked; hour++)
            {
                const int *val = std::find(std::begin(TIME_MEANINGS), std::end(TIME_MEANINGS), hour);
                int index = std::distance(TIME_MEANINGS, val);
                dataOptions->timeNames.insert(hour, timeNamesStrings.at(index));
            }
            for(int hour = dataOptions->lateHourAsked + 1; hour < MAX_BLOCKS_PER_DAY; hour++)
            {
                const int *val = std::find(std::begin(TIME_MEANINGS), std::end(TIME_MEANINGS), hour);
                int index = std::distance(TIME_MEANINGS, val);
                dataOptions->timeNames << timeNamesStrings.at(index);
            }
        }
    }

    // If each student's home timezone was used, ask what should be used as the base timezone that they should all be adjusted to
    if(dataOptions->homeTimezoneUsed)
    {
        auto *window = new baseTimezoneDialog(this);
        window->exec();
        dataOptions->baseTimezone = window->baseTimezoneVal;
        window->deleteLater();
    }

    // Having read the header row and determined time names, if any, read each remaining row as a student record
    surveyFile.readDataRow(true);    // put cursor back to beginning and read first row
    if(surveyFile.hasHeaderRow)
    {
        // that first row was headers, so get next row
        surveyFile.readDataRow();
    }
    numStudents = 0;            // counter for the number of records in the file; used to set the number of students to be teamed for the rest of the program
    do
    {
        student[numStudents].parseRecordFromStringList(surveyFile.fieldValues, dataOptions);
        student[numStudents].ID = dataOptions->latestStudentID;
        dataOptions->latestStudentID++;

        // see if this record is a duplicate; assume it isn't and then check
        student[numStudents].duplicateRecord = false;
        for(int index = 0; index < numStudents; index++)
        {
            if((student[numStudents].firstname + student[numStudents].lastname == student[index].firstname + student[index].lastname) ||
                    ((student[numStudents].email == student[index].email) && !student[numStudents].email.isEmpty()))
            {
                student[numStudents].duplicateRecord = true;
                student[index].duplicateRecord = true;
            }
        }

        numStudents++;
    }
    while(surveyFile.readDataRow() && numStudents < MAX_STUDENTS);
    dataOptions->numStudentsInSystem = numStudents;

    if(numStudents == MAX_STUDENTS)
    {
        QMessageBox::warning(this, tr("Reached maximum number of students."),
                                   tr("The maximum number of students have been read from the file."
                                      " This version of gruepr does not allow more than ") + QString(MAX_STUDENTS) + tr("."), QMessageBox::Ok);
    }
    else if(numStudents < 4)
    {
        QMessageBox::critical(this, tr("Insufficient number of students."),
                                    tr("There are not enough survey responses in the file."
                                       " There must be at least 4 students for gruepr to work properly."), QMessageBox::Ok);
        //reset the data
        delete[] student;
        student = new StudentRecord[MAX_STUDENTS];
        dataOptions->reset();

        surveyFile.close();
        return false;
    }

    // Set the attribute question options and numerical values for each student
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        if(dataOptions->attributeField[attribute] != -1)
        {
            auto &responses = dataOptions->attributeQuestionResponses[attribute];
            auto &attributeType = dataOptions->attributeType[attribute];
            // gather all unique attribute question responses, then remove a blank response if it exists in a list with other responses
            for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
            {
                if(!responses.contains(student[index].attributeResponse[attribute]))
                {
                    responses << student[index].attributeResponse[attribute];
                }
            }
            if(responses.size() > 1)
            {
                responses.removeAll(QString(""));
            }

            // Figure out what type of attribute this is: timezone, ordered/numerical, categorical (one response), or categorical (mult. responses)
            // If this is the timezone field, it's timezone type;
            // otherwise if every response starts with an integer, then it is ordered (numerical);
            // but if any response is missing the number at the start, then it is categorical or multicategorical--multicategorical if any response contains a comma
            // The regex to recognize ordered/numerical is:
            // digit(s) then, optionally, "." or "," then end; OR digit(s) then "." or "," then any character but digits; OR digit(s) then any character but "." or ","
            QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
            if(dataOptions->attributeField[attribute] == dataOptions->timezoneField)
            {
                attributeType = DataOptions::timezone;
            }
            else
            {
                attributeType = DataOptions::ordered;           // assume it's ordered
                for(const auto &response : qAsConst(responses))
                {
                    // type stays ordered until and unless we don't have a regex match
                    if((attributeType != DataOptions::ordered) || !(startsWithInteger.match(response).hasMatch()))
                    {
                        // type stays categorical until and unless we have a response with a comma in it
                        if((attributeType == DataOptions::multicategorical) || (response.contains(',')))
                        {
                            attributeType = DataOptions::multicategorical;
                        }
                        else
                        {
                            attributeType = DataOptions::categorical;
                        }
                    }
                }
            }

            // for multicategorical, have to reprocess the responses to delimit at the commas
            if(attributeType == DataOptions::multicategorical)
            {
                for(int originalResponseNum = 0, numOriginalResponses = responses.size(); originalResponseNum < numOriginalResponses; originalResponseNum++)
                {
                    QStringList newResponses = responses.takeFirst().split(',');
                    for(auto &newResponse : newResponses)
                    {
                        newResponse = newResponse.trimmed();
                        if(!responses.contains(newResponse))
                        {
                            responses << newResponse;
                        }
                    }
                }
                responses.removeAll(QString(""));
            }

            // sort alphanumerically unless it's timezone, in which case sort according to offset from GMT
            if(attributeType != DataOptions::timezone)
            {
                QCollator sortAlphanumerically;
                sortAlphanumerically.setNumericMode(true);
                sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
                std::sort(responses.begin(), responses.end(), sortAlphanumerically);
            }
            else
            {
                QRegularExpression zoneFinder(".*\\[GMT(.*):(.*)\\].*");  // characters after "[GMT" are +hh:mm "]"
                std::sort(responses.begin(), responses.end(), [zoneFinder] (const QString &A, const QString &B) {
                                                                            float timezoneA = 0, timezoneB = 0;
                                                                            QRegularExpressionMatch zoneA = zoneFinder.match(A);
                                                                            QRegularExpressionMatch zoneB = zoneFinder.match(B);
                                                                            if(zoneA.hasMatch())
                                                                            {
                                                                                float hours = zoneA.captured(1).toFloat();
                                                                                float minutes = zoneA.captured(2).toFloat();
                                                                                timezoneA = hours + (hours < 0? (-minutes/60) : (minutes/60));
                                                                            }
                                                                            if(zoneB.hasMatch())
                                                                            {
                                                                                float hours = zoneB.captured(1).toFloat();
                                                                                float minutes = zoneB.captured(2).toFloat();
                                                                                timezoneB = hours + (hours < 0? (-minutes/60) : (minutes/60));
                                                                            }
                                                                            return timezoneA < timezoneB;});
            }

            // set values associated with each response and create a spot to hold the responseCounts
            if(attributeType == DataOptions::ordered)
            {
                // ordered/numerical values. value is based on number at start of response
                for(const auto &response : qAsConst(responses))
                {
                    dataOptions->attributeVals[attribute].insert(startsWithInteger.match(response).captured(1).toInt());
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({response, 0});
                }
            }
            else if((attributeType == DataOptions::categorical) || (attributeType == DataOptions::multicategorical))
            {
                // categorical or multicategorical. value is based on index of response within sorted list
                for(int i = 1; i <= responses.size(); i++)
                {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }
            else
            {
                // TODO: for timezone, values should be the GMT offsets (rounded to nearest hour?); currently, same as categorical
                for(int i = 1; i <= responses.size(); i++)
                {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }

            // set numerical value of each student's response and record in dataOptions a tally for each response
            for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
            {
                const QString &currentStudentResponse = student[index].attributeResponse[attribute];
                QVector<int> &currentStudentAttributeVals = student[index].attributeVals[attribute];
                if(!student[index].attributeResponse[attribute].isEmpty())
                {
                    if(attributeType == DataOptions::ordered)
                    {
                        // for numerical/ordered, set numerical value of students' attribute responses according to the number at the start of the response
                        currentStudentAttributeVals << startsWithInteger.match(currentStudentResponse).captured(1).toInt();
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if((attributeType == DataOptions::categorical) || (attributeType == DataOptions::timezone))
                    {
                        // set numerical value instead according to their place in the sorted list of responses
                        currentStudentAttributeVals << responses.indexOf(currentStudentResponse) + 1;
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else
                    {
                        //multicategorical - set numerical values according to each value
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent)
                        {
                            currentStudentAttributeVals << responses.indexOf(responseFromStudent.trimmed()) + 1;
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                }
                else
                {
                    currentStudentAttributeVals << -1;
                }
            }
        }
    }

    // gather all unique URM and section question responses and sort
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        if(!dataOptions->URMResponses.contains(student[index].URMResponse, Qt::CaseInsensitive))
        {
            dataOptions->URMResponses << student[index].URMResponse;
        }
        if(!(dataOptions->sectionNames.contains(student[index].section, Qt::CaseInsensitive)))
        {
            dataOptions->sectionNames << student[index].section;
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
    std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);

    // set all of the students' tooltips
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        student[index].createTooltip(dataOptions);
    }

    surveyFile.close();
    return true;
}


//////////////////
// Read the survey datafile, setting the data options and loading all of the student records, returning true if successful and false if file is invalid
//////////////////
bool gruepr::loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails)
{
    // Read the header row
    if(!rosterFile.readHeader())
    {
        // header row could not be read as valid data
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        return false;
    }

    // Ask user what the columns mean
    // Preloading the selector boxes with "unused" except first time "email", "first name", "last name", and "name" are found
    QVector<possFieldMeaning> rosterFieldOptions  = {{"First Name", "((first)|(given)|(preferred)).*(name)", 1},
                                                     {"Last Name", "((last)|(sur)|(family)).*(name)", 1},
                                                     {"Email Address", "(e).*(mail)", 1},
                                                     {"Full Name (First Last)", "(name)", 1},
                                                     {"Full Name (Last, First)", "(name)", 1}};;
    if(rosterFile.chooseFieldMeaningsDialog(rosterFieldOptions, this)->exec() == QDialog::Rejected)
    {
        return false;
    }

    // set field values now according to uer's selection of field meanings (defulting to -1 if not chosen)
    int emailField = rosterFile.fieldMeanings.indexOf("Email Address");
    int firstNameField = rosterFile.fieldMeanings.indexOf("First Name");
    int lastNameField = rosterFile.fieldMeanings.indexOf("Last Name");
    int firstLastNameField = rosterFile.fieldMeanings.indexOf("Full Name (First Last)");
    int lastFirstNameField = rosterFile.fieldMeanings.indexOf("Full Name (Last, First)");

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
            QMessageBox::critical(this, tr("File error."), tr("This roster does not contain student names."), QMessageBox::Ok);
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

    ui->studentTable->setColumnCount(dataOptions->sectionIncluded? 6 : 5);
    QIcon unsortedIcon(":/icons/updown_arrow.png");
    int column = 0;
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("Survey\nSubmission\nTime")));
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("First Name")));
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("Last Name")));
    if(dataOptions->sectionIncluded)
    {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("Section")));
    }
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(tr("Edit")));
    ui->studentTable->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("Remove")));

    ui->studentTable->setRowCount(dataOptions->numStudentsInSystem);
    numStudents = 0;
    for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
    {
        if((ui->sectionSelectionBox->currentIndex() == 0) || (student[index].section == ui->sectionSelectionBox->currentText()))
        {
            bool duplicate = student[index].duplicateRecord;

            auto *timestamp = new SortableTableWidgetItem(SortableTableWidgetItem::datetime, student[index].surveyTimestamp.toString(Qt::SystemLocaleShortDate));
            timestamp->setToolTip(student[index].tooltip);
            if(duplicate)
            {
                timestamp->setBackground(QBrush(HIGHLIGHTYELLOW));
            }
            ui->studentTable->setItem(numStudents, 0, timestamp);

            auto *firstName = new QTableWidgetItem(student[index].firstname);
            firstName->setToolTip(student[index].tooltip);
            if(duplicate)
            {
                firstName->setBackground(QBrush(HIGHLIGHTYELLOW));
            }
            ui->studentTable->setItem(numStudents, 1, firstName);

            auto *lastName = new QTableWidgetItem(student[index].lastname);
            lastName->setToolTip(student[index].tooltip);
            if(duplicate)
            {
                lastName->setBackground(QBrush(HIGHLIGHTYELLOW));
            }
            ui->studentTable->setItem(numStudents, 2, lastName);

            int column = 3;
            if(dataOptions->sectionIncluded)
            {
                auto *section = new SortableTableWidgetItem(SortableTableWidgetItem::alphanumeric, student[index].section);
                section->setToolTip(student[index].tooltip);
                if(duplicate)
                {
                    section->setBackground(QBrush(HIGHLIGHTYELLOW));
                }
                ui->studentTable->setItem(numStudents, column, section);
                column++;
            }

            auto *editButton = new PushButtonWithMouseEnter(QIcon(":/icons/edit.png"), "", this);
            editButton->setToolTip("<html>" + tr("Edit") + " " + student[index].firstname + " " + student[index].lastname + tr("'s data.") + "</html>");
            editButton->setProperty("StudentIndex", index);
            editButton->setProperty("duplicate", duplicate);
            if(duplicate)
            {
                editButton->setStyleSheet("QPushButton {background-color: #ffff3b; border: none;}");
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
            ui->studentTable->setCellWidget(numStudents, column, editButton);
            column++;

            auto *removerButton = new PushButtonWithMouseEnter(QIcon(":/icons/delete.png"), "", this);
            removerButton->setToolTip("<html>" + tr("Remove") + " " + student[index].firstname + " " + student[index].lastname + " " +
                                                 tr("from the current data set.") + "</html>");
            removerButton->setProperty("StudentIndex", index);
            removerButton->setProperty("duplicate", duplicate);
            if(duplicate)
            {
                removerButton->setStyleSheet("QPushButton {background-color: #ffff3b; border: none;}");
            }
            connect(removerButton, &PushButtonWithMouseEnter::clicked, this, [this, index, removerButton] {
                                                                                removerButton->disconnect();
                                                                                removeAStudent(index);});
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
            ui->studentTable->setCellWidget(numStudents, column, removerButton);

            numStudents++;
        }
    }
    ui->studentTable->setRowCount(numStudents);

    QString sectiontext = (ui->sectionSelectionBox->currentIndex() == 0? "All sections" : " Section: " + sectionName);
    statusBarLabel->setText(statusBarLabel->text().split("\u2192")[0].trimmed() + "  \u2192 " + sectiontext + "  \u2192 " + QString::number(numStudents) + " students");

    ui->studentTable->setUpdatesEnabled(true);
    ui->studentTable->resizeColumnsToContents();
    ui->studentTable->setSortingEnabled(true);
}


////////////////////////////////////////////
// Create and optimize teams using genetic algorithm
////////////////////////////////////////////
QVector<int> gruepr::optimizeTeams(const int *const studentIndexes)
{
    // create and seed the pRNG (need to specifically do it here because this is happening in a new thread)
#ifdef Q_OS_MACOS
            std::random_device randDev;
            std::mt19937 pRNG(randDev());
#endif
#ifdef Q_OS_WIN32
            std::mt19937 pRNG{static_cast<long unsigned int>(time(nullptr))};
#endif

    // Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
    // Each genome in this generation stores (by permutation) which students are in which team.
    // Array has one entry per student and lists, in order, the "ID number" of the
    // student, referring to the order of the student in the students[] array.
    // For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
    // students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

    // allocate memory for a genepool for current generation and a next generation as it is being created
    int **genePool = new int*[POPULATIONSIZE];
    int **nextGenGenePool = new int*[POPULATIONSIZE];
    // allocate memory for current and next generation's ancestors
    int **ancestors = new int*[POPULATIONSIZE];
    int **nextGenAncestors = new int*[POPULATIONSIZE];
    int numAncestors = 2;           //always track mom & dad
    for(int generation = 0; generation < NUMGENERATIONSOFANCESTORS; generation++)
    {
        numAncestors += (4<<generation);   //add an additional 2^(n+1) ancestors for the next level of (great)grandparents
    }
    for(int genome = 0; genome < POPULATIONSIZE; genome++)
    {
        genePool[genome] = new int[numStudents];
        nextGenGenePool[genome] = new int[numStudents];
        ancestors[genome] = new int[numAncestors];
        nextGenAncestors[genome] = new int[numAncestors];
    }
    // allocate memory for array of indexes, to be sorted in order of score (so genePool[orderedIndex[0]] is the one with the top score)
    int *orderedIndex = new int[POPULATIONSIZE];
    for(int genome = 0; genome < POPULATIONSIZE; genome++)
    {
        orderedIndex[genome] = genome;
    }

    // create an initial population
    // start with an array of all the student IDs in order
    int *randPerm = new int[numStudents];
    for(int i = 0; i < numStudents; i++)
    {
        randPerm[i] = studentIndexes[i];
    }
    // then make "populationSize" number of random permutations for initial population, store in genePool
    for(int genome = 0; genome < POPULATIONSIZE; genome++)
    {
        std::shuffle(randPerm, randPerm+numStudents, pRNG);
        for(int ID = 0; ID < numStudents; ID++)
        {
            genePool[genome][ID] = randPerm[ID];
        }
    }
    delete[] randPerm;

    // just use random values for the initial "ancestor" values
    std::uniform_int_distribution<unsigned int> randAncestor(0, POPULATIONSIZE);
    for(int genome = 0; genome < POPULATIONSIZE; genome++)
    {
        for(int ancestor = 0; ancestor < numAncestors; ancestor++)
        {
            ancestors[genome][ancestor] = randAncestor(pRNG);
        }
    }

    int *teamSizes = new int[MAX_TEAMS];
    for(int team = 0; team < numTeams; team++)
    {
        teamSizes[team] = teams[team].size;
    }

    // calculate this first generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
    QVector<float> scores(POPULATIONSIZE);
    float *unusedTeamScores = nullptr, *schedScore = nullptr;
    float **attributeScore = nullptr;
    int *penaltyPoints = nullptr;
    bool **availabilityChart = nullptr;
#pragma omp parallel default(none) shared(scores, student, genePool, numTeams, teamSizes, teamingOptions, dataOptions) private(unusedTeamScores, attributeScore, schedScore, availabilityChart, penaltyPoints)
    {
        unusedTeamScores = new float[numTeams];
        attributeScore = new float*[dataOptions->numAttributes];
        for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
        {
            attributeScore[attrib] = new float[numTeams];
        }
        schedScore = new float[numTeams];
        availabilityChart = new bool*[dataOptions->dayNames.size()];
        for(int day = 0; day < dataOptions->dayNames.size(); day++)
        {
            availabilityChart[day] = new bool[dataOptions->timeNames.size()];
        }
        penaltyPoints = new int[numTeams];
#pragma omp for
        for(int genome = 0; genome < POPULATIONSIZE; genome++)
        {
            scores[genome] = getGenomeScore(student, genePool[genome], numTeams, teamSizes,
                                            teamingOptions, dataOptions, unusedTeamScores,
                                            attributeScore, schedScore, availabilityChart, penaltyPoints);
        }
        delete[] penaltyPoints;
        for(int day = 0; day < dataOptions->dayNames.size(); day++)
        {
            delete[] availabilityChart[day];
        }
        delete[] availabilityChart;
        delete[] schedScore;
        for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
        {
            delete[] attributeScore[attrib];
        }
        delete[] attributeScore;
        delete[] unusedTeamScores;
    }

    // get genome indexes in order of score, largest to smallest
    std::sort(orderedIndex, orderedIndex+POPULATIONSIZE, [&scores](const int i, const int j){return (scores.at(i) > scores.at(j));});
    emit generationComplete(scores, orderedIndex, 0, 0);

    int child[MAX_STUDENTS];
    int *mom=nullptr, *dad=nullptr;                 // pointer to genome of mom and dad
    float bestScores[GENERATIONS_OF_STABILITY]={0};	// historical record of best score in the genome, going back generationsOfStability generations
    float scoreStability = 0;
    int generation = 0;
    bool localOptimizationStopped = false;

    // now optimize
    do						// allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
    {
        do					// keep optimizing until reach stability or maxGenerations
        {
            // clone the elites in genePool into nextGenGenePool, shifting their ancestor arrays as if "self-mating"
            for(int genome = 0; genome < NUM_ELITES; genome++)
            {
                for(int ID = 0; ID < numStudents; ID++)
                {
                    nextGenGenePool[genome][ID] = genePool[orderedIndex[genome]][ID];
                }

                nextGenAncestors[genome][0] = nextGenAncestors[genome][1] = orderedIndex[genome];   // both parents are this genome
                int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;  // parents are 0 & 1, so grandparents are 2, 3, 4, & 5
                for(int generation = 1; generation < NUMGENERATIONSOFANCESTORS; generation++)
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
            for(int genome = NUM_ELITES; genome < POPULATIONSIZE; genome++)
            {
                //get a couple of parents
                GA::tournamentSelectParents(genePool, orderedIndex, ancestors, mom, dad, nextGenAncestors[genome], pRNG);

                //mate them and put child in nextGenGenePool
                GA::mate(mom, dad, teamSizes, numTeams, child, numStudents, pRNG);
                for(int ID = 0; ID < numStudents; ID++)
                {
                    nextGenGenePool[genome][ID] = child[ID];
                }
            }

            // take all but the single top-scoring elite genome and mutate with some probability; if a mutation occurred, mutate same genome again with same probability
            std::uniform_int_distribution<unsigned int> randProbability(1, 100);
            for(int genome = 1; genome < POPULATIONSIZE; genome++)
            {
                while(randProbability(pRNG) < MUTATIONLIKELIHOOD)
                {
                    GA::mutate(&nextGenGenePool[genome][0], numStudents, pRNG);
                }
            }

            // swap pointers to make nextGen's genePool and ancestors into this generation's
            std::swap(genePool, nextGenGenePool);
            std::swap(ancestors, nextGenAncestors);

            generation++;

            // calculate this generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
#pragma omp parallel default(none) shared(scores, student, genePool, numTeams, teamSizes, teamingOptions, dataOptions) private(unusedTeamScores, attributeScore, schedScore, availabilityChart, penaltyPoints)
            {
                unusedTeamScores = new float[numTeams];
                attributeScore = new float*[dataOptions->numAttributes];
                for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
                {
                    attributeScore[attrib] = new float[numTeams];
                }
                schedScore = new float[numTeams];
                availabilityChart = new bool*[dataOptions->dayNames.size()];
                for(int day = 0; day < dataOptions->dayNames.size(); day++)
                {
                    availabilityChart[day] = new bool[dataOptions->timeNames.size()];
                }
                penaltyPoints = new int[numTeams];
#pragma omp for nowait
                for(int genome = 0; genome < POPULATIONSIZE; genome++)
                {
                    scores[genome] = getGenomeScore(student, genePool[genome], numTeams, teamSizes,
                                                    teamingOptions, dataOptions, unusedTeamScores,
                                                    attributeScore, schedScore, availabilityChart, penaltyPoints);
                }
                delete[] penaltyPoints;
                for(int day = 0; day < dataOptions->dayNames.size(); day++)
                {
                    delete[] availabilityChart[day];
                }
                delete[] availabilityChart;
                delete[] schedScore;
                for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
                {
                    delete[] attributeScore[attrib];
                }
                delete[] attributeScore;
                delete[] unusedTeamScores;
            }

            // get genome indexes in order of score, largest to smallest
            std::sort(orderedIndex, orderedIndex+POPULATIONSIZE, [&scores](const int i, const int j){return (scores.at(i) > scores.at(j));});

            // determine best score, save in historical record, and calculate score stability
            float maxScoreInThisGeneration = scores[orderedIndex[0]];
            float maxScoreFromGenerationsAgo = bestScores[(generation+1)%GENERATIONS_OF_STABILITY];
            bestScores[generation%GENERATIONS_OF_STABILITY] = maxScoreInThisGeneration;	//best scores from most recent generationsOfStability, wrapping storage location

            if(maxScoreInThisGeneration == maxScoreFromGenerationsAgo)
            {
                scoreStability = maxScoreInThisGeneration / 0.0001F;
            }
            else
            {
                scoreStability = maxScoreInThisGeneration / (maxScoreInThisGeneration - maxScoreFromGenerationsAgo);
            }

            emit generationComplete(scores, orderedIndex, generation, scoreStability);

            optimizationStoppedmutex.lock();
            localOptimizationStopped = optimizationStopped;
            optimizationStoppedmutex.unlock();
        }
        while(!localOptimizationStopped && ((generation < MIN_GENERATIONS) || ((generation < MAX_GENERATIONS) && (scoreStability < MIN_SCORE_STABILITY))));

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
    teamSetScore = bestScores[generation%GENERATIONS_OF_STABILITY];

    //copy best team set into a QVector to return
    QVector<int> bestTeamSet;
    bestTeamSet.reserve(numStudents);
    for(int ID = 0; ID < numStudents; ID++)
    {
        bestTeamSet << genePool[orderedIndex[0]][ID];
    }

    // deallocate memory
    for(int genome = 0; genome < POPULATIONSIZE; ++genome)
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
float gruepr::getGenomeScore(const StudentRecord _student[], const int _teammates[], const int _numTeams, const int _teamSizes[],
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
                    attributeLevelsInTeam.insert(_student[_teammates[studentNum]].attributeVals[attribute].constBegin(),
                                                 _student[_teammates[studentNum]].attributeVals[attribute].constEnd());
                    if(thisIsTimezone)
                    {
                        timezoneLevelsInTeam.insert(_student[_teammates[studentNum]].timezone);
                    }
                    studentNum++;
                }

                // Add a penalty per pair of incompatible attribute responses found
                if(_teamingOptions->haveAnyIncompatibleAttributes[attribute])
                {
                    // go through each pair found in teamingOptions->incompatibleAttributeValues[attribute] list and see if both are found in attributeLevelsInTeam
                    for(const auto &pair : qAsConst(_teamingOptions->incompatibleAttributeValues[attribute]))
                    {
                        int n = attributeLevelsInTeam.count(pair.first);
                        if(pair.first == pair.second)
                        {
                            _penaltyPoints[team] += (n * (n-1))/ 2;  // number of incompatible pairings is the sum 1 -> n-1 (calculation = 0 if n == 0 or n == 1)
                        }
                        else
                        {
                            int m = attributeLevelsInTeam.count(pair.second);
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
                    else if(_dataOptions->attributeType[attribute] == DataOptions::ordered)
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
        const int numDays = _dataOptions->dayNames.size();
        const int numTimes = _dataOptions->timeNames.size();

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
            const auto &firstStudentOnTeam = _student[_teammates[studentNum]];
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
                const auto &currStudent = _student[_teammates[studentNum]];
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
            if(_schedScore[team] > _teamingOptions->desiredTimeBlocksOverlap)		// if team has > desiredTimeBlocksOverlap, additional overlaps count less
            {
                _schedScore[team] = 1 + ((_schedScore[team] - _teamingOptions->desiredTimeBlocksOverlap) /
                                        (HIGHSCHEDULEOVERLAPSCALE * _teamingOptions->desiredTimeBlocksOverlap));
            }
            else if(_schedScore[team] >= _teamingOptions->minTimeBlocksOverlap)	// if team has between minimum and desired amount of schedule overlap
            {
                _schedScore[team] /= _teamingOptions->desiredTimeBlocksOverlap;	// normal schedule score is number of overlaps / desired number of overlaps
            }
            else													// if team has fewer than minTimeBlocksOverlap, apply penalty
            {
                _schedScore[team] = 0;
                _penaltyPoints[team]++;
            }

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
                if(_student[_teammates[studentNum]].gender == StudentRecord::man)
                {
                    numMen++;
                }
                else if(_student[_teammates[studentNum]].gender == StudentRecord::woman)
                {
                    numWomen++;
                }
                else if(_student[_teammates[studentNum]].gender == StudentRecord::nonbinary)
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
                if(_student[_teammates[studentNum]].URM)
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
                IDsBeingTeamed.insert(_student[_teammates[studentNum]].ID);
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
                currStudent = &_student[_teammates[studentNum]];
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
    savedSettings.setValue("dataFileLocation", dataOptions->dataFile.canonicalFilePath());
    bool dontActuallyExit = false;
    bool saveSettings = savedSettings.value("saveDefaultsOnExit", false).toBool();

    if(savedSettings.value("askToSaveDefaultsOnExit", true).toBool())
    {
        QApplication::beep();
        QMessageBox saveOptionsOnClose(this);
        saveOptionsOnClose.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        QCheckBox neverShowAgain(tr("Don't ask me this again."), &saveOptionsOnClose);

        saveOptionsOnClose.setIcon(QMessageBox::Question);
        saveOptionsOnClose.setWindowTitle(tr("Save Options?"));
        saveOptionsOnClose.setText(tr("Before exiting, should we save the\ncurrent teaming options as defaults?"));
        saveOptionsOnClose.setCheckBox(&neverShowAgain);
        saveOptionsOnClose.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        saveOptionsOnClose.setButtonText(QMessageBox::Discard, tr("Don't Save"));
        saveOptionsOnClose.exec();

        if(saveOptionsOnClose.result() == QMessageBox::Save)
        {
            saveSettings = true;
        }
        else if(saveOptionsOnClose.result() == QMessageBox::Cancel)
        {
            dontActuallyExit = true;
        }

        if(neverShowAgain.checkState() == Qt::Checked)
        {
            savedSettings.setValue("askToSaveDefaultsOnExit", false);
            savedSettings.setValue("saveDefaultsOnExit", saveSettings);
        }
    }

    if(dontActuallyExit)
    {
        event->ignore();
    }
    else
    {
        if(saveSettings)
        {
            savedSettings.setValue("idealTeamSize", ui->idealTeamSizeBox->value());
            savedSettings.setValue("isolatedWomenPrevented", teamingOptions->isolatedWomenPrevented);
            savedSettings.setValue("isolatedMenPrevented", teamingOptions->isolatedMenPrevented);
            savedSettings.setValue("isolatedNonbinaryPrevented", teamingOptions->isolatedNonbinaryPrevented);
            savedSettings.setValue("singleGenderPrevented", teamingOptions->singleGenderPrevented);
            savedSettings.setValue("isolatedURMPrevented", teamingOptions->isolatedURMPrevented);
            savedSettings.setValue("minTimeBlocksOverlap", teamingOptions->minTimeBlocksOverlap);
            savedSettings.setValue("desiredTimeBlocksOverlap", teamingOptions->desiredTimeBlocksOverlap);
            savedSettings.setValue("meetingBlockSize", teamingOptions->meetingBlockSize);
            savedSettings.setValue("scheduleWeight", ui->scheduleWeight->value());
            savedSettings.beginWriteArray("Attributes");
            for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum)
            {
                savedSettings.setArrayIndex(attribNum);
                savedSettings.setValue("desireHomogeneous", teamingOptions->desireHomogeneous[attribNum]);
                savedSettings.setValue("weight", teamingOptions->attributeWeights[attribNum]);
                savedSettings.remove("incompatibleResponses");  //clear any existing values
                savedSettings.beginWriteArray("incompatibleResponses");
                for(int incompResp = 0; incompResp < teamingOptions->incompatibleAttributeValues[attribNum].size(); incompResp++)
                {
                    savedSettings.setArrayIndex(incompResp);
                    savedSettings.setValue("incompatibleResponses",
                                           (QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).first) + "," +
                                            QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).second)));
                }
                savedSettings.endArray();
            }
            savedSettings.endArray();
            savedSettings.setValue("requestedTeammateNumber", ui->requestedTeammateNumberBox->value());
        }

        event->accept();
    }
}
