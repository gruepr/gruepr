#include "surveyMakerPage.h"
#include "gruepr_globals.h"

SurveyMakerPage::SurveyMakerPage(QWidget *parent)
    : QWizardPage(parent)
{
    preview = new QTextEdit(this);
    preview->setTextColor(GRUEPRDARKBLUE);
    preview->setTextBackgroundColor(GRUEPRVERYLIGHTBLUE);
    preview->setReadOnly(true);

    layout = new QGridLayout;
    setLayout(layout);
    layout->addWidget(preview, 0, 1, -1, 1);
}

SurveyMakerPage::~SurveyMakerPage()
{
    delete topLabel;
    delete[] questions;
    delete preview;
    delete layout;
}
