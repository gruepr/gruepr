#include "ui_gruepr.h"
#include "gruepr.h"
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QTextBrowser>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QtNetwork>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QPrintDialog>
#include <QPainter>
#include <QFont>


gruepr::gruepr(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::gruepr)
{
    //Setup the main window
    ui->setupUi(this);
    ui->statusBar->setSizeGripEnabled(false);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowIcon(QIcon(":/icons/gruepr.png"));

    //Add the teamDataTree widget
    teamDataTree = new TeamTreeWidget(this);
    teamDataTree->setGeometry(0,0,624,626);
    ui->teamDataLayout->insertWidget(1, teamDataTree);
    connect(teamDataTree, QOverload<int, int>::of(&TeamTreeWidget::swapPlaces), this, &gruepr::swapTeammates);
    connect(teamDataTree, QOverload<int, int, int, int>::of(&TeamTreeWidget::swapPlaces), this, &gruepr::swapTeams);
    connect(teamDataTree, &TeamTreeWidget::teamInfoChanged, this, &gruepr::refreshTeamInfo);
    connect(teamDataTree, &QTreeWidget::itemCollapsed, teamDataTree, &TeamTreeWidget::collapseItem);
    connect(teamDataTree, &QTreeWidget::itemExpanded, teamDataTree, &TeamTreeWidget::expandItem);

    //UNIMPLEMENTED: URM isolation; for now, hiding checkbox
    ui->isolatedURMCheckBox->hide();

    //Remove register button if registered
    QSettings savedSettings;
    registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    if(registeredUser.isEmpty() || UserID != QString(QCryptographicHash::hash((registeredUser.toUtf8()),QCryptographicHash::Md5).toHex()))
    {
        ui->statusBar->showMessage(tr("This copy of gruepr is unregistered"));
        ui->statusBar->setStyleSheet("background-color: rgb(255, 105, 105)");
    }
    else
    {
        ui->registerButton->hide();
        ui->statusBar->showMessage(tr("This copy of gruepr is registered to ") + registeredUser);
        ui->statusBar->setStyleSheet("");
    }

    //Connect genetic algorithm progress signals to slots
    connect(this, &gruepr::generationComplete, this, &gruepr::updateOptimizationProgress);
    connect(this, &gruepr::optimizationMightBeComplete, this, &gruepr::askWhetherToContinueOptimizing);
    connect(&futureWatcher, &QFutureWatcher<void>::finished, this, &gruepr::optimizationComplete);

    // load all of the saved default values (if they exist)
    on_loadSettingsButton_clicked();

    // use arabic numbers for standard teamNames
    for(int team = 0; team < maxStudents/2; team++)
    {
        teamNames << QString::number(team+1);
    }

    student = new studentRecord[maxStudents];
}

gruepr::~gruepr()
{
    delete teamDataTree;
    delete [] student;
    delete ui;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Slots
/////////////////////////////////////////////////////////////////////////////////////////////////////////


void gruepr::on_loadSurveyFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Survey Data File"), dataOptions.dataFile.canonicalPath(), tr("Survey Data File (*.csv *.txt);;All Files (*)"));

    if (!fileName.isEmpty())
    {
        dataOptions.dataFile = QFileInfo(fileName);
        // Reset the various UI components
        ui->minMeetingTimes->setEnabled(false);
        ui->desiredMeetingTimes->setEnabled(false);
        ui->meetingLength->setEnabled(false);
        ui->requiredTeammatesButton->setEnabled(false);
        ui->preventedTeammatesButton->setEnabled(false);
        ui->sectionSelectionBox->clear();
        ui->sectionSelectionBox->setEnabled(false);
        ui->attributeLabel->clear();
        ui->attributeLabel->setEnabled(false);
        ui->attributeScrollBar->setEnabled(false);
        ui->attributeTextEdit->setPlainText("");
        ui->attributeTextEdit->setEnabled(false);
        ui->attributeWeight->setEnabled(false);
        ui->attributeHomogeneousBox->setEnabled(false);
        ui->scheduleWeight->setEnabled(false);
        ui->studentTable->clear();
        ui->studentTable->setRowCount(0);
        ui->studentTable->setColumnCount(0);
        ui->studentTable->setEnabled(false);
        ui->addStudentFirstName->setEnabled(false);
        ui->addStudentLastName->setEnabled(false);
        ui->addStudentEmail->setEnabled(false);
        ui->addStudentGenderComboBox->setEnabled(false);
        ui->addStudentURMComboBox->setEnabled(false);
        ui->addStudentSectionComboBox->setEnabled(false);
        ui->addStudentSectionComboBox->clear();
        ui->addStudentPushButton->setEnabled(false);
        ui->teamDataLayout->setEnabled(false);
        ui->teamDataText->clear();
        ui->teamDataText->setEnabled(false);
        teamDataTree->setEnabled(false);
        ui->label_23->setEnabled(false);
        ui->expandAllButton->setEnabled(false);
        ui->collapseAllButton->setEnabled(false);
        ui->sortTeamsButton->setEnabled(false);
        ui->label_14->setEnabled(false);
        ui->teamNamesComboBox->setEnabled(false);
        ui->tabWidget->setCurrentIndex(0);
        ui->isolatedWomenCheckBox->setEnabled(false);
        ui->isolatedMenCheckBox->setEnabled(false);
        ui->mixedGenderCheckBox->setEnabled(false);
        ui->isolatedURMCheckBox->setEnabled(false);
        ui->teamSizeBox->clear();
        ui->teamSizeBox->setEnabled(false);
        ui->idealTeamSizeBox->setEnabled(false);
        ui->letsDoItButton->setEnabled(false);
        ui->saveTeamsButton->setEnabled(false);
        ui->printTeamsButton->setEnabled(false);

        if(loadSurveyData(fileName))
        {
            ui->saveSettingsButton->setEnabled(true);
            ui->loadSettingsButton->setEnabled(true);
            ui->statusBar->showMessage("File: " + dataOptions.dataFile.fileName());
            ui->studentTable->setEnabled(true);
            ui->addStudentFirstName->setEnabled(true);
            ui->addStudentLastName->setEnabled(true);
            ui->addStudentEmail->setEnabled(true);
            ui->addStudentPushButton->setEnabled(true);
            ui->teamDataLayout->setEnabled(true);
            ui->minMeetingTimes->setEnabled(true);
            ui->desiredMeetingTimes->setEnabled(true);
            ui->meetingLength->setEnabled(true);
            ui->requiredTeammatesButton->setEnabled(true);
            ui->preventedTeammatesButton->setEnabled(true);

            if(dataOptions.sectionIncluded)
            {
                //get number of sections
                QStringList sectionNames;
                for(int ID = 0; ID < numStudents; ID++)
                {
                    if(!sectionNames.contains(student[ID].section))
                    {
                        sectionNames.append(student[ID].section);
                    }
                }
                if(sectionNames.size() > 1)
                {
                    sectionNames.sort();
                    ui->sectionSelectionBox->setEnabled(true);
                    ui->sectionSelectionBox->addItem(tr("Students in all sections together"));
                    ui->sectionSelectionBox->insertSeparator(1);
                    ui->sectionSelectionBox->addItems(sectionNames);
                    ui->addStudentSectionComboBox->show();
                    ui->addStudentSectionComboBox->setEnabled(true);
                    ui->addStudentSectionComboBox->addItems(sectionNames);
                }
                else
                {
                    ui->sectionSelectionBox->setEnabled(false);
                    ui->sectionSelectionBox->addItem(tr("Only one section in the data."));
                    ui->addStudentSectionComboBox->setEnabled(false);
                    ui->addStudentSectionComboBox->hide();
                }
            }
            else
            {
                ui->sectionSelectionBox->setEnabled(false);
                ui->sectionSelectionBox->addItem("No section data.");
                ui->addStudentSectionComboBox->setEnabled(false);
                ui->addStudentSectionComboBox->hide();
            }
            on_sectionSelectionBox_currentIndexChanged(ui->sectionSelectionBox->currentText());

            if(dataOptions.numAttributes > 0)
            {
                ui->attributeScrollBar->setMinimum(0);
                ui->attributeScrollBar->setMaximum(dataOptions.numAttributes-1);
                ui->attributeScrollBar->setEnabled(dataOptions.numAttributes > 1);
                ui->attributeScrollBar->setValue(0);
                on_attributeScrollBar_valueChanged(0);
                ui->attributeLabel->setText(tr("1  of  ") + QString::number(dataOptions.numAttributes));
                ui->attributeLabel->setEnabled(true);
                ui->attributeTextEdit->setEnabled(true);
                ui->attributeWeight->setEnabled(true);
                ui->attributeHomogeneousBox->setEnabled(true);
                ui->scheduleWeight->setEnabled(true);
            }
            else
            {
                ui->attributeScrollBar->setMaximum(-1);     // auto-sets the value and the minimum to all equal -1
            }

            if(dataOptions.genderIncluded)
            {
                ui->addStudentGenderComboBox->show();
                ui->addStudentGenderComboBox->setEnabled(true);
                ui->isolatedWomenCheckBox->setEnabled(true);
                ui->isolatedMenCheckBox->setEnabled(true);
                ui->mixedGenderCheckBox->setEnabled(true);
            }
            else
            {
                ui->addStudentGenderComboBox->hide();
                ui->addStudentGenderComboBox->setEnabled(false);
                ui->isolatedWomenCheckBox->setEnabled(false);
                ui->isolatedWomenCheckBox->setChecked(false);
                ui->isolatedMenCheckBox->setEnabled(false);
                ui->isolatedMenCheckBox->setChecked(false);
                ui->mixedGenderCheckBox->setEnabled(false);
                ui->mixedGenderCheckBox->setChecked(false);
            }

            if(dataOptions.URMIncluded)
            {
                ui->addStudentURMComboBox->show();
                ui->addStudentURMComboBox->setEnabled(true);
                ui->isolatedURMCheckBox->setEnabled(true);
            }
            else
            {
                ui->addStudentURMComboBox->hide();
                ui->addStudentURMComboBox->setEnabled(false);
                ui->isolatedURMCheckBox->setEnabled(false);
                ui->isolatedURMCheckBox->setChecked(false);
            }

            ui->idealTeamSizeBox->setEnabled(true);
            ui->teamSizeBox->setEnabled(true);
            on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary

            ui->letsDoItButton->setEnabled(true);
        }
    }
}


void gruepr::on_loadSettingsButton_clicked()
{
    //Load default settings (from saved settings, if they exist)
    QSettings savedSettings;
    dataOptions.dataFile.setFile(savedSettings.value("dataFileLocation", "").toString());
    teamingOptions.isolatedWomenPrevented = savedSettings.value("isolatedWomenPrevented", false).toBool();
    ui->isolatedWomenCheckBox->setChecked(teamingOptions.isolatedWomenPrevented);
    teamingOptions.isolatedMenPrevented = savedSettings.value("isolatedMenPrevented", false).toBool();
    ui->isolatedMenCheckBox->setChecked(teamingOptions.isolatedMenPrevented);
    teamingOptions.mixedGenderPreferred = savedSettings.value("mixedGenderPreferred", false).toBool();
    ui->mixedGenderCheckBox->setChecked(teamingOptions.isolatedMenPrevented);
    teamingOptions.isolatedURMPrevented = savedSettings.value("isolatedURMPrevented", false).toBool();
    ui->isolatedURMCheckBox->setChecked(teamingOptions.isolatedURMPrevented);
    teamingOptions.desiredTimeBlocksOverlap = savedSettings.value("desiredTimeBlocksOverlap", 8).toInt();
    ui->desiredMeetingTimes->setValue(teamingOptions.desiredTimeBlocksOverlap);
    teamingOptions.minTimeBlocksOverlap = savedSettings.value("minTimeBlocksOverlap", 4).toInt();
    ui->minMeetingTimes->setValue(teamingOptions.minTimeBlocksOverlap);
    teamingOptions.meetingBlockSize = savedSettings.value("meetingBlockSize", 1).toInt();
    ui->meetingLength->setCurrentIndex(teamingOptions.meetingBlockSize-1);
    ui->idealTeamSizeBox->setValue(savedSettings.value("idealTeamSize", ui->idealTeamSizeBox->value()).toInt());
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
    savedSettings.beginReadArray("Attributes");
    for (int i = 0; i < maxAttributes; ++i)
    {
        savedSettings.setArrayIndex(i);
        teamingOptions.desireHomogeneous[i] = savedSettings.value("desireHomogeneous", false).toBool();
        teamingOptions.attributeWeights[i] = savedSettings.value("Weight", 1).toDouble();
    }
    savedSettings.endArray();
    if(ui->attributeScrollBar->value() == 0)
    {
        on_attributeScrollBar_valueChanged(0);      // displays the correct attribute weight, homogeneity, text in case scrollbar is already at 0
    }
    else
    {
        ui->attributeScrollBar->setValue(0);
    }
    teamingOptions.scheduleWeight = savedSettings.value("scheduleWeight", 4).toDouble();
    ui->scheduleWeight->setValue(teamingOptions.scheduleWeight);

    QStringList keys = savedSettings.allKeys();
    keys.removeOne("registeredUser");
    keys.removeOne("askToSaveDefaultsOnExit");
    ui->clearSettingsButton->setEnabled(!keys.isEmpty());  // setting this value based on whether there ARE saved settings
}


