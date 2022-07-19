#include "ui_surveymaker.h"
#include "surveymaker.h"
#include "dialogs/customResponseOptionsDialog.h"
#include "widgets/attributeTabItem.h"
#include <QClipboard>
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
                                                                     generateSurvey = &SurveyMaker::createGoogleForm;
                                                                     on_makeSurveyButton_clicked();
                                                                     generateSurvey = prevSurveyMethod;});
    connect(ui->actionText_files, &QAction::triggered, this, [this]{auto prevSurveyMethod = generateSurvey;
                                                                     generateSurvey = &SurveyMaker::createFiles;
                                                                     on_makeSurveyButton_clicked();
                                                                     generateSurvey = prevSurveyMethod;});
    connect(ui->actionCanvas_quiz, &QAction::triggered, this, [this]{auto prevSurveyMethod = generateSurvey;
                                                                     generateSurvey = &SurveyMaker::createCanvasQuiz;
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
    connect(ui->genderResponsesComboBox, &QComboBox::currentTextChanged, this, &SurveyMaker::buildSurvey);
    connect(ui->URMCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->timezoneCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->scheduleCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->busyFreeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SurveyMaker::buildSurvey);
    connect(ui->timeStartEdit, &QTimeEdit::timeChanged, this, &SurveyMaker::buildSurvey);
    connect(ui->timeEndEdit, &QTimeEdit::timeChanged, this, &SurveyMaker::buildSurvey);
    connect(ui->sectionCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->preferredTeammatesCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->preferredNonTeammatesCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);
    connect(ui->numAllowedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SurveyMaker::buildSurvey);
    connect(ui->additionalQuestionsCheckBox, &QPushButton::toggled, this, &SurveyMaker::buildSurvey);

    //Create RegEx for punctuation not allowed within a URL
    noInvalidPunctuation = new QRegularExpressionValidator(QRegularExpression("[^,&<>/]*"), this);

    //put timezones into combobox
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->insertItem(TimezoneType::noneOrHome, tr("[no timezone given]"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::noneOrHome+1);
    baseTimezoneComboBox->insertItem(TimezoneType::custom, tr("Custom timezone:"));
    baseTimezoneComboBox->insertSeparator(TimezoneType::custom+1);
    for(int zone = 0; zone < timeZoneNames.size(); zone++)
    {
        const QString &zonename = timeZoneNames.at(zone);
        baseTimezoneComboBox->insertItem(TimezoneType::set + zone, zonename);
        baseTimezoneComboBox->setItemData(TimezoneType::set + zone, zonename, Qt::ToolTipRole);
    }
    ui->scheduleLayout->replaceWidget(ui->fillinTimezoneComboBox, baseTimezoneComboBox, Qt::FindChildrenRecursively);
    ui->fillinTimezoneComboBox->setParent(nullptr);
    ui->fillinTimezoneComboBox->deleteLater();
    connect(baseTimezoneComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SurveyMaker::baseTimezoneComboBox_currentIndexChanged);

    //load in local day names, and connect subwindow ui to slots
    defaultDayNames.reserve(MAX_DAYS);
    dayNames.reserve(MAX_DAYS);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        defaultDayNames << sunday.addDays(day).toString("dddd");
        dayNames << defaultDayNames.at(day);
        dayCheckBoxes[day] = new QCheckBox;
        dayLineEdits[day] = new QLineEdit;
        dayCheckBoxes[day]->setChecked(true);
        dayLineEdits[day]->setText(defaultDayNames.at(day));
        dayLineEdits[day]->setPlaceholderText(tr("Day ") + QString::number(day + 1) + tr(" name"));
        connect(dayLineEdits[day], &QLineEdit::textChanged, this, [this, day](const QString &text) {day_LineEdit_textChanged(text, dayLineEdits[day], dayNames[day]);});
        connect(dayLineEdits[day], &QLineEdit::editingFinished, this, [this, day] {if(dayNames.at(day).isEmpty()){dayCheckBoxes[day]->setChecked(false);};});
        connect(dayCheckBoxes[day], &QCheckBox::toggled, this, [this, day](bool checked) {day_CheckBox_toggled(checked, dayLineEdits[day], defaultDayNames.at(day));});
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
        connect(attributeTab->attributeResponses, QOverload<int>::of(&QComboBox::activated), this, &SurveyMaker::attributeResponseChanged);
        connect(attributeTab->allowMultipleResponses, &QCheckBox::toggled, this, &SurveyMaker::buildSurvey);
        connect(attributeTab, &attributeTabItem::closeRequested, this, &SurveyMaker::attributeTabClose);

        ui->attributesTabWidget->addTab(attributeTab, QString::number(tab+1) + "   ");
        ui->attributesTabWidget->setTabVisible(tab, tab < numAttributes);
    }
    refreshAttributeTabBar(ui->attributesTabWidget->currentIndex());
    responseOptions.prepend(tr(" "));
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::currentChanged, this, &SurveyMaker::refreshAttributeTabBar);
    connect(ui->attributesTabWidget->tabBar(), &QTabBar::tabMoved, this, &SurveyMaker::attributeTabBarMoveTab);

    //create a survey
    survey = new Survey;
    buildSurvey();
}

