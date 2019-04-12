#include "ui_gruepr.h"
#include "gruepr.h"
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QTextBrowser>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPrinter>
#include <QPrintDialog>


gruepr::gruepr(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::gruepr)
{
    //Setup the main window
    ui->setupUi(this);
    ui->statusBar->setSizeGripEnabled(false);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    setWindowIcon(QIcon(":/icons/gruepr.png"));

    //Remove register button if registered
    QSettings savedSettings;
    registeredUser = savedSettings.value("registeredUser", "").toString();
    if(registeredUser.isEmpty())
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
}

gruepr::~gruepr()
{
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
        ui->addStudentSectionComboBox->setEnabled(false);
        ui->addStudentSectionComboBox->clear();
        ui->addStudentPushButton->setEnabled(false);
        ui->teamData->clear();
        ui->teamData->setEnabled(false);
        ui->tabWidget->setCurrentIndex(0);
        ui->isolatedWomenCheckBox->setEnabled(false);
        ui->isolatedMenCheckBox->setEnabled(false);
        ui->teamSizeBox->clear();
        ui->teamSizeBox->setEnabled(false);
        ui->idealTeamSizeBox->setEnabled(false);
        ui->letsDoItButton->setEnabled(false);
        ui->saveTeamsButton->setEnabled(false);
        ui->printTeamsButton->setEnabled(false);
        ui->adjustTeamsButton->setEnabled(false);

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
            ui->teamData->setEnabled(true);
            ui->teamData->setPlainText("No teams yet created.");
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
            }
            else
            {
                ui->addStudentGenderComboBox->hide();
                ui->addStudentGenderComboBox->setEnabled(false);
                ui->isolatedWomenCheckBox->setEnabled(false);
                ui->isolatedMenCheckBox->setEnabled(false);
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


void gruepr::on_studentTable_cellEntered(int row, int column)
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
        student[dataOptions.numStudentsInSystem].ID = numStudents + 1000;   //flag for added students is an ID > 1000
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
        on_idealTeamSizeBox_valueChanged(ui->idealTeamSizeBox->value());    // load new team sizes in selection box, if necessary
    }
    else
    {
        QMessageBox::warning(this, tr("Cannot add student."), tr("Sorry, we cannot add another student.\nThis version of gruepr does not allow more than ") + QString(maxStudents) + tr("."), QMessageBox::Ok);
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
    gatherTeammatesDialog *window = new gatherTeammatesDialog(gatherTeammatesDialog::required, student, dataOptions.numStudentsInSystem, (ui->sectionSelectionBox->currentIndex()==0)?"":sectionName, this);

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
    gatherTeammatesDialog *window = new gatherTeammatesDialog(gatherTeammatesDialog::prevented, student, dataOptions.numStudentsInSystem, (ui->sectionSelectionBox->currentIndex()==0)?"":sectionName, this);

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
            (teamingOptions.smallerTeamsSizes[student%teamingOptions.smallerTeamsNumTeams])++;                      // add one student to each team (with 1 additional team relative to before) in turn until we run out of students
            smallerTeamsSizeA = teamingOptions.smallerTeamsSizes[student%teamingOptions.smallerTeamsNumTeams];      // the larger of the two (uneven) team sizes
            numSmallerATeams = (student%teamingOptions.smallerTeamsNumTeams)+1;                                     // the number of larger teams
        }
        smallerTeamsSizeB = smallerTeamsSizeA - 1;                  // the smaller of the two (uneven) team sizes

        // And what are the team sizes when desiredTeamSize represents a minimum size?
        teamingOptions.largerTeamsNumTeams = numTeams;
        for(int student = 0; student < numStudents; student++)	// run through every student
        {
            (teamingOptions.largerTeamsSizes[student%teamingOptions.largerTeamsNumTeams])++;                        // add one student to each team in turn until we run out of students
            largerTeamsSizeA = teamingOptions.largerTeamsSizes[student%teamingOptions.largerTeamsNumTeams];         // the larger of the two (uneven) team sizes
            numLargerATeams = (student%teamingOptions.largerTeamsNumTeams)+1;                                       // the number of larger teams
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
    if(ui->sectionSelectionBox->currentIndex() != 0)
    {
        // Move students from desired section to the front of students[] and change numStudents accordingly
        int numStudentsInSection = 0;
        for(int ID = 0; ID < dataOptions.numStudentsInSystem; ID++)
        {
            if(student[ID].section == ui->sectionSelectionBox->currentText())
            {
                std::swap(student[numStudentsInSection], student[ID]);
                numStudentsInSection++;
            }
        }
        numStudents = numStudentsInSection;
    }

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
    ui->teamData->setPlainText("Now creating teams.");
    ui->sectionSelectionBox->setEnabled(false);
    ui->loadSurveyFileButton->setEnabled(false);
    ui->saveTeamsButton->setEnabled(false);
    ui->printTeamsButton->setEnabled(false);
    ui->adjustTeamsButton->setEnabled(false);
    ui->letsDoItButton->setEnabled(false);
    ui->letsDoItButton->hide();
    ui->cancelOptimizationButton->setEnabled(true);
    ui->cancelOptimizationButton->show();

    // Allow a stoppage and set up futureWatcher to grab results
    optimizationStopped = false;
    future = QtConcurrent::run(this, &gruepr::optimizeTeams);       // spin optimization off into a separate thread
    futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
}


