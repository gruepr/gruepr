#include "ui_surveymaker.h"
#include "surveymaker.h"
#include "widgets/attributeTabItem.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QResizeEvent>
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

    //Restore window geometry
    QSettings savedSettings;
    restoreGeometry(savedSettings.value("surveyMakerWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

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
    connect(ui->actiongruepr_Homepage, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl(GRUEPRHOMEPAGE));});
    connect(ui->actionBugReport, &QAction::triggered, this, [] {QDesktopServices::openUrl(QUrl(BUGREPORTPAGE));});

    //Connect the simple UI interactions to a simple refresh of survey data
    connect(ui->surveyTitleLineEdit, &QLineEdit::textChanged, this, &SurveyMaker::buildSurvey);
    connect(ui->firstNameCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->lastNameCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->emailCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->genderCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->URMCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->busyFreeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SurveyMaker::buildSurvey);
    connect(ui->sectionCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->preferredTeammatesCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->preferredNonTeammatesCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->numAllowedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SurveyMaker::buildSurvey);
    connect(ui->additionalQuestionsCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);

    //Create RegEx for punctuation not allowed within a URL (can remove if/when changing the form data upload to be a POST instead of GET
    noInvalidPunctuation = new QRegularExpressionValidator(QRegularExpression("[^,&<>/]*"), this);

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
        auto *attributeTab = new attributeTabItem(attributeTabItem::surveyMaker, tab, this);
        connect(attributeTab->attributeText, &QTextEdit::textChanged, this, &SurveyMaker::attributeTextChanged);
        connect(attributeTab->attributeResponses, QOverload<int>::of(&ComboBoxWithElidedContents::currentIndexChanged), this, &SurveyMaker::buildSurvey);
        connect(attributeTab->allowMultipleResponses, &QCheckBox::toggled, this, &SurveyMaker::buildSurvey);
        connect(attributeTab, &attributeTabItem::closeRequested, this, &SurveyMaker::attributeTabClose);

        ui->attributesTabWidget->addTab(attributeTab, QString::number(tab+1) + "   ");
        ui->attributesTabWidget->setTabVisible(tab, tab < numAttributes);
    }
    refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
    responseOptions.prepend(tr("custom options, to be added after creating the form"));
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::currentChanged, this, &SurveyMaker::refreshAttributeTabBar);
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::tabMoved, this, &SurveyMaker::attributeTabBarMoveTab);

    //create a survey
    survey = new Survey;
    buildSurvey();
}

SurveyMaker::~SurveyMaker()
{
    delete ui;
}

void SurveyMaker::resizeEvent(QResizeEvent *event)
{
    event->accept();
    refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
}