void gruepr::on_saveSettingsButton_clicked()
{
    QSettings savedSettings;
    savedSettings.setValue("dataFileLocation", dataOptions.dataFile.canonicalFilePath());
    savedSettings.setValue("isolatedWomenPrevented", teamingOptions.isolatedWomenPrevented);
    savedSettings.setValue("isolatedMenPrevented", teamingOptions.isolatedMenPrevented);
    savedSettings.setValue("mixedGenderPreferred", teamingOptions.mixedGenderPreferred);
    savedSettings.setValue("isolatedURMPrevented", teamingOptions.isolatedURMPrevented);
    savedSettings.setValue("desiredTimeBlocksOverlap", teamingOptions.desiredTimeBlocksOverlap);
    savedSettings.setValue("minTimeBlocksOverlap", teamingOptions.minTimeBlocksOverlap);
    savedSettings.setValue("meetingBlockSize", teamingOptions.meetingBlockSize);
    savedSettings.setValue("idealTeamSize", ui->idealTeamSizeBox->value());
    savedSettings.beginWriteArray("Attributes");
    for (int i = 0; i < maxAttributes; ++i)
    {
        savedSettings.setArrayIndex(i);
        savedSettings.setValue("desireHomogeneous", teamingOptions.desireHomogeneous[i]);
        savedSettings.setValue("Weight", teamingOptions.attributeWeights[i]);
    }
    savedSettings.endArray();
    savedSettings.setValue("scheduleWeight", ui->scheduleWeight->value());

    ui->clearSettingsButton->setEnabled(true);
}


void gruepr::on_clearSettingsButton_clicked()
{
    // Clear all settings
    QSettings savedSettings;

    //Uncomment the line below and the one two below in order to prevent clearing the setting about "don't show this box again" on app exit
    //bool askToSave = savedSettings.value("askToSaveDefaultsOnExit",true).toBool();
    savedSettings.clear();
    //savedSettings.setValue("askToSaveDefaultsOnExit", askToSave);

    //put the registered user name back in if it exists
    if(!registeredUser.isEmpty())
    {
        savedSettings.setValue("registeredUser", registeredUser);
    }

    ui->clearSettingsButton->setEnabled(false);
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

    ui->idealTeamSizeBox->setMaximum(numStudents/2);
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
}


void gruepr::on_studentTable_cellEntered(int row, int /*unused column value*/)
{
    ui->studentTable->selectRow(row);
}


void gruepr::removeAStudent()
{
    //Search through all the students and, once we found the one with the matching ID, move all remaining ones ahead by one and decrement numStudentsInSystem
    bool foundIt = false;
    for(int i = 0; i < dataOptions.numStudentsInSystem; i++)
    {
        if(sender()->property("StudentID").toInt() == student[i].ID)
        {
            foundIt = true;
        }
        if(foundIt)
        {
            student[i] = student[i+1];
        }
    }
    dataOptions.numStudentsInSystem--;

    refreshStudentDisplay();

    ui->idealTeamSizeBox->setMaximum(numStudents/2);
    on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
}


void gruepr::on_addStudentPushButton_clicked()
{
    if(dataOptions.numStudentsInSystem < maxStudents)
    {

        student[dataOptions.numStudentsInSystem].firstname = (ui->addStudentFirstName->text()).trimmed();
        student[dataOptions.numStudentsInSystem].firstname[0] = student[dataOptions.numStudentsInSystem].firstname[0].toUpper();
        student[dataOptions.numStudentsInSystem].lastname = (ui->addStudentLastName->text()).trimmed();
        student[dataOptions.numStudentsInSystem].lastname[0] = student[dataOptions.numStudentsInSystem].lastname[0].toUpper();
        student[dataOptions.numStudentsInSystem].email = (ui->addStudentEmail->text()).trimmed();
        student[dataOptions.numStudentsInSystem].section = ui->addStudentSectionComboBox->currentText();
        if(dataOptions.genderIncluded)
        {
            if(ui->addStudentGenderComboBox->currentText()==tr("woman"))
            {
                student[dataOptions.numStudentsInSystem].gender = studentRecord::woman;
            }
            else if(ui->addStudentGenderComboBox->currentText()==tr("man"))
            {
                student[dataOptions.numStudentsInSystem].gender = studentRecord::man;
            }
            else
            {
                student[dataOptions.numStudentsInSystem].gender = studentRecord::neither;
            }
        }
        if(dataOptions.URMIncluded)
        {
            student[dataOptions.numStudentsInSystem].URM = (ui->addStudentURMComboBox->currentText()==tr("URM"));
        }
        student[dataOptions.numStudentsInSystem].ID = dataOptions.numStudentsInSystem + maxStudents;   //flag for added students is an ID > maxStudents
        for(int i = 0; i < maxAttributes; i++)
        {
            student[dataOptions.numStudentsInSystem].attribute[i] = -1;     //set all attribute levels to -1 as a flag to ignore during the teaming
        }
        for(int time = 0; time < numTimeBlocks; time++)
        {
            student[dataOptions.numStudentsInSystem].unavailable[time] = false;
        }
        dataOptions.numStudentsInSystem++;

        refreshStudentDisplay();

        ui->idealTeamSizeBox->setMaximum(numStudents/2);
        on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box
    }
    else
    {
        QMessageBox::warning(this, tr("Cannot add student."),
                             tr("Sorry, we cannot add another student.\nThis version of gruepr does not allow more than ") + QString(maxStudents) + tr("."), QMessageBox::Ok);
    }
}


void gruepr::on_isolatedWomenCheckBox_stateChanged(int arg1)
{
    teamingOptions.isolatedWomenPrevented = arg1;
}


void gruepr::on_isolatedMenCheckBox_stateChanged(int arg1)
{
    teamingOptions.isolatedMenPrevented = arg1;
}


void gruepr::on_mixedGenderCheckBox_stateChanged(int arg1)
{
    teamingOptions.mixedGenderPreferred = arg1;
}


void gruepr::on_isolatedURMCheckBox_stateChanged(int arg1)
{
    teamingOptions.isolatedURMPrevented = arg1;
}


void gruepr::on_attributeScrollBar_valueChanged(int value)
{
    if(value >= 0)    // needed for when scroll bar is cleared, when value gets set to -1
    {
        ui->attributeTextEdit->setPlainText(dataOptions.attributeQuestionText[value]);
        ui->attributeWeight->setValue(teamingOptions.attributeWeights[value]);
        ui->attributeHomogeneousBox->setChecked(teamingOptions.desireHomogeneous[value]);
        ui->attributeLabel->setText(QString::number(value+1) + tr("  of  ") + QString::number(dataOptions.numAttributes));
    }
}


void gruepr::on_attributeWeight_valueChanged(double arg1)
{
    teamingOptions.attributeWeights[ui->attributeScrollBar->value()] = arg1;
}


void gruepr::on_attributeHomogeneousBox_stateChanged(int arg1)
{
    teamingOptions.desireHomogeneous[ui->attributeScrollBar->value()] = arg1;
}


void gruepr::on_scheduleWeight_valueChanged(double arg1)
{
    teamingOptions.scheduleWeight = arg1;
}


void gruepr::on_minMeetingTimes_valueChanged(int arg1)
{
    teamingOptions.minTimeBlocksOverlap = arg1;
    if(ui->desiredMeetingTimes->value() < (arg1+1))
    {
        ui->desiredMeetingTimes->setValue(arg1+1);
    }
}


void gruepr::on_desiredMeetingTimes_valueChanged(int arg1)
{
    teamingOptions.desiredTimeBlocksOverlap = arg1;
    if(ui->minMeetingTimes->value() > (arg1-1))
    {
        ui->minMeetingTimes->setValue(arg1-1);
    }
}


void gruepr::on_meetingLength_currentIndexChanged(int index)
{
    teamingOptions.meetingBlockSize = (index + 1);
}


