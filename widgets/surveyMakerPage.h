#ifndef SURVEYMAKERPAGE_H
#define SURVEYMAKERPAGE_H

#include <QWizardPage>
#include "widgets/surveyMakerQuestionWithSwitch.h"
#include <QLabel>
#include <QTextEdit>

class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(QWidget *parent = nullptr);
    ~SurveyMakerPage();

protected:
    QGridLayout *layout = nullptr;
    QLabel *topLabel = nullptr;
    SurveyMakerQuestionWithSwitch *questions = nullptr;
    QTextEdit *preview = nullptr;
};

#endif // SURVEYMAKERPAGE_H