void gruepr::updateOptimizationProgress(double score, int generation, double scoreStability)
{
    ui->generationsBox->setValue(generation);
    ui->scoreBox->setValue(score);
    if(generation >= generationsOfStability)
    {
        ui->stabilityProgressBar->setValue((scoreStability<100)?int(scoreStability):100);
    }
    if(generation >= minGenerations)
    {
        ui->stabilityProgressBar->setEnabled(true);
        ui->generationsBox->setStyleSheet("background-color:palegreen");
    }
    else
    {
        ui->stabilityProgressBar->setEnabled(false);
        ui->generationsBox->setStyleSheet("background-color:lightcyan");
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
    questionWindow.setWindowTitle((generation < maxGenerations)?tr("The score seems to be stable."):(tr("We have reached ") + QString::number(maxGenerations) + tr(" generations.")));
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
    ui->sectionSelectionBox->setEnabled(true);
    ui->loadSurveyFileButton->setEnabled(true);
    ui->tabWidget->setCurrentIndex(1);
    ui->saveTeamsButton->setEnabled(true);
    ui->printTeamsButton->setEnabled(true);
    ui->adjustTeamsButton->setEnabled(true);
    ui->letsDoItButton->setEnabled(true);
    ui->letsDoItButton->show();
    ui->cancelOptimizationButton->setEnabled(false);
    ui->cancelOptimizationButton->hide();

    // Unpack the best team set and print teams list on the screen
    QList<int> bestTeamSet = future.result();
    for(int ID = 0; ID < numStudents; ID++)
    {
        bestGenome[ID] = bestTeamSet[ID];
    }
    double teamScores[maxStudents];
    getTeamScores(bestGenome, teamScores);
    printTeams(bestGenome, teamScores, "");
    ui->teamData->setFocus();
}


void gruepr::on_saveTeamsButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Results Files"), "", tr("Text File (*.txt);;All Files (*)"));

    if (!fileName.isEmpty())
    {
        // Unpack the best team set
        QList<int> bestTeamSet = future.result();
        int genome[maxStudents];
        for(int ID = 0; ID < numStudents; ID++)
        {
            genome[ID] = bestTeamSet[ID];
        }

        // Get the scores for each team
        double teamScores[maxStudents];
        getTeamScores(genome, teamScores);

        //Save data to files.
        printTeams(genome, teamScores, fileName);
    }
}


void gruepr::on_printTeamsButton_clicked()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer);
    printDialog.setWindowTitle(tr("Printer"));
    if (printDialog.exec() == QDialog::Accepted)
    {
        ui->teamData->print(&printer);
    }
}


