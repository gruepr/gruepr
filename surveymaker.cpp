#include "ui_surveymaker.h"
#include "surveymaker.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTextBrowser>
#include <QtNetwork>

SurveyMaker::SurveyMaker(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SurveyMaker)
{
    //Setup the main window
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowIcon(QIcon(":/icons/surveymaker.png"));

    //Setup the main window menu items
    connect(ui->actionOpenSurvey, &QAction::triggered, this, &SurveyMaker::openSurvey);
    connect(ui->actionSaveSurvey, &QAction::triggered, this, &SurveyMaker::saveSurvey);
    connect(ui->actionGoogle_form, &QAction::triggered, this, [this]{auto prevSurveyMethod = generateSurvey;
                                                                     generateSurvey = &SurveyMaker::postGoogleURL;
                                                                     on_makeSurveyButton_clicked();
                                                                     generateSurvey = prevSurveyMethod;});
    connect(ui->actionText_files, &QAction::triggered, this, [this]{auto prevSurveyMethod = generateSurvey;
                                                                     generateSurvey = &SurveyMaker::createFiles;
                                                                     on_makeSurveyButton_clicked();
                                                                     generateSurvey = prevSurveyMethod;});
    ui->actionExit->setMenuRole(QAction::QuitRole);
    connect(ui->actionExit, &QAction::triggered, this, &SurveyMaker::close);
    //ui->actionSettings->setMenuRole(QAction::PreferencesRole);
    //connect(ui->actionSettings, &QAction::triggered, this, &SurveyMaker::settingsWindow);
    connect(ui->actionHelp, &QAction::triggered, this, &SurveyMaker::helpWindow);
    ui->actionAbout->setMenuRole(QAction::AboutRole);
    connect(ui->actionAbout, &QAction::triggered, this, &SurveyMaker::aboutWindow);
    connect(ui->actiongruepr_Homepage, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl("https://bit.ly/grueprFromApp"));});

    noInvalidPunctuation = new QRegularExpressionValidator(QRegularExpression("[^,&<>]*"), this);

    //load in local day names
    ui->day1LineEdit->setText(day1name);
    ui->day2LineEdit->setText(day2name);
    ui->day3LineEdit->setText(day3name);
    ui->day4LineEdit->setText(day4name);
    ui->day5LineEdit->setText(day5name);
    ui->day6LineEdit->setText(day6name);
    ui->day7LineEdit->setText(day7name);

    //Restore window geometry
    QSettings savedSettings;
    restoreGeometry(savedSettings.value("surveyMakerWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

    //Add items to response options combobox
    ui->attributeComboBox->addItem("Choose the response options...");
    ui->attributeComboBox->insertSeparator(1);
    for(int response = 0; response < responseOptions.size(); response++)
    {
        ui->attributeComboBox->addItem(responseOptions.at(response));
        ui->attributeComboBox->setItemData(response + 2, responseOptions.at(response), Qt::ToolTipRole);
    }
    responseOptions.insert(0, "custom options, to be added after creating the form");

    refreshPreview();
}

SurveyMaker::~SurveyMaker()
{
    delete ui;
}

void SurveyMaker::refreshPreview()
{
    int currPos = ui->previewText->verticalScrollBar()->value();

    // generate preview text
    QString preview = "<h2>" + title + "</h2>";
    preview += "<h3>First, some basic information</h3>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your first name (or the name you prefer to be called)?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your last name?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your email address?<br></p>";
    preview += gender?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;With which gender do you identify?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>woman </i><b>|</b><i> man </i><b>|</b><i> non— binary </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += URM?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;How do you identify your race, ethnicity, or cultural heritage?<br></p>"
                : "";
    preview += "<hr>";
    if(numAttributes > 0)
    {
        preview += "<h3>This set of questions is about your past experiences/education or teamwork preferences.</h3>";
        for(int attrib = 0; attrib < numAttributes; attrib++)
        {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;" +
                           (attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : attributeTexts[attrib])
                           + "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                           "<small>options: <b>{ </b><i>" + responseOptions.at(attributeResponses[attrib]).split("/").join("</i><b>|</b><i>") + "</i><b> }</b></small>";
        }
        preview += "<hr>";
    }
    if(schedule)
    {
        preview += "<h3>Please tell us about your weekly schedule.</h3>";
        preview += "<p>Check the times that you are ";
        preview += ((busyOrFree == busy)? "BUSY and will be UNAVAILABLE" : "FREE and will be AVAILABLE");
        preview += " for group work.</p><p>&nbsp;&nbsp;&nbsp;<i>grid of checkboxes:</i></p>";
        preview += "<p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
        for(int time = startTime; time <= endTime; time++)
        {
            preview += QTime(time, 0).toString("hA") + "&nbsp;&nbsp;&nbsp;&nbsp;";
        }
        preview += "</p>";
        for(const auto & dayName : dayNames)
        {
            if(!(dayName.isEmpty()))
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;" + dayName + "</p>";
            }
        }
        preview += "<hr>";
    }
    if(section || additionalQuestions)
    {
        preview += "<h3>Some final questions.</h3>";
        if(section)
        {
            preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;In which section are you enrolled?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                       "<small>options: <b>{ </b><i>";
            for(int sect = 0; sect < sectionNames.size(); sect++)
            {
                if(sect > 0)
                {
                    preview += " </i><b>|</b><i> ";
                }
                preview += sectionNames[sect];
            }
            preview += "</i><b> }</b></small></p>";
        }
        if(additionalQuestions)
        {
            preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Any additional things we should know about you before we form the teams?</p>";
        }
        preview += "<hr>";
    }

    ui->previewText->setHtml(preview);
    ui->previewText->verticalScrollBar()->setValue(currPos);
}