SurveyMaker::~SurveyMaker()
{
    delete canvas;
    delete google;
    delete noInvalidPunctuation;
    delete survey;
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
        survey->questions << Question(FIRSTNAMEQUESTION, Question::QuestionType::shorttext);
    }

    // Last name
    lastname = ui->lastNameCheckBox->isChecked();
    if(lastname)
    {
        survey->questions << Question(LASTNAMEQUESTION, Question::QuestionType::shorttext);
    }

    // Email
    email = ui->emailCheckBox->isChecked();
    if(email)
    {
        survey->questions << Question(EMAILQUESTION, Question::QuestionType::shorttext);
    }

    // Gender
    gender = ui->genderCheckBox->isChecked();
    ui->genderResponsesComboBox->setEnabled(gender);
    ui->genderResponsesLabel->setEnabled(gender);
    genderType = static_cast<GenderType>(ui->genderResponsesComboBox->currentIndex());
    if(gender)
    {
        QString genderQuestion;
        QStringList genderOptions;
        if(genderType == GenderType::biol)
        {
            genderQuestion = GENDERQUESTION;
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(genderType == GenderType::adult)
        {
            genderQuestion = GENDERQUESTION;
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(genderType == GenderType::child)
        {
            genderQuestion = GENDERQUESTION;
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else //if(genderType == GenderType::pronoun)
        {
            genderQuestion = PRONOUNQUESTION;
            genderOptions = QString(PRONOUNS).split('/');
        }
        genderOptions.replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
        survey->questions << Question(genderQuestion, Question::QuestionType::dropdown, genderOptions);
    }

    // URM
    URM = ui->URMCheckBox->isChecked();
    if(URM)
    {
        survey->questions << Question(URMQUESTION, Question::QuestionType::shorttext);
    }

    // Attributes (including timezone if no schedule question)
    numAttributes = ui->attributeCountSpinBox->value();
    timezone = ui->timezoneCheckBox->isChecked();
    schedule = ui->scheduleCheckBox->isChecked();
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
        QString questionText = (attributeTexts[attrib].isEmpty()? "{" + tr("Attribute question ") + QString::number(attrib+1) + "}" : attributeTexts[attrib]);
        QStringList options = attribTab->attributeResponses->currentData().toStringList();
        Question::QuestionType questionType = Question::QuestionType::radiobutton;
        if(attributeAllowMultipleResponses[attrib])
        {
            questionType = Question::QuestionType::checkbox;
        }

        if(timezone && !schedule && attrib == (numAttributes-1))
        {
            questionText = TIMEZONEQUESTION;
            options = timeZoneNames;
            questionType = Question::QuestionType::dropdown;
        }

        if(attrib < numAttributes)
        {
            survey->questions << Question(questionText, questionType, options);
        }
    }

    // Schedule (including timezone if applicable)
    ui->busyFreeLabel->setEnabled(schedule);
    ui->busyFreeComboBox->setEnabled(schedule);
    busyOrFree = ((ui->busyFreeComboBox->currentText() == tr("busy"))? busy : free);
    ui->daysComboBox->setEnabled(schedule);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        dayCheckBoxes[day]->setEnabled(schedule);
        dayLineEdits[day]->setEnabled(schedule && dayCheckBoxes[day]->isChecked());
    }
    ui->timeStartEdit->setEnabled(schedule);
    ui->timeStartEdit->setMaximumTime(ui->timeEndEdit->time());
    ui->timeEndEdit->setEnabled(schedule);
    ui->timeEndEdit->setMinimumTime(ui->timeStartEdit->time());
    startTime = ui->timeStartEdit->time().hour();
    endTime = ui->timeEndEdit->time().hour();
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
    }
    // if asking students about timezone and schedule, should require survey to define timezone of schedule
    bool timezoneAndSchedule = timezone && schedule;
    baseTimezoneComboBox->setEnabled(timezoneAndSchedule);
    ui->baseTimezoneLineEdit->setEnabled(timezoneAndSchedule);
    baseTimezoneComboBox->setItemText(TimezoneType::noneOrHome, timezoneAndSchedule? tr("[student's home timezone]") : tr("[no timezone given]"));
    if(schedule)
    {
        if(timezone)
        {
            survey->questions << Question(TIMEZONEQUESTION, Question::QuestionType::dropdown, timeZoneNames);
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
        survey->questions << Question(questionText, Question::QuestionType::schedule);
        survey->schedDayNames.clear();
        for(int day = 0; day < MAX_DAYS; day++)
        {
            if(!dayNames.at(day).isEmpty())
            {
                survey->schedDayNames << dayNames.at(day);
            }
        }
        survey->schedStartTime = startTime;
        survey->schedEndTime = endTime;
    }

    // Section
    section = ui->sectionCheckBox->isChecked();
    ui->sectionNamesTextEdit->setEnabled(section);
    if(section)
    {
        survey->questions << Question(SECTIONQUESTION, Question::QuestionType::radiobutton, sectionNames);
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
            survey->questions << Question(PREF1TEAMMATEQUESTION, Question::QuestionType::shorttext);
        }
        else
        {
            survey->questions << Question(PREFMULTQUESTION1 + QString::number(numPreferredAllowed) + PREFMULTQUESTION2YES, Question::QuestionType::longtext);
        }
    }
    if(preferredNonTeammates)
    {
        if(numPreferredAllowed == 1)
        {
            survey->questions << Question(PREF1NONTEAMMATEQUESTION, Question::QuestionType::shorttext);
        }
        else
        {
            survey->questions << Question(PREFMULTQUESTION1 + QString::number(numPreferredAllowed) + PREFMULTQUESTION2NO, Question::QuestionType::longtext);
        }
    }

    // Additional questions
    additionalQuestions = ui->additionalQuestionsCheckBox->isChecked();
    if(additionalQuestions)
    {
        survey->questions << Question(ADDLQUESTION, Question::QuestionType::longtext);
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
        if((question.type == Question::QuestionType::dropdown) || (question.type == Question::QuestionType::radiobutton) || (question.type == Question::QuestionType::checkbox))
        {
            preview += QUESTIONOPTIONSHEAD;
            if(question.options.size() <= 10)
            {
                preview += question.options.join(" </i><b>|</b><i> ");
            }
            else
            {
                QStringList first6;
                for(int i = 0; i < 6; i++)
                {
                    first6 << question.options.at(i);

                }
                first6 << "...";
                preview += first6.join(" </i><b>|</b><i> ");
            }
            preview += QUESTIONOPTIONSTAIL;
            if(question.type == Question::QuestionType::checkbox)
            {
                preview += "</b><br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>* " + tr("Multiple responses allowed") + "</small>";
            }
        }
        else if(question.type == Question::QuestionType::schedule)
        {
            preview += "</p><p>&nbsp;&nbsp;&nbsp;<i>set of checkboxes:</i></p>";
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
                preview += "<p>&nbsp;&nbsp;&nbsp;<small>" + dayName + "</small></p>";
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
    if(section && sectionNames.isEmpty())
    {
        QMessageBox::critical(this, tr("Error!"), tr("The Section question is enabled, but no section "
                                                     "names have beed added for the students to select.\n"
                                                     "The survey has NOT been created."));
        return;
    }

    if(std::any_of(attributeTexts, attributeTexts + ui->attributeCountSpinBox->value(),
                   [](const QString &questionText){return questionText.isEmpty();}))
    {
        QMessageBox::critical(this, tr("Error!"), tr("One or more Attribute questions is blank.\n"
                                                     "The survey has NOT been created."));
        return;
    }

    if(std::any_of(survey->questions.constBegin(), survey->questions.constEnd(),
                   [](const Question &question){return (((question.type == Question::QuestionType::dropdown) ||
                                                         (question.type == Question::QuestionType::radiobutton) ||
                                                         (question.type == Question::QuestionType::checkbox)) &&
                                                         (question.options.size() <= 1));}))
    {
        QMessageBox::critical(this, tr("Error!"), tr("One or more Attribute questions has no response options.\n"
                                                     "The survey has NOT been created."));
        return;
    }

    generateSurvey(this);
}

void SurveyMaker::createFiles(SurveyMaker *surveyMaker)
{
    //give instructions about how this option works
    QMessageBox createSurvey;
    createSurvey.setIcon(QMessageBox::Information);
    createSurvey.setWindowTitle(tr("Survey Creation"));
    createSurvey.setText(tr("The next step will save two files to your computer:\n\n"
                            "  » A text file that lists the questions you should include in your survey.\n\n"
                            "  » A csv file that gruepr can read after you paste into it the survey data you receive."));
    createSurvey.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
    if(createSurvey.exec() == QMessageBox::Cancel)
    {
        return;
    }

    //get the filenames and location
    QString fileName = QFileDialog::getSaveFileName(surveyMaker, tr("Save File"), surveyMaker->saveFileLocation.canonicalFilePath(), tr("text and survey files (*);;All Files (*)"));
    if(fileName.isEmpty())
    {
        QMessageBox::critical(surveyMaker, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
        return;
    }
    //create the files
    QFile saveFile(fileName + ".txt"), saveFile2(fileName + ".csv");
    if(!saveFile.open(QIODevice::WriteOnly | QIODevice::Text) || !saveFile2.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(surveyMaker, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the files to disk."));
        return;
    }

    QString textFileContents, csvFileContents = "Timestamp";
    int questionNumber = 0;
    const Survey *const survey = surveyMaker->survey;
    if(!survey->title.isEmpty())
    {
        textFileContents += survey->title + "\n\n";
    }

    textFileContents += tr("Your survey (and/or other data sources) should collect the following information to paste into the csv file \"") + fileName + ".csv\":";
    for(const auto &question : qAsConst(survey->questions))
    {
        textFileContents += "\n\n  " + QString::number(++questionNumber) + ") " + question.text;
        if(question.type == Question::QuestionType::schedule)
        {
            textFileContents += "\n                 ";
            for(int time = survey->schedStartTime; time <= survey->schedEndTime; time++)
            {
                textFileContents += QTime(time, 0).toString("hA") + "    ";
            }
            textFileContents += "\n";
            for(const auto &dayName : qAsConst(survey->schedDayNames))
            {
                if(!(dayName.isEmpty()))
                {
                    textFileContents += "\n      " + dayName + "\n";
                    csvFileContents +=  "," + (question.text.contains(',')? "\"" + question.text + "\"" : question.text) + "[" + dayName + "]";
                }
            }
        }
        else
        {
            csvFileContents += "," + (question.text.contains(',')? "\"" + question.text + "\"" : question.text);
            if((question.type == Question::QuestionType::dropdown) || (question.type == Question::QuestionType::radiobutton) || (question.type == Question::QuestionType::checkbox))
            {
                textFileContents += "\n     " + tr("choices") + ": [" + question.options.join(" | ") + "]";
                if(question.type == Question::QuestionType::checkbox)
                {
                    textFileContents += "\n     (" + tr("Multiple responses allowed") + ")";
                }
            }
        }
    }

    //write the files
    QTextStream output(&saveFile), output2(&saveFile2);
    output << textFileContents;
    output2 << csvFileContents;
    saveFile.close();
    saveFile2.close();

    surveyMaker->surveyCreated = true;
}

void SurveyMaker::createGoogleForm(SurveyMaker *surveyMaker)
{
    if(!internetIsGood())
    {
        return;
    }

    //create googleHandler and/or authenticate as needed
    if(surveyMaker->google == nullptr)
    {
        surveyMaker->google = new GoogleHandler();
    }
    if(!surveyMaker->google->authenticated)
    {
        auto *loginDialog = new QMessageBox(surveyMaker);
        QPixmap icon(":/icons/google.png");
        loginDialog->setIconPixmap(icon.scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE));
        loginDialog->setText(tr("The next step will open a browser window so you can sign in with Google.\n\n"
                                "  » Your computer may ask whether gruepr can access the network. "
                                "This access is needed so that gruepr and Google can communicate.\n\n"
                                "  » In the browser, Google may ask whether you authorize gruepr to do 3 things: (1) create a Google Form, (2) create a Google Sheet, and (3) access these files on your Google Drive. "
                                "All 3 authorizations are needed so that the survey Form and the results Sheet can be created and saved in your Drive.\n\n"
                                "  » All data associated with this survey, including the questions asked and responses received, will exist in your Google Drive only. "
                                "No data from or about this survey will ever be stored or sent anywhere else."));
        loginDialog->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
        auto *okButton = loginDialog->button(QMessageBox::Ok);
        auto *cancelButton = loginDialog->button(QMessageBox::Cancel);
        int height = okButton->height();
        QPixmap loginpic(":/icons/google_signin_button.png");
        loginpic = loginpic.scaledToHeight(1.5*height, Qt::SmoothTransformation);
        okButton->setText("");
        okButton->setIconSize(loginpic.rect().size());
        okButton->setIcon(loginpic);
        okButton->adjustSize();
        QPixmap cancelpic(":/icons/cancel_signin_button.png");
        cancelpic = cancelpic.scaledToHeight(1.5*height, Qt::SmoothTransformation);
        cancelButton->setText("");
        cancelButton->setIconSize(cancelpic.rect().size());
        cancelButton->setIcon(cancelpic);
        cancelButton->adjustSize();
        if(loginDialog->exec() == QMessageBox::Cancel)
        {
            delete loginDialog;
            return;
        }

        surveyMaker->google->authenticate();

        loginDialog->setText(tr("Please use your browser to log in to Google and then return here."));
        loginDialog->setStandardButtons(QMessageBox::Cancel);
        connect(surveyMaker->google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
        if(loginDialog->exec() == QMessageBox::Cancel)
        {
            delete loginDialog;
            return;
        }
        delete loginDialog;
    }

    //upload the survey as a form then, if successful, finalize the form by sending to the finalize script
    auto *busyBox = surveyMaker->google->busy();
    QStringList URLs;
    auto form = surveyMaker->google->createSurvey(surveyMaker->survey);
    if(!form.name.isEmpty()) {
        URLs = surveyMaker->google->sendSurveyToFinalizeScript(form);
    }
    busyBox->hide();

    QPixmap icon;
    if(URLs.isEmpty()) {
        busyBox->setText(tr("Error. The survey was not created."));
        icon.load(":/icons/delete.png");
    }
    else {
        // append this survey to the saved values
        QSettings settings;
        int currentArraySize = settings.beginReadArray("GoogleForm");
        settings.endArray();
        settings.beginWriteArray("GoogleForm");
        settings.setArrayIndex(currentArraySize);
        settings.setValue("name", form.name);
        settings.setValue("ID", form.ID);
        settings.setValue("createdTime", form.createdTime);
        settings.setValue("downloadURL", URLs.at(2));
        settings.endArray();

        // add the URL to fill out the form to the clipboard
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(URLs.at(1));

        busyBox->setTextFormat(Qt::RichText);
        busyBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
        QApplication::restoreOverrideCursor();
        busyBox->setText(tr("Success! Survey created in your Google Drive.<br><br>"
                            "If you'd like to view or edit the survey, you can <a href='") + URLs.at(0) + tr("'>click here</a>.<br>"
                            "If you wish, you can modify:<br>"
                            "  » the survey title<br>"
                            "  » the instructions shown at the top of the survey<br>"
                            "  » the \"description text\" shown with any of the questions<br>"
                            "Changing the wording of a question or the order of the questions is not recommended.<br><br>"
                            "Students should fill out the survey by going to:<br><br>") +
                            URLs.at(1) +
                            tr("<br><br>This URL has been copied to your clipboard and should now be pasted elsewhere to save."));
        icon.load(":/icons/ok.png");
    }
    QSize iconSize = busyBox->iconPixmap().size();
    busyBox->setIconPixmap(icon.scaled(iconSize));
    busyBox->setStandardButtons(QMessageBox::Ok);
    busyBox->exec();
    surveyMaker->google->notBusy(busyBox);

    surveyMaker->surveyCreated = true;
}

void SurveyMaker::createCanvasQuiz(SurveyMaker *surveyMaker)
{
    if(!internetIsGood())
    {
        return;
    }

    //create canvasHandler and/or authenticate as needed
    if(surveyMaker->canvas == nullptr)
    {
        surveyMaker->canvas = new CanvasHandler();
    }
    if(!surveyMaker->canvas->authenticated)
    {
        //IN BETA--GETS USER'S API TOKEM MANUALLY
        QSettings savedSettings;
        QString savedCanvasURL = savedSettings.value("canvasURL").toString();
        QString savedCanvasToken = savedSettings.value("canvasToken").toString();

        QStringList newURLAndToken = surveyMaker->canvas->askUserForManualToken(savedCanvasURL, savedCanvasToken);
        if(newURLAndToken.isEmpty())
        {
            return;
        }

        savedCanvasURL = (newURLAndToken.at(0).isEmpty() ? savedCanvasURL : newURLAndToken.at(0));
        savedCanvasToken =  (newURLAndToken.at(1).isEmpty() ? savedCanvasToken : newURLAndToken.at(1));
        savedSettings.setValue("canvasURL", savedCanvasURL);
        savedSettings.setValue("canvasToken", savedCanvasToken);

        surveyMaker->canvas->setBaseURL(savedCanvasURL);
        surveyMaker->canvas->authenticate(savedCanvasToken);
    }

    //ask the user in which course we're creating the survey
    auto *busyBox = surveyMaker->canvas->busy();
    QStringList courseNames = surveyMaker->canvas->getCourses();
    surveyMaker->canvas->notBusy(busyBox);

    auto *canvasCourses = new QDialog(surveyMaker);
    canvasCourses->setWindowTitle(tr("Choose Canvas course"));
    canvasCourses->setWindowIcon(QIcon(":/icons/canvas.png"));
    canvasCourses->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    int i = 1;
    auto *label = new QLabel(tr("In which course should this survey be created?"));
    auto *coursesComboBox = new QComboBox;
    for(const auto &courseName : qAsConst(courseNames))
    {
        coursesComboBox->addItem(courseName);
        coursesComboBox->setItemData(i++, QString::number(surveyMaker->canvas->getStudentCount(courseName)) + " students", Qt::ToolTipRole);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesComboBox);
    vLayout->addWidget(buttonBox);
    canvasCourses->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, canvasCourses, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, canvasCourses, &QDialog::reject);
    if((canvasCourses->exec() == QDialog::Rejected))
    {
        delete canvasCourses;
        return;
    }

    //upload the survey as a quiz
    busyBox = surveyMaker->canvas->busy();
    QPixmap icon;
    if(surveyMaker->canvas->createSurvey(coursesComboBox->currentText(), surveyMaker->survey)) {
        busyBox->hide();
        busyBox->setTextFormat(Qt::RichText);
        busyBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
        QApplication::restoreOverrideCursor();
        busyBox->setText(tr("Success! Survey created in your Canvas course.<br>"
                            "If you'd like to preview or edit the survey, you can find it under the \"Quizzes\" navigation item in your course Canvas site.<br>"
                            "If you wish, you can modify:<br>"
                            "  » the survey title<br>"
                            "  » the Quiz Instructions shown at the start of the survey<br>"
                            "Changing the wording of a question or the order of the questions is not recommended.<br><br>"
                            "<strong>The survey is currently \"Unpublished\".</strong><br>"
                            "When you are ready for students to fill out the survey, you must log in to your course Canvas site and \"Publish\" it."));
        icon.load(":/icons/ok.png");
    }
    else {
        busyBox->hide();
        busyBox->setText(tr("Error. The survey was not created."));
        icon.load(":/icons/delete.png");
    }
    QSize iconSize = busyBox->iconPixmap().size();
    busyBox->setIconPixmap(icon.scaled(iconSize));
    busyBox->setStandardButtons(QMessageBox::Ok);
    busyBox->exec();
    surveyMaker->canvas->notBusy(busyBox);

    delete canvasCourses;

    surveyMaker->surveyCreated = true;
}

void SurveyMaker::on_surveyDestinationBox_currentIndexChanged(const QString &arg1)
{
    if(arg1 == "Google form")
    {
        ui->makeSurveyButton->setToolTip("<html>Upload the survey to your Google Drive.</html>");
        generateSurvey = &SurveyMaker::createGoogleForm;
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

    if(tabs->count() < 3)
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
    int numNonTimezoneAttributes = ui->attributeCountSpinBox->value();
    // count up tabs until we hit one whose right boundary exceeds that of the tabWidget itself (or we hit the last possible index)
    while((tabs->tabBar()->tabRect(lastVisibleIndex).right() < widthOfTabWidget) && (lastVisibleIndex < numNonTimezoneAttributes))
    {
        lastVisibleIndex++;
    }
    auto lastTabGeom = tabs->tabBar()->tabRect(lastVisibleIndex);
    // move back one if this last tab is less than 1/2 visible or if we've exceeded the number of tabs possible
    if((((lastTabGeom.left() + lastTabGeom.right()) / 2) > widthOfTabWidget) || (lastVisibleIndex == numNonTimezoneAttributes))
    {
        lastVisibleIndex--;
        lastTabGeom = tabs->tabBar()->tabRect(lastVisibleIndex);
    }

    if((index == firstVisibleIndex) && (firstVisibleIndex != 0))
    {
        // expanding one or two tabs to left if the current tab is the first visible and there are previous tabs to expand
        tabs->setTabVisible(firstVisibleIndex-1, true);
        firstVisibleIndex--;
        if(firstVisibleIndex != 0)
        {
            tabs->setTabVisible(firstVisibleIndex-1, true);
            firstVisibleIndex--;
        }
    }
    else if((firstVisibleIndex != 0) && (lastVisibleIndex == numNonTimezoneAttributes - 1) && (lastTabGeom.right() + lastTabGeom.width() < widthOfTabWidget))
    {
        // expanding one tab to the left if there are previous tabs to expand and there's simply enough room now due to closing a tab or expanding the window
        tabs->setTabVisible(firstVisibleIndex-1, true);
        firstVisibleIndex--;
    }
    else if((lastVisibleIndex != numNonTimezoneAttributes-1) && (index >= lastVisibleIndex))
    {
        // expanding one or two tabs to right if there are subsequent tabs to expand and the current one is the last one visible
        tabs->setTabVisible(firstVisibleIndex, false);
        firstVisibleIndex++;
        lastVisibleIndex++;
        if(lastVisibleIndex != numNonTimezoneAttributes-1)
        {
            tabs->setTabVisible(firstVisibleIndex, false);
            firstVisibleIndex++;
        }
    }

    // redetermine the last visible index now for the sake of labeling the tabs
    lastVisibleIndex = firstVisibleIndex;
    // count up tabs until we hit one whose right boundary exceeds that of the tabWidget itself (or we hit the last possible index)
    while((tabs->tabBar()->tabRect(lastVisibleIndex).right() < widthOfTabWidget) && (lastVisibleIndex < numNonTimezoneAttributes))
    {
        lastVisibleIndex++;
    }
    // move back one if this last tab is less than 1/2 visible or if we've exceeded the number of tabs possible
    if((((tabs->tabBar()->tabRect(lastVisibleIndex).left() + tabs->tabBar()->tabRect(lastVisibleIndex).right()) / 2) > widthOfTabWidget)
            || (lastVisibleIndex == numNonTimezoneAttributes))
    {
        lastVisibleIndex--;
    }

    // reset the label for every tab, with 'scroll triangles' on the ends if needed
    for(int tab = 0; tab < MAX_ATTRIBUTES; tab++)
    {
        QString label;
        if((tab != 0) && (tab == firstVisibleIndex))
        {
            label = QString(LEFTARROW) + "  " + QString::number(tab+1);
        }
        else if((tab != numNonTimezoneAttributes-1) && (tab == lastVisibleIndex))
        {
            label = QString::number(tab+1) + "  " + QString(RIGHTARROW);
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
        connect(attributeTab->attributeResponses, QOverload<int>::of(&QComboBox::activated), this, &SurveyMaker::attributeResponseChanged);
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
    if((index == currIndex) && (currIndex == ui->attributeCountSpinBox->value() - 1))
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

    ui->attributeCountSpinBox->setValue(ui->attributeCountSpinBox->value() - 1);
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

void SurveyMaker::attributeResponseChanged()
{
    static int prevIndex = 0;
    static QStringList currentCustomOptions;
    const int currAttribute = ui->attributesTabWidget->currentIndex();
    //see if custom options being enabled
    auto *attribTab = qobject_cast<attributeTabItem *>(ui->attributesTabWidget->widget(currAttribute));
    auto &responseComboBox = attribTab->attributeResponses;
    if(responseComboBox->currentText() == tr("Custom options..."))
    {
        auto *window = new customResponseOptionsDialog(currentCustomOptions, this);

        // If user clicks OK, use these options
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
            bool currentValue = responseComboBox->blockSignals(true);
            responseComboBox->setItemText(responseOptions.size(), tr("Current options"));
            prevIndex = responseOptions.size();
            currentCustomOptions = window->options;
            responseComboBox->setItemData(responseOptions.size(), currentCustomOptions);

            responseComboBox->removeItem(responseOptions.size()+1);
            responseComboBox->addItem(tr("Custom options..."));
            responseComboBox->blockSignals(currentValue);
        }
        else
        {
            bool currentValue = responseComboBox->blockSignals(true);
            responseComboBox->setCurrentIndex(prevIndex);
            responseComboBox->blockSignals(currentValue);
        }

        delete window;
    }
    else
    {
        currentCustomOptions = responseComboBox->currentData().toStringList();
        prevIndex = responseComboBox->currentIndex();
    }

    // Put list of options back to just built-ins plus "Custom options"
    if(responseComboBox->currentIndex() < responseOptions.size())
    {
        responseComboBox->removeItem(responseOptions.size()+1);
        responseComboBox->removeItem(responseOptions.size());
        responseComboBox->addItem(tr("Custom options..."));
    }

    buildSurvey();
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
    sectionNames.removeAll("");

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
            if(loadObject.contains("GenderType") && loadObject["GenderType"].isDouble())
            {
                ui->genderResponsesComboBox->setCurrentIndex(loadObject["GenderType"].toInt());
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
                if(loadObject.contains("Attribute" + QString::number(attribute+1)+"Options") &&
                        loadObject["Attribute" + QString::number(attribute+1)+"Options"].isString())
                {
                     QStringList options = loadObject["Attribute" + QString::number(attribute+1)+"Options"].toString().split('/');
                     auto &responseComboBox = attribTab->attributeResponses;

                     responseComboBox->setItemText(responseOptions.size(), tr("Current options"));
                     responseComboBox->setItemData(responseOptions.size(), options);

                     responseComboBox->removeItem(responseOptions.size()+1);
                     responseComboBox->addItem(tr("Custom options..."));
                }
            }
            // show first attribute question
            ui->attributesTabWidget->setCurrentIndex(0);

            if(loadObject.contains("Schedule") && loadObject["Schedule"].isBool())
            {
                ui->scheduleCheckBox->setChecked(loadObject["Schedule"].toBool());
            }
            if(loadObject.contains("ScheduleAsBusy") && loadObject["ScheduleAsBusy"].isBool())
            {
                ui->busyFreeComboBox->setCurrentText(loadObject["ScheduleAsBusy"].toBool()? "busy" : "free");
            }
            if(loadObject.contains("Timezone") && loadObject["Timezone"].isBool())
            {
                ui->timezoneCheckBox->setChecked(loadObject["Timezone"].toBool());
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
                QString dayName = defaultDayNames.at(day);
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
            saveObject["GenderType"] = static_cast<int>(genderType);
            saveObject["URM"] = URM;
            const int numRealAttributes = numAttributes - ((timezone && !schedule) ? 1 : 0);
            saveObject["numAttributes"] = numRealAttributes;
            for(int attribute = 0; attribute < numRealAttributes; attribute++)
            {
                saveObject["Attribute" + QString::number(attribute+1)+"Question"] = attributeTexts[attribute];
                saveObject["Attribute" + QString::number(attribute+1)+"Response"] = attributeResponses[attribute];
                saveObject["Attribute" + QString::number(attribute+1)+"AllowMultiResponse"] = attributeAllowMultipleResponses[attribute];
                if(attributeResponses[attribute] == (responseOptions.size()-1)) // custom options being used
                {
                    auto *attribTab = qobject_cast<attributeTabItem*>(ui->attributesTabWidget->widget(attribute));
                    saveObject["Attribute" + QString::number(attribute+1)+"Options"] = attribTab->attributeResponses->currentData().toStringList().join('/');
                }
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
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString user = registeredUser.isEmpty()? tr("UNREGISTERED") : (tr("registered to ") + registeredUser);
    QMessageBox::about(this, tr("About gruepr"), ABOUTWINDOWCONTENT  + tr("<p><b>This copy of gruepr is ") + user + "</b>.");
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