void gruepr::on_requiredTeammatesButton_clicked()
{
    //Open specialized dialog box to collect pairings that are required
    gatherTeammatesDialog *window =
            new gatherTeammatesDialog(gatherTeammatesDialog::required, student, dataOptions.numStudentsInSystem, (ui->sectionSelectionBox->currentIndex()==0)? "" : sectionName, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = window->exec();
    if(reply == QDialog::Accepted)
    {
        for(int i = 0; i < dataOptions.numStudentsInSystem; i++)
        {
            this->student[i] = window->student[i];
        }
    }

    delete window;
}


void gruepr::on_preventedTeammatesButton_clicked()
{
    //Open specialized dialog box to collect pairings that are prevented
    gatherTeammatesDialog *window =
            new gatherTeammatesDialog(gatherTeammatesDialog::prevented, student, dataOptions.numStudentsInSystem, (ui->sectionSelectionBox->currentIndex()==0)? "" : sectionName, this);

    //If user clicks OK, replace student database with copy that has had pairings added
    int reply = window->exec();
    if(reply == QDialog::Accepted)
    {
        for(int i = 0; i < numStudents; i++)
        {
            this->student[i] = window->student[i];
        }
    }

    delete window;
}


void gruepr::on_idealTeamSizeBox_valueChanged(int arg1)
{
    ui->teamSizeBox->clear();

    numTeams = numStudents/arg1;
    teamingOptions.smallerTeamsNumTeams = numTeams;
    teamingOptions.largerTeamsNumTeams = numTeams;

    if(numStudents%arg1 != 0)       //if teams can't be evenly divided into this size
    {
        int smallerTeamsSizeA=0, smallerTeamsSizeB=0, numSmallerATeams=0, largerTeamsSizeA=0, largerTeamsSizeB=0, numLargerATeams=0;

        // reset the potential team sizes
        for(int student = 0; student < maxStudents; student++)
        {
            teamingOptions.smallerTeamsSizes[student] = 0;
            teamingOptions.largerTeamsSizes[student] = 0;
        }

        // What are the team sizes when desiredTeamSize represents a maximum size?
        teamingOptions.smallerTeamsNumTeams = numTeams+1;
        for(int student = 0; student < numStudents; student++)      // run through every student
        {
            // add one student to each team (with 1 additional team relative to before) in turn until we run out of students
            (teamingOptions.smallerTeamsSizes[student%teamingOptions.smallerTeamsNumTeams])++;
            smallerTeamsSizeA = teamingOptions.smallerTeamsSizes[student%teamingOptions.smallerTeamsNumTeams];  // the larger of the two (uneven) team sizes
            numSmallerATeams = (student%teamingOptions.smallerTeamsNumTeams)+1;                                 // the number of larger teams
        }
        smallerTeamsSizeB = smallerTeamsSizeA - 1;                  // the smaller of the two (uneven) team sizes

        // And what are the team sizes when desiredTeamSize represents a minimum size?
        teamingOptions.largerTeamsNumTeams = numTeams;
        for(int student = 0; student < numStudents; student++)	// run through every student
        {
            // add one student to each team in turn until we run out of students
            (teamingOptions.largerTeamsSizes[student%teamingOptions.largerTeamsNumTeams])++;
            largerTeamsSizeA = teamingOptions.largerTeamsSizes[student%teamingOptions.largerTeamsNumTeams];     // the larger of the two (uneven) team sizes
            numLargerATeams = (student%teamingOptions.largerTeamsNumTeams)+1;                                   // the number of larger teams
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
        if((numSmallerATeams > 0) && ((numTeams+1-numSmallerATeams) > 0))
        {
            smallerTeamOption += " + ";
        }
        if((numTeams+1-numSmallerATeams) > 0)
        {
            smallerTeamOption += QString::number(numTeams+1-numSmallerATeams) + tr(" team");
            if((numTeams+1-numSmallerATeams) > 1)
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
        if((numTeams-numLargerATeams) > 0)
        {
            largerTeamOption += QString::number(numTeams-numLargerATeams) + tr(" team");
            if((numTeams-numLargerATeams) > 1)
            {
                largerTeamOption += "s";
            }
            largerTeamOption += " of " + QString::number(largerTeamsSizeB) + tr(" student");
            if(largerTeamsSizeB > 1)
            {
                largerTeamOption += "s";
            }
        }
        if(((numTeams-numLargerATeams) > 0) && (numLargerATeams > 0))
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
        ui->teamSizeBox->addItem(QString::number(numTeams) + tr(" teams of ") + QString::number(arg1) + tr(" students"));
    }
    ui->teamSizeBox->insertSeparator(ui->teamSizeBox->count());
    ui->teamSizeBox->addItem(tr("Custom team sizes"));
}


void gruepr::on_teamSizeBox_currentIndexChanged(int index)
{
    if(ui->teamSizeBox->currentText() == (QString::number(numTeams) + tr(" teams of ") + QString::number(ui->idealTeamSizeBox->value()) + tr(" students")))
    {
        // Evenly divisible teams, all same size
        setTeamSizes(ui->idealTeamSizeBox->value());
    }
    else if(ui->teamSizeBox->currentText() == tr("Custom team sizes"))
    {
        //Open specialized dialog box to collect teamsizes
        customTeamsizesDialog *window = new customTeamsizesDialog(numStudents, ui->idealTeamSizeBox->value(), this);

        //If user clicks OK, use these team sizes, otherwise revert to option 1, smaller team sizes
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
            numTeams = window->numTeams;
            setTeamSizes(window->teamsizes);
        }
        else
        {
            // Set to smaller teams if cancelled
            bool oldState = ui->teamSizeBox->blockSignals(true);
            ui->teamSizeBox->setCurrentIndex(0);
            numTeams = teamingOptions.smallerTeamsNumTeams;
            setTeamSizes(teamingOptions.smallerTeamsSizes);
            ui->teamSizeBox->blockSignals(oldState);
        }

        delete window;
    }
    else if(index == 0)
    {
        // Smaller teams desired
        numTeams = teamingOptions.smallerTeamsNumTeams;
        setTeamSizes(teamingOptions.smallerTeamsSizes);
    }
    else if (index == 1)
    {
        // Larger teams desired
        numTeams = teamingOptions.largerTeamsNumTeams;
        setTeamSizes(teamingOptions.largerTeamsSizes);
    }
}


void gruepr::on_letsDoItButton_clicked()
{
    // Update UI
    ui->scoreBox->setEnabled(true);
    ui->scoreBox->clear();
    ui->label_11->setEnabled(true);
    ui->generationsBox->setEnabled(true);
    ui->generationsBox->clear();
    ui->label_3->setEnabled(true);
    ui->stabilityProgressBar->setEnabled(true);
    ui->stabilityProgressBar->reset();
    ui->label_12->setEnabled(true);
    ui->label_13->setEnabled(true);
    ui->sectionSelectionBox->setEnabled(false);
    ui->loadSurveyFileButton->setEnabled(false);
    ui->saveTeamsButton->setEnabled(false);
    ui->printTeamsButton->setEnabled(false);
    ui->letsDoItButton->setEnabled(false);
    ui->letsDoItButton->hide();
    ui->cancelOptimizationButton->setEnabled(true);
    ui->cancelOptimizationButton->show();

    // Set up to show progess on windows taskbar
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(windowHandle());
    taskbarProgress = taskbarButton->progress();
    taskbarProgress->show();
    taskbarProgress->setMaximum(0);

    // Get the IDs of students from desired section and change numStudents accordingly
    int numStudentsInSection = 0;
    studentIDs = new int[dataOptions.numStudentsInSystem];
    for(int ID = 0; ID < dataOptions.numStudentsInSystem; ID++)
    {
        if(ui->sectionSelectionBox->currentIndex() == 0 || ui->sectionSelectionBox->currentText() == student[ID].section)
        {
            studentIDs[numStudentsInSection] = ID;
            numStudentsInSection++;
        }
    }
    numStudents = numStudentsInSection;

    // Set up the flag to allow a stoppage and set up futureWatcher to know when results are available
    optimizationStopped = false;
    future = QtConcurrent::run(this, &gruepr::optimizeTeams, studentIDs);       // spin optimization off into a separate thread
    futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
}


void gruepr::updateOptimizationProgress(double score, int generation, double scoreStability)
{
    ui->generationsBox->setValue(generation);
    ui->scoreBox->setValue(score);
    if(generation >= generationsOfStability)
    {
        ui->stabilityProgressBar->setValue((scoreStability<100)? int(scoreStability) : 100);
        taskbarProgress->setMaximum(100);
        taskbarProgress->setValue((scoreStability<100)? int(scoreStability) : 100);
    }
    if(generation >= minGenerations)
    {
        ui->stabilityProgressBar->setEnabled(true);
        ui->generationsBox->setStyleSheet("");
    }
    else
    {
        ui->stabilityProgressBar->setEnabled(false);
        ui->generationsBox->setStyleSheet("background-color: #f283a5");
    }
}


void gruepr::on_cancelOptimizationButton_clicked()
{
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    connect(this, &gruepr::turnOffBusyCursor, this, &QApplication::restoreOverrideCursor);
    optimizationStoppedmutex.lock();
    optimizationStopped = true;
    optimizationStoppedmutex.unlock();
}


void gruepr::askWhetherToContinueOptimizing(int generation)
{
    QApplication::beep();
    QApplication::alert(this);

    QMessageBox questionWindow(this);
    questionWindow.setText(tr("Should we show the teams or continue optimizing?"));
    questionWindow.setWindowTitle((generation < maxGenerations)? tr("The score seems to be stable.") : (tr("We have reached ") + QString::number(maxGenerations) + tr(" generations.")));
    questionWindow.setIcon(QMessageBox::Question);
    questionWindow.setWindowModality(Qt::ApplicationModal);
    questionWindow.addButton(tr("Show Teams"), QMessageBox::YesRole);
    QPushButton *keepGoing = questionWindow.addButton(tr("Continue Optimizing"), QMessageBox::NoRole);

    questionWindow.exec();

    keepOptimizing = (questionWindow.clickedButton() == keepGoing);

    emit haveOurKeepOptimizingValue();
}


void gruepr::optimizationComplete()
{
    // update UI
    ui->scoreBox->setEnabled(false);
    ui->scoreBox->clear();
    ui->label_11->setEnabled(false);
    ui->generationsBox->setEnabled(false);
    ui->generationsBox->clear();
    ui->generationsBox->setStyleSheet("");
    ui->label_3->setEnabled(false);
    ui->stabilityProgressBar->setEnabled(false);
    ui->stabilityProgressBar->reset();
    ui->label_12->setEnabled(false);
    ui->label_13->setEnabled(false);
    ui->sectionSelectionBox->setEnabled(true);
    ui->loadSurveyFileButton->setEnabled(true);
    ui->tabWidget->setCurrentIndex(1);
    ui->saveTeamsButton->setEnabled(true);
    ui->printTeamsButton->setEnabled(true);
    ui->letsDoItButton->setEnabled(true);
    ui->letsDoItButton->show();
    ui->cancelOptimizationButton->setEnabled(false);
    ui->cancelOptimizationButton->hide();
    ui->teamDataLayout->setEnabled(true);
    ui->teamDataText->setEnabled(true);
    teamDataTree->setEnabled(true);
    teamDataTree->setHeaderHidden(false);
    ui->expandAllButton->setEnabled(true);
    ui->collapseAllButton->setEnabled(true);
    ui->sortTeamsButton->setEnabled(true);
    ui->label_14->setEnabled(true);
    ui->label_23->setEnabled(true);
    ui->teamNamesComboBox->setEnabled(true);
    taskbarProgress->hide();

    // free memory used to save array of IDs of students being teamed
    delete[] studentIDs;

    // Unpack the best team set and print teams list on the screen
    QList<int> bestTeamSet = future.result();
    for(int ID = 0; ID < numStudents; ID++)
    {
        bestGenome[ID] = bestTeamSet[ID];
    }

    refreshTeamInfo();
}


void gruepr::on_expandAllButton_clicked()
{
    teamDataTree->expandAll();
}


void gruepr::on_collapseAllButton_clicked()
{
    teamDataTree->collapseAll();
}


void gruepr::on_sortTeamsButton_clicked()
{
    double teamScores[maxStudents];
    teamSetScore = getTeamScores(bestGenome, teamScores);
    for (int i = 0; i < numTeams-1; i++)
        for (int j = 0; j < numTeams-i-1; j++)
            if (teamScores[j] > teamScores[j+1])
            {
                swapTeams(j, teamSize[j], j+1, teamSize[j+1]);
                std::swap(teamScores[j], teamScores[j+1]);
            }
    refreshTeamInfo();
}


void gruepr::on_teamNamesComboBox_currentIndexChanged(int index)
{
    static int prevIndex = 0;
    //set team names to:
    if(index == 0)
    {
        // arabic numbers
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = QString::number(team+1);
        }
        prevIndex = 0;
    }
    else if(index == 1)
    {
        // roman numerals
        QString M[] = {"","M","MM","MMM"};
        QString C[] = {"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
        QString X[] = {"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
        QString I[] = {"","I","II","III","IV","V","VI","VII","VIII","IX"};
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = M[(team+1)/1000]+C[((team+1)%1000)/100]+X[((team+1)%100)/10]+I[((team+1)%10)];
        }
        prevIndex = 1;
    }
    else if(index == 2)
    {
        // hexadecimal numbers
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = QString::number(team, 16).toUpper();
        }
        prevIndex = 2;
    }
    else if(index == 3)
    {
        // English letters
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = QString((team/26)+1, char((team%26)+65));
        }
        prevIndex = 3;
    }
    else if(index == 4)
    {
        // Greek letters
        QStringList Greekletter = (QString("Alpha,Beta,Gamma,Delta,Epsilon,Zeta,Eta,Theta,Iota,Kappa,"
                                           "Lambda,Mu,Nu,Xi,Omicron,Pi,Rho,Sigma,Tau,Upsilon,Phi,Chi,Psi,Omega").split(","));
        //Cycle through list as often as needed, adding a repetition every time through the list
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = (Greekletter[team%(Greekletter.size())]+" ").repeated((team/Greekletter.size())+1).trimmed();
        }
        prevIndex = 4;
    }
    else if(index == 5)
    {
        // NATO phonetic alphabet
        QStringList NATOletter = (QString("Alfa,Bravo,Charlie,Delta,Echo,Foxtrot,Golf,Hotel,India,Juliett,Kilo,"
                                          "Lima,Mike,November,Oscar,Papa,Quebec,Romeo,Sierra,Tango,Uniform,Victor,Whiskey,X-ray,Yankee,Zulu").split(","));
        //Cycle through list as often as needed, adding a repetition every time through the list
        for(int team = 0; team < numTeams; team++)
        {
            teamNames[team] = (NATOletter[team%(NATOletter.size())]+" ").repeated((team/NATOletter.size())+1).trimmed();
        }
        prevIndex = 5;
    }
    else
    {
        //Open specialized dialog box to collect teamnames
        customTeamnamesDialog *window = new customTeamnamesDialog(numTeams, teamNames, this);

        //If user clicks OK, use these team sizes, otherwise revert to previous option
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
            for(int team = 0; team < numTeams; team++)
            {
                teamNames[team] = (window->teamName[team].text().isEmpty()? QString::number(team+1) : window->teamName[team].text());
            }
            prevIndex = 6;
            bool currentValue = ui->teamNamesComboBox->blockSignals(true);
            ui->teamNamesComboBox->setCurrentIndex(prevIndex);
            ui->teamNamesComboBox->setItemText(6, tr("Current names"));
            ui->teamNamesComboBox->removeItem(7);
            ui->teamNamesComboBox->addItem(tr("Custom names"));
            ui->teamNamesComboBox->blockSignals(currentValue);
        }
        else
        {
            bool currentValue = ui->teamNamesComboBox->blockSignals(true);
            ui->teamNamesComboBox->setCurrentIndex(prevIndex);
            ui->teamNamesComboBox->blockSignals(currentValue);
        }

        delete window;
    }

    if(ui->teamNamesComboBox->currentIndex() < 6)
    {
        ui->teamNamesComboBox->removeItem(7);
        ui->teamNamesComboBox->removeItem(6);
        ui->teamNamesComboBox->addItem(tr("Custom names"));
    }

    refreshTeamInfo();
}