void SurveyMaker::buildSurvey()
{    
    // Title
    QString currText = ui->surveyTitleLineEdit->text();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        badExpression(ui->surveyTitleLineEdit, currText);
    }
    survey->title = currText.trimmed();

    survey->questions.clear();

    // First name
    firstname = ui->firstNameCheckBox->isChecked();
    if(firstname)
    {
        survey->questions << Question(FIRSTNAMEQUESTION, Question::shorttext);
    }

    // Last name
    lastname = ui->lastNameCheckBox->isChecked();
    if(lastname)
    {
        survey->questions << Question(LASTNAMEQUESTION, Question::shorttext);
    }

    // Email
    email = ui->emailCheckBox->isChecked();
    if(email)
    {
        survey->questions << Question(EMAILQUESTION, Question::shorttext);
    }

    // Gender
    gender = ui->genderCheckBox->isChecked();
    if(gender)
    {
        survey->questions << Question(GENDERQUESTION, Question::radiobutton, GENDEROPTIONS.split('/'));
    }

    // URM
    URM = ui->URMCheckBox->isChecked();
    if(URM)
    {
        survey->questions << Question(URMQUESTION, Question::shorttext);
    }

    // Attributes (including timezone if no schedule question)
    numAttributes = ui->attributeCountSpinBox->value();
    if(timezone && !schedule)
    {
        numAttributes++;
    }
    for(int attrib = 0; attrib < MAX_ATTRIBUTES; attrib++)
    {
        auto *attribTab = qobject_cast<attributeTabItem*>(ui->attributesTabWidget->widget(attrib));
        attributeTexts[attrib] = attribTab->attributeText->toPlainText().simplified();
        attributeResponses[attrib] = ((attribTab->attributeResponses->currentIndex()>1) ? (attribTab->attributeResponses->currentIndex()-1) : 0);
        attributeAllowMultipleResponses[attrib] = attribTab->allowMultipleResponses->isChecked();
        QString questionText = (attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : attributeTexts[attrib]);
        QString options = responseOptions.at(attributeResponses[attrib]);
        Question::QuestionType questionType = Question::radiobutton;
        if(attributeAllowMultipleResponses[attrib])
        {
            questionType = Question::checkbox;
        }

        if(timezone && !schedule && attrib == (numAttributes-1))
        {
            questionText = TIMEZONEQUESTION;
            options = TIMEZONEOPTIONS;
            questionType = Question::dropdown;
        }

        if(attrib < numAttributes)
        {
            survey->questions << Question(questionText, questionType, options.split('/'));
        }
    }

    // Schedule (including timezone if applicable)
    busyOrFree = ((ui->busyFreeComboBox->currentText() == tr("busy"))? busy : free);
    if(schedule)
    {
        if(timezone)
        {
            survey->questions << Question(TIMEZONEQUESTION, Question::dropdown, TIMEZONEOPTIONS.split('/'));
        }

        QString questionText = SCHEDULEQUESTION1 + ((busyOrFree == busy)? SCHEDULEQUESTION2BUSY : SCHEDULEQUESTION2FREE) + SCHEDULEQUESTION3;
        if(timezone)
        {
            questionText += SCHEDULEQUESTION4;
            if(baseTimezone.isEmpty())
            {
                questionText += SCHEDULEQUESTIONHOME;
            }
            else
            {
                questionText += baseTimezone;
            }
            questionText += SCHEDULEQUESTION5;
        }
        survey->questions << Question(questionText, Question::schedule);
        for(int day = 0; day < MAX_DAYS; day++)
        {
            survey->schedDayNames[day] = dayNames[day];
        }
        survey->schedStartTime = startTime;
        survey->schedEndTime = endTime;
    }

    // Section
    section = ui->sectionCheckBox->isChecked();
    ui->sectionNamesTextEdit->setEnabled(section);
    if(section)
    {
        QStringList options;
        for(int sect = 0; sect < sectionNames.size(); sect++)
        {
            options << sectionNames[sect];
        }
        survey->questions << Question(SECTIONQUESTION, Question::radiobutton, options);
    }

    // Preferred teammates and / or non-teammates
    preferredTeammates = ui->preferredTeammatesCheckBox->isChecked();
    preferredNonTeammates = ui->preferredNonTeammatesCheckBox->isChecked();
    ui->numAllowedLabel->setEnabled(preferredTeammates || preferredNonTeammates);
    ui->numAllowedSpinBox->setEnabled(preferredTeammates || preferredNonTeammates);
    numPreferredAllowed = ui->numAllowedSpinBox->value();
    if(preferredTeammates)
    {
        if(numPreferredAllowed == 1)
        {
            survey->questions << Question(PREF1TEAMMATEQUESTION, Question::shorttext);
        }
        else
        {
            survey->questions << Question(PREFMULTQUESTION1 + QString::number(numPreferredAllowed) + PREFMULTQUESTION2YES, Question::longtext);
        }
    }
    if(preferredNonTeammates)
    {
        if(numPreferredAllowed == 1)
        {
            survey->questions << Question(PREF1NONTEAMMATEQUESTION, Question::shorttext);
        }
        else
        {
            survey->questions << Question(PREFMULTQUESTION1 + QString::number(numPreferredAllowed) + PREFMULTQUESTION2NO, Question::longtext);
        }
    }

    // Additional questions
    additionalQuestions = ui->additionalQuestionsCheckBox->isChecked();
    if(additionalQuestions)
    {
        survey->questions << Question(ADDLQUESTION, Question::longtext);
    }

    currPos = ui->previewText->verticalScrollBar()->value();
    ui->previewText->setHtml(createPreview(survey));
    ui->previewText->verticalScrollBar()->setValue(currPos);
}

