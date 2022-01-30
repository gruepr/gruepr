#include "ui_surveymaker.h"
#include "surveymaker.h"
#include "widgets/attributeTabItem.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QScrollBar>
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
    connect(ui->actionBugReport, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl("http://bit.ly/grueprBugReportFromApp"));});

    noInvalidPunctuation = new QRegularExpressionValidator(QRegularExpression("[^,&<>]*"), this);

    //put timezones into combobox
    QStringList timeZones = QString(TIMEZONENAMES).split(";");
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->insertItem(TimezoneType::noneOrHome, tr("[no timezone given]"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::noneOrHome+1);
    baseTimezoneComboBox->insertItem(TimezoneType::custom, tr("Custom timezone:"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::custom+1);
    for(int zone = 0; zone < timeZones.size(); zone++)
    {
        QString zonename = timeZones.at(zone);
        zonename.remove('"');
        baseTimezoneComboBox->insertItem(TimezoneType::set + zone, zonename);
        baseTimezoneComboBox->setItemData(TimezoneType::set + zone, zonename, Qt::ToolTipRole);
    }
    ui->scheduleLayout->replaceWidget(ui->fillinTimezoneComboBox, baseTimezoneComboBox, Qt::FindChildrenRecursively);
    ui->fillinTimezoneComboBox->setParent(nullptr);
    ui->fillinTimezoneComboBox->deleteLater();
    connect(baseTimezoneComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SurveyMaker::baseTimezoneComboBox_currentIndexChanged);

    //load in local day names, and connect subwindow ui to slots
    defaultDayNames.reserve(MAX_DAYS);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        defaultDayNames << sunday.addDays(day).toString("dddd");
        dayNames[day] = defaultDayNames.at(day);
        dayCheckBoxes[day] = new QCheckBox;
        dayLineEdits[day] = new QLineEdit;
        dayCheckBoxes[day]->setChecked(true);
        dayLineEdits[day]->setText(defaultDayNames.at(day));
        dayLineEdits[day]->setPlaceholderText(tr("Day ") + QString::number(day + 1) + tr(" name"));
        connect(dayLineEdits[day], &QLineEdit::textChanged, this, [this, day](const QString &text) {day_LineEdit_textChanged(text, dayLineEdits[day], dayNames[day]);});
        connect(dayLineEdits[day], &QLineEdit::editingFinished, this, [this, day] {if(dayNames[day].isEmpty()){dayCheckBoxes[day]->setChecked(false);};});
        connect(dayCheckBoxes[day], &QCheckBox::toggled, this, [this, day](bool checked) {day_CheckBox_toggled(checked, dayLineEdits[day], defaultDayNames[day]);});
    }
    daysWindow = new dayNamesDialog(dayCheckBoxes, dayLineEdits, this);

    //Restore window geometry
    QSettings savedSettings;
    restoreGeometry(savedSettings.value("surveyMakerWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

    //Make sure we can read placeholder text in "custom timezone" box
    QString placeholder = tr("Custom timezone");
    ui->baseTimezoneLineEdit->setPlaceholderText(placeholder);
    QFontMetrics fm(ui->baseTimezoneLineEdit->font());
    QMargins margin = ui->baseTimezoneLineEdit->textMargins();
    const int widthOfPlaceholder = fm.size(Qt::TextSingleLine, placeholder).width() + margin.left() + margin.right();
    ui->baseTimezoneLineEdit->setMinimumWidth(widthOfPlaceholder);

    //Add tabs for each attribute and items to each response options combobox
    ui->attributesTabWidget->clear();
    ui->fillinTab->deleteLater();
    responseOptions = QString(RESPONSE_OPTIONS).split(';');
    for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
    {
        auto *attributeTab = new attributeTabItem(attributeTabItem::surveyMaker, this);
        attributeTab->attributeText->setPlaceholderText(tr("Enter attribute question ") + QString::number(tab + 1));
        connect(attributeTab->attributeText, &QTextEdit::textChanged, this, &SurveyMaker::attributeTextChanged);
        connect(attributeTab->attributeResponses, QOverload<int>::of(&ComboBoxWithElidedContents::currentIndexChanged), this, &SurveyMaker::refreshPreview);
        connect(attributeTab->allowMultipleResponses, &QCheckBox::toggled, this, &SurveyMaker::refreshPreview);

        ui->attributesTabWidget->addTab(attributeTab, QString::number(tab+1) + "   ");
        ui->attributesTabWidget->tabBar()->tabButton(tab, QTabBar::RightSide)->resize(TABCLOSEICONSIZE);
        ui->attributesTabWidget->setTabVisible(tab, tab < numAttributes);
    }
    responseOptions.prepend("custom options, to be added after creating the form");
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::currentChanged, this, &SurveyMaker::attributeTabBarScrollVisibleTabs);
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::tabMoved, this, &SurveyMaker::attributeTabBarMoveTab);
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::tabCloseRequested, this, &SurveyMaker::attributeTabClose);

    refreshPreview();
}

