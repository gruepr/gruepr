#include "surveyMakerWizard.h"
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>

SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_Demographics, new DemographicsPage);
    setPage(Page_MultipleChoice, new MultipleChoicePage);
    setPage(Page_Schedule, new SchedulePage);
    setPage(Page_CourseInfo, new CourseInfoPage);
    setPage(Page_PreviewAndExport, new PreviewAndExportPage);

    setStartId(Page_Intro);

    setWindowTitle(tr("Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage);
    setOption(QWizard::NoCancelButton);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    button(QWizard::CancelButton)->setStyleSheet(stdButtonStyle);
    button(QWizard::BackButton)->setStyleSheet(stdButtonStyle);
    setButtonText(QWizard::BackButton, "\u2B60  Previous Step");
    button(QWizard::NextButton)->setStyleSheet(getStartedButtonStyle);
    setButtonText(QWizard::NextButton, "Get Started  \u2B62");
    button(QWizard::FinishButton)->setStyleSheet(nextButtonStyle);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton << QWizard::Stretch;
    setButtonLayout(buttonLayout);
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    //need to have a title & subtitle in order to show the banner pixmap, so using a minimal background-colored dot for each
    setTitle("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-size:1pt;\">.</span>");
    setSubTitle("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-size:1pt;\">.</span>");
    setPixmap(QWizard::BannerPixmap, QPixmap(":/icons_new/surveyMakerWizardBanner.png"));

    topLabel = new QLabel(tr(""));
    topLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    setLayout(layout);
}

int IntroPage::nextId() const
{
    return SurveyMakerWizard::Page_Demographics;
}


DemographicsPage::DemographicsPage(QWidget *parent)
    : SurveyMakerPage(parent)
{
    setTitle(tr("Demographic Questions"));

    const int numQuestions = 5;
    questions = new SurveyMakerQuestionWithSwitch[numQuestions];
    QStringList questionLabels = {"First name", "Last name", "Email", "Gender", "Race / ethnicity"};
    for(int i = 0; i < numQuestions; i++) {
        questions[i].setLabel(questionLabels[i]);
        layout->addWidget(&questions[i], i, 0, 1, 1);
    }
}

int DemographicsPage::nextId() const
{
    return SurveyMakerWizard::Page_MultipleChoice;
}

void DemographicsPage::initializePage()
{
    auto palette = wizard()->palette();
    palette.setColor(QPalette::Window, GRUEPRDARKBLUE);
    wizard()->setPalette(palette);

    wizard()->button(QWizard::NextButton)->setStyleSheet(nextButtonStyle);
    wizard()->setButtonText(QWizard::NextButton, "Next Step  \u2B62");
    wizard()->setOption(QWizard::NoCancelButton, false);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton;
    wizard()->setButtonLayout(buttonLayout);
}

void DemographicsPage::cleanupPage()
{
    auto palette = wizard()->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wizard()->setPalette(palette);

    wizard()->button(QWizard::NextButton)->setStyleSheet(getStartedButtonStyle);
    wizard()->setButtonText(QWizard::NextButton, "Get Started  \u2B62");
    wizard()->setOption(QWizard::NoCancelButton);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::FinishButton << QWizard::Stretch;
    wizard()->setButtonLayout(buttonLayout);
}


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(parent)
{
    setTitle(tr("Multiple Choice Questions"));

}

int MultipleChoicePage::nextId() const
{
    return SurveyMakerWizard::Page_Schedule;
}


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(parent)
{
    setTitle(tr("Schedule Questions"));

}

int SchedulePage::nextId() const
{
    return SurveyMakerWizard::Page_CourseInfo;
}


CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(parent)
{
    setTitle(tr("Course Info Questions"));

}

int CourseInfoPage::nextId() const
{
    return SurveyMakerWizard::Page_PreviewAndExport;
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

int PreviewAndExportPage::nextId() const
{
    return -1;
}

void PreviewAndExportPage::initializePage()
{
    QString licenseText;

    if (wizard()->hasVisitedPage(SurveyMakerWizard::Page_MultipleChoice)) {
        licenseText = tr("<u>Evaluation License Agreement:</u> "
                         "You can use this software for 30 days and make one "
                         "backup, but you are not allowed to distribute it.");
    } else {
        licenseText = tr("<u>Upgrade License Agreement:</u> "
                         "This software is licensed under the terms of your "
                         "current license.");
    }
    bottomLabel->setText(licenseText);
}