void SurveyMaker::on_makeSurveyButton_clicked()
{
    //make sure we have at least one attribute or the schedule question
     if(((numAttributes == 0) && !schedule))
    {
        QMessageBox::critical(this, tr("Error!"), tr("A gruepr survey must have at least one\nattribute question and/or a schedule question.\nThe survey has NOT been created."));
        return;
    }

    generateSurvey(this);
}

void SurveyMaker::postGoogleURL(SurveyMaker *survey)
{
    //make sure we can connect to google
    auto *manager = new QNetworkAccessManager(survey);
    QEventLoop loop;
    QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
    connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    bool weGotProblems = (networkReply->bytesAvailable() == 0);
    delete manager;

    if(weGotProblems)
    {
        QMessageBox::critical(survey, tr("Error!"), tr("There does not seem to be an internet connection.\nCheck your network connection and try again.\nThe survey has NOT been created."));
        return;
    }

    //generate Google URL
    QString URL = "https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/exec?";
    URL += "title=" + QUrl::toPercentEncoding(survey->ui->surveyTitleLineEdit->text()) + "&";
    URL += "gend=" + QString(survey->gender? "true" : "false") + "&";
    URL += "urm=" + QString(survey->URM? "true" : "false") + "&";
    URL += "numattr=" + QString::number(survey->numAttributes) + "&";
    QString allAttributeTexts;
    for(int attrib = 0; attrib < survey->numAttributes; attrib++)
    {
        if(attrib != 0)
        {
            allAttributeTexts += ",";
        }

        if(!(survey->attributeTexts[attrib].isEmpty()))
        {
            allAttributeTexts += QUrl::toPercentEncoding(survey->attributeTexts[attrib]);
        }
        else
        {
            allAttributeTexts += QUrl::toPercentEncoding(tr("Question ") + QString::number(attrib+1));
        }
    }
    URL += "attrtext=" + allAttributeTexts + "&";
    QString allAttributeResponses;
    for(int attrib = 0; attrib < survey->numAttributes; attrib++)
    {
        if(attrib != 0)
        {
            allAttributeResponses += ",";
        }
        allAttributeResponses += QString::number(survey->attributeResponses[attrib]);
    }
    URL += "attrresps=" + allAttributeResponses + "&";
    URL += "sched=" + QString(survey->schedule? "true" : "false") + "&";
    URL += "busy=" + QString((survey->busyOrFree == busy)? "true" : "false") + "&";
    URL += "start=" + QString::number(survey->startTime) + "&end=" + QString::number(survey->endTime);
    QString allDayNames;
    bool firstDay = true;
    for(const auto & dayName : survey->dayNames)
    {
        if(!(dayName.isEmpty()))
        {
            if(!firstDay)
            {
                allDayNames += ",";
            }
            allDayNames += QUrl::toPercentEncoding(dayName);
            firstDay = false;
        }
    }
    URL += "&days=" + allDayNames + "&";
    URL += "sect=" + QString(survey->section? "true" : "false") + "&";
    QString allSectionNames;
    if(survey->section)
    {
        for(int sect = 0; sect < survey->sectionNames.size(); sect++)
        {
            if(sect != 0)
            {
                allSectionNames += ",";
            }
            allSectionNames += QUrl::toPercentEncoding(survey->sectionNames[sect]);
        }
    }
    URL += "sects=" + allSectionNames + "&";
    URL += "addl=" + QString(survey->additionalQuestions? "true" : "false");
    //qDebug() << URL;

    //upload to Google
    QMessageBox createSurvey;
    createSurvey.setIcon(QMessageBox::Information);
    createSurvey.setWindowTitle(tr("Survey Creation"));
    createSurvey.setText(tr("The next step will open a browser window and connect to Google.\n\n"
                            "  » You may be asked first to log in to Google and/or authorize gruepr to access your Google Drive. "
                            "This authorization is needed so that the Google Form and the results spreadsheet can be created for you.\n\n"
                            "  » The survey, the survey responses, and all data associated with this survey will exist in your Google Drive. "
                            "No data from this survey is stored or sent anywhere else.\n\n"
                            "  » The survey creation process will take 10 - 20 seconds. During this time, the browser window will be blank. "
                            "A screen with additional information will be shown in your browser window as soon as the process is complete."));
    createSurvey.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
    if(createSurvey.exec() == QMessageBox::Ok)
    {
        QDesktopServices::openUrl(QUrl(URL));
        survey->surveyCreated = true;
    }
}