SurveyMaker::~SurveyMaker()
{
    delete ui;
}

void SurveyMaker::refreshPreview()
{
    // update attribute data
    for(int attrib = 0; attrib < MAX_ATTRIBUTES; attrib++)
    {
        auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(attrib));
        attributeTexts[attrib] = attribTab->attributeText->toPlainText().simplified();//attributeTabItems.at(attrib)->attributeText->toPlainText();
        attributeResponses[attrib] = ((attribTab->attributeResponses->currentIndex()>1) ? (attribTab->attributeResponses->currentIndex()-1) : 0);//((attributeTabItems.at(attrib)->attributeResponses->currentIndex()>1) ? (attributeTabItems.at(attrib)->attributeResponses->currentIndex()-1) : 0);
        attributeAllowMultipleResponses[attrib] = attribTab->allowMultipleResponses->isChecked();//attributeTabItems.at(attrib)->allowMultipleResponses->isChecked();
    }

    if(timezone && !schedule)
    {
        attributeTexts[numAttributes] = "What time zone will you be based in during this class?";
        attributeResponses[numAttributes] = TIMEZONE_RESPONSE_OPTION;
        numAttributes++;
    }

    int currPos = ui->previewText->verticalScrollBar()->value();

    // generate preview text
    QString preview = "<h2>" + title + "</h2>";
    preview += "<h3>First, some basic information</h3>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your first name (or the name you prefer to be called)?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your last name?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your email address?<br></p>";
    preview += gender?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;With which gender do you identify most closely?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>woman </i><b>|</b><i> man </i><b>|</b><i> nonbinary </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += URM?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;How do you identify your race, ethnicity, or cultural heritage?<br></p>"
                : "";
    preview += "<hr>";
    if(numAttributes > 0)
    {
        preview += "<h3>This set of questions is about you, your past experiences, and / or your teamwork preferences.</h3>";
        for(int attrib = 0; attrib < numAttributes; attrib++)
        {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;" +
                           (attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : attributeTexts[attrib])
                           + "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                           "<small>options: <b>{ </b><i>";
                if(attributeResponses[attrib] < responseOptions.size())
                {
                    preview += responseOptions.at(attributeResponses[attrib]).split("/").join("</i><b>|</b><i>");
                }
                else if(attributeResponses[attrib] == TIMEZONE_RESPONSE_OPTION)
                {
                    preview += "dropdown box of world timezones";
                }
                if(attributeAllowMultipleResponses[attrib])
                {
                    preview += "</i><b> }</b><br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;* Multiple responses allowed</small> ";
                }
                else
                {
                    preview += "</i><b> }</b></small>";
                }
        }
        preview += "<hr>";
    }
    if(schedule)
    {
        preview += "<h3>Please tell us about your weekly schedule.</h3>";
        if(timezone)
        {
            preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What time zone will you be based in during this class?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                       "<small>options: <b>{ </b><i>dropdown box of world timezones</i><b> }</b></small><br></p>";
        }

        preview += "<p>Check the times that you are ";
        preview += ((busyOrFree == busy)? "BUSY and will be UNAVAILABLE" : "FREE and will be AVAILABLE");
        preview += " for group work.";

        if(timezone && baseTimezone.isEmpty())
        {
            preview += tr(" These times refer to <u><strong>your home</strong></u> timezone.");
        }
        else if(!baseTimezone.isEmpty())
        {
            preview += " These times refer to <u><strong>" + baseTimezone + "</strong></u> time.";
        }

        preview += "</p><p>&nbsp;&nbsp;&nbsp;<i>grid of checkboxes:</i></p>";
        preview += "<p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>";
        for(int time = startTime; time <= endTime; time++)
        {
            preview += QTime(time, 0).toString("hA");
            if(time != endTime)
            {
                preview += "&nbsp;&nbsp;&nbsp;";
            }
        }
        preview += "</small></p>";
        for(const auto & dayName : dayNames)
        {
            if(!(dayName.isEmpty()))
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;<small>" + dayName + "</small></p>";
            }
        }
        preview += "<hr>";
    }
    if(section || preferredTeammates || preferredNonTeammates || additionalQuestions)
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
        if(preferredTeammates)
        {
            if(numPreferredAllowed == 1)
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Please write the name of someone who you would like to have on your team. "
                            "Write their first and last name only.</p>";
            }
            else
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Please list the name(s) of up to " + QString::number(numPreferredAllowed) +
                            " people who you would like to have on your team. "
                            "Write their first and last name, and put a comma between multiple names.</p>";
            }
        }
        if(preferredNonTeammates)
        {
            if(numPreferredAllowed == 1)
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Please write the name of someone who you would like to NOT have on your team. "
                            "Write their first and last name only.</p>";
            }
            else
            {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Please list the name(s) of up to " + QString::number(numPreferredAllowed) +
                            " people who you would like to NOT have on your team. "
                            "Write their first and last name, and put a comma between multiple names.</p>";
            }
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
        QMessageBox::critical(this, tr("Error!"), tr("A gruepr survey must have at least one\n"
                                                     "attribute question and/or a schedule question.\n"
                                                     "The survey has NOT been created."));
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
        QMessageBox::critical(survey, tr("Error!"), tr("There does not seem to be an internet connection.\n"
                                                       "Check your network connection and try again.\n"
                                                       "The survey has NOT been created."));
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
    QString allAttributeMultAllowed;
    for(int attrib = 0; attrib < survey->numAttributes; attrib++)
    {
        if(attrib != 0)
        {
            allAttributeMultAllowed += ",";
        }
        allAttributeMultAllowed += ((survey->attributeAllowMultipleResponses[attrib])? "true" : "false");
    }
    URL += "attrmulti=" + allAttributeMultAllowed + "&";
    URL += "sched=" + QString(survey->schedule? "true" : "false") + "&";
    URL += "tzone=" + QString((survey->timezone && survey->schedule)? "true" : "false") + "&";
    URL += "bzone=" + survey->baseTimezone + "&";
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
    URL += "prefmate=" + QString(survey->preferredTeammates? "true" : "false") + "&";
    URL += "prefnon=" + QString(survey->preferredNonTeammates? "true" : "false") + "&";
    URL += "numprefs=" + QString::number(survey->numPreferredAllowed) + "&";
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
                    textFileContents += tr("With which gender do you identify most closely?");
                    textFileContents += "\n     " + tr("choices: [woman | man | nonbinary | prefer not to answer]");
                    csvFileContents += ",With which gender do you identify most closely?";
                }
                if(survey->URM)
                {
                    textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                    textFileContents += tr("How do you identify your race, ethnicity, or cultural heritage?");
                    csvFileContents += ",\"How do you identify your race, ethnicity, or cultural heritage?\"";
                }
                if(survey->numAttributes > 0)
                {
                    textFileContents += "\n\n\n" + tr("Section") + " " + QString::number(++sectionNumber) + ", " + tr("Attributes") + ":";
                    for(int attrib = 0; attrib < survey->numAttributes; attrib++)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        textFileContents += survey->attributeTexts[attrib].isEmpty()?
                                                "{" + tr("Attribute question") + " " + QString::number(attrib+1) + "}" : survey->attributeTexts[attrib];
                        textFileContents += "\n     " + tr("choices") + ": [";
                        QStringList responses;
                        if(survey->attributeResponses[attrib] < survey->responseOptions.size())
                        {
                            responses = survey->responseOptions.at(survey->attributeResponses[attrib]).split("/");
                        }
                        else if(survey->attributeResponses[attrib] == TIMEZONE_RESPONSE_OPTION)
                        {
                            responses << tr("list of relevant timezones");
                        }

                        for(int resp = 0; resp < responses.size(); resp++)
                        {
                            if(resp != 0)
                            {
                                textFileContents += " | ";
                            }
                            if( (survey->attributeResponses[attrib] > 0) && (survey->attributeResponses[attrib] <= LAST_LIKERT_RESPONSE) )
                            {
                                textFileContents += QString::number(resp+1) + ". ";
                            }
                            textFileContents += responses.at(resp);
                        }
                        textFileContents += "]\n     (" + ((survey->attributeAllowMultipleResponses[attrib]) ?
                                                          tr("Multiple responses allowed") : tr("Only one response allowed")) + ")";
                        csvFileContents += ",\"" + (survey->attributeTexts[attrib].isEmpty()?
                                                        tr("Attribute question") + " " + QString::number(attrib+1) : survey->attributeTexts[attrib]) + "\"";
                    }
                }
                if(survey->schedule)
                {
                    textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Schedule") + ":";
                    if(survey->timezone)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        textFileContents += tr("What time zone will you be based in during this class?");
                        textFileContents += "\n     " + tr("choices") + ": [" + tr("list of relevant timezones") + "]\n\n";
                        csvFileContents += ",What time zone will you be based in during this class?";
                    }
                    textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                    textFileContents += tr("Please tell us about your weekly schedule.") + "\n";
                    textFileContents += "     " + tr("Check the times that you are");
                    textFileContents += ((survey->busyOrFree == busy)? tr(" BUSY and will be UNAVAILABLE ") : tr(" FREE and will be AVAILABLE "));
                    textFileContents += tr("for group work.");
                    if(survey->timezone && survey->baseTimezone.isEmpty())
                    {
                        textFileContents += tr(" These times refer to your home timezone.");
                    }
                    else if(!(survey->baseTimezone.isEmpty()))
                    {
                        textFileContents += tr(" These times refer to ") + survey->baseTimezone + tr(" time.");
                    }
                    textFileContents += "\n                 ";
                    for(int time = survey->startTime; time <= survey->endTime; time++)
                    {
                        textFileContents += QTime(time, 0).toString("hA") + "    ";
                    }
                    textFileContents += "\n";
                    for(const auto &dayName : survey->dayNames)
                    {
                        if(!(dayName.isEmpty()))
                        {
                            textFileContents += "\n      " + dayName + "\n";
                            csvFileContents += ",Check the times that you are";
                            csvFileContents += ((survey->busyOrFree == busy)? " BUSY and will be UNAVAILABLE " : " FREE and will be AVAILABLE ");
                            csvFileContents += "for group work.";
                            if(!(survey->baseTimezone.isEmpty()))
                            {
                                csvFileContents += " These times refer to " + survey->baseTimezone + " time. ";
                            }
                            csvFileContents += "[" + dayName + "]";
                        }
                    }
                }
                if(survey->section || survey->preferredTeammates || survey->preferredNonTeammates || survey->additionalQuestions)
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
                    if(survey->preferredTeammates)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        if(survey->numPreferredAllowed == 1)
                        {
                            textFileContents += tr("{Please write the name of someone you would like to have on your team. "
                                                   "Write their first and last name only.}");
                            csvFileContents += ",Please write the name of someone who you would like to have on your team.";
                        }
                        else
                        {
                            textFileContents += tr("{Please list the name(s) of up to ") + QString::number(survey->numPreferredAllowed) +
                                                tr( " people who you would like to have on your team. "
                                                   "Write their first and last name, and put a comma between multiple names.}");
                            csvFileContents += ",Please list the name(s) of people who you would like to have on your team.";
                        }
                    }
                    if(survey->preferredNonTeammates)
                    {
                        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                        if(survey->numPreferredAllowed == 1)
                        {
                            textFileContents += tr("{Please write the name of someone you would like to NOT have on your team. "
                                                   "Write their first and last name only.}");
                            csvFileContents += ",Please write the name of someone who you would like to NOT have on your team.";
                        }
                        else
                        {
                            textFileContents += tr("{Please list the name(s) of up to ") + QString::number(survey->numPreferredAllowed) +
                                                tr( " people who you would like to NOT have on your team. "
                                                   "Write their first and last name, and put a comma between multiple names.}");
                            csvFileContents += ",Please list the name(s) of people who you would like to NOT have on your team.";
                        }
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
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the survey title:\n"
                                                          "    ,  &  <  >\n"
                                                          "Other punctuation is allowed."));
    }

    title = currText.trimmed();
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
    if(timezone && !schedule)
    {
        numAttributes++;
    }

    ui->attributesTabWidget->setTabEnabled(0, 0 < arg1);
    for(int i = 1; i < MAX_ATTRIBUTES; i++)
    {
        ui->attributesTabWidget->setTabVisible(i, i < arg1);
    }

    refreshPreview();
}