void gruepr::on_saveTeamsButton_clicked()
{
    QStringList previews = {studentsFileContents.left(1000) + "...", instructorsFileContents.left(1000) + "...", spreadsheetFileContents.left(1000) + "..."};

    //Open specialized dialog box to choose which file(s) to save
    whichFilesDialog *window = new whichFilesDialog(whichFilesDialog::save, previews, this);
    int result = window->exec();

    if(result == QDialogButtonBox::Save && (window->instructorFile->isChecked() || window->studentFile->isChecked() || window->spreadsheetFile->isChecked()))
    {
        //save to text files
        QString baseFileName = QFileDialog::getSaveFileName(this, tr("Choose a location and base filename"), "", tr("Text File (*.txt);;All Files (*)"));
        if ( !(baseFileName.isEmpty()) )
        {
            bool problemSaving = false;
            if(window->instructorFile->isChecked())
            {
                QFile instructorsFile(QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_instructor." + QFileInfo(baseFileName).suffix());
                if(instructorsFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&instructorsFile);
                    out << instructorsFileContents;
                    instructorsFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(window->studentFile->isChecked())
            {
                QFile studentsFile(QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_student." + QFileInfo(baseFileName).suffix());
                if(studentsFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&studentsFile);
                    out << studentsFileContents;
                    studentsFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(window->spreadsheetFile->isChecked())
            {
                QFile spreadsheetFile(QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_spreadsheet." + QFileInfo(baseFileName).suffix());
                if(spreadsheetFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&spreadsheetFile);
                    out << spreadsheetFileContents;
                    spreadsheetFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(problemSaving)
            {
                QMessageBox::critical(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
            }
        }
    }
    else if(result == QDialogButtonBox::SaveAll && (window->instructorFile->isChecked() || window->studentFile->isChecked() || window->spreadsheetFile->isChecked()))
    {
        //save as formatted pdf files
        printFiles(window->instructorFile->isChecked(), window->studentFile->isChecked(), window->spreadsheetFile->isChecked(), true);
    }
    delete window;
}


void gruepr::on_printTeamsButton_clicked()
{
    QStringList previews = {studentsFileContents.left(1000) + "...", instructorsFileContents.left(1000) + "...", spreadsheetFileContents.left(1000) + "..."};

    //Open specialized dialog box to choose which file(s) to print
    whichFilesDialog *window = new whichFilesDialog(whichFilesDialog::print, previews, this);
    int result = window->exec();

    if(result == QDialogButtonBox::Ok && (window->instructorFile->isChecked() || window->studentFile->isChecked() || window->spreadsheetFile->isChecked()))
    {
        printFiles(window->instructorFile->isChecked(), window->studentFile->isChecked(), false, false);
    }
    delete window;
}


void gruepr::swapTeammates(int studentAID, int studentBID)
{
    if(studentAID == studentBID)
    {
        return;
    }

    //find them then swap them
    int studentAIndex = static_cast<int>(std::distance(bestGenome, std::find(bestGenome, bestGenome+numStudents, studentAID)));
    int studentBIndex = static_cast<int>(std::distance(bestGenome, std::find(bestGenome, bestGenome+numStudents, studentBID)));
    std::swap(bestGenome[studentAIndex], bestGenome[studentBIndex]);
}


void gruepr::swapTeams(int teamA, int teamAsize, int teamB, int teamBsize)
{
    if(teamA == teamB)
    {
        return;
    }

    // make sure teamA is the first one in the genome
    if(teamA > teamB)
    {
        std::swap(teamA, teamB);
        std::swap(teamAsize, teamBsize);
    }

    // get ID of first element in teamA and teamB
    int ID = 0, teamAID = 0, teamBID = 0, team;
    for(team = 0; team < numTeams; team++)
    {
        if(team == teamA)
        {
            teamAID = ID;
        }
        if(team == teamB)
        {
            teamBID = ID;
        }
        ID += teamSize[team];
    }

    int *newGenome = new int[maxStudents];
    int oldID = 0, newID = 0;

    // not at first team yet
    for(team = 0; team < teamA; team++)
    {
        std::copy(bestGenome + oldID, bestGenome + oldID + teamSize[team], newGenome + newID);
        newID += teamSize[team];
        oldID += teamSize[team];
    }

    // at first team
    std::copy(bestGenome + teamBID, bestGenome + teamBID + teamBsize, newGenome + newID);
    newID += teamBsize;
    oldID += teamAsize;

    // beyond first team but not at second team yet
    for(team = teamA + 1; team < teamB; team++)
    {
        std::copy(bestGenome + oldID, bestGenome + oldID + teamSize[team], newGenome + newID);
        newID += teamSize[team];
        oldID += teamSize[team];
    }

    // at second team
    std::copy(bestGenome + teamAID, bestGenome + teamAID + teamAsize, newGenome + newID);
    newID += teamAsize;
    oldID += teamBsize;

    // beyond second team
    for(team = teamB + 1; team < numTeams; team++)
    {
        std::copy(bestGenome + oldID, bestGenome + oldID + teamSize[team], newGenome + newID);
        newID += teamSize[team];
        oldID += teamSize[team];
    }

    std::copy(newGenome, newGenome+maxStudents, bestGenome);
    delete [] newGenome;
    std::swap(teamSize[teamA], teamSize[teamB]);
}



void gruepr::on_HelpButton_clicked()
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
    helpContents.setHtml(tr("<h1>gruepr " GRUEPR_VERSION_NUMBER "</h1>"
                            "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                            "<p>Joshua Hertz <a href = mailto:j.hertz@neu.edu>j.hertz@neu.edu</a>"
                            "<p>Project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>"));
    helpContents.append(helpFile.readAll());
    helpFile.close();
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(600,600);
    helpWindow.exec();
}


void gruepr::on_AboutButton_clicked()
{
    QString user = registeredUser.isEmpty()? tr("UNREGISTERED") : (tr("registered to ") + registeredUser);
    QMessageBox::about(this, tr("About gruepr"),
                       tr("<h2>gruepr " GRUEPR_VERSION_NUMBER "</h2>"
                          "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                          "<br>Joshua Hertz<br><a href = mailto:j.hertz@neu.edu>j.hertz@neu.edu</a>"
                          "<p>This copy of gruepr is ") + user + tr("."
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>."
                          "<p>gruepr incorporates:"
                              "<ul><li>Code from <a href = http://qt.io>Qt, v 5.12.1</a>, released under the  GNU Lesser General Public License version 3;</li>"
                              "<li>Icons from <a href = https://icons8.com>Icons8</a>, released under Creative Commons license \"Attribution-NoDerivs 3.0 Unported\"; and</li>"
                              "<li>The font <a href = https://www.fontsquirrel.com/fonts/oxygen-mono>Oxygen Mono</a>, Copyright &copy; 2012, Vernon Adams (vern@newtypography.co.uk), released under SIL OPEN FONT LICENSE V1.1.</li></ul>"
                          "<h3>Disclaimer</h3>"
                          "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
                          "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                          "<p>This program is free software: you can redistribute it and/or modify it under the terms of the <a href = https://www.gnu.org/licenses/gpl.html>"
                          "GNU General Public License</a> as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."));
}


void gruepr::on_registerButton_clicked()
{
    //make sure we can connect to google
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QEventLoop loop;
    QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
    connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(!(networkReply->bytesAvailable()))
    {
        //no internet right now
        QMessageBox::critical(this, tr("No Internet Connection"), tr("There does not seem to be an internet connection.\nPlease register at another time."));
        delete networkReply;
        delete manager;
        return;
    }
    else
    {
        //we can connect, so gather name, institution, and email address for submission
        registerDialog *window = new registerDialog(this);
        int reply = window->exec();
        //If user clicks OK, email registration info and add to saved settings
        if(reply == QDialog::Accepted)
        {
            if(QDesktopServices::openUrl(QUrl(USER_REGISTRATION_URL
                                              "?name="+window->name->text()+
                                              "&institution="+window->institution->text()+
                                              "&email="+window->email->text())))
            {
                registeredUser = window->name->text();
                QSettings savedSettings;
                savedSettings.setValue("registeredUser", registeredUser);
                savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()),QCryptographicHash::Md5).toHex()));
                ui->registerButton->hide();
                ui->statusBar->setStyleSheet("");
                if(ui->statusBar->currentMessage()==tr("This copy of gruepr is unregistered"))
                {
                    ui->statusBar->showMessage(tr("This copy of gruepr is registered to ") + registeredUser);
                }
            }
            else
            {
                QMessageBox::critical(this, tr("No Connection"),
                                      tr("There seems to be a problem with submitting your registration.\nPlease try again at another time or contact <j.hertz@neu.edu>."));
            }
        }
        delete networkReply;
        delete manager;
        delete window;
    }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////
// Set the "official" team sizes using an array of different sizes or a single, constant size
//////////////////
void gruepr::setTeamSizes(int teamSizes[])
{
    for(int team = 0; team < numTeams; team++)	// run through every team
    {
        teamSize[team] = teamSizes[team];
    }
}
void gruepr::setTeamSizes(int singleSize)
{
    for(int team = 0; team < numTeams; team++)	// run through every team
    {
        teamSize[team] = singleSize;
    }
}

//////////////////
// Read the survey datafile, setting the options and loading all of the student records, returning false if file is invalid
//////////////////
bool gruepr::loadSurveyData(QString fileName)
{
    QFile inputFile(fileName);
    inputFile.open(QIODevice::ReadOnly);
    QTextStream in(&inputFile);

    // Read the header row to determine what data is included
    QStringList fields = ReadCSVLine(in.readLine());
    if(fields.empty())
    {
        inputFile.close();
        return false;
    }

    // Read the optional gender/URM/attribute questions
    int fieldnum = 4;     // skipping past first fields: timestamp(0), first name(1), last name(2), email address(3)
    QString field = fields.at(fieldnum).toLocal8Bit().constData();
    // See if gender data is included
    if(field.contains("gender", Qt::CaseInsensitive))
    {
        dataOptions.genderIncluded = true;
        fieldnum++;
        field = fields.at(fieldnum).toLocal8Bit().constData();				// move on to next field
    }
    else
    {
        dataOptions.genderIncluded = false;
    }

    // UNIMPLEMENTED: see if URM data is included
    dataOptions.URMIncluded = false;

    // Count the number of attributes by counting number of questions from here until one includes "Check the times". Save attribute question texts, if any, into string list.
    dataOptions.numAttributes = 0;                                          // how many skill/attitude rankings are there?
    while(!field.contains("check the times", Qt::CaseInsensitive))
    {
        dataOptions.attributeQuestionText[dataOptions.numAttributes] = field;
        dataOptions.numAttributes++;
        fieldnum++;
        field = fields.at(fieldnum).toLocal8Bit().constData();				// move on to next field
    }

    //Current field should be the first schedule question, so get past the last schedule question to any remaining questions
    fieldnum += 7;
    if(fields.size() > fieldnum)                                            // There is at least 1 additional field in header
    {
        field = fields.at(fieldnum).toLocal8Bit().constData();
        if(field.contains("section", Qt::CaseInsensitive))					// next field is a section question
        {
            fieldnum++;
            if(fields.size() > fieldnum)                                    // if there are any more fields after section
            {
                dataOptions.sectionIncluded = true;
                dataOptions.notesIncluded = true;
            }
            else
            {
                dataOptions.sectionIncluded = true;
                dataOptions.notesIncluded = false;
            }
        }
        else
        {
            dataOptions.sectionIncluded = false;
            dataOptions.notesIncluded = true;
        }
    }
    else
    {
        dataOptions.notesIncluded = false;
        dataOptions.sectionIncluded = false;
    }

    // Having read the header row, read each remaining row as a student record
    numStudents = 0;    // counter for the number of records in the file; used to set the number of students to be teamed for the rest of the program
    fields = ReadCSVLine(in.readLine());
    if(fields.empty())
    {
        inputFile.close();
        return false;
    }
    while(!fields.empty())
    {
        student[numStudents] = readOneRecordFromFile(fields);
        student[numStudents].ID = numStudents;
        numStudents++;
        fields = ReadCSVLine(in.readLine());
    }
    dataOptions.numStudentsInSystem = numStudents;

    inputFile.close();
    return true;
}


//////////////////
// Read one student's info from the survey datafile
//////////////////
studentRecord gruepr::readOneRecordFromFile(QStringList fields)
{
    studentRecord student;

    // Append empty final field if needed (ReadCSVLine function trims off an empty last field)
    if(fields.size() < (4+dataOptions.numAttributes+7+((dataOptions.genderIncluded)? 1 : 0)+((dataOptions.sectionIncluded)? 1 : 0)+((dataOptions.notesIncluded)? 1 : 0)))
    {
        fields.append(" ");
    }

    // first 4 fields: timestamp, first or preferred name, last name, email address
    int fieldnum = 0;
    student.surveyTimestamp = QDateTime::fromString(fields.at(fieldnum).left(fields.at(fieldnum).size()-4), TIMESTAMP_FORMAT1);
    if(student.surveyTimestamp.isNull())
    {
        student.surveyTimestamp = QDateTime::fromString(fields.at(fieldnum).left(fields.at(fieldnum).size()-4), TIMESTAMP_FORMAT2);
    }

    fieldnum++;
    student.firstname = fields.at(fieldnum).toLocal8Bit().trimmed().constData();
    student.firstname[0] = student.firstname[0].toUpper();

    fieldnum++;
    student.lastname = fields.at(fieldnum).toLocal8Bit().trimmed().constData();
    student.lastname[0] = student.lastname[0].toUpper();

    fieldnum++;
    student.email = fields.at(fieldnum).toLocal8Bit().trimmed().constData();

    // optional 5th field in line; might be the gender
    fieldnum++;
    if(dataOptions.genderIncluded)
    {
        QString field = fields.at(fieldnum).toLocal8Bit().constData();
        if(field.contains(tr("woman"), Qt::CaseInsensitive))
        {
            student.gender = studentRecord::woman;
        }
        else if(field.contains(tr("man"), Qt::CaseInsensitive))
        {
            student.gender = studentRecord::man;
        }
        else
        {
            student.gender = studentRecord::neither;
        }
        fieldnum++;
    }
    else
    {
        student.gender = studentRecord::neither;
    }

    // UNIMPLEMENTED: optional next field in line; might be underrpresented minority status?
    if(dataOptions.URMIncluded)
    {

    }
    else
    {
        student.URM = false;
    }

    // optional next 9 fields in line; might be the attributes
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
    {
        QString field = fields.at(fieldnum).toLocal8Bit().constData();
        int chrctr = 0;
        // search through this field character by character until we find a numeric digit (or reach the end)
        while((field[chrctr].digitValue() < 0 || field[chrctr].digitValue() > 9) && (chrctr < field.size()))
        {
            chrctr++;
        }
        if(field[chrctr].digitValue() >= 0 && field[chrctr].digitValue() <= 9)
        {
            student.attribute[attribute] = field[chrctr].digitValue();
            // attribute scores all start at 1, and this allows us to auto-calibrate the max value for each question
            if(student.attribute[attribute] > dataOptions.attributeLevels[attribute])
            {
                dataOptions.attributeLevels[attribute] = student.attribute[attribute];
            }
        }
        fieldnum++;
    }

    // next 7 fields; should be the schedule
    for(int day = 0; day < 7; day++)
    {
        QString field = fields.at(fieldnum).toLocal8Bit().constData();
        for(int time = 0; time < dailyTimeBlocks; time++)
        {
            student.unavailable[(day*dailyTimeBlocks)+time] = field.contains(timeNames[time], Qt::CaseInsensitive);
        }
        fieldnum++;
    }
    student.availabilityChart = tr("Availability:\n           ");
    for(int day = 0; day < 7; day++)
    {
        student.availabilityChart += "\t" + dayNames[day];
    }
    for(int time = 0; time < dailyTimeBlocks; time++)
    {
        student.availabilityChart += "\n " + timeNames[time];
        if(timeNames[time].size() < 6)
        {
            student.availabilityChart += QString((6-timeNames[time].size()), ' ');
        }
        for(int day = 0; day < 7; day++)
        {
            student.availabilityChart += "\t  " + QString(student.unavailable[(day*dailyTimeBlocks)+time]? " " : "√") + "  ";
        }
    }

    // optional last fields; might be section and/or additional notes
    if(dataOptions.sectionIncluded)
    {
        student.section = fields.at(fieldnum).toLocal8Bit().trimmed().constData();
        if(student.section.startsWith("section",Qt::CaseInsensitive))
        {
            student.section = student.section.right(student.section.size()-7).trimmed();    //removing as redundant the word "section" if at the start of the section name
        }
        fieldnum++;
    }
    if(dataOptions.notesIncluded)
    {
        student.notes = fields.at(fieldnum).toLocal8Bit().simplified().constData();     //.simplified() removes leading and trailing whitespace and converts all internal whitespaces to a single space each
    }

    return student;
}


//////////////////
// Read one line from a CSV file, smartly handling commas within fields that are enclosed by quotation marks
//////////////////
QStringList gruepr::ReadCSVLine(QString line)
{
    enum State {Normal, Quote} state = Normal;
    QStringList fields;
    QString value;

    for(int i = 0; i < line.size(); i++)
    {
        QChar current=line.at(i);

        // Normal state
        if (state == Normal)
        {
            // Comma
            if (current == ',')
            {
                // Save field
                fields.append(value.trimmed());
                value.clear();
            }

            // Double-quote
            else if (current == '"')
            {
                state = Quote;
                value += current;
            }

            // Other character
            else
                value += current;
        }

        // In-quote state
        else if (state == Quote)
        {
            // Another double-quote
            if (current == '"')
            {
                if (i < line.size())
                {
                    // A double double-quote?
                    if (i+1 < line.size() && line.at(i+1) == '"')
                    {
                        value += '"';

                        // Skip a second quote character in a row
                        i++;
                    }
                    else
                    {
                        state = Normal;
                        value += '"';
                    }
                }
            }

            // Other character
            else
                value += current;
        }
    }
    if (!value.isEmpty())
    {
        fields.append(value.trimmed());
    }

    // Quotes are left in until here; so when fields are trimmed, only whitespace outside of
    // quotes is removed.  The quotes are removed here.
    for (int i=0; i<fields.size(); ++i)
    {
        if (fields[i].length()>=1 && fields[i].left(1)=='"')
        {
            fields[i]=fields[i].mid(1);
            if (fields[i].length()>=1 && fields[i].right(1)=='"')
            {
                fields[i]=fields[i].left(fields[i].length()-1);
            }
        }
    }

    return fields;
}


//////////////////
// Update current student info in table
//////////////////
void gruepr::refreshStudentDisplay()
{
    ui->tabWidget->setCurrentIndex(0);
    ui->studentTable->clearContents();
    ui->studentTable->setSortingEnabled(false); // have to disable sorting temporarily while adding items
    ui->studentTable->setColumnCount(dataOptions.sectionIncluded? 5 : 4);
    ui->studentTable->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Survey\nSubmission\nTime")));
    ui->studentTable->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("First Name")));
    ui->studentTable->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Last Name")));
    int i = 3;
    if(dataOptions.sectionIncluded)
    {
        ui->studentTable->setHorizontalHeaderItem(i, new QTableWidgetItem(tr("Section")));
        i++;
    }
    ui->studentTable->setHorizontalHeaderItem(i, new QTableWidgetItem(tr("Remove\nStudent")));

    ui->studentTable->setRowCount(dataOptions.numStudentsInSystem);

    numStudents = 0;
    for(int i = 0; i < dataOptions.numStudentsInSystem; i++)
    {
        if((ui->sectionSelectionBox->currentIndex() == 0) || (student[i].section == ui->sectionSelectionBox->currentText()))
        {
            QString studentToolTip;
            studentToolTip = student[i].firstname + " " + student[i].lastname + "  <" + student[i].email + ">";
            if(dataOptions.genderIncluded)
            {
                studentToolTip += "\n" + tr("Gender:  ");
                if(student[i].gender == studentRecord::woman)
                {
                    studentToolTip += tr("woman");
                }
                else if(student[i].gender == studentRecord::man)
                {
                    studentToolTip += tr("man");
                }
                else
                {
                    studentToolTip += tr("nonbinary/unknown");
                }
            }
            for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
            {
                studentToolTip += "\n" + tr("Attribute ") + QString::number(attribute + 1) + ":  ";
                if(student[i].attribute[attribute] != -1)
                {
                    studentToolTip += QString::number(student[i].attribute[attribute]);
                }
                else
                {
                    studentToolTip += "x";
                }
            }
            studentToolTip += "\n" + student[i].availabilityChart;

            TimestampTableWidgetItem *timestamp = new TimestampTableWidgetItem(student[i].surveyTimestamp.toString("d-MMM. h:mm AP"));
            timestamp->setToolTip(studentToolTip);
            ui->studentTable->setItem(numStudents, 0, timestamp);

            QTableWidgetItem *firstName = new QTableWidgetItem(student[i].firstname);
            firstName->setToolTip(studentToolTip);
            ui->studentTable->setItem(numStudents, 1, firstName);

            QTableWidgetItem *lastName = new QTableWidgetItem(student[i].lastname);
            lastName->setToolTip(studentToolTip);
            ui->studentTable->setItem(numStudents, 2, lastName);

            int j = 3;
            if(dataOptions.sectionIncluded)
            {
                QTableWidgetItem *section = new QTableWidgetItem(student[i].section);
                section->setToolTip(studentToolTip);
                ui->studentTable->setItem(numStudents, j, section);
                j++;
            }

            QPushButton *remover = new QPushButton(QIcon(":/icons/delete.png") , "", this);
            remover->setFlat(true);
            remover->setIconSize(QSize(20,20));
            remover->setToolTip(tr("Remove this record from the current dataset"));
            remover->setProperty("StudentID", student[i].ID);
            connect(remover, &QPushButton::clicked, this, &gruepr::removeAStudent);
            ui->studentTable->setCellWidget(numStudents, j, remover);

            numStudents++;
        }
    }
    ui->studentTable->setRowCount(numStudents);

    QString sectiontext = (ui->sectionSelectionBox->currentIndex() == 0? "All sections" : " Section: " + sectionName);
    ui->statusBar->showMessage(ui->statusBar->currentMessage().split("->")[0].trimmed() + "  -> " + sectiontext + "  [" + QString::number(numStudents) + " students]");

    ui->studentTable->resizeColumnsToContents();
    ui->studentTable->setSortingEnabled(true);
}


////////////////////////////////////////////
// Create and optimize teams using genetic algorithm
////////////////////////////////////////////
QList<int> gruepr::optimizeTeams(int *studentIDs)
{
    // seed the pRNG (need to specifically do it here because this is happening in a new thread)
    srand(unsigned(time(nullptr)));

    // Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
    // Each genome in this generation stores (by permutation) which students are in which team.
    // Array has one entry per student and lists, in order, the "ID number" of the
    // student, referring to the order of the student in the students[] array.
    // For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
    // students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

    // allocate memory for genepool
    int** genePool = new int*[populationSize];
    for(int i = 0; i < populationSize; ++i)
        genePool[i] = new int[numStudents];

    //start with randPerm as just the array of student.IDs
    int randPerm[maxStudents];
    for(int i = 0; i < numStudents; i++)
    {
        randPerm[i] = studentIDs[i];
    }

    //then make "populationSize" number of random permutations for initial population, store in genePool
    for(int i = 0; i < populationSize; i++)
    {
        std::random_shuffle(randPerm, randPerm+numStudents);
        for(int ID = 0; ID < numStudents; ID++)
        {
            genePool[i][ID] = randPerm[ID];
        }
    }

    // allocate memory for temporary genepool to hold each next generation before copying back into genepool
    int** tempGen = new int*[populationSize];
    for(int i = 0; i < populationSize; ++i)
        tempGen[i] = new int[numStudents];

    //now optimize
    double teamScores[maxStudents];
    int temp[maxStudents];
    double scores[populationSize], tempScores[populationSize];					// total score for each genome in the gene pool
    int indexOfBestTeamset;                         // holds index of best teamset in the genome
    double bestScores[generationsOfStability]={0};	// historical record of best score in the genome, going back generationsOfStability generations
    int generation = 0;
    int extraGenerations = 0;		// keeps track of "extra generations" to include in generation number displayed, used when user has chosen to continue optimizing further
    double scoreStability;
    bool localOptimizationStopped = false;
    do								// allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
    {
        do							// keep optimizing until reach maxGenerations or stable
        {
            tourneyPlayer players[tournamentSize];
            int tourneyPicks[tournamentSize];

            //calculate all of this generation's scores
            for(int i = 0; i < populationSize; i++)
            {
                scores[i] = getTeamScores(&genePool[i][0], teamScores);
            }

            //find the elites (best scores) in genePool and copy each to tempGen
            //store and use a temporary scores array so we can manipulate
            for(int i = 0; i < populationSize; i++)
            {
                tempScores[i] = scores[i];
            }
            for(int i = 0; i < numElites; i++)
            {
                indexOfBestTeamset = static_cast<int>(std::distance(tempScores, std::max_element(tempScores, tempScores+populationSize)));
                for(int ID = 0; ID < numStudents; ID++)
                {
                    tempGen[i][ID] = genePool[indexOfBestTeamset][ID];
                }
                // set this tempScores value to the minimum one, so we can find the next biggest one during the next time through the loop
                tempScores[indexOfBestTeamset] = *std::min_element(tempScores, tempScores+populationSize);
            }

            //create populationSize-numElites children and place in tempGen
            for(int i = numElites; i < populationSize; i++)
            {
                //get tournamentSize random values from 0 -> populationSize and copy those index-valued genePool genomes and scores into players[]
                for(int j = 0; j < tournamentSize; j++)
                {
                    tourneyPicks[j] = rand()%populationSize;
                    for(int ID = 0; ID < numStudents; ID++)
                    {
                        players[j].genome[ID] = genePool[tourneyPicks[j]][ID];
                    }
                    players[j].score = scores[tourneyPicks[j]];
                }

                //sort tournament genomes so top genomes in tournament are at the beginning
                std::sort(players, players+tournamentSize, [](tourneyPlayer i,tourneyPlayer j){return i.score>j.score;});

                //pick two genomes from tournament, most likely from the beginning so that best genomes are more likely have offspring
                int parent[2], choice = 0, play = 0;
                while(choice < 2)
                {
                    //choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on; 2nd parent then chosen from remaining (lower) players in tournament
                    if(rand() < topGenomeLikelihood)
                    {
                        parent[choice] = play%tournamentSize;       // using play%tournamentSize to wrap around from end of tournament back to the beginning, just in case
                        choice++;
                    }
                    play++;
                }

                //mate top two genomes and put child in tempGen
                GA::mate(players[parent[0]].genome, players[parent[1]].genome, teamSize, numTeams, temp, numStudents);
                for(int ID = 0; ID < numStudents; ID++)
                {
                    tempGen[i][ID] = temp[ID];
                }
            }

            //mutate genomes in tempGen with some probability--if a mutation occurs, mutate same genome again with same probability
            for(int i = 0; i < populationSize; i++)
            {
                while(rand() < mutationLikelihood)
                {
                    GA::mutate(&tempGen[i][0], numStudents);
                }
            }

            //copy all of tempGen into genePool
            for(int genome = 0; genome < populationSize; genome++)
            {
                for(int ID = 0; ID < numStudents; ID++)
                {
                    genePool[genome][ID] = tempGen[genome][ID];
                }
            }

            generation++;

            //determine best score and save in historical record
            for(int i = 0; i < populationSize; i++)
            {
                scores[i] = getTeamScores(&genePool[i][0], teamScores);
            }
            indexOfBestTeamset = static_cast<int>(std::distance(scores, std::max_element(scores, scores+populationSize)));    //index of largest element in scores[]
            bestScores[generation%generationsOfStability] = scores[indexOfBestTeamset];	//array of the best scores from the last generationsOfStability generations, wrapping around the storage location

            scoreStability = scores[indexOfBestTeamset] / (*std::max_element(bestScores,bestScores+generationsOfStability) - *std::min_element(bestScores,bestScores+generationsOfStability));

            emit generationComplete(scores[indexOfBestTeamset], generation+extraGenerations, scoreStability);

            optimizationStoppedmutex.lock();
            localOptimizationStopped = optimizationStopped;
            optimizationStoppedmutex.unlock();
        }
        while(!localOptimizationStopped && ((generation < minGenerations) || ((generation < maxGenerations) && (scoreStability < 100))));

        if(localOptimizationStopped)
        {
            keepOptimizing = false;
            emit turnOffBusyCursor();
        }
        else
        {
            emit optimizationMightBeComplete(generation);

            // wait for user to enter their choice in the dialogbox
            QEventLoop waitForUserToChoose;
            connect(this, &gruepr::haveOurKeepOptimizingValue, &waitForUserToChoose, &QEventLoop::quit);
            waitForUserToChoose.exec();

            if(keepOptimizing)
            {
                extraGenerations += generation;
                generation = 0;
            }
        }
    }
    while(keepOptimizing);

    finalGeneration = generation + extraGenerations;
    teamSetScore = bestScores[generation%generationsOfStability];

    //copy best team set into a QList to return
    QList<int> bestTeamSet;
    for(int ID = 0; ID < numStudents; ID++)
    {
        bestTeamSet << genePool[indexOfBestTeamset][ID];
    }

    // free memory for genepool
    for(int i = 0; i < populationSize; ++i)
        delete [] genePool[i];
    delete[] genePool;

    // free memory for tempGen
    for(int i = 0; i < populationSize; ++i)
        delete [] tempGen[i];
    delete[] tempGen;

    return bestTeamSet;
}


//////////////////
// Calculate team scores, returning the total score (which is, typically, the harmonic mean of all team scores)
//////////////////
double gruepr::getTeamScores(int teammates[], double teamScores[])
{
    // Normalize attribute and schedule weights such that the sum of all weights = numAttributes + 1 (the +1 is for schedule)
    double realAttributeWeights[maxAttributes];
    double totalWeight = teamingOptions.scheduleWeight + std::accumulate(teamingOptions.attributeWeights, teamingOptions.attributeWeights + dataOptions.numAttributes, 0.0);
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
    {
        realAttributeWeights[attribute] = teamingOptions.attributeWeights[attribute] * (dataOptions.numAttributes + 1) / totalWeight;
    }
    double realScheduleWeight = teamingOptions.scheduleWeight * (dataOptions.numAttributes + 1) / totalWeight;

    // Create and initialize each component score
    double attributeScore[maxAttributes][maxStudents];
    double schedScore[maxStudents];
    int genderAdj[maxStudents];
    int URMAdj[maxStudents];
    double prevTeammateAdj[maxStudents];
    double reqTeammateAdj[maxStudents];
    for(int team = 0; team < numTeams; team++)
    {
        for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
        {
            attributeScore[attribute][team] = 0;
        }
        schedScore[team] = 0;
        genderAdj[team] = 0;
        prevTeammateAdj[team] = 0;
        reqTeammateAdj[team] = 0;
    }
    int ID;

    // Calculate each component score:

    // Calculate attribute scores for each attribute for each team:
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
    {
        if(realAttributeWeights[attribute] > 0)
        {
            ID = 0;
            for(int team = 0; team < numTeams; team++)
            {
                int maxLevel = student[teammates[ID]].attribute[attribute], minLevel = student[teammates[ID]].attribute[attribute];
                for(int teammate = 0; teammate < teamSize[team]; teammate++)
                {
                    if(student[teammates[ID]].attribute[attribute] != -1)       // students added manually have attribute levels of -1, so don't consider their "score"
                    {
                        if(student[teammates[ID]].attribute[attribute] > maxLevel)
                        {
                            maxLevel = student[teammates[ID]].attribute[attribute];
                        }
                        if(student[teammates[ID]].attribute[attribute] < minLevel)
                        {
                            minLevel = student[teammates[ID]].attribute[attribute];
                        }
                    }
                    ID++;
                }
                attributeScore[attribute][team] = (maxLevel - minLevel) / (dataOptions.attributeLevels[attribute] - 1.0);	// range in team's values divided by total possible range
                if(teamingOptions.desireHomogeneous[attribute])	//attribute scores are 0 if homogeneous and +1 if full range of values are in a team, so flip if want homogeneous
                {
                    attributeScore[attribute][team] = 1 - attributeScore[attribute][team];
                }
                attributeScore[attribute][team] *= realAttributeWeights[attribute];
            }
        }
    }

    // Calculate schedule scores for each team:
    if(realScheduleWeight > 0)
    {
        ID = 0;
        for(int team = 0; team < numTeams; team++)
        {
            int firstStudentInTeam = ID;
            // combine each student's schedule array into a team schedule array
            bool teamAvailability[numTimeBlocks];
            for(int time = 0; time < numTimeBlocks; time++)
            {
                ID = firstStudentInTeam;
                teamAvailability[time] = true;
                for(int teammate = 0; teammate < teamSize[team]; teammate++)
                {
                    teamAvailability[time] = teamAvailability[time] && !student[teammates[ID]].unavailable[time];	// logical "and" each student's not-unavailability
                    ID++;
                }
            }
            // count how many free time blocks there are
            if(teamingOptions.meetingBlockSize == 1)
            {
                for(int time = 0; time < numTimeBlocks; time++)
                {
                    if(teamAvailability[time])
                    {
                        schedScore[team]++;
                    }
                }
            }
            else
            {
                for(int day = 0; day < 7; day++)
                {
                    for(int time = 0; time < dailyTimeBlocks-1; time++)
                    {
                        if(teamAvailability[(day*dailyTimeBlocks)+time])
                        {
                            time++;
                            if(teamAvailability[(day*dailyTimeBlocks)+time])
                            {
                                schedScore[team]++;
                            }
                        }
                    }
                }
            }
            // convert counts to a schedule score
            if(schedScore[team] > teamingOptions.desiredTimeBlocksOverlap)			// if team has more than desiredTimeBlocksOverlap, the "extra credit" is 1/4 of the additional overlaps
            {
                schedScore[team] = 1 + ((schedScore[team] - teamingOptions.desiredTimeBlocksOverlap) / (4*teamingOptions.desiredTimeBlocksOverlap));
                schedScore[team] *= realScheduleWeight;
            }
            else if(schedScore[team] >= teamingOptions.minTimeBlocksOverlap)		// if team has between minimum and desired amount of schedule overlap
            {
                schedScore[team] /= teamingOptions.desiredTimeBlocksOverlap;		// normal schedule score is number of overlaps / desired number of overlaps
                schedScore[team] *= realScheduleWeight;
            }
            else													// if team has fewer than minTimeBlocksOverlap, apply penalty
            {
                schedScore[team] = -(dataOptions.numAttributes + 1);
            }
        }
    }

    // Determine adjustments for isolated woman teams
    if(teamingOptions.isolatedWomenPrevented && dataOptions.genderIncluded)
    {
        ID = 0;
        for(int team = 0; team < numTeams; team++)
        {
            int numWomen = 0;
            for(int teammate = 0; teammate < teamSize[team]; teammate++)
            {
                if(student[teammates[ID]].gender == studentRecord::woman)
                {
                    numWomen++;
                }
                ID++;
            }
            if(numWomen == 1)
            {
                genderAdj[team] -= (dataOptions.numAttributes + 1);
            }
        }
    }

    // Determine adjustments for isolated man teams
    if(teamingOptions.isolatedMenPrevented && dataOptions.genderIncluded)
    {
        ID = 0;
        for(int team = 0; team < numTeams; team++)
        {
            int numMen = 0;
            for(int teammate = 0; teammate < teamSize[team]; teammate++)
            {
                if(student[teammates[ID]].gender == studentRecord::man)
                {
                    numMen++;
                }
                ID++;
            }
            if(numMen == 1)
            {
                genderAdj[team] -= (dataOptions.numAttributes + 1);
            }
        }
    }

    // Determine adjustments for single gender teams
    if(teamingOptions.mixedGenderPreferred && dataOptions.genderIncluded)
    {
        ID = 0;
        for(int team = 0; team < numTeams; team++)
        {
            int numMen = 0;
            int numWomen = 0;
            for(int teammate = 0; teammate < teamSize[team]; teammate++)
            {
                if(student[teammates[ID]].gender == studentRecord::man)
                {
                    numMen++;
                }
                else if(student[teammates[ID]].gender == studentRecord::woman)
                {
                    numWomen++;
                }
                ID++;
            }
            if(numMen == 0 || numWomen == 0)
            {
                genderAdj[team] -= (dataOptions.numAttributes + 1);
            }
        }
    }

    // Determine adjustments for isolated URM teams
    if(teamingOptions.isolatedURMPrevented && dataOptions.URMIncluded)
    {
        ID = 0;
        for(int team = 0; team < numTeams; team++)
        {
            int numURM = 0;
            for(int teammate = 0; teammate < teamSize[team]; teammate++)
            {
                if(student[teammates[ID]].URM)
                {
                    numURM++;
                }
                ID++;
            }
            if(numURM == 1)
            {
                URMAdj[team] -= (dataOptions.numAttributes + 1);
            }
        }
    }

    // Determine adjustments for prevented teammates on same team
    int firstStudentInTeam=0;
    // Loop through each team
    for(int team = 0; team < numTeams; team++)
    {
        //loop studentA from first student in team to 2nd-to-last student in team
        for(int studentA = firstStudentInTeam; studentA < (firstStudentInTeam + (teamSize[team]-1)); studentA++)
        {
            //loop studentB from studentA+1 to last student in team
            for(int studentB = (studentA+1); studentB < (firstStudentInTeam + teamSize[team]); studentB++)
            {
                //if pairing prevented, adjustment = -(numAttributes + 1)
                if(student[teammates[studentA]].preventedWith[teammates[studentB]])
                {
                    prevTeammateAdj[team] = -(dataOptions.numAttributes + 1);
                }
            }
        }
        firstStudentInTeam += teamSize[team];
    }

    // Determine adjustments for required teammates NOT on same team
    firstStudentInTeam=0;
    // Loop through each team
    for(int team = 0; team < numTeams; team++)
    {
        //loop through all students in team
        for(int studentA = firstStudentInTeam; studentA < (firstStudentInTeam + teamSize[team]); studentA++)
        {
            //loop through ALL other students
            for(int studentB = 0; studentB < numStudents; studentB++)
            {
                //if this pairing is required and studentB is in a/the section being teamed
                if(student[teammates[studentA]].requiredWith[teammates[studentB]] && (ui->sectionSelectionBox->currentIndex() == 0 || student[teammates[studentB]].section == sectionName))
                {
                    bool studentBOnTeam = false;
                    //loop through all of studentA's current teammates
                    for(int currMates = firstStudentInTeam; currMates < (firstStudentInTeam + teamSize[team]); currMates++)
                    {
                        //if this pairing is found, then the required teammate is on the team!
                        if(teammates[currMates] == teammates[studentB])
                        {
                            studentBOnTeam = true;
                        }
                    }
                    //if the pairing was not found, then adjustment = -(numAttributes + 1)
                    if(!studentBOnTeam)
                    {
                        reqTeammateAdj[team] = -(dataOptions.numAttributes + 1);
                    }
                }
            }
        }
        firstStudentInTeam += teamSize[team];
    }

    //Bring component scores together for final team scores and, ultimately, a net score:

    //final team scores are normalized to be out of 100 (but with possible "extra credit" for more than desiredTimeBlocksOverlap hours w/ 100% team availability)
    for(int team = 0; team < numTeams; team++)
    {
        // remove extra credit if any of the penalties are being applied, so that a very high schedule overlap doesn't cancel out the desired rule
        if(schedScore[team] > 1 && (genderAdj[team] < 0 || URMAdj[team] < 0 || prevTeammateAdj[team] < 0 || reqTeammateAdj[team] < 0))
        {
            schedScore[team] = 1;
        }

        teamScores[team] = schedScore[team] + prevTeammateAdj[team] + reqTeammateAdj[team] + genderAdj[team];
        for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
        {
            teamScores[team] += attributeScore[attribute][team];
        }
        teamScores[team] = 100*teamScores[team] / (dataOptions.numAttributes + 1);
    }

    //Use the harmonic mean for the "total score"
    //This value, the inverse of the average of the inverses, is skewed towards the smaller members so that we optimize for better values of the worse teams
    double harmonicSum = 0;
    for(int team = 0; team < numTeams; team++)
    {
        //very poor teams have 0 or negative scores, and this makes the harmonic mean meaningless
        //if any teamScore is <= 0, return the arithmetic mean punished by reducing towards negative infinity by half the arithmetic mean
        if(teamScores[team] <= 0)
        {
            double mean = std::accumulate(teamScores, teamScores+numTeams, 0.0)/numTeams;		// accumulate() is from <numeric>, and it sums an array
            if(mean < 0)
            {
                return(mean + (mean/2));
            }
            else
            {
                return(mean - (mean/2));
            }
        }
        harmonicSum += 1/teamScores[team];
    }
    return(numTeams/harmonicSum);
}


//////////////////
// Update current team info in tree display as well as the text output options
//////////////////
void gruepr::refreshTeamInfo()
{
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    double teamScores[maxStudents];
    teamSetScore = getTeamScores(bestGenome, teamScores);

    QStringList teamToolTips;

    QString elidedDataFileName = (dataOptions.dataFile.filePath().size()>90)?
                (dataOptions.dataFile.filePath().left(27)+"..."+dataOptions.dataFile.filePath().right(60)) : dataOptions.dataFile.filePath();

    spreadsheetFileContents = tr("Section\tTeam\tName\tEmail\n");
    instructorsFileContents = tr("File: ") + dataOptions.dataFile.filePath() + "\n" + tr("Section: ") + sectionName + "\n" + tr("Optimized over ") +
            QString::number(finalGeneration) + tr(" generations") + "\n" + tr("Net score: ") + QString::number(teamSetScore) + "\n\n\n";
    studentsFileContents = "";

    //loop through every team
    int ID = 0;
    for(int team = 0; team < numTeams; team++)
    {
        QString teamToolTip;
        instructorsFileContents += tr("Team ") + teamNames[team] + tr("  -  Score = ") + QString::number(teamScores[team], 'f', 2) + "\n\n";
        studentsFileContents += tr("Team ") + teamNames[team] + "\n\n";

        //loop through each teammate in the team
        int canMeetAt[7][dailyTimeBlocks]={{0}};
        for(int teammate = 0; teammate < teamSize[team]; teammate++)
        {
            instructorsFileContents += "  ";
            studentsFileContents += "  ";
            if(student[bestGenome[ID]].gender == studentRecord::woman)
            {
                instructorsFileContents += " woman ";
            }
            else if(student[bestGenome[ID]].gender == studentRecord::man)
            {
                instructorsFileContents += "  man  ";
            }
            else
            {
                instructorsFileContents += "   x   ";
            }
            for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
            {
                if(student[bestGenome[ID]].attribute[attribute] != -1)
                {
                    instructorsFileContents += QString::number(student[bestGenome[ID]].attribute[attribute]) + "  ";
                }
                else
                {
                    instructorsFileContents += "x  ";
                }
            }
            int nameSize = (student[bestGenome[ID]].firstname + " " + student[bestGenome[ID]].lastname).size();
            instructorsFileContents += student[bestGenome[ID]].firstname + " " + student[bestGenome[ID]].lastname +
                    QString(std::max(2,30-nameSize), ' ') + student[bestGenome[ID]].email + "\n";
            studentsFileContents += student[bestGenome[ID]].firstname + " " + student[bestGenome[ID]].lastname +
                    QString(std::max(2,30-nameSize), ' ') + student[bestGenome[ID]].email + "\n";
            spreadsheetFileContents += student[bestGenome[ID]].section + "\t" + teamNames[team] + "\t" + student[bestGenome[ID]].firstname +
                    " " + student[bestGenome[ID]].lastname + "\t" + student[bestGenome[ID]].email + "\n";

            for(int day = 0; day < 7; day++)
            {
                for(int time = 0; time < dailyTimeBlocks; time++)
                {
                    if(!student[bestGenome[ID]].unavailable[(day*dailyTimeBlocks)+time])
                    {
                        canMeetAt[day][time]++;
                    }
                }
            }
            ID++;
        }

        teamToolTip = tr("Team availability:") + "\n";
        instructorsFileContents += ("\n" + tr("Availability:") + "\n");
        studentsFileContents += ("\n" + tr("Availability:") + "\n");

        teamToolTip += "              ";
        instructorsFileContents += "            ";
        studentsFileContents += "            ";

        for(int day = 0; day < 7; day++)
        {
            teamToolTip += dayNames[day] + "\t";
            instructorsFileContents += "  " + dayNames[day] + "  ";
            studentsFileContents += "  " + dayNames[day] + "  ";
        }
        teamToolTip += "\n";
        instructorsFileContents += "\n";
        studentsFileContents += "\n";

        for(int time = 0; time < dailyTimeBlocks; time++)
        {
            teamToolTip += timeNames[time] + "\t";
            instructorsFileContents += timeNames[time] + QString((11-timeNames[time].size()), ' ');
            studentsFileContents += timeNames[time] + QString((11-timeNames[time].size()), ' ');
            for(int day = 0; day < 7; day++)
            {
                QString percentage = QString::number((100*canMeetAt[day][time])/teamSize[team]) + "% ";
                teamToolTip += percentage + "\t";
                instructorsFileContents += QString((4+dayNames[day].size())-percentage.size(), ' ') + percentage;
                studentsFileContents += QString((4+dayNames[day].size())-percentage.size(), ' ') + percentage;
            }
            teamToolTip += "\n";
            instructorsFileContents += "\n";
            studentsFileContents += "\n";
        }
        instructorsFileContents += "\n\n";
        studentsFileContents += "\n\n";
        teamToolTips += teamToolTip;
    }

    ui->tabWidget->setCurrentIndex(1);
    ui->teamDataText->setText(tr("  File: ") + elidedDataFileName + tr("\n  Section: ") + sectionName + tr("\n  Optimized over ") +
                              QString::number(finalGeneration) + tr(" generations to a net score of ") + QString::number(teamSetScore, 'f', 2));
    teamDataTree->setColumnCount(1 + (dataOptions.genderIncluded? 1 : 0) + dataOptions.numAttributes + 1);          // name, gender?, each attribute, schedule
    QStringList headerLabels;
    headerLabels << tr("name");
    if(dataOptions.genderIncluded)
    {
        headerLabels << tr("gender");
    }
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
    {
        headerLabels << tr("attribute ") + QString::number(attribute+1);
    }
    headerLabels << tr("available times");
    teamDataTree->setHeaderLabels(headerLabels);
    teamDataTree->setFocus();

    // clear table, but remember which teams are shown expanded and which are collapsed
    bool *expanded = new bool[numTeams];
    for(int team = 0; team < numTeams; team++)
    {
        if(teamDataTree->topLevelItem(team))    //can't get value if it doesn't exist
        {
            expanded[team] = teamDataTree->topLevelItem(team)->isExpanded();
        }
        else
        {
            expanded[team] = false;
        }
    }
    teamDataTree->clear();

    // build a tree map of the teams: key = team number, value = list of studentrecords
    QMap<int, QList<studentRecord>> treeMap;
    ID = 0;
    for(int team = 0; team < numTeams; team++)
    {
        QList<studentRecord> studentsOnTeam;
        for(int teammate = 0; teammate < teamSize[team]; teammate++)
        {
            studentsOnTeam.append(student[bestGenome[ID]]);
            ID++;
        }
        treeMap.insert(team, studentsOnTeam);
    }

    //iterate through every key and every value in every key to display the tree of teams and students
    QTreeWidgetItem *parentItem;
    QTreeWidgetItem *childItem;
    QMapIterator<int, QList<studentRecord>> i(treeMap);
    int teamnum = 0;
    while (i.hasNext())
    {
        //create team items
        i.next();
        parentItem = new QTreeWidgetItem(teamDataTree);
        parentItem->setText(0, tr("Team ") + teamNames[i.key()] + tr("  -  Score = ") + QString::number(teamScores[i.key()], 'f', 2));
        parentItem->setData(0, Qt::UserRole, teamnum);
        parentItem->setToolTip(0, teamToolTips.at(teamnum));
        parentItem->setExpanded(true);
        teamnum++;

        //create student items in each team
        foreach (const auto& studentOnThisTeam, i.value())
        {
            childItem = new QTreeWidgetItem;
            int j = 0;
            childItem->setText(j, studentOnThisTeam.firstname + " " + studentOnThisTeam.lastname);
            childItem->setData(j,Qt::UserRole,studentOnThisTeam.ID);
            childItem->setToolTip(j, studentOnThisTeam.availabilityChart);
            teamDataTree->resizeColumnToContents(j);
            if(dataOptions.genderIncluded)
            {
                j++;
                QString gendr;
                if(studentOnThisTeam.gender == studentRecord::woman)
                {
                    gendr = tr("woman");

                }
                else if(studentOnThisTeam.gender == studentRecord::man)
                {
                    gendr = tr("man");
                }
                else
                {
                    gendr = tr("non-binary/unknown");
                }
                childItem->setText(j, gendr);
                childItem->setToolTip(j, studentOnThisTeam.availabilityChart);
                teamDataTree->resizeColumnToContents(j);
            }
            for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
            {
                j++;
                QString att;
                if(studentOnThisTeam.attribute[attribute] != -1)
                {
                    att += QString::number(studentOnThisTeam.attribute[attribute]);
                }
                else
                {
                    att += "X";
                }
                childItem->setText(j, att);
                childItem->setToolTip(j, studentOnThisTeam.availabilityChart);
                teamDataTree->resizeColumnToContents(j);
            }
            j++;
            childItem->setText(j, QString::number(studentOnThisTeam.availabilityChart.count("√")));

            parentItem->addChild(childItem);
        }
    }

    // restore expanded/collapsed teams
    for(int team = 0; team < numTeams; team++)
    {
         teamDataTree->topLevelItem(team)->setExpanded(expanded[team]);
    }
    delete [] expanded;

    QApplication::restoreOverrideCursor();
}


//////////////////
//Setup printer and then print paginated file(s) in boxes
//////////////////
void gruepr::printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF)
{
    // connecting to the printer is spun off into a separate thread because sometimes it causes ~30 second hang
    // message box explains what's happening
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setText(printToPDF? tr("Setting up PDF writer...") : tr("Connecting to printer..."));
    msgBox->setStandardButtons(nullptr);        // no buttons
    msgBox->setModal(false);
    msgBox->show();
    QEventLoop loop;
    connect(this, &gruepr::connectedToPrinter, &loop, &QEventLoop::quit);
    QPrinter *printer = nullptr;
    QFuture<QPrinter*> future = QtConcurrent::run(this, &gruepr::setupPrinter);
    loop.exec();
    printer = future.result();
    msgBox->close();
    msgBox->deleteLater();
    QApplication::restoreOverrideCursor();

    bool doIt;
    QString baseFileName;
    if(printToPDF)
    {
        printer->setOutputFormat(QPrinter::PdfFormat);
        baseFileName = QFileDialog::getSaveFileName(this, tr("Choose a location and base filename"), "", tr("PDF File (*.pdf);;All Files (*)"));
        doIt = !(baseFileName.isEmpty());
    }
    else
    {
        printer->setOutputFormat(QPrinter::NativeFormat);
        QPrintDialog printDialog(printer);
        printDialog.setWindowTitle(tr("Print"));
        doIt = (printDialog.exec() == QDialog::Accepted);
    }

    if(doIt)
    {
        QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");
        QFont printFont = QFont("Oxygen Mono", 10, QFont::Normal);

        if(printInstructorsFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_instructor." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            printOneFile(instructorsFileContents, "\n\n\n", printFont, printer);
        }
        if(printStudentsFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_student." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            printOneFile(studentsFileContents, "\n\n\n", printFont, printer);

        }
        if(printSpreadsheetFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName() + "_spreadsheet." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            QTextDocument textDocument(spreadsheetFileContents, this);
            printFont.setPointSize(9);
            textDocument.setDefaultFont(printFont);
            printer->setOrientation(QPrinter::Landscape);
            textDocument.print(printer);
        }
    }
    delete printer;
}

