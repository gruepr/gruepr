#include "surveyMakerWizard.h"
#include "gruepr_globals.h"


SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumWidth(800);
    setMinimumHeight(600);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    button(QWizard::CancelButton)->setStyleSheet(NEXTBUTTONSTYLE);
    setButtonText(QWizard::CancelButton, "\u00AB  Cancel");
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, "\u2B60  Previous Step");
    button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    setButtonText(QWizard::NextButton, "Next Step  \u2B62");
    button(QWizard::FinishButton)->setStyleSheet(NEXTBUTTONSTYLE);

    addPage(new IntroPage);
    addPage(new DemographicsPage);
    addPage(new MultipleChoicePage);
    addPage(new SchedulePage);
    addPage(new CourseInfoPage);
    addPage(new PreviewAndExportPage);

    setOption(QWizard::NoBackButtonOnStartPage);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(buttonLayout);
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    pageTitle = new QLabel("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\">"
                           " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>", this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setScaledContents(true);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    auto *bannerLeft = new QLabel(this);
    bannerLeft->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png);");
    bannerLeft->setMinimumSize(0, 120);
    bannerLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *bannerRight = new QLabel(this);
    bannerRight->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png);");
    bannerRight->setMinimumSize(0, 120);
    bannerRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    banner = new QLabel(this);
    banner->setStyleSheet("border-image: url(:/icons_new/surveyMakerWizardBanner.png);");
    banner->setAlignment(Qt::AlignCenter);
    banner->setScaledContents(true);
    QPixmap bannerpixmap(":/icons_new/surveyMakerWizardBanner.png");
    banner->setMinimumSize(bannerpixmap.width()*120/bannerpixmap.height(), 120);
    banner->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto *bannerWidg = new QWidget(this);
    auto *bannerLayout = new QHBoxLayout(bannerWidg);
    bannerLayout->setSpacing(0);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    bannerLayout->addWidget(bannerLeft);
    bannerLayout->addWidget(banner);
    bannerLayout->addWidget(bannerRight);

    topLabel = new QLabel(this);
    topLabel->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-size: 12pt; font-family: DM Sans;\">" +
                      tr("Survey Name") + "</span>");
    topLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    surveyTitle = new QLineEdit(this);
    surveyTitle->setPlaceholderText(tr("Enter Text"));
    surveyTitle->setStyleSheet("color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-size: 14pt; font-family: DM Sans; "
                               "border-style: outset; border-width: 2px; border-color: #" + QString(GRUEPRDARKBLUEHEX) + "; ");
    surveyTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    registerField("SurveyTitle", surveyTitle);

    bottomLabel = new QLabel(this);
    bottomLabel->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-size: 10pt; font-family: DM Sans\">" +
                         tr("This will be the name of the survey you send to your students!") + "</span>");
    bottomLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    getStartedButton = new QPushButton("Get Started  \u2B62", this);
    getStartedButton->setStyleSheet(GETSTARTEDBUTTONSTYLE);
    getStartedButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    layout = new QGridLayout;
    int row = 0;
    layout->addWidget(pageTitle, row++, 0, 1, -1);
    layout->addWidget(bannerWidg, row++, 0, 1, -1);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(topLabel, row++, 1, Qt::AlignLeft | Qt::AlignBottom);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(surveyTitle, row++, 1);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(bottomLabel, row++, 1, Qt::AlignLeft | Qt::AlignTop);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(getStartedButton, row++, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 1);
    layout->setSpacing(0);
    setLayout(layout);
}

void IntroPage::initializePage()
{
    auto *wiz = wizard();
    connect(getStartedButton, &QPushButton::clicked, wiz, &QWizard::next);
}