void SurveyMaker::attributeTabBarScrollVisibleTabs(int index)
{
    const bool wasBlocked = ui->attributesTabWidget->tabBar()->blockSignals(true);
    ui->attributesTabWidget->setCurrentIndex((index > 2)? index - 2 : 0);
    ui->attributesTabWidget->setCurrentIndex((index < MAX_ATTRIBUTES - 3)? index + 2 : MAX_ATTRIBUTES - 1);
    ui->attributesTabWidget->setCurrentIndex(index);
    ui->attributesTabWidget->tabBar()->blockSignals(wasBlocked);
}

void SurveyMaker::attributeTabBarMoveTab(int /*indexFrom*/, int /*indexTo*/)
{
    // reset for every tab the text label and the placeholder text
    for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
    {
        ui->attributesTabWidget->setTabText(tab, QString::number(tab+1) + "   ");
        auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(tab));
        attribTab->attributeText->setPlaceholderText(tr("Enter attribute question ") + QString::number(tab + 1));
    }

    refreshPreview();
}

void SurveyMaker::attributeTabClose(int index)
{
    const int currIndex = ui->attributesTabWidget->currentIndex();
    const bool wasBlocked = ui->attributesTabWidget->tabBar()->blockSignals(true);

    // if this is the last attribute, closing it should just be the same as if clicking down to 0 attributes; otherwise, move this one to the end
    if(ui->attributeCountSpinBox->value() != 1)
    {
        // remove and delete this tab, then add a new one to the end
        auto *oldWidget = ui->attributesTabWidget->widget(index);
        ui->attributesTabWidget->removeTab(index);
        oldWidget->deleteLater();

        auto *attributeTab = new attributeTabItem(attributeTabItem::surveyMaker, this);
        connect(attributeTab->attributeText, &QTextEdit::textChanged, this, &SurveyMaker::attributeTextChanged);
        connect(attributeTab->attributeResponses, QOverload<int>::of(&ComboBoxWithElidedContents::currentIndexChanged), this, &SurveyMaker::refreshPreview);
        connect(attributeTab->allowMultipleResponses, &QCheckBox::toggled, this, &SurveyMaker::refreshPreview);

        ui->attributesTabWidget->addTab(attributeTab, QString::number(MAX_ATTRIBUTES) + "   ");
        ui->attributesTabWidget->tabBar()->tabButton(MAX_ATTRIBUTES-1, QTabBar::RightSide)->resize(TABCLOSEICONSIZE);

        // reset for every tab the text label and the placeholder text
        for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
        {
            ui->attributesTabWidget->setTabText(tab, QString::number(tab+1) + "   ");
            auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(tab));
            attribTab->attributeText->setPlaceholderText(tr("Enter attribute question ") + QString::number(tab + 1));
        }
    }

    ui->attributesTabWidget->tabBar()->blockSignals(wasBlocked);
    ui->attributesTabWidget->setCurrentIndex(currIndex);
    ui->attributeCountSpinBox->setValue(numAttributes - 1);
}