QString SurveyMaker::createPreview(const Survey *const survey)
{
    // generate preview text and survey object
    QString preview;
    if(!survey->title.isEmpty())
    {
        preview += "<h2>" + survey->title + "</h2>";
    }

    for(const auto &question : qAsConst(survey->questions))
    {
        preview += QUESTIONPREVIEWHEAD + question.text;
        if((question.type == Question::dropdown) || (question.type == Question::radiobutton) || (question.type == Question::checkbox))
        {
            preview += QUESTIONOPTIONSHEAD + question.options.join(" </i><b>|</b><i> ") + QUESTIONOPTIONSTAIL;
            if(question.type == Question::checkbox)
            {
                preview += "</b><br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>* " + tr("Multiple responses allowed") + "</small>";
            }
        }
        else if(question.type == Question::schedule)
        {
            preview += "</p><p>&nbsp;&nbsp;&nbsp;<i>grid of checkboxes:</i></p>";
            preview += "<p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>";
            for(int time = survey->schedStartTime; time <= survey->schedEndTime; time++)
            {
                preview += QTime(time, 0).toString("hA");
                if(time != survey->schedEndTime)
                {
                    preview += "&nbsp;&nbsp;&nbsp;";
                }
            }
            preview += "</small></p>";
            for(const auto &dayName : qAsConst(survey->schedDayNames))
            {
                if(!(dayName.isEmpty()))
                {
                    preview += "<p>&nbsp;&nbsp;&nbsp;<small>" + dayName + "</small></p>";
                }
            }
        }
        preview += QUESTIONPREVIEWTAIL;
    }
    return preview;
}

void SurveyMaker::badExpression(QWidget *textWidget, QString &currText)
{
    auto *lineEdit = qobject_cast<QLineEdit*>(textWidget);
    if(lineEdit != nullptr)
    {
        lineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
    }
    else
    {
        auto *textEdit = qobject_cast<QTextEdit*>(textWidget);
        if(textEdit != nullptr)
        {
            textEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
        }
    }
    QApplication::beep();
    QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed:\n"
                                                      "    ,  &  <  > / \n"
                                                      "Other punctuation is allowed."));
}