QPrinter* gruepr::setupPrinter()
{
    QPrinter *printer = new QPrinter(QPrinter::HighResolution);
    printer->setOrientation(QPrinter::Portrait);
    emit connectedToPrinter();
    return printer;
}

void gruepr::printOneFile(QString file, QString delimiter, QFont &font, QPrinter* printer)
{
    QPainter painter(printer);
    painter.setFont(font);
    QFont titleFont = font;
    titleFont.setBold(true);
    int LargeGap = printer->logicalDpiY()/2, MediumGap = LargeGap/2, SmallGap = MediumGap/2;
    int pageHeight = painter.window().height() - 2*LargeGap;

    QStringList eachTeam = file.split(delimiter, QString::SkipEmptyParts);

    //paginate the output
    QStringList currentPage;
    QList<QStringList> pages;
    int y = 0;
    QStringList::const_iterator it = eachTeam.begin();
    while (it != eachTeam.end())
    {
        //calculate height on page of this team text
        int textWidth = painter.window().width() - 2*LargeGap - 2*SmallGap;
        int maxHeight = painter.window().height();
        QRect textRect = painter.boundingRect(0, 0, textWidth, maxHeight, Qt::TextWordWrap, *it);
        int height = textRect.height() + 2*SmallGap;
        if (y + height > pageHeight && !currentPage.empty())
        {
            pages.push_back(currentPage);
            currentPage.clear();
            y = 0;
        }
        currentPage.push_back(*it);
        y += height + MediumGap;
        ++it;
    }
    if (!currentPage.empty())
    {
        pages.push_back(currentPage);
    }

    //print each page, 1 at a time
    for (int pagenum = 0; pagenum < pages.size(); pagenum++)
    {
        if (pagenum > 0)
        {
            printer->newPage();
        }
        QTransform savedTransform = painter.worldTransform();
        painter.translate(0, LargeGap);
        QStringList::const_iterator it = pages[pagenum].begin();
        while (it != pages[pagenum].end())
        {
            QString title = it->left(it->indexOf('\n')) + " ";
            QString body = it->right(it->size() - (it->indexOf('\n')+1));
            int boxWidth = painter.window().width() - 2*LargeGap;
            int textWidth = boxWidth - 2*SmallGap;
            int maxHeight = painter.window().height();
            QRect titleRect = painter.boundingRect(LargeGap+SmallGap, SmallGap, textWidth, maxHeight, Qt::TextWordWrap, title);
            QRect bodyRect = painter.boundingRect(LargeGap+SmallGap, SmallGap+titleRect.height(), textWidth, maxHeight, Qt::TextWordWrap, body);
            int boxHeight = titleRect.height() + bodyRect.height() + 2 * SmallGap;
            painter.setPen(QPen(Qt::black, 2, Qt::SolidLine));
            painter.setBrush(Qt::white);
            painter.drawRect(LargeGap, 0, boxWidth, boxHeight);
            painter.setFont(titleFont);
            painter.drawText(titleRect, Qt::TextWordWrap, title);
            painter.setFont(font);
            painter.drawText(bodyRect, Qt::TextWordWrap, body);
            painter.translate(0, boxHeight);
            painter.translate(0, MediumGap);
            ++it;
        }
        painter.setWorldTransform(savedTransform);
        painter.drawText(painter.window(), Qt::AlignHCenter | Qt::AlignBottom, QString::number(pagenum + 1));
    }
}