void SurveyMaker::attributeTextChanged()
{
    const int currAttribute = ui->attributesTabWidget->currentIndex();
    //validate entry
    auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(currAttribute));
    QString currText = attribTab->attributeText->toPlainText();//attributeTabItems.at(currAttribute)->attributeText->toPlainText();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        attribTab->attributeText->setPlainText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the question text:\n"
                                                          "    ,  &  <  >\n"
                                                          "Other punctuation is allowed."));
    }
    else if(currText.contains(tr("In which section are you enrolled"), Qt::CaseInsensitive))
    {
        attribTab->attributeText->setPlainText(currText.replace(tr("In which section are you enrolled"), tr("_"), Qt::CaseInsensitive));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, attribute questions may not containt the exact wording:\n"
                                                          "\"In which section are you enrolled\"\nwithin the question text.\n"
                                                          "A section question may be added using the \"Section\" checkbox."));
    }

    refreshPreview();
}

void SurveyMaker::on_timezoneCheckBox_clicked(bool checked)
{
    timezone = checked;

    checkTimezoneAndSchedule();

    refreshPreview();
}

void SurveyMaker::on_scheduleCheckBox_clicked(bool checked)
{
    schedule = checked;

    ui->busyFreeLabel->setEnabled(checked);
    ui->busyFreeComboBox->setEnabled(checked);
    baseTimezoneComboBox->setEnabled(checked);
    ui->baseTimezoneLineEdit->setEnabled(checked);
    ui->daysComboBox->setEnabled(checked);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        dayCheckBoxes[day]->setEnabled(checked);
        dayLineEdits[day]->setEnabled(checked && dayCheckBoxes[day]->isChecked());
    }
    ui->timeStartEdit->setEnabled(checked);
    ui->timeEndEdit->setEnabled(checked);

    checkTimezoneAndSchedule();

    refreshPreview();
}

