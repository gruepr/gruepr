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

    QStringList questionLabels = {tr("First name"), tr("Last name"), tr("Email"), tr("Gender"), tr("Race / ethnicity")};
    QStringList questionBottomLabels = {"", "", "", tr("Dropdown options: they/them, she/hers, he/him, other, prefer not to answer"), ""};
    auto fn = new QLineEdit(tr("What is your first name / preferred name?"));
    fn->setReadOnly(true);
    fn->setStyleSheet(previewLineEditStyle);
    auto ln = new QLineEdit(tr("What is your last name?"));
    ln->setReadOnly(true);
    ln->setStyleSheet(previewLineEditStyle);
    auto em = new QLineEdit(tr("What is your email address?"));
    em->setReadOnly(true);
    em->setStyleSheet(previewLineEditStyle);
    ge = new QComboBox;
    ge->addItem("What are your pronouns?");
    ge->setStyleSheet(previewComboBoxStyle);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(previewLabelStyle);
    genderResponsesLabel->setEnabled(false);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(previewComboBoxStyle);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    questions[3].addWidget(genderResponsesLabel, 1, 0, false);
    questions[3].addWidget(genderResponsesComboBox, 2, 0, false);
    connect(&questions[3], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    auto re = new QLineEdit(tr("How do you identify your race, ethnicity, or cultural heritage?"));
    re->setReadOnly(true);
    re->setStyleSheet(previewLineEditStyle);
    QList<QWidget*> questionPreviewTypes = {fn, ln, em, ge, re};
    for(int i = 0; i < numQuestions; i++) {
        questions[i].setLabel(questionLabels[i]);

        questionPreviewTopLabels[i].setText(questionLabels[i]);
        questionPreviewLayouts[i].addWidget(&questionPreviewTopLabels[i]);
        questionPreviewLayouts[i].addWidget(questionPreviewTypes[i]);
        if(!questionBottomLabels[i].isEmpty()) {
            questionPreviewBottomLabels[i].setText(questionBottomLabels[i]);
            questionPreviewLayouts[i].addWidget(&questionPreviewBottomLabels[i]);
        }
        questionPreviews[i].hide();
    }
    registerField("FirstName", &questions[0], "value", "valueChanged");
    registerField("LastName", &questions[1], "value", "valueChanged");
    registerField("Email", &questions[2], "value", "valueChanged");
    registerField("Gender", &questions[3], "value", "valueChanged");
    registerField("RaceEthnicity", &questions[4], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

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
    : SurveyMakerPage(0, parent)
{
    pageTitle->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "\">Survey Name"
                       " &ensp;|&ensp; Demographics &ensp;|&ensp; Multiple Choice &ensp;|&ensp; Scheduling</span><span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "\"> &ensp;|&ensp; Course Info &ensp;|&ensp; Preview & Export</span>");
    topLabel->setText(tr("  Schedule Questions"));

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