void SurveyMaker::on_makeSurveyButton_clicked()
{
     if(!survey->isValid())
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
        if(fileName.isEmpty())
        {
            QMessageBox::critical(survey, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
            return;
        }
        //create the files
        QFile saveFile(fileName + ".txt"), saveFile2(fileName + ".csv");
        if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text) && saveFile2.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            int questionNumber = 0, sectionNumber = 0;
            QString textFileContents = survey->survey->title + "\n\n";
            QString csvFileContents = "Timestamp";

            textFileContents += tr("Your survey (and/or other data sources) should collect the following information to paste into the csv file \"") + fileName + ".csv\":";
            textFileContents += "\n\n\n" + tr("Section ") + QString::number(++sectionNumber) + ", " + tr("Basic Information") + ":";

            if(survey->firstname)
            {
                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += FIRSTNAMEQUESTION;
                csvFileContents += "," + FIRSTNAMEQUESTION;
            }

            if(survey->lastname)
            {
                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += LASTNAMEQUESTION;
                csvFileContents += "," + LASTNAMEQUESTION;
            }

            if(survey->email)
            {
                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += EMAILQUESTION;
                csvFileContents += "," + EMAILQUESTION;
            }

            if(survey->gender)
            {
                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += GENDERQUESTION;
                textFileContents += "\n     " + tr("choices: [") + GENDEROPTIONS.split('/').join(" | ") + "]";
                csvFileContents += "," + GENDERQUESTION;
            }

            if(survey->URM)
            {
                textFileContents += "\n\n  " + QString::number(++questionNumber) + ") ";
                textFileContents += URMQUESTION;
                csvFileContents += ",\"" + URMQUESTION + "\"";
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

void SurveyMaker::on_attributeCountSpinBox_valueChanged(int arg1)
{
    ui->attributesTabWidget->setTabEnabled(0, 0 < arg1);
    for(int i = 1; i < MAX_ATTRIBUTES; i++)
    {
        ui->attributesTabWidget->setTabVisible(i, i < arg1);
    }

    refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
    buildSurvey();
}

void SurveyMaker::refreshAttributeTabBar(int index)
{
    auto &tabs = ui->attributesTabWidget;

    if(tabs->count() < 2)
    {
        return;
    }

    // first figure out which tabs are visible
    int firstVisibleIndex = 0;
    while(!tabs->isTabVisible(firstVisibleIndex))
    {
        firstVisibleIndex++;
    }

    const int widthOfTabWidget = tabs->size().width();
    int lastVisibleIndex = firstVisibleIndex;
    // count up tabs until we hit one whose right boundary exceeds that of the tabWidget itself (or we hit the last possible index)
    while((tabs->tabBar()->tabRect(lastVisibleIndex).right() < widthOfTabWidget) && (lastVisibleIndex < numAttributes))
    {
        lastVisibleIndex++;
    }
    auto lastTabGeom = tabs->tabBar()->tabRect(lastVisibleIndex);
    // move back one if this last tab is less than 1/2 visible or if we've exceeded the number of tabs possible
    if((((lastTabGeom.left() + lastTabGeom.right()) / 2) > widthOfTabWidget) || (lastVisibleIndex == numAttributes))
    {
        lastVisibleIndex--;
        lastTabGeom = tabs->tabBar()->tabRect(lastVisibleIndex);
    }

    if((firstVisibleIndex != 0) && (index == firstVisibleIndex))
    {
        // expanding one or two tabs to left if there are previous tabs to expand the current tab is the first visible
        tabs->setTabVisible(firstVisibleIndex-1, true);
        firstVisibleIndex--;
        if(firstVisibleIndex != 0)
        {
            tabs->setTabVisible(firstVisibleIndex-1, true);
            firstVisibleIndex--;
        }
    }
    else if((firstVisibleIndex != 0) && (lastVisibleIndex == numAttributes - 1) && (lastTabGeom.right() + lastTabGeom.width() < widthOfTabWidget))
    {
        // expanding one tab to the left if there are previous tabs to expand and there's simply enough room now due to closing a tab or expanding the window
        tabs->setTabVisible(firstVisibleIndex-1, true);
        firstVisibleIndex--;
    }
    else if((lastVisibleIndex != numAttributes-1) && (index >= lastVisibleIndex))
    {
        // expanding one or two tabs to right if there are subsequent tabs to expand and the current one is the last one visible
        tabs->setTabVisible(firstVisibleIndex, false);
        firstVisibleIndex++;
        lastVisibleIndex++;
        if(lastVisibleIndex != numAttributes-1)
        {
            tabs->setTabVisible(firstVisibleIndex, false);
            firstVisibleIndex++;
        }
    }

    // redetermine the last visible index now for the sake of labeling the tabs
    lastVisibleIndex = firstVisibleIndex;
    // count up tabs until we hit one whose right boundary exceeds that of the tabWidget itself (or we hit the last possible index)
    while((tabs->tabBar()->tabRect(lastVisibleIndex).right() < widthOfTabWidget) && (lastVisibleIndex < numAttributes))
    {
        lastVisibleIndex++;
    }
    // move back one if this last tab is less than 1/2 visible or if we've exceeded the number of tabs possible
    if((((tabs->tabBar()->tabRect(lastVisibleIndex).left() + tabs->tabBar()->tabRect(lastVisibleIndex).right()) / 2) > widthOfTabWidget)
            || (lastVisibleIndex == numAttributes))
    {
        lastVisibleIndex--;
    }

    // reset the label for every tab, with 'scroll triangles' on the ends if needed
    for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
    {
        QString label;
        if((tab != 0) && (tab == firstVisibleIndex))
        {
            label = QString(LEFTARROW) + " " + QString::number(tab+1);
        }
        else if((tab != numAttributes-1) && (tab == lastVisibleIndex))
        {
            label = QString::number(tab+1) + " " + QString(RIGHTARROW);
        }
        else
        {
            label = QString::number(tab+1) + "   ";
        }
        tabs->setTabText(tab, label);
    }
}

void SurveyMaker::attributeTabBarMoveTab(int /*indexFrom*/, int /*indexTo*/)
{
    // reset for every tab the internal index
    for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
    {
        auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(tab));
        attribTab->setTabNum(tab);
    }

    refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
    buildSurvey();
}

void SurveyMaker::attributeTabClose(int index)
{
    const int currIndex = ui->attributesTabWidget->currentIndex();

    // if this is the last attribute, closing it should just be the same as if clicking down to 0 attributes; otherwise, actually delete this one
    if(ui->attributeCountSpinBox->value() != 1)
    {
        // remove and delete this tab, then add a new one to the end
        auto *oldWidget = ui->attributesTabWidget->widget(index);
        ui->attributesTabWidget->removeTab(index);
        oldWidget->deleteLater();

        auto *attributeTab = new attributeTabItem(attributeTabItem::surveyMaker, MAX_ATTRIBUTES-1, this);
        connect(attributeTab->attributeText, &QTextEdit::textChanged, this, &SurveyMaker::attributeTextChanged);
        connect(attributeTab->attributeResponses, QOverload<int>::of(&ComboBoxWithElidedContents::currentIndexChanged), this, &SurveyMaker::buildSurvey);
        connect(attributeTab->allowMultipleResponses, &QCheckBox::toggled, this, &SurveyMaker::buildSurvey);
        connect(attributeTab, &attributeTabItem::closeRequested, this, &SurveyMaker::attributeTabClose);

        ui->attributesTabWidget->addTab(attributeTab, QString::number(MAX_ATTRIBUTES) + "   ");

        // reset for every tab the placeholder text
        for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
        {
            auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(tab));
            attribTab->setTabNum(tab);
        }
        refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
    }

    // keep the current tab open unless it is the one being closed; if it is being closed, open the most sensible one
    if((index == currIndex) && (currIndex == numAttributes - 1))
    {
        // the open tab is being closed and is the last visible one, so open the next one down
        ui->attributesTabWidget->setCurrentIndex(currIndex-1);
    }
    else if(index < currIndex)
    {
        // the tab being closed is below the open one, so the current one's index has decreased
        ui->attributesTabWidget->setCurrentIndex(currIndex-1);
    }
    else
    {
        // the tab being closed is the current one or is above the current one, so keep this index open
        ui->attributesTabWidget->setCurrentIndex(index < currIndex? currIndex-1 : currIndex);
    }

    ui->attributeCountSpinBox->setValue(numAttributes - 1);
}

