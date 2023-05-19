#include "surveyMakerPage.h"

SurveyMakerPage::SurveyMakerPage(QWidget *parent)
    : QWizardPage(parent)
{

}

SurveyMakerPage::~SurveyMakerPage()
{
    delete topLabel;
    delete[] questions;
}
