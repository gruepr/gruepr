#ifndef SURVEYMAKERPAGE_H
#define SURVEYMAKERPAGE_H

#include <QWizardPage>
#include "gruepr_globals.h"
#include "widgets/surveyMakerQuestionWithSwitch.h"
#include <QLabel>
#include <QScrollArea>


const QString titleStyle = "font-size: 12pt; font-family: DM Sans; border-image: url(:/icons_new/surveyMakerWizardTitleBackground.png);";
const QString topLabelStyle = "color: white; font-size: 14pt; font-family: DM Sans;"
                              "border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png); height: 50px;";
const QString previewLabelStyle = "QLabel {color: " + QString(GRUEPRDARKBLUEHEX) + "; font-size: 10pt; font-family: DM Sans;}"
                                  "QLabel:disabled {color: darkGray; font-size: 10pt; font-family: DM Sans;}";
const QString previewLineEditStyle = "background-color: white; color: " + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 12pt;";
const QString previewComboBoxStyle = "QComboBox {background-color: white; color: " + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 12pt;}"
                                     "QComboBox:disabled {background-color: lightGray; color: darkGray; font-family: 'DM Sans'; font-size: 12pt;}"
                                     "QComboBox::drop-down {border-width: 0px;}"
                                     "QComboBox::down-arrow {image: url(:/icons_new/ComboBoxButton.png); border-width: 0px;}";


class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(int numQuestions, QWidget *parent = nullptr);
    ~SurveyMakerPage();

protected:
    QGridLayout *layout = nullptr;
    int numQuestions;
    int row = 0;
    QLabel *pageTitle = nullptr;
    QLabel *topLabel = nullptr;
    SurveyMakerQuestionWithSwitch *questions = nullptr;
    QScrollArea *preview = nullptr;
    QWidget *previewWidget = nullptr;
    QVBoxLayout *previewLayout = nullptr;
    QWidget *questionPreviews = nullptr;
    QVBoxLayout *questionPreviewLayouts = nullptr;
    QLabel *questionPreviewTopLabels = nullptr;
    QLabel *questionPreviewBottomLabels = nullptr;
};

#endif // SURVEYMAKERPAGE_H