void SurveyMaker::attributeTextChanged()
{
    const int currAttribute = ui->attributesTabWidget->currentIndex();
    //validate entry
    auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(currAttribute));
    QString currText = attribTab->attributeText->toPlainText();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        badExpression(attribTab->attributeText, currText);
    }
    else if(currText.contains(tr("In which section are you enrolled"), Qt::CaseInsensitive))
    {
        attribTab->attributeText->setPlainText(currText.replace(tr("In which section are you enrolled"), tr("_"), Qt::CaseInsensitive));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, attribute questions may not containt the exact wording:\n"
                                                          "\"In which section are you enrolled\"\nwithin the question text.\n"
                                                          "A section question may be added using the \"Section\" checkbox."));
    }

    buildSurvey();
}

void SurveyMaker::on_timezoneCheckBox_clicked(bool checked)
{
    timezone = checked;

    checkTimezoneAndSchedule();

    buildSurvey();
}

void SurveyMaker::on_scheduleCheckBox_clicked(bool checked)
{
    schedule = checked;

    ui->busyFreeLabel->setEnabled(checked);
    ui->busyFreeComboBox->setEnabled(checked);
    ui->daysComboBox->setEnabled(checked);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        dayCheckBoxes[day]->setEnabled(checked);
        dayLineEdits[day]->setEnabled(checked && dayCheckBoxes[day]->isChecked());
    }
    ui->timeStartEdit->setEnabled(checked);
    ui->timeEndEdit->setEnabled(checked);

    checkTimezoneAndSchedule();

    buildSurvey();
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
    bool timezoneAndSchedule = timezone && schedule;
    baseTimezoneComboBox->setEnabled(timezoneAndSchedule);
    ui->baseTimezoneLineEdit->setEnabled(timezoneAndSchedule);
    baseTimezoneComboBox->setItemText(TimezoneType::noneOrHome, timezoneAndSchedule? tr("[student's home timezone]") : tr("[no timezone given]"));
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

    buildSurvey();
}

