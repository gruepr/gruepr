#ifndef SURVEYMAKERPAGE_H
#define SURVEYMAKERPAGE_H

#include <QWizardPage>
#include "gruepr_globals.h"
#include "widgets/surveyMakerQuestionWithSwitch.h"
#include <QLabel>
#include <QScrollArea>


inline static const QString TITLESTYLE = "font-size: 12pt; font-family: DM Sans; border-image: url(:/icons_new/surveyMakerWizardTitleBackground.png);";
inline static const QString TOPLABELSTYLE = "color: white; font-size: 14pt; font-family: DM Sans;"
                                            "border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png); height: 50px;";
inline static const QString PREVIEWLABELSTYLE = "QLabel {color: " + QString(GRUEPRDARKBLUEHEX) + "; font-size: 10pt; font-family: DM Sans;}"
                                                "QLabel:disabled {color: darkGray; font-size: 10pt; font-family: DM Sans;}";
inline static const QString PREVIEWLINEEDITSTYLE = "background-color: white; color: " + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 12pt;";
inline static const QString PREVIEWCOMBOBOXSTYLE = "QComboBox {background-color: white; color: " + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 12pt;}"
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