void SurveyMaker::createFiles(SurveyMaker *survey)
{
    //give instructions about how this option works
    QMessageBox createSurvey;
    createSurvey.setIcon(QMessageBox::Information);
    createSurvey.setWindowTitle(tr("Survey Creation"));
    createSurvey.setText(tr("The next step will save two files to your computer:\n\n"
                            "  » A text file that lists the questions you should include in your survey.\n\n"
                            "  » A csv file that gruepr can read after you paste into it the survey data you receive."));
    createSurvey.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
    if(createSurvey.exec() == QMessageBox::Ok)
    {
        //get the filenames and location
        QString fileName = QFileDialog::getSaveFileName(survey, tr("Save File"), survey->saveFileLocation.canonicalFilePath(), tr("text and survey files (*);;All Files (*)"));
        if( !(fileName.isEmpty()) )
        {
            //create the files
            QFile saveFile(fileName + ".txt"), saveFile2(fileName + ".csv");
            if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text) && saveFile2.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                int questionNumber = 0, sectionNumber = 0;
                QString textFileContents = survey->title + "\n\n";
                QString csvFileContents = "Timestamp";

                textFileContents += tr("Your survey (and/or other data sources) should collect the following information to paste into the csv file \"") + fileName + ".csv\":";
                textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Basic Information") + ":";

                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += tr("What is your first name (or the name you prefer to be called)?");
                csvFileContents += ",What is your first name (or the name you prefer to be called)?";

                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += tr("What is your last name?");
                csvFileContents += ",What is your last name?";

                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += tr("What is your email address?");
                csvFileContents += ",What is your email address?";

                if(survey->gender)
                {
                    textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                    textFileContents += tr("With which gender do you identify?");
                    textFileContents += "\n     " + tr("choices: [woman | man | non-binary | prefer not to answer]");
                    csvFileContents += ",With which gender do you identify?";
                }
                if(survey->URM)
                {
                    textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                    textFileContents += tr("How do you identify your race, ethnicity, or cultural heritage?");
                    csvFileContents += ",\"How do you identify your race, ethnicity, or cultural heritage?\"";
                }
                if(survey->numAttributes > 0)
                {
                    textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Attributes") + ":";
                    for(int attrib = 0; attrib < survey->numAttributes; attrib++)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        textFileContents += survey->attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : survey->attributeTexts[attrib];
                        textFileContents += "\n     " + tr("choices") + ": [";
                        QStringList responses = survey->responseOptions.at(survey->attributeResponses[attrib]).split("/");
                        for(int resp = 0; resp < responses.size(); resp++)
                        {
                            if(resp != 0)
                            {
                                textFileContents += " | ";
                            }
                            if( (survey->attributeResponses[attrib] > 0) && (survey->attributeResponses[attrib] < 26) )
                            {
                                textFileContents += QString::number(resp+1) + ". ";
                            }
                            textFileContents += responses.at(resp);
                        }
                        textFileContents += "]";
                        csvFileContents += ",\"" + (survey->attributeTexts[attrib].isEmpty()? "Attribute question " + QString::number(attrib+1) : survey->attributeTexts[attrib]) + "\"";
                    }
                }
                if(survey->schedule)
                {
                    textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Schedule") + ":";
                    textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                    textFileContents += tr("Please tell us about your weekly schedule.") + "\n";
                    textFileContents += "     " + tr("Check the times that you are");
                    textFileContents += ((survey->busyOrFree == busy)? tr(" BUSY and will be UNAVAILABLE ") : tr(" FREE and will be AVAILABLE "));
                    textFileContents += tr("for group work.") + "\n";
                    textFileContents += "                 ";
                    for(int time = survey->startTime; time <= survey->endTime; time++)
                    {
                        textFileContents += QTime(time, 0).toString("hA") + "    ";
                    }
                    textFileContents += "\n";
                    for(const auto & dayName : survey->dayNames)
                    {
                        if(!(dayName.isEmpty()))
                        {
                            textFileContents += "\n      " + dayName + "\n";
                            csvFileContents += ",Check the times that you are";
                            csvFileContents += ((survey->busyOrFree == busy)? " BUSY and will be UNAVAILABLE " : " FREE and will be AVAILABLE ");
                            csvFileContents += "for group work. [" + dayName + "]";
                        }
                    }
                }
                if(survey->section || survey->additionalQuestions)
                {
                    textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Additional Information") + ":";
                    if(survey->section)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        textFileContents += tr("In which section are you enrolled?");
                        textFileContents += "\n     " + tr("choices") + ": [";
                        for(int sect = 0; sect < survey->sectionNames.size(); sect++)
                        {
                            if(sect > 0)
                            {
                                textFileContents += " | ";
                            }

                            textFileContents += survey->sectionNames[sect];
                        }
                        textFileContents += " ]";
                        csvFileContents += ",In which section are you enrolled?";
                    }
                    if(survey->additionalQuestions)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        textFileContents += tr("{Any additional questions you wish to include in the survey but not use for forming groups.}");
                        csvFileContents += ",Any additional questions you wish to include in the survey but not use for forming groups.";
                    }
                }

                //write the files
                QTextStream output(&saveFile), output2(&saveFile2);
                output << textFileContents;
                output2 << csvFileContents;
                saveFile.close();
                saveFile2.close();

                survey->surveyCreated = true;
            }
        }
        else
        {
            QMessageBox::critical(survey, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
        }
    }
}