void SurveyMaker::on_baseTimezoneLineEdit_textChanged()
{
    baseTimezoneComboBox->setCurrentIndex(TimezoneType::custom);

    //validate entry
    QString currText = ui->baseTimezoneLineEdit->text();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        badExpression(ui->baseTimezoneLineEdit, currText);
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
    buildSurvey();
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
        badExpression(dayLineEdit, currText);
    }
    dayname = currText.trimmed();
    buildSurvey();
}

void SurveyMaker::on_timeStartEdit_timeChanged(QTime time)
{
    startTime = time.hour();
    if(ui->timeEndEdit->time() <= time)
    {
        ui->timeEndEdit->setTime(QTime(time.hour(), 0));
    }
    buildSurvey();
}

void SurveyMaker::on_timeEndEdit_timeChanged(QTime time)
{
    endTime = time.hour();
    if(ui->timeStartEdit->time() >= time)
    {
        ui->timeStartEdit->setTime(QTime(time.hour(), 0));
    }
    buildSurvey();
}

void SurveyMaker::on_sectionNamesTextEdit_textChanged()
{
    //validate entry
    QString currText = ui->sectionNamesTextEdit->toPlainText();
    int currPos = 0;
    if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
    {
        badExpression(ui->sectionNamesTextEdit, currText);
    }

    // split the input at every newline, then remove any blanks (including just spaces)
    sectionNames = currText.split("\n");
    for (int line = 0; line < sectionNames.size(); line++)
    {
        sectionNames[line] = sectionNames.at(line).trimmed();
    }
    sectionNames.removeAll(QString(""));

    buildSurvey();
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
            }
            if(loadObject.contains("URM") && loadObject["URM"].isBool())
            {
                ui->URMCheckBox->setChecked(loadObject["URM"].toBool());
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
                ui->scheduleCheckBox->setChecked(loadObject["Schedule"].toBool());
                on_scheduleCheckBox_clicked(loadObject["Schedule"].toBool());
            }
            if(loadObject.contains("ScheduleAsBusy") && loadObject["ScheduleAsBusy"].isBool())
            {
                ui->busyFreeComboBox->setCurrentText(loadObject["ScheduleAsBusy"].toBool()? "busy" : "free");
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
            }
            if(loadObject.contains("SectionNames") && loadObject["SectionNames"].isString())
            {
                ui->sectionNamesTextEdit->setPlainText(loadObject["SectionNames"].toString().replace(',', '\n'));
            }

            if(loadObject.contains("PreferredTeammates") && loadObject["PreferredTeammates"].isBool())
            {
                ui->preferredTeammatesCheckBox->setChecked(loadObject["PreferredTeammates"].toBool());
            }
            if(loadObject.contains("PreferredNonTeammates") && loadObject["PreferredNonTeammates"].isBool())
            {
                ui->preferredNonTeammatesCheckBox->setChecked(loadObject["PreferredNonTeammates"].toBool());
            }
            if(loadObject.contains("numPrefTeammates") && loadObject["numPrefTeammates"].isDouble())
            {
                ui->numAllowedSpinBox->setValue(loadObject["numPrefTeammates"].toInt());
            }

            if(loadObject.contains("AdditionalQuestions") && loadObject["AdditionalQuestions"].isBool())
            {
                ui->additionalQuestionsCheckBox->setChecked(loadObject["AdditionalQuestions"].toBool());
            }
            loadFile.close();

            buildSurvey();
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
            saveObject["Title"] = survey->title;
            saveObject["FirstName"] = firstname;
            saveObject["LastName"] = lastname;
            saveObject["Email"] = email;
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
                            "<p>Joshua Hertz <a href = mailto:info@gruepr.com>info@gruepr.com</a>"
                            "<p>Project homepage: <a href = ") + GRUEPRHOMEPAGE + ">" + GRUEPRHOMEPAGE + "</a>");
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
                          "<br>Joshua Hertz<br><a href = mailto:info@gruepr.com>info@gruepr.com</a>"
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://gruepr.com>gruepr.com</a>."
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