DemographicsPage::DemographicsPage(QWidget *parent)
    : SurveyMakerPage(5, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("  Demographic Questions"));

    questions[firstname].setLabel(tr("First name"));
    questionPreviewTopLabels[firstname].setText(tr("First name"));
    questionPreviewLayouts[firstname].addWidget(&questionPreviewTopLabels[firstname]);
    fn = new QLineEdit(FIRSTNAMEQUESTION);
    fn->setReadOnly(true);
    fn->setCursorPosition(0);
    fn->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[firstname].addWidget(fn);
    questionPreviewBottomLabels[firstname].setText("");
    //questionPreviewLayouts[firstname].addWidget(&questionPreviewBottomLabels[firstname]);
    questionPreviews[firstname].hide();
    registerField("FirstName", &questions[firstname], "value", "valueChanged");

    questions[lastname].setLabel(tr("Last name"));
    questionPreviewTopLabels[lastname].setText(tr("Last name"));
    questionPreviewLayouts[lastname].addWidget(&questionPreviewTopLabels[lastname]);
    ln = new QLineEdit(LASTNAMEQUESTION);
    ln->setReadOnly(true);
    ln->setCursorPosition(0);
    ln->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[lastname].addWidget(ln);
    questionPreviewBottomLabels[lastname].setText("");
    //questionPreviewLayouts[lastname].addWidget(&questionPreviewBottomLabels[lastname]);
    questionPreviews[lastname].hide();
    registerField("LastName", &questions[lastname], "value", "valueChanged");

    questions[email].setLabel(tr("Email"));
    questionPreviewTopLabels[email].setText(tr("Email"));
    questionPreviewLayouts[email].addWidget(&questionPreviewTopLabels[email]);
    em = new QLineEdit(EMAILQUESTION);
    em->setReadOnly(true);
    em->setCursorPosition(0);
    em->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[email].addWidget(em);
    questionPreviewBottomLabels[email].setText("");
    //questionPreviewLayouts[email].addWidget(&questionPreviewBottomLabels[email]);
    questionPreviews[email].hide();
    registerField("Email", &questions[email], "value", "valueChanged");

    questions[gender].setLabel(tr("Gender"));
    connect(&questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(PREVIEWLABELSTYLE);
    genderResponsesLabel->setEnabled(false);
    questions[gender].addWidget(genderResponsesLabel, 1, 0, false);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    questions[gender].addWidget(genderResponsesComboBox, 1, 1, false);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[gender].setText(tr("Gender"));
    questionPreviewLayouts[gender].addWidget(&questionPreviewTopLabels[gender]);
    ge = new QComboBox;
    ge->addItem(PRONOUNQUESTION);
    ge->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    questionPreviewLayouts[gender].addWidget(ge);
    questionPreviewBottomLabels[gender].setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    questionPreviewLayouts[gender].addWidget(&questionPreviewBottomLabels[gender]);
    questionPreviews[gender].hide();
    registerField("Gender", &questions[gender], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

    questions[urm].setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[urm].setText(tr("Race / ethnicity"));
    questionPreviewLayouts[urm].addWidget(&questionPreviewTopLabels[urm]);
    re = new QLineEdit(URMQUESTION);
    re->setReadOnly(true);
    re->setCursorPosition(0);
    re->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[urm].addWidget(re);
    questionPreviewBottomLabels[urm].setText("");
    //questionPreviewLayouts[urm].addWidget(&questionPreviewBottomLabels[urm]);
    questionPreviews[urm].hide();
    registerField("RaceEthnicity", &questions[urm], "value", "valueChanged");

    update();
}

void DemographicsPage::initializePage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, GRUEPRDARKBLUE);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);
}

void DemographicsPage::cleanupPage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(NEXTBUTTONSTYLE);
}