void SurveyMaker::checkTimezoneAndSchedule()
{
    // If user just turned on timezone but already had full number of attributes, need to first remove last attribute to make room
    // Note that changing the spinbox value will automatically add the timezone question to the end of the attributes
    if(timezone && ui->attributeCountSpinBox->value() == MAX_ATTRIBUTES)
    {
        ui->attributeCountSpinBox->setValue(MAX_ATTRIBUTES - 1);
        ui->attributeCountSpinBox->setMaximum(MAX_ATTRIBUTES - 1);
    }
    else
    {
        // reduce max attributes by 1 anytime we have timezone on, then refresh attribute questions
        ui->attributeCountSpinBox->setMaximum(MAX_ATTRIBUTES - (timezone? 1 : 0));
        on_attributeCountSpinBox_valueChanged(ui->attributeCountSpinBox->value());
    }

    // if asking students about timezone and schedule, should require survey to define timezone of schedule
    if(timezone && schedule)
    {
        baseTimezoneComboBox->setItemText(TimezoneType::noneOrHome, tr("[student's home timezone]"));
    }
    else
    {
        baseTimezoneComboBox->setItemText(TimezoneType::noneOrHome, tr("[no timezone given]"));
    }
}

void SurveyMaker::on_busyFreeComboBox_currentIndexChanged(const QString &arg1)
{
    busyOrFree = ((arg1 == "busy")? busy : free);
    refreshPreview();
}