void gruepr::on_adjustTeamsButton_clicked()
{
    //Open specialized dialog box to collect students to swap
    swapTeammatesDialog *window = new swapTeammatesDialog(student, numStudents, this);

    int reply = window->exec();

    //If user clicks OK, swap chosen students
    if(reply == QDialog::Accepted)
    {
        //swap them
        int studentAIndex = static_cast<int>(std::distance(bestGenome, std::find(bestGenome, bestGenome+numStudents, window->studentA->currentIndex()-1)));
        int studentBIndex = static_cast<int>(std::distance(bestGenome, std::find(bestGenome, bestGenome+numStudents, window->studentB->currentIndex()-1)));
        std::swap(bestGenome[studentAIndex], bestGenome[studentBIndex]);
    }

    delete window;

    // Reprint the teams list on the screen
    double teamScores[maxStudents];
    getTeamScores(bestGenome, teamScores);
    printTeams(bestGenome, teamScores, "");
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
    QString user = registeredUser.isEmpty()?tr("UNREGISTERED"):(tr("registered to ") + registeredUser);
    QMessageBox::about(this, tr("About gruepr"),
                       tr("<h2>gruepr " GRUEPR_VERSION_NUMBER "</h2>"
                          "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                          "<br>Joshua Hertz<br><a href = mailto:j.hertz@neu.edu>j.hertz@neu.edu</a>"
                          "<p>This copy of gruepr is ") + user + tr("."
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>."
                          "<p>gruepr incorporates code from the <a href = http://qt.io>open source Qt libraries, v 5.12.1</a>."
                          "<br>The icons were created by (or modified from) the <a href = https://icons8.com>Icons8</a> library."
                          "<h3>Disclaimer</h3>"
                          "<p>This program is free software: you can redistribute it and/or modify it under the terms of the <a href = https://www.gnu.org/licenses/gpl.html>GNU General Public License</a> as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."
                          "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."));
}