//////////////////
// Before closing the main application window, see if we want to save the current settings as defaults
//////////////////
void gruepr::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    bool dontActuallyExit = false;

    if(savedSettings.value("askToSaveDefaultsOnExit",true).toBool() && ui->saveSettingsButton->isEnabled())
    {
        QApplication::beep();
        QMessageBox saveOptionsOnClose(this);
        saveOptionsOnClose.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        QCheckBox neverShowAgain(tr("Don't ask me this again."), &saveOptionsOnClose);

        saveOptionsOnClose.setIcon(QMessageBox::Question);
        saveOptionsOnClose.setWindowTitle(tr("Save Options?"));
        saveOptionsOnClose.setText(tr("Before exiting, should we save all of the current options as defaults?"));
        saveOptionsOnClose.setCheckBox(&neverShowAgain);
        saveOptionsOnClose.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        saveOptionsOnClose.setButtonText(QMessageBox::Discard, tr("Don't Save"));

        saveOptionsOnClose.exec();
        if(saveOptionsOnClose.result() == QMessageBox::Save)
        {
            on_saveSettingsButton_clicked();
        }
        else if(saveOptionsOnClose.result() == QMessageBox::Cancel)
        {
            dontActuallyExit = true;
        }

        if(neverShowAgain.checkState() == Qt::Checked)
        {
            savedSettings.setValue("askToSaveDefaultsOnExit", false);
        }
    }

    if(dontActuallyExit)
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}
