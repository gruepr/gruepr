#include "surveyMakerWizard.h"
#include "gruepr_globals.h"
#include <QComboBox>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>

const QString stdButtonStyle = "background-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                               "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                               "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
const QString getStartedButtonStyle = "background-color: #" + QString(GRUEPRMEDBLUEHEX) + "; "
                                      "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                                      "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
const QString nextButtonStyle = "background-color: white; "
                                "border-style: outset; border-width: 2px; border-radius: 5px; border-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                                "color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
const QString invisButtonStyle = "background-color: rgba(0,0,0,0%); border-style: none; color: rgba(0,0,0,0%); font-size: 1pt; padding: 0px;";


SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    addPage(new IntroPage);
    addPage(new DemographicsPage);
    addPage(new MultipleChoicePage);
    addPage(new SchedulePage);
    addPage(new CourseInfoPage);
    addPage(new PreviewAndExportPage);

    setWindowTitle(tr("Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage);
    setMinimumWidth(800);
    setMinimumHeight(600);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    button(QWizard::CancelButton)->setStyleSheet(nextButtonStyle);
    setButtonText(QWizard::CancelButton, "\u00AB  Cancel");
    button(QWizard::BackButton)->setStyleSheet(stdButtonStyle);
    setButtonText(QWizard::BackButton, "\u2B60  Previous Step");
    button(QWizard::NextButton)->setStyleSheet(invisButtonStyle);
    setButtonText(QWizard::NextButton, "Next Step  \u2B62");
    button(QWizard::FinishButton)->setStyleSheet(nextButtonStyle);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
    setButtonLayout(buttonLayout);
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    pageTitle = new QLabel("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\">"
                           " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>", this);
    pageTitle->setStyleSheet(titleStyle);
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
    getStartedButton->setStyleSheet(getStartedButtonStyle);
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

    int questionNum = 0;

    questions[questionNum].setLabel(tr("First name"));
    questionPreviewTopLabels[questionNum].setText(tr("First name"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewTopLabels[questionNum]);
    fn = new QLineEdit(tr("What is your first name / preferred name?"));
    fn->setReadOnly(true);
    fn->setStyleSheet(previewLineEditStyle);
    questionPreviewLayouts[questionNum].addWidget(fn);
    questionPreviewBottomLabels[questionNum].setText("");
    //questionPreviewLayouts[questionNum].addWidget(&questionPreviewBottomLabels[questionNum]);
    questionPreviews[questionNum].hide();
    registerField("FirstName", &questions[questionNum], "value", "valueChanged");
    questionNum++;

    questions[questionNum].setLabel(tr("Last name"));
    questionPreviewTopLabels[questionNum].setText(tr("Last name"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewTopLabels[questionNum]);
    ln = new QLineEdit(tr("What is your last name?"));
    ln->setReadOnly(true);
    ln->setStyleSheet(previewLineEditStyle);
    questionPreviewLayouts[questionNum].addWidget(ln);
    questionPreviewBottomLabels[questionNum].setText("");
    //questionPreviewLayouts[questionNum].addWidget(&questionPreviewBottomLabels[questionNum]);
    questionPreviews[questionNum].hide();
    registerField("LastName", &questions[questionNum], "value", "valueChanged");
    questionNum++;

    questions[questionNum].setLabel(tr("Email"));
    questionPreviewTopLabels[questionNum].setText(tr("Email"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewTopLabels[questionNum]);
    em = new QLineEdit(tr("What is your email address?"));
    em->setReadOnly(true);
    em->setStyleSheet(previewLineEditStyle);
    questionPreviewLayouts[questionNum].addWidget(em);
    questionPreviewBottomLabels[questionNum].setText("");
    //questionPreviewLayouts[questionNum].addWidget(&questionPreviewBottomLabels[questionNum]);
    questionPreviews[questionNum].hide();
    registerField("Email", &questions[questionNum], "value", "valueChanged");
    questionNum++;

    questions[questionNum].setLabel(tr("Gender"));
    connect(&questions[questionNum], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(previewLabelStyle);
    genderResponsesLabel->setEnabled(false);
    questions[questionNum].addWidget(genderResponsesLabel, 1, 0, false);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(previewComboBoxStyle);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    questions[questionNum].addWidget(genderResponsesComboBox, 2, 0, false);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[questionNum].setText(tr("Gender"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewTopLabels[questionNum]);
    ge = new QComboBox;
    ge->addItem("What are your pronouns?");
    ge->setStyleSheet(previewComboBoxStyle);
    questionPreviewLayouts[questionNum].addWidget(ge);
    questionPreviewBottomLabels[questionNum].setText(tr("Dropdown options: they/them, she/hers, he/him, other, prefer not to answer"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewBottomLabels[questionNum]);
    questionPreviews[questionNum].hide();
    registerField("Gender", &questions[questionNum], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);
    questionNum++;

    questions[questionNum].setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[questionNum].setText(tr("Race / ethnicity"));
    questionPreviewLayouts[questionNum].addWidget(&questionPreviewTopLabels[questionNum]);
    re = new QLineEdit(tr("How do you identify your race, ethnicity, or cultural heritage?"));
    re->setReadOnly(true);
    re->setStyleSheet(previewLineEditStyle);
    questionPreviewLayouts[questionNum].addWidget(re);
    questionPreviewBottomLabels[questionNum].setText("");
    //questionPreviewLayouts[questionNum].addWidget(&questionPreviewBottomLabels[questionNum]);
    questionPreviews[questionNum].hide();
    registerField("RaceEthnicity", &questions[questionNum], "value", "valueChanged");
    questionNum++;

    update();
}

void DemographicsPage::initializePage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, GRUEPRDARKBLUE);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(nextButtonStyle);
    wiz->button(QWizard::CancelButton)->setStyleSheet(stdButtonStyle);
}

void DemographicsPage::cleanupPage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(invisButtonStyle);
    wiz->button(QWizard::CancelButton)->setStyleSheet(nextButtonStyle);
}

void DemographicsPage::update()
{
    genderResponsesLabel->setEnabled((&questions[3])->getValue());
    genderResponsesComboBox->setEnabled((&questions[3])->getValue());
    ge->clear();
    GenderType genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    if(genderType == GenderType::biol)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[3])->setText(tr("Options: ") + QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::adult)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[3])->setText(tr("Options: ") + QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::child)
    {
        ge->addItem(GENDERQUESTION);
        (&questionPreviewBottomLabels[3])->setText(tr("Options: ") + QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else //if(genderType == GenderType::pronoun)
    {
        ge->addItem(PRONOUNQUESTION);
        (&questionPreviewBottomLabels[3])->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
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

    questions[0].setLabel(tr("Timezone"));
    connect(&questions[0], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[0].setText(tr("Timezone"));
    questionPreviewLayouts[0].addWidget(&questionPreviewTopLabels[0]);
    tz = new QComboBox;
    tz->addItem("What timezone will you be in for this course?");
    tz->setStyleSheet(previewComboBoxStyle);
    questionPreviewLayouts[0].addWidget(tz);
    questionPreviewBottomLabels[0].setText(tr("Dropdown options: List of global timezones"));
    questionPreviewLayouts[0].addWidget(&questionPreviewBottomLabels[0]);
    questionPreviews[0].hide();
    registerField("Timezone", &questions[0], "value", "valueChanged");

    questions[1].setLabel(tr("Schedule"));
    connect(&questions[1], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[1].setText(tr("Select the times that you are available for group work."));
    questionPreviewLayouts[1].addWidget(&questionPreviewTopLabels[1]);
    questionPreviewBottomLabels[1].setText(tr(""));
    //questionPreviewLayouts[1].addWidget(&questionPreviewBottomLabels[1]);
    questionPreviews[1].hide();

    int row = 1;
    baseTimezoneLabel = new QLabel(tr("Select timezone"));
    baseTimezoneLabel->setStyleSheet(previewLabelStyle);
    baseTimezoneLabel->hide();
    questions[1].addWidget(baseTimezoneLabel, row++, 0, false);
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setStyleSheet(previewComboBoxStyle);
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
    questions[1].addWidget(baseTimezoneComboBox, row++, 0, false);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTimezone", baseTimezoneComboBox);

    daysLabel = new QLabel(tr("Select Days"));
    daysLabel->setStyleSheet(previewLabelStyle);
    daysLabel->setEnabled(false);
    questions[1].addWidget(daysLabel, row++, 0, false);
    daysComboBox = new QComboBox;
    daysComboBox->addItems({tr("Choose an option"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setStyleSheet(previewComboBoxStyle);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    questions[1].addWidget(daysComboBox, row++, 0, false);
    connect(daysComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("Schedule", &questions[1], "value", "valueChanged");
    registerField("days", daysComboBox);

    update();
}

void SchedulePage::update()
{
    daysLabel->setEnabled((&questions[1])->getValue());
    daysComboBox->setEnabled((&questions[1])->getValue());
    bool timezoneAndSchedule = questions[0].getValue() && questions[1].getValue();
    baseTimezoneLabel->setVisible(timezoneAndSchedule);
    baseTimezoneComboBox->setVisible(timezoneAndSchedule);
    baseTimezone = baseTimezoneComboBox->currentText();
    QString previewlabelText = tr("Select the times that you are available for group work.");
    if(timezoneAndSchedule) {
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
    questionPreviewTopLabels[1].setText(previewlabelText);
}

CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(0, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling &ensp;|&ensp; Course Info</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("      Course Info Questions"));

}

void CourseInfoPage::initializePage()
{
    wizard()->setButtonText(QWizard::NextButton, "View Preview");
}

void CourseInfoPage::cleanupPage()
{
    wizard()->setButtonText(QWizard::NextButton, "Next Step  \u2B62");
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
