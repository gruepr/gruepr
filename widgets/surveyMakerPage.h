#ifndef SURVEYMAKERPAGE_H
#define SURVEYMAKERPAGE_H

#include <QWizardPage>
#include "widgets/surveyMakerQuestionWithSwitch.h"
#include <QLabel>

class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(QWidget *parent = nullptr);
    ~SurveyMakerPage();

protected:
    QLabel *topLabel;
    SurveyMakerQuestionWithSwitch *questions;
};

#endif // SURVEYMAKERPAGE_H