void SurveyMaker::baseTimezoneComboBox_currentIndexChanged(int arg1)
{
    if(arg1 == TimezoneType::noneOrHome)
    {
        baseTimezone.clear();
    }
    else if(arg1 == TimezoneType::custom)
    {
        baseTimezone = ui->baseTimezoneLineEdit->text().simplified();
        ui->baseTimezoneLineEdit->setFocus();
    }
    else
    {
        baseTimezone = baseTimezoneComboBox->currentText();
    }

    refreshPreview();
}

void SurveyMaker::on_baseTimezoneLineEdit_textChanged()
{
    baseTimezoneComboBox->setCurrentIndex(TimezoneType::custom);

    //validate entry
    QString currText = ui->baseTimezoneLineEdit->text();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->baseTimezoneLineEdit->setText(baseTimezone = currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the timezone name:\n"
                                                          "    ,  &  <  >\n"
                                                          "Other punctuation is allowed."));
    }
    else if(currText.contains(tr("In which section are you enrolled"), Qt::CaseInsensitive))
    {
        ui->baseTimezoneLineEdit->setText(currText.replace(tr("In which section are you enrolled"), tr("_"), Qt::CaseInsensitive));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the timezone name may not containt the exact wording:\n"
                                                          "\"In which section are you enrolled\".\n"
                                                          "A section question may be added using the \"Section\" checkbox."));
    }

    baseTimezone = currText.simplified();
    if(baseTimezone.isEmpty())
    {
        baseTimezoneComboBox->setCurrentIndex(TimezoneType::noneOrHome);
    }
    refreshPreview();
}