void SurveyMaker::on_surveyDestinationBox_currentIndexChanged(const QString &arg1)
{
    if(arg1 == "Google form")
    {
        ui->makeSurveyButton->setToolTip("<html>Upload the survey to your Google Drive.</html>");
        generateSurvey = &SurveyMaker::postGoogleURL;
    }
    else if(arg1 == "text files")
    {
        ui->makeSurveyButton->setToolTip("<html>Create a text file with the required question texts and a csv file to paste the results in afterwards.</html>");
        generateSurvey = &SurveyMaker::createFiles;
    }
}

void SurveyMaker::on_surveyTitleLineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->surveyTitleLineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the survey title:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    title = ui->surveyTitleLineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_genderCheckBox_clicked(bool checked)
{
    gender = checked;
    refreshPreview();
}

void SurveyMaker::on_URMCheckBox_clicked(bool checked)
{
    URM = checked;
    refreshPreview();
}

void SurveyMaker::on_attributeCountSpinBox_valueChanged(int arg1)
{
    numAttributes = arg1;
    ui->attributeScrollBar->setMaximum(std::max(arg1-1,0));
    ui->attributeScrollBar->setEnabled(numAttributes > 0);
    ui->attributeTextEdit->setEnabled(numAttributes > 0);
    ui->attributeComboBox->setEnabled(numAttributes > 0);
    refreshPreview();
}

void SurveyMaker::on_attributeScrollBar_valueChanged(int value)
{
    ui->attributeTextEdit->setPlainText(attributeTexts[value]);
    ui->attributeTextEdit->setPlaceholderText(tr("Enter attribute question ") + QString::number(value+1) + ".");
    ui->attributeComboBox->setCurrentIndex((attributeResponses[value] > 0) ? attributeResponses[value] + 1 : 0);
}

void SurveyMaker::on_attributeTextEdit_textChanged()
{
    //validate entry
    QString currText = ui->attributeTextEdit->toPlainText();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->attributeTextEdit->setPlainText(ui->attributeTextEdit->toPlainText().remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the question text:\n    ,  &  <  >\nOther punctuation is allowed."));
    }
    else if(currText.contains(tr("In which section are you enrolled"), Qt::CaseInsensitive))
    {
        ui->attributeTextEdit->setPlainText(ui->attributeTextEdit->toPlainText().replace(tr("In which section are you enrolled"), tr("_"), Qt::CaseInsensitive));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, attribute questions may not containt the exact wording:\n"
                                                          "\"In which section are you enrolled\"\nwithin the question text.\n"
                                                          "A section question may be added using the \"Section\" checkbox."));
    }

    attributeTexts[ui->attributeScrollBar->value()] = ui->attributeTextEdit->toPlainText().simplified();
    refreshPreview();
}

void SurveyMaker::on_attributeComboBox_currentIndexChanged(int index)
{
    attributeResponses[ui->attributeScrollBar->value()] = ((index>1) ? (index-1) : 0);
    refreshPreview();
}

void SurveyMaker::on_scheduleCheckBox_clicked(bool checked)
{
    schedule = checked;
    ui->busyFreeLabel->setEnabled(checked);
    ui->busyFreeComboBox->setEnabled(checked);
    ui->daysComboBox->setEnabled(checked);
    ui->day1CheckBox->setEnabled(checked);
    ui->day1LineEdit->setEnabled(checked);
    ui->day2CheckBox->setEnabled(checked);
    ui->day2LineEdit->setEnabled(checked);
    ui->day3CheckBox->setEnabled(checked);
    ui->day3LineEdit->setEnabled(checked);
    ui->day4CheckBox->setEnabled(checked);
    ui->day4LineEdit->setEnabled(checked);
    ui->day5CheckBox->setEnabled(checked);
    ui->day5LineEdit->setEnabled(checked);
    ui->day6CheckBox->setEnabled(checked);
    ui->day6LineEdit->setEnabled(checked);
    ui->day7CheckBox->setEnabled(checked);
    ui->day7LineEdit->setEnabled(checked);
    ui->timeStartEdit->setEnabled(checked);
    ui->timeEndEdit->setEnabled(checked);
    refreshPreview();
}

void SurveyMaker::on_busyFreeComboBox_currentIndexChanged(const QString &arg1)
{
    busyOrFree = ((arg1 == "busy")? busy : free);
    refreshPreview();
}