void DemographicsPage::update()
{
    genderResponsesLabel->setEnabled((&questions[gender])->getValue());
    genderResponsesComboBox->setEnabled((&questions[gender])->getValue());
    ge->clear();
    GenderType genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    if(genderType == GenderType::biol)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::adult)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::child)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else //if(genderType == GenderType::pronoun)
    {
        ge->addItem(PRONOUNQUESTION);
        (&questionPreviewBottomLabels[gender])->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
}


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(0, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("  Multiple Choice Questions"));

}


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(2, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("  Schedule Questions"));

    questions[timezone].setLabel(tr("Timezone"));
    connect(&questions[timezone], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[timezone].setText(tr("Timezone"));
    questionPreviewLayouts[timezone].addWidget(&questionPreviewTopLabels[timezone]);
    tz = new QComboBox;
    tz->addItem(TIMEZONEQUESTION);
    tz->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    questionPreviewLayouts[timezone].addWidget(tz);
    questionPreviewBottomLabels[timezone].setText(tr("Dropdown options: List of global timezones"));
    questionPreviewLayouts[timezone].addWidget(&questionPreviewBottomLabels[timezone]);
    questionPreviews[timezone].hide();
    registerField("Timezone", &questions[timezone], "value", "valueChanged");

    questions[schedule].setLabel(tr("Schedule"));
    connect(&questions[schedule], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[schedule].setText(SCHEDULEQUESTION1 + SCHEDULEQUESTION2FREE + SCHEDULEQUESTION3);
    questionPreviewLayouts[schedule].addWidget(&questionPreviewTopLabels[schedule]);
    questionPreviewBottomLabels[schedule].setText(tr(""));
    //questionPreviewLayouts[schedule].addWidget(&questionPreviewBottomLabels[schedule]);
    questionPreviews[schedule].hide();
    registerField("Schedule", &questions[schedule], "value", "valueChanged");

    //subItems inside schedule question
    int row = 1;

    busyOrFreeLabel = new QLabel(tr("Ask as: "));
    busyOrFreeLabel->setStyleSheet(PREVIEWLABELSTYLE);
    busyOrFreeLabel->setEnabled(false);
    questions[schedule].addWidget(busyOrFreeLabel, row, 0, false);
    busyOrFreeComboBox = new QComboBox;
    busyOrFreeComboBox->addItems({tr("Free"), tr("Busy")});
    busyOrFreeComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    busyOrFreeComboBox->setEnabled(false);
    busyOrFreeComboBox->setCurrentIndex(0);
    questions[schedule].addWidget(busyOrFreeComboBox, row++, 1, false);
    connect(busyOrFreeComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleBusyOrFree", busyOrFreeComboBox);

    baseTimezoneLabel = new QLabel(tr("Select timezone"));
    baseTimezoneLabel->setStyleSheet(PREVIEWLABELSTYLE);
    baseTimezoneLabel->hide();
    questions[schedule].addWidget(baseTimezoneLabel, row++, 0, false);
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
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
    questions[schedule].addWidget(baseTimezoneComboBox, row++, 0, true);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTimezone", baseTimezoneComboBox);

    timespanLabel = new QLabel(tr("Timespan:"));
    timespanLabel->setStyleSheet(PREVIEWLABELSTYLE);
    timespanLabel->setEnabled(false);
    questions[schedule].addWidget(timespanLabel, row++, 0, false);
    daysComboBox = new QComboBox;
    daysComboBox->addItems({tr("All days"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    questions[schedule].addWidget(daysComboBox, row++, 0, true, Qt::AlignLeft);
    connect(daysComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleDays", daysComboBox);

    fromLabel = new QLabel(tr("From"));
    fromLabel->setStyleSheet(PREVIEWLABELSTYLE);
    fromLabel->setEnabled(false);
    questions[schedule].addWidget(fromLabel, row, 0, false, Qt::AlignRight);
    fromComboBox = new QComboBox;
    fromComboBox->addItems({"12am", "1am", "2am", "3am", "4am", "5am", "6am", "7am", "8am", "9am", "10am", "11am",
                            "12pm", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm", "7pm", "8pm", "9pm", "10pm", "11pm"});
    fromComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    fromComboBox->setEnabled(false);
    fromComboBox->setCurrentIndex(8);
    questions[schedule].addWidget(fromComboBox, row, 1, false, Qt::AlignLeft);
    connect(fromComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleFrom", fromComboBox);
    toLabel = new QLabel(tr("to"));
    toLabel->setStyleSheet(PREVIEWLABELSTYLE);
    toLabel->setEnabled(false);
    questions[schedule].addWidget(toLabel, row, 2, false, Qt::AlignRight);
    toComboBox = new QComboBox;
    toComboBox->addItems({"12am", "1am", "2am", "3am", "4am", "5am", "6am", "7am", "8am", "9am", "10am", "11am",
                          "12pm", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm", "7pm", "8pm", "9pm", "10pm", "11pm"});
    toComboBox->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    toComboBox->setEnabled(false);
    toComboBox->setCurrentIndex(18);
    questions[schedule].addWidget(toComboBox, row++, 3, true, Qt::AlignLeft);
    connect(toComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTo", toComboBox);

    update();
}

void SchedulePage::update()
{
    bool scheduleOn = questions[schedule].getValue(), timezoneOn = questions[timezone].getValue();

    baseTimezoneLabel->setVisible(timezoneOn);
    baseTimezoneLabel->setEnabled(scheduleOn);
    baseTimezoneComboBox->setVisible(timezoneOn);
    baseTimezoneComboBox->setEnabled(scheduleOn);
    baseTimezone = baseTimezoneComboBox->currentText();
    if(baseTimezone == tr("[no timezone given]")) {
        baseTimezone.clear();
    }

    QString previewlabelText = SCHEDULEQUESTION1;
    previewlabelText += ((busyOrFreeComboBox->currentIndex() == 0)? SCHEDULEQUESTION2FREE : SCHEDULEQUESTION2BUSY);
    previewlabelText += SCHEDULEQUESTION3;
    if(timezoneOn && scheduleOn) {
        previewlabelText += SCHEDULEQUESTION4;
        if(baseTimezone.isEmpty())
        {
            previewlabelText += SCHEDULEQUESTIONHOME;
        }
        else
        {
            previewlabelText += baseTimezone;
        }
        previewlabelText += SCHEDULEQUESTION5;
    }
    questionPreviewTopLabels[schedule].setText(previewlabelText);

    busyOrFreeLabel->setEnabled(scheduleOn);
    busyOrFreeComboBox->setEnabled(scheduleOn);
    timespanLabel->setEnabled(scheduleOn);
    daysComboBox->setEnabled(scheduleOn);
    fromLabel->setEnabled(scheduleOn);
    fromComboBox->setEnabled(scheduleOn);
    toLabel->setEnabled(scheduleOn);
    toComboBox->setEnabled(scheduleOn);
}

CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(4, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("      Course Info Questions"));

    questions[section].setLabel(tr("Section"));
    questionPreviewTopLabels[section].setText(tr("Section"));
    questionPreviewLayouts[section].addWidget(&questionPreviewTopLabels[section]);
    sc = new QComboBox;
    sc->addItem(SECTIONQUESTION);
    sc->setStyleSheet(PREVIEWCOMBOBOXSTYLE);
    questionPreviewLayouts[section].addWidget(sc);
    questionPreviewLayouts[section].addWidget(&questionPreviewBottomLabels[section]);
    questionPreviews[section].hide();
    registerField("Section", &questions[section], "value", "valueChanged");

    questions[wantToWorkWith].setLabel(tr("Classmates I want to work with"));
    connect(&questions[wantToWorkWith], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToWorkWith].setText(tr("Classmate info"));
    questionPreviewLayouts[wantToWorkWith].addWidget(&questionPreviewTopLabels[wantToWorkWith]);
    ww = new QLineEdit(PREF1TEAMMATEQUESTION);
    ww->setReadOnly(true);
    ww->setCursorPosition(0);
    ww->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[wantToWorkWith].addWidget(ww);
    questionPreviewBottomLabels[wantToWorkWith].setText(tr(""));
    //questionPreviewLayouts[wantToWorkWith].addWidget(&questionPreviewBottomLabels[wantToWorkWith]);
    questionPreviews[wantToWorkWith].hide();
    registerField("PrefTeammate", &questions[wantToWorkWith], "value", "valueChanged");

    layout->setRowMinimumHeight(6, 0);

    questions[wantToAvoid].setLabel(tr("Classmates I want to avoid"));
    connect(&questions[wantToAvoid], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToAvoid].setText(tr("Classmate info"));
    questionPreviewLayouts[wantToAvoid].addWidget(&questionPreviewTopLabels[wantToAvoid]);
    wa = new QLineEdit(PREF1NONTEAMMATEQUESTION);
    wa->setReadOnly(true);
    wa->setCursorPosition(0);
    wa->setStyleSheet(PREVIEWLINEEDITSTYLE);
    questionPreviewLayouts[wantToAvoid].addWidget(wa);
    questionPreviewBottomLabels[wantToAvoid].setText(tr(""));
    //questionPreviewLayouts[wantToAvoid].addWidget(&questionPreviewBottomLabels[wantToAvoid]);
    questionPreviews[wantToAvoid].hide();
    registerField("PrefNonTeammate", &questions[wantToAvoid], "value", "valueChanged");

    layout->setRowMinimumHeight(8, 0);

    questions[selectFromList].setLabel(tr("Select from list of classmates"));
    connect(&questions[selectFromList], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviews[selectFromList].hide();
    registerField("ClassmateListed", &questions[selectFromList], "value", "valueChanged");

    update();
}

void CourseInfoPage::initializePage()
{
    wizard()->setButtonText(QWizard::NextButton, "View Preview");
}

void CourseInfoPage::cleanupPage()
{
    wizard()->setButtonText(QWizard::NextButton, "Next Step  \u2B62");
}

void CourseInfoPage::update()
{
    questionPreviewTopLabels[wantToAvoid].setHidden(questions[wantToWorkWith].getValue() && questions[wantToAvoid].getValue());
}


PreviewAndExportPage::PreviewAndExportPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Complete Your Registration"));

    bottomLabel = new QLabel;
    bottomLabel->setWordWrap(true);

    agreeCheckBox = new QCheckBox(tr("I agree to the terms of the license"));

    registerField("conclusion.agree*", agreeCheckBox);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(bottomLabel);
    layout->addWidget(agreeCheckBox);
    setLayout(layout);
}


void PreviewAndExportPage::initializePage()
{
    bottomLabel->setText("Title: " + field("SurveyTitle").toString() + "  First Name: " + (field("FirstName").toBool()?"Yes":"No") + "  Gender Options: " + QString::number(field("genderOptions").toInt()));
}