void SurveyMaker::on_daysComboBox_activated(int index)
{
    ui->daysComboBox->blockSignals(true);
    if(index == 0)
    {
        //All Days
        for(auto &dayCheckBox : dayCheckBoxes)
        {
            dayCheckBox->setChecked(true);
        }
    }
    else if(index == 1)
    {
        //Weekdays
        dayCheckBoxes[Sun]->setChecked(false);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(true);
        }
        dayCheckBoxes[Sat]->setChecked(false);
    }
    else if(index == 2)
    {
        //Weekends
        dayCheckBoxes[Sun]->setChecked(true);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(false);
        }
        dayCheckBoxes[Sat]->setChecked(true);
    }
    else
    {
        //Custom Days, open subwindow
        daysWindow->exec();
        checkDays();
    }
    ui->daysComboBox->blockSignals(false);
}

void SurveyMaker::day_CheckBox_toggled(bool checked, QLineEdit *dayLineEdit, const QString &dayname)
{
    dayLineEdit->setText(checked? dayname : "");
    dayLineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::checkDays()
{
    bool weekends = dayCheckBoxes[Sun]->isChecked() && dayCheckBoxes[Sat]->isChecked();
    bool noWeekends = !(dayCheckBoxes[Sun]->isChecked() || dayCheckBoxes[Sat]->isChecked());
    bool weekdays = dayCheckBoxes[Mon]->isChecked() && dayCheckBoxes[Tue]->isChecked() &&
                    dayCheckBoxes[Wed]->isChecked() && dayCheckBoxes[Thu]->isChecked() && dayCheckBoxes[Fri]->isChecked();
    bool noWeekdays = !(dayCheckBoxes[Mon]->isChecked() || dayCheckBoxes[Tue]->isChecked() ||
                        dayCheckBoxes[Wed]->isChecked() || dayCheckBoxes[Thu]->isChecked() || dayCheckBoxes[Fri]->isChecked());
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

void SurveyMaker::day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname)
{
    //validate entry
    QString currText = text;
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        dayLineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the day name:\n"
                                                          "    ,  &  <  >\n"
                                                          "Other punctuation is allowed."));
    }

    dayname = currText.trimmed();
    refreshPreview();
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
        ui->sectionNamesTextEdit->setPlainText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed in the section names:"
                                                          "\n    ,  &  <  >"
                                                          "\nOther punctuation is allowed.\nPut each section's name on a new line."));
    }

    // split the input at every newline, then remove any blanks (including just spaces)
    sectionNames = currText.split("\n");
    for (int line = 0; line < sectionNames.size(); line++)
    {
        sectionNames[line] = sectionNames.at(line).trimmed();
    }
    sectionNames.removeAll(QString(""));

    refreshPreview();
}

void SurveyMaker::on_preferredTeammatesCheckBox_clicked(bool checked)
{
    preferredTeammates = checked;
    ui->numAllowedLabel->setEnabled(preferredTeammates || preferredNonTeammates);
    ui->numAllowedSpinBox->setEnabled(preferredTeammates || preferredNonTeammates);
    refreshPreview();
}

void SurveyMaker::on_preferredNonTeammatesCheckBox_clicked(bool checked)
{
    preferredNonTeammates = checked;
    ui->numAllowedLabel->setEnabled(preferredTeammates || preferredNonTeammates);
    ui->numAllowedSpinBox->setEnabled(preferredTeammates || preferredNonTeammates);
    refreshPreview();
}