void SurveyMaker::on_daysComboBox_currentIndexChanged(int index)
{
    if(index == 0)
    {
        //All Days
        ui->day1CheckBox->setChecked(true);
        ui->day2CheckBox->setChecked(true);
        ui->day3CheckBox->setChecked(true);
        ui->day4CheckBox->setChecked(true);
        ui->day5CheckBox->setChecked(true);
        ui->day6CheckBox->setChecked(true);
        ui->day7CheckBox->setChecked(true);
    }
    else if(index == 1)
    {
        //Weekdays
        ui->day1CheckBox->setChecked(false);
        ui->day2CheckBox->setChecked(true);
        ui->day3CheckBox->setChecked(true);
        ui->day4CheckBox->setChecked(true);
        ui->day5CheckBox->setChecked(true);
        ui->day6CheckBox->setChecked(true);
        ui->day7CheckBox->setChecked(false);

    }
    else if(index == 2)
    {
        //Weekends
        ui->day1CheckBox->setChecked(true);
        ui->day2CheckBox->setChecked(false);
        ui->day3CheckBox->setChecked(false);
        ui->day4CheckBox->setChecked(false);
        ui->day5CheckBox->setChecked(false);
        ui->day6CheckBox->setChecked(false);
        ui->day7CheckBox->setChecked(true);
    }
    else
    {
        //Custom Days
    }
}