void gruepr::on_registerButton_clicked()
{
    //make sure we can connect to google
    QNetworkAccessManager nam;
    QNetworkReply *networkReply = nam.get(QNetworkRequest(QUrl("http://www.google.com")));
    QEventLoop loop;
    connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if(!(networkReply->bytesAvailable()))
    {
        //no internet right now
        QMessageBox::critical(this, tr("No Internet Connection"), tr("You are not connected to the internet.\nPlease register at another time."));
    }
    else
    {
        //we can connect, so gather name, institution, and email address for submission
        registerDialog *window = new registerDialog(this);
        int reply = window->exec();
        //If user clicks OK, email registration info and add to saved settings
        if(reply == QDialog::Accepted)
        {
            QDesktopServices::openUrl(QUrl(USER_REGISTRATION_FORM_URL "/formResponse?usp=pp_url"
                                           "&entry.1817313817="+window->name->text()+
                                           "&entry.1128502893="+window->institution->text()+
                                           "&entry.2127230564="+window->email->text()+
                                           "&submit=Submit"));
            registeredUser = window->name->text();
            QSettings savedSettings;
            savedSettings.setValue("registeredUser", registeredUser);
            ui->registerButton->hide();
            ui->statusBar->setStyleSheet("");
            if(ui->statusBar->currentMessage()==tr("This copy of gruepr is unregistered"))
            {
                ui->statusBar->showMessage(tr("This copy of gruepr is registered to ") + registeredUser);
            }
        }
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
    numStudents = 0;                                                        // counter for the number of records in the file; used to set the number of students to be teamed for the rest of the program
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
    if(fields.size() < (4+dataOptions.numAttributes+7+((dataOptions.genderIncluded)?1:0)+((dataOptions.sectionIncluded)?1:0)+((dataOptions.notesIncluded)?1:0)))
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
        while((field[chrctr].digitValue() < 0 || field[chrctr].digitValue() > 9) && (chrctr < field.size()))	// search through this field character by character until we find a numeric digit (or reach the end)
        {
            chrctr++;
        }
        if(field[chrctr].digitValue() >= 0 && field[chrctr].digitValue() <= 9)
        {
            student.attribute[attribute] = field[chrctr].digitValue();
            if(student.attribute[attribute] > dataOptions.attributeLevels[attribute])					// attribute scores all start at 1, and this allows us to auto-calibrate the max value for each question
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
    ui->studentTable->setColumnCount(dataOptions.sectionIncluded?5:4);
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
        QString studentToolTip;
        if((ui->sectionSelectionBox->currentIndex() == 0) || (student[i].section == ui->sectionSelectionBox->currentText()))
        {
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
            studentToolTip += tr("\nAvailability:\n");
            studentToolTip += "           ";
            for(int day = 0; day < 7; day++)
            {
                studentToolTip += "\t" + dayNames[day];
            }
            for(int time = 0; time < dailyTimeBlocks; time++)
            {
                studentToolTip += "\n " + timeNames[time];
                if(timeNames[time].size() < 6)
                {
                    studentToolTip += QString((6-timeNames[time].size()), ' ');
                }
                for(int day = 0; day < 7; day++)
                {
                    studentToolTip += "\t  " + QString(student[i].unavailable[(day*dailyTimeBlocks)+time]?" ":"√") + "  ";
                }
            }

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

    QString sectiontext = (ui->sectionSelectionBox->currentIndex() == 0?"All sections":" Section: " + sectionName);
    ui->statusBar->showMessage(ui->statusBar->currentMessage().split("->")[0].trimmed() + "  -> " + sectiontext + "  [" + QString::number(numStudents) + " students]");

    ui->studentTable->resizeColumnsToContents();
    ui->studentTable->setSortingEnabled(true);
}


////////////////////////////////////////////
// Create and optimize teams using genetic algorithm
////////////////////////////////////////////
QList<int> gruepr::optimizeTeams()
{
    // seed the pRNG (need to specifically do it here because this is happening in a new thread)
    srand(unsigned(time(nullptr)));

    // allocate memory for genepool
    int** genePool = new int*[populationSize];
    for(int i = 0; i < populationSize; ++i)
        genePool[i] = new int[numStudents];

    // allocate memory for temporary genepool to hold each next generation before copying back into genepool
    int** tempGen = new int*[populationSize];
    for(int i = 0; i < populationSize; ++i)
        tempGen[i] = new int[numStudents];

    int best;
    double teamScores[maxStudents];
    // Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
    // Each genome in this generation stores (by permutation) which students are in which team.
    // Array has one entry per student and lists, in order, the "ID number" of the
    // student, referring to the order of the student in the students[] array.
    // For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
    // students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

    //start with randPerm as just the sorted array {0, 1, 2, 3,..., numStudents}
    int randPerm[maxStudents];
    for(int i = 0; i < numStudents; i++)
    {
        randPerm[i] = i;
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

    //now optimize
    int temp[maxStudents];
    double scores[populationSize], tempScores[populationSize];					// total score for each genome in the gene pool
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
                best = static_cast<int>(std::distance(tempScores, std::max_element(tempScores, tempScores+populationSize)));
                for(int ID = 0; ID < numStudents; ID++)
                {
                    tempGen[i][ID] = genePool[best][ID];
                }
                tempScores[best] = *std::min_element(tempScores, tempScores+populationSize);      // set this tempScores value to the minimum one, so we can find the next biggest one during the next time through the loop
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
                    if(rand() < topGenomeLikelihood)	//choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on; 2nd parent then chosen from remaining (lower) players in tournament
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
            best = static_cast<int>(std::distance(scores, std::max_element(scores, scores+populationSize)));    //index of largest element in scores[]
            bestScores[generation%generationsOfStability] = scores[best];	//array of the best scores from the last generationsOfStability generations, wrapping around the storage location

            scoreStability = scores[best] / (*std::max_element(bestScores,bestScores+generationsOfStability) - *std::min_element(bestScores,bestScores+generationsOfStability));

            emit generationComplete(scores[best], generation+extraGenerations, scoreStability);

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

    finalGeneration = QString::number(generation + extraGenerations);
    finalTeamSetScore = QString::number(bestScores[generation%generationsOfStability]);

    //copy best team set into a QList to return
    QList<int> bestTeamSet;
    for(int ID = 0; ID < numStudents; ID++)
    {
        bestTeamSet << genePool[best][ID];
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
                //if this pairing is required
                if(student[teammates[studentA]].requiredWith[teammates[studentB]])
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

    //final team scores are normalized to be out of 100 (but with "extra credit" for more than desiredTimeBlocksOverlap hours w/ 100% team availability
    for(int team = 0; team < numTeams; team++)
    {
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
// Print teams (to screen or text file), including names, emails, genders, attribute scores, and notes of each student plus table of weekly availability
//////////////////
void gruepr::printTeams(int teammates[], double teamScores[], QString filename)
{
    // create a fileobject and textstream for each of the three files
    QFile instructorsFile(filename);
    QTextStream instructorsFileContents;
    QFile studentsFile(QFileInfo(filename).path() + "/" + QFileInfo(filename).completeBaseName() + "_students." + QFileInfo(filename).suffix());
    QTextStream studentsFileContents;
    QFile teammatesFile(QFileInfo(filename).path() + "/" + QFileInfo(filename).completeBaseName() + "_TEAMMATES." + QFileInfo(filename).suffix());
    QTextStream teammatesFileContents;

    // if writing this to file, open the output files, associate the textstream to each, and add appropriate headers
    if(filename != "")
    {
        instructorsFile.open(QIODevice::WriteOnly | QIODevice::Text);
        instructorsFileContents.setDevice(&instructorsFile);
        studentsFile.open(QIODevice::WriteOnly | QIODevice::Text);
        studentsFileContents.setDevice(&studentsFile);
        teammatesFile.open(QIODevice::WriteOnly | QIODevice::Text);
        teammatesFileContents.setDevice(&teammatesFile);
        teammatesFileContents << tr("Section\tTeam\tName\tEmail\n");
        instructorsFileContents << tr("  File: ") << dataOptions.dataFile.fileName() << tr("\n  Section: ") << sectionName << tr("\n  Optimized over ") << finalGeneration << tr(" generations\n  Net score: ") << finalTeamSetScore << "\n----------------\n";
    }

    // create a string for screen output
    QString screenOutput; //= filename section generation teamsetscore;
    screenOutput = tr("  File: ") + dataOptions.dataFile.fileName() + tr("\n  Section: ") + sectionName + tr("\n  Optimized over ") + finalGeneration + tr(" generations\n  Net score: ") + finalTeamSetScore + "\n----------------\n";

    //loop through every team
    int ID = 0;
    for(int team = 0; team < numTeams; team++)
    {
        int canMeetAt[7][dailyTimeBlocks]={{0}};
        screenOutput += tr("\nTeam ") + QString::number(team+1) + tr("  -  Score = ") + QString::number(teamScores[team]) + "\n\n";
        if(filename != "")
        {
            instructorsFileContents << tr("\nTeam ") << QString::number(team+1) << tr("  -  Score = ") << QString::number(teamScores[team]) << "\n\n";
            studentsFileContents << tr("\nTeam ") << QString::number(team+1) << "\n\n";
        }

        //loop through each teammate in the team
        for(int teammate = 0; teammate < teamSize[team]; teammate++)
        {
            screenOutput += "  ";
            if(filename != "")
            {
                instructorsFileContents << "  ";
                studentsFileContents << "  ";
            }
            if(student[teammates[ID]].gender == studentRecord::woman)
            {
                screenOutput += "  f  ";
                if(filename != "")
                {
                    instructorsFileContents << "  f  ";
                }
            }
            else if(student[teammates[ID]].gender == studentRecord::man)
            {
                screenOutput += "  m  ";
                if(filename != "")
                {
                    instructorsFileContents << "  m  ";
                }
            }
            else
            {
                screenOutput += "  x  ";
                if(filename != "")
                {
                    instructorsFileContents << "  x  ";
                }            }
            for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
            {
                if(student[teammates[ID]].attribute[attribute] != -1)
                {
                    screenOutput += QString::number(student[teammates[ID]].attribute[attribute]) + "  ";
                    if(filename != "")
                    {
                        instructorsFileContents << QString::number(student[teammates[ID]].attribute[attribute]) + "  ";
                    }
                }
                else
                {
                    screenOutput += "X  ";
                    if(filename != "")
                    {
                        instructorsFileContents << "X  ";
                    }
                }
            }
            screenOutput += student[teammates[ID]].firstname + " " + student[teammates[ID]].lastname + "\t" + student[teammates[ID]].email + "\n";
            if(filename != "")
            {
                instructorsFileContents << student[teammates[ID]].firstname << " " << student[teammates[ID]].lastname << "\t\t" << student[teammates[ID]].email << "\n";
                studentsFileContents << student[teammates[ID]].firstname << " " << student[teammates[ID]].lastname << "\t\t" << student[teammates[ID]].email << "\n";
                teammatesFileContents << student[teammates[ID]].section << "\t" << QString::number(team+1) << "\t" << student[teammates[ID]].firstname << " " << student[teammates[ID]].lastname << "\t" << student[teammates[ID]].email << endl;
            }
            for(int day = 0; day < 7; day++)
            {
                for(int time = 0; time < dailyTimeBlocks; time++)
                {
                    if(!student[teammates[ID]].unavailable[(day*dailyTimeBlocks)+time])
                    {
                        canMeetAt[day][time]++;
                    }
                }
            }
            ID++;
        }

        screenOutput += tr("Availability:\n");
        if(filename != "")
        {
            instructorsFileContents << tr("Availability:\n");
            studentsFileContents << tr("Availability:\n");
        }

        screenOutput += "               ";
        if(filename != "")
        {
            instructorsFileContents << "               ";
            studentsFileContents << "               ";
        }
        for(int day = 0; day < 7; day++)
        {
            screenOutput += "  " + dayNames[day] + "  ";
            if(filename != "")
            {
                instructorsFileContents << "  " << dayNames[day] << "  ";
                studentsFileContents << "  " << dayNames[day] << "  ";
            }
        }
        screenOutput += "\n";
        if(filename != "")
        {
            instructorsFileContents << "\n";
            studentsFileContents << "\n";
        }
        for(int time = 0; time < dailyTimeBlocks; time++)
        {
            screenOutput += timeNames[time] + QString((14-timeNames[time].size()), ' ');
            if(filename != "")
            {
                instructorsFileContents << timeNames[time] + QString((14-timeNames[time].size()), ' ');
                studentsFileContents << timeNames[time] + QString((14-timeNames[time].size()), ' ');
            }
            for(int day = 0; day < 7; day++)
            {
                QString percentage = QString::number((100*canMeetAt[day][time])/teamSize[team]);
                screenOutput += QString(5-percentage.size(), ' ') + percentage + "% ";
                if(filename != "")
                {
                    instructorsFileContents << QString(5-percentage.size(), ' ') + percentage + "% ";
                    studentsFileContents << QString(5-percentage.size(), ' ') + percentage + "% ";
                }
            }
            screenOutput += "\n";
            if(filename != "")
            {
                instructorsFileContents << "\n";
                studentsFileContents << "\n";
            }
        }
        screenOutput += "\n\n";
        if(filename != "")
        {
            instructorsFileContents << "\n\n";
            studentsFileContents << "\n\n";
        }
    }

    if(filename != "")
    {
        instructorsFile.close();
        studentsFile.close();
        teammatesFile.close();
    }
    else
    {
        ui->teamData->setPlainText(screenOutput);
        ui->tabWidget->setCurrentIndex(1);
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