void SurveyMaker::on_numAllowedSpinBox_valueChanged(int arg1)
{
    numPreferredAllowed = arg1;
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
                auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(attribute));
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Question") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Question"].isString())
                {
                     attributeTexts[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"Question"].toString();
                     attribTab->attributeText->setPlainText(attributeTexts[attribute]);
                }
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Response") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Response"].isDouble())
                {
                     attributeResponses[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"Response"].toInt();
                     attribTab->attributeResponses->setCurrentIndex(attributeResponses[attribute] + 1);
                }
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"AllowMultiResponse") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"AllowMultiResponse"].isBool())
                {
                     attributeAllowMultipleResponses[attribute] = loadObject["Attribute" + QString::number(attribute+1)+"AllowMultiResponse"].toBool();
                     attribTab->allowMultipleResponses->setChecked(attributeAllowMultipleResponses[attribute]);
                }
            }
            // show first attribute question
            ui->attributesTabWidget->setCurrentIndex(0);

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
            if(loadObject.contains("Timezone") && loadObject["Timezone"].isBool())
            {
                ui->timezoneCheckBox->setChecked(loadObject["Timezone"].toBool());
                on_timezoneCheckBox_clicked(loadObject["Timezone"].toBool());
            }
            if(loadObject.contains("baseTimezone") && loadObject["baseTimezone"].isString())
            {
                int index = baseTimezoneComboBox->findText(loadObject["baseTimezone"].toString());
                if(index == -1)
                {
                    ui->baseTimezoneLineEdit->setText(loadObject["baseTimezone"].toString());
                }
                else
                {
                    baseTimezoneComboBox->setCurrentIndex(index);
                }
            }
            for(int day = 0; day < MAX_DAYS; day++)
            {
                QString dayString1 = "scheduleDay" + QString::number(day+1);
                QString dayString2 = dayString1 + "Name";
                QString dayName = defaultDayNames[day];
                if(loadObject.contains(dayString2) && loadObject[dayString2].isString())
                {
                    dayLineEdits[day]->setText(loadObject[dayString2].toString());
                    dayName = loadObject[dayString2].toString();
                }

                if(loadObject.contains(dayString1) && loadObject[dayString1].isBool())
                {
                    dayCheckBoxes[day]->setChecked(loadObject[dayString1].toBool());
                    day_CheckBox_toggled(loadObject[dayString1].toBool(), dayLineEdits[day], dayName);
                }
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

            if(loadObject.contains("PreferredTeammates") && loadObject["PreferredTeammates"].isBool())
            {
                ui->preferredTeammatesCheckBox->setChecked(loadObject["PreferredTeammates"].toBool());
                on_preferredTeammatesCheckBox_clicked(loadObject["PreferredTeammates"].toBool());
            }
            if(loadObject.contains("PreferredNonTeammates") && loadObject["PreferredNonTeammates"].isBool())
            {
                ui->preferredNonTeammatesCheckBox->setChecked(loadObject["PreferredNonTeammates"].toBool());
                on_preferredNonTeammatesCheckBox_clicked(loadObject["PreferredNonTeammates"].toBool());
            }
            if(loadObject.contains("numPrefTeammates") && loadObject["numPrefTeammates"].isDouble())
            {
                ui->numAllowedSpinBox->setValue(loadObject["numPrefTeammates"].toInt());
                on_numAllowedSpinBox_valueChanged(loadObject["numPrefTeammates"].toInt());
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
                saveObject["Attribute" + QString::number(attribute+1)+"AllowMultiResponse"] = attributeAllowMultipleResponses[attribute];
            }
            saveObject["Schedule"] = schedule;
            saveObject["ScheduleAsBusy"] = (busyOrFree == busy);
            saveObject["Timezone"] = timezone;
            saveObject["baseTimezone"] = baseTimezone;
            //below is not performed with a for-loop because checkboxes are not in an array
            for(int day = 0; day < MAX_DAYS; day++)
            {
                QString dayString = "scheduleDay" + QString::number(day+1);
                saveObject[dayString] = dayCheckBoxes[day]->isChecked();
                dayString = "scheduleDay" + QString::number(day+1) + "Name";
                saveObject[dayString] = dayNames[day];
            }
            saveObject["scheduleStartHour"] = startTime;
            saveObject["scheduleEndHour"] = endTime;
            saveObject["Section"] = section;
            saveObject["SectionNames"] = sectionNames.join(',');
            saveObject["PreferredTeammates"] = preferredTeammates;
            saveObject["PreferredNonTeammates"] = preferredNonTeammates;
            saveObject["numPrefTeammates"] = numPreferredAllowed;
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
    helpWindow.resize(LG_DLG_SIZE, LG_DLG_SIZE);
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
                              "<ul><li>Code libraries from <a href = http://qt.io>Qt, v 5.15</a>, released under the GNU Lesser General Public License version 3</li>"
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