void SurveyMaker::on_day1CheckBox_toggled(bool checked)
{
    ui->day1LineEdit->setText(checked? day1name : "");
    ui->day1LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day2CheckBox_toggled(bool checked)
{
    ui->day2LineEdit->setText(checked? day2name : "");
    ui->day2LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day3CheckBox_toggled(bool checked)
{
    ui->day3LineEdit->setText(checked? day3name : "");
    ui->day3LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day4CheckBox_toggled(bool checked)
{
    ui->day4LineEdit->setText(checked? day4name : "");
    ui->day4LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day5CheckBox_toggled(bool checked)
{
    ui->day5LineEdit->setText(checked? day5name : "");
    ui->day5LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day6CheckBox_toggled(bool checked)
{
    ui->day6LineEdit->setText(checked? day6name : "");
    ui->day6LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day7CheckBox_toggled(bool checked)
{
    ui->day7LineEdit->setText(checked? day7name : "");
    ui->day7LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::checkDays()
{
    bool weekends = ui->day1CheckBox->isChecked() && ui->day7CheckBox->isChecked();
    bool noWeekends = !(ui->day1CheckBox->isChecked() || ui->day7CheckBox->isChecked());
    bool weekdays = ui->day2CheckBox->isChecked() && ui->day3CheckBox->isChecked() &&
                    ui->day4CheckBox->isChecked() && ui->day5CheckBox->isChecked() && ui->day6CheckBox->isChecked();
    bool noWeekdays = !(ui->day2CheckBox->isChecked() || ui->day3CheckBox->isChecked() ||
                        ui->day4CheckBox->isChecked() || ui->day5CheckBox->isChecked() || ui->day6CheckBox->isChecked());
    if(weekends && weekdays)
    {
        ui->daysComboBox->setCurrentIndex(0);
    }
    else if(weekdays && noWeekends)
    {
        ui->daysComboBox->setCurrentIndex(1);
    }
    else if(weekends && noWeekdays)
    {
        ui->daysComboBox->setCurrentIndex(2);
    }
    else
    {
        ui->daysComboBox->setCurrentIndex(3);
    }
}

void SurveyMaker::on_day1LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day1LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[0] = ui->day1LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day1LineEdit_editingFinished()
{
    if((dayNames[0].isEmpty()))
    {
        ui->day1CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day2LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day2LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[1] = ui->day2LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day2LineEdit_editingFinished()
{
    if((dayNames[1].isEmpty()))
    {
        ui->day2CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day3LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day3LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[2] = ui->day3LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day3LineEdit_editingFinished()
{
    if((dayNames[2].isEmpty()))
    {
        ui->day3CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day4LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day4LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[3] = ui->day4LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day4LineEdit_editingFinished()
{
    if((dayNames[3].isEmpty()))
    {
        ui->day4CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day5LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day5LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[4] = ui->day5LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day5LineEdit_editingFinished()
{
    if((dayNames[4].isEmpty()))
    {
        ui->day5CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day6LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day6LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[5] = ui->day6LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day6LineEdit_editingFinished()
{
    if((dayNames[5].isEmpty()))
    {
        ui->day6CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day7LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day7LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[6] = ui->day7LineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_day7LineEdit_editingFinished()
{
    if((dayNames[6].isEmpty()))
    {
        ui->day7CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_timeStartEdit_timeChanged(QTime time)
{
    startTime = time.hour();
    if(ui->timeEndEdit->time() <= time)
    {
        ui->timeEndEdit->setTime(QTime(time.hour(), 0));
    }
    refreshPreview();
}

void SurveyMaker::on_timeEndEdit_timeChanged(QTime time)
{
    endTime = time.hour();
    if(ui->timeStartEdit->time() >= time)
    {
        ui->timeStartEdit->setTime(QTime(time.hour(), 0));
    }
    refreshPreview();
}

void SurveyMaker::on_sectionCheckBox_clicked(bool checked)
{
    section = checked;
    ui->sectionNamesTextEdit->setEnabled(checked);
    refreshPreview();
}

void SurveyMaker::on_sectionNamesTextEdit_textChanged()
{
    //validate entry
    QString currText = ui->sectionNamesTextEdit->toPlainText();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->sectionNamesTextEdit->setPlainText(ui->sectionNamesTextEdit->toPlainText().remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the section names:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    // split the input at every newline, then remove any blanks (including just spaces)
    sectionNames = ui->sectionNamesTextEdit->toPlainText().split("\n");
    for (int line = 0; line < sectionNames.size(); line++)
    {
        sectionNames[line] = sectionNames.at(line).trimmed();
    }
    sectionNames.removeAll(QString(""));

    refreshPreview();
}

void SurveyMaker::on_additionalQuestionsCheckBox_clicked(bool checked)
{
    additionalQuestions = checked;
    refreshPreview();
}

void SurveyMaker::openSurvey()
{
    //read all options from a text file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), saveFileLocation.canonicalFilePath(), tr("gruepr survey File (*.gru);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile loadFile(fileName);
        if(loadFile.open(QIODevice::ReadOnly))
        {
            saveFileLocation = QFileInfo(fileName).canonicalPath();
            QJsonDocument loadDoc(QJsonDocument::fromJson(loadFile.readAll()));
            QJsonObject loadObject = loadDoc.object();

            if(loadObject.contains("Title") && loadObject["Title"].isString())
            {
                ui->surveyTitleLineEdit->setText(loadObject["Title"].toString());
            }
            if(loadObject.contains("FirstName") && loadObject["FirstName"].isBool())
            {
                ui->firstNameCheckBox->setChecked(loadObject["FirstName"].toBool());
            }
            if(loadObject.contains("LastName") && loadObject["LastName"].isBool())
            {
                ui->lastNameCheckBox->setChecked(loadObject["LastName"].toBool());
            }
            if(loadObject.contains("Email") && loadObject["Email"].isBool())
            {
                ui->emailCheckBox->setChecked(loadObject["Email"].toBool());
            }
            if(loadObject.contains("Gender") && loadObject["Gender"].isBool())
            {
                ui->genderCheckBox->setChecked(loadObject["Gender"].toBool());
                on_genderCheckBox_clicked(loadObject["Gender"].toBool());
            }
            if(loadObject.contains("URM") && loadObject["URM"].isBool())
            {
                ui->URMCheckBox->setChecked(loadObject["URM"].toBool());
                on_URMCheckBox_clicked(loadObject["URM"].toBool());
            }
            if(loadObject.contains("numAttributes") && loadObject["numAttributes"].isDouble())
            {
                ui->attributeCountSpinBox->setValue(loadObject["numAttributes"].toInt());
            }
            for(int attribute = 0; attribute < numAttributes; attribute++)
            {
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Question") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Question"].isString())
                {
                     attributeTexts[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"Question"].toString();
                }
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Response") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Response"].isDouble())
                {
                     attributeResponses[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"Response"].toInt();
                }
            }
            // reload first attribute question and responses on screen
            ui->attributeScrollBar->setValue(0);
            on_attributeScrollBar_valueChanged(0);
            if(loadObject.contains("Schedule") && loadObject["Schedule"].isBool())
            {
                ui->sectionCheckBox->setChecked(loadObject["Schedule"].toBool());
                on_sectionCheckBox_clicked(loadObject["Schedule"].toBool());
            }
            if(loadObject.contains("ScheduleAsBusy") && loadObject["ScheduleAsBusy"].isBool())
            {
                ui->busyFreeComboBox->setCurrentText(loadObject["ScheduleAsBusy"].toBool()? "busy" : "free");
                on_busyFreeComboBox_currentIndexChanged(loadObject["ScheduleAsBusy"].toBool()? "busy" : "free");
            }
            //below is not performed with a for-loop because checkboxes and lineedits are not in an array
            if(loadObject.contains("scheduleDay1") && loadObject["scheduleDay1"].isBool())
            {
                ui->day1CheckBox->setChecked(loadObject["scheduleDay1"].toBool());
                on_day1CheckBox_toggled(loadObject["scheduleDay1"].toBool());
            }
            if(loadObject.contains("scheduleDay1Name") && loadObject["scheduleDay1Name"].isString())
            {
                ui->day1LineEdit->setText(loadObject["scheduleDay1Name"].toString());
            }
            if(loadObject.contains("scheduleDay2") && loadObject["scheduleDay2"].isBool())
            {
                ui->day2CheckBox->setChecked(loadObject["scheduleDay2"].toBool());
                on_day2CheckBox_toggled(loadObject["scheduleDay2"].toBool());
            }
            if(loadObject.contains("scheduleDay2Name") && loadObject["scheduleDay2Name"].isString())
            {
                ui->day2LineEdit->setText(loadObject["scheduleDay2Name"].toString());
            }
            if(loadObject.contains("scheduleDay3") && loadObject["scheduleDay3"].isBool())
            {
                ui->day3CheckBox->setChecked(loadObject["scheduleDay3"].toBool());
                on_day3CheckBox_toggled(loadObject["scheduleDay3"].toBool());
            }
            if(loadObject.contains("scheduleDay3Name") && loadObject["scheduleDay3Name"].isString())
            {
                ui->day3LineEdit->setText(loadObject["scheduleDay3Name"].toString());
            }
            if(loadObject.contains("scheduleDay4") && loadObject["scheduleDay4"].isBool())
            {
                ui->day4CheckBox->setChecked(loadObject["scheduleDay4"].toBool());
                on_day4CheckBox_toggled(loadObject["scheduleDay4"].toBool());
            }
            if(loadObject.contains("scheduleDay4Name") && loadObject["scheduleDay4Name"].isString())
            {
                ui->day4LineEdit->setText(loadObject["scheduleDay4Name"].toString());
            }
            if(loadObject.contains("scheduleDay5") && loadObject["scheduleDay5"].isBool())
            {
                ui->day5CheckBox->setChecked(loadObject["scheduleDay5"].toBool());
                on_day5CheckBox_toggled(loadObject["scheduleDay5"].toBool());
            }
            if(loadObject.contains("scheduleDay5Name") && loadObject["scheduleDay5Name"].isString())
            {
                ui->day5LineEdit->setText(loadObject["scheduleDay5Name"].toString());
            }
            if(loadObject.contains("scheduleDay6") && loadObject["scheduleDay6"].isBool())
            {
                ui->day6CheckBox->setChecked(loadObject["scheduleDay6"].toBool());
                on_day6CheckBox_toggled(loadObject["scheduleDay6"].toBool());
            }
            if(loadObject.contains("scheduleDay6Name") && loadObject["scheduleDay6Name"].isString())
            {
                ui->day6LineEdit->setText(loadObject["scheduleDay6Name"].toString());
            }
            if(loadObject.contains("scheduleDay7") && loadObject["scheduleDay7"].isBool())
            {
                ui->day7CheckBox->setChecked(loadObject["scheduleDay7"].toBool());
                on_day7CheckBox_toggled(loadObject["scheduleDay7"].toBool());
            }
            if(loadObject.contains("scheduleDay7Name") && loadObject["scheduleDay7Name"].isString())
            {
                ui->day7LineEdit->setText(loadObject["scheduleDay7Name"].toString());
            }
            if(loadObject.contains("scheduleStartHour") && loadObject["scheduleStartHour"].isDouble())
            {
                ui->timeStartEdit->setTime(QTime(loadObject["scheduleStartHour"].toInt(),0));
            }
            if(loadObject.contains("scheduleEndHour") && loadObject["scheduleEndHour"].isDouble())
            {
                ui->timeEndEdit->setTime(QTime(loadObject["scheduleEndHour"].toInt(),0));
            }
            if(loadObject.contains("Section") && loadObject["Section"].isBool())
            {
                ui->sectionCheckBox->setChecked(loadObject["Section"].toBool());
                on_sectionCheckBox_clicked(loadObject["Section"].toBool());
            }
            if(loadObject.contains("SectionNames") && loadObject["SectionNames"].isString())
            {
                ui->sectionNamesTextEdit->setPlainText(loadObject["SectionNames"].toString().replace(',', '\n'));
            }
            if(loadObject.contains("AdditionalQuestions") && loadObject["AdditionalQuestions"].isBool())
            {
                ui->additionalQuestionsCheckBox->setChecked(loadObject["AdditionalQuestions"].toBool());
                on_additionalQuestionsCheckBox_clicked(loadObject["AdditionalQuestions"].toBool());
            }
            loadFile.close();

            refreshPreview();
        }
        else
        {
            QMessageBox::critical(this, tr("File Error"), tr("This file cannot be read."));
        }
    }
}

void SurveyMaker::saveSurvey()
{
    //save all options to a text file
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), saveFileLocation.canonicalFilePath(), tr("gruepr survey File (*.gru);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile saveFile(fileName);
        if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            saveFileLocation = QFileInfo(fileName).canonicalPath();
            QJsonObject saveObject;
            saveObject["Title"] = title;
            saveObject["FirstName"] = true;
            saveObject["LastName"] = true;
            saveObject["Email"] = true;
            saveObject["Gender"] = gender;
            saveObject["URM"] = URM;
            saveObject["numAttributes"] = numAttributes;
            for(int attribute = 0; attribute < numAttributes; attribute++)
            {
                saveObject["Attribute" + QString::number(attribute+1)+"Question"] = attributeTexts[attribute];
                saveObject["Attribute" + QString::number(attribute+1)+"Response"] = attributeResponses[attribute];
            }
            saveObject["Schedule"] = schedule;
            saveObject["ScheduleAsBusy"] = (busyOrFree == busy);
            //below is not performed with a for-loop because checkboxes are not in an array
            saveObject["scheduleDay1"] = ui->day1CheckBox->isChecked();
            saveObject["scheduleDay1Name"] = dayNames[0];
            saveObject["scheduleDay2"] = ui->day2CheckBox->isChecked();
            saveObject["scheduleDay2Name"] = dayNames[1];
            saveObject["scheduleDay3"] = ui->day3CheckBox->isChecked();
            saveObject["scheduleDay3Name"] = dayNames[2];
            saveObject["scheduleDay4"] = ui->day4CheckBox->isChecked();
            saveObject["scheduleDay4Name"] = dayNames[3];
            saveObject["scheduleDay5"] = ui->day5CheckBox->isChecked();
            saveObject["scheduleDay5Name"] = dayNames[4];
            saveObject["scheduleDay6"] = ui->day6CheckBox->isChecked();
            saveObject["scheduleDay6Name"] = dayNames[5];
            saveObject["scheduleDay7"] = ui->day7CheckBox->isChecked();
            saveObject["scheduleDay7Name"] = dayNames[6];
            saveObject["scheduleStartHour"] = startTime;
            saveObject["scheduleEndHour"] = endTime;
            saveObject["Section"] = section;
            saveObject["SectionNames"] = sectionNames.join(',');
            saveObject["AdditionalQuestions"] = additionalQuestions;

            QJsonDocument saveDoc(saveObject);
            saveFile.write(saveDoc.toJson());
            saveFile.close();
        }
        else
        {
            QMessageBox::critical(this, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the file to disk."));
        }
    }
}


void SurveyMaker::settingsWindow()
{
}

void SurveyMaker::helpWindow()
{
    QFile helpFile(":/help-surveymaker.html");
    if(!helpFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QDialog helpWindow(this);
    helpWindow.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    helpWindow.setSizeGripEnabled(true);
    helpWindow.setWindowTitle("Help");
    QGridLayout theGrid(&helpWindow);
    QTextBrowser helpContents(&helpWindow);
    helpContents.setHtml(tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr: SurveyMaker " GRUEPR_VERSION_NUMBER "</h1>"
                            "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                            "<p>Joshua Hertz <a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                            "<p>Project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>"));
    helpContents.append(helpFile.readAll());
    helpFile.close();
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(600,600);
    helpWindow.exec();
}

void SurveyMaker::aboutWindow()
{
    QMessageBox::about(this, tr("About gruepr: SurveyMaker"),
                       tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr: SurveyMaker " GRUEPR_VERSION_NUMBER "</h1>"
                          "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                          "<br>Joshua Hertz<br><a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>."
                          "<p>gruepr incorporates:"
                              "<ul><li>Code libraries from <a href = http://qt.io>Qt, v 5.12 or 5.13</a>, released under the GNU Lesser General Public License version 3</li>"
                              "<li>Icons from <a href = https://icons8.com>Icons8</a>, released under Creative Commons license \"Attribution-NoDerivs 3.0 Unported\"</li>"
                              "<li><span style=\"font-family:'Oxygen Mono';\">The font <a href = https://www.fontsquirrel.com/fonts/oxygen-mono>"
                                                                    "Oxygen Mono</a>, Copyright &copy; 2012, Vernon Adams (vern@newtypography.co.uk),"
                                                                    " released under SIL OPEN FONT LICENSE V1.1.</span></li>"
                              "<li>A photo of a grouper, courtesy Rich Whalen</li></ul>"
                          "<h3>Disclaimer</h3>"
                          "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
                          "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                          "<p>This program is free software: you can redistribute it and/or modify it under the terms of the <a href = https://www.gnu.org/licenses/gpl.html>"
                          "GNU General Public License</a> as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."));
}


//////////////////
// Before closing the application window, save the window geometry for next time and then, if the user never created a survey, ask them if they really want to leave
//////////////////
void SurveyMaker::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    savedSettings.setValue("surveyMakerWindowGeometry", saveGeometry());
    savedSettings.setValue("surveyMakerSaveFileLocation", saveFileLocation.canonicalFilePath());

    bool actuallyExit = surveyCreated || savedSettings.value("ExitNowEvenIfNoSurveyCreated", false).toBool();
    if(!actuallyExit)
    {
        QMessageBox exitNoSurveyBox(this);
        exitNoSurveyBox.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        QCheckBox neverShowAgain(tr("Don't ask me this again."), &exitNoSurveyBox);

        exitNoSurveyBox.setIcon(QMessageBox::Warning);
        exitNoSurveyBox.setWindowTitle(tr("Are you sure you want to exit?"));
        exitNoSurveyBox.setText(tr("You have not created a survey yet. Are you sure you want to exit?"));
        exitNoSurveyBox.setCheckBox(&neverShowAgain);
        exitNoSurveyBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        QAbstractButton* pButtonYes = exitNoSurveyBox.button(QMessageBox::Yes);
        pButtonYes->setText(tr("Yes, exit without creating a survey"));
        QAbstractButton* pButtonNo = exitNoSurveyBox.button(QMessageBox::No);
        pButtonNo->setText(tr("No, please take me back to SurveyMaker"));

        exitNoSurveyBox.exec();

        actuallyExit = (exitNoSurveyBox.clickedButton() == pButtonYes);

        if(neverShowAgain.checkState() == Qt::Checked)
        {
            savedSettings.setValue("ExitNowEvenIfNoSurveyCreated", true);
        }
    }

    if(actuallyExit)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
