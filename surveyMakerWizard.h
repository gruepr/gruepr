#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include "gruepr_globals.h"
#include "widgets/surveyMakerPage.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

const QString stdButtonStyle = "background-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                               "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                               "color: white; font-family: 'DM Sans'; font-size: 12pt; padding: 15px;";
const QString getStartedButtonStyle = "background-color: #" + QString(GRUEPRMEDBLUEHEX) + "; "
                                      "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                                      "color: white; font-family: 'DM Sans'; font-size: 12pt; padding: 15px;";
const QString nextButtonStyle = "background-color: white; "
                                "border-style: outset; border-width: 2px; border-radius: 5px; border-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                                "color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 12pt; padding: 15px;";

class SurveyMakerWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_Intro, Page_Demographics, Page_MultipleChoice, Page_Schedule, Page_CourseInfo, Page_PreviewAndExport};

    SurveyMakerWizard(QWidget *parent = nullptr);
};


class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *topLabel = nullptr;
    QLineEdit *surveyTitle = nullptr;
};

class DemographicsPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    DemographicsPage(QWidget *parent = nullptr);

    int nextId() const override;
    void initializePage() override;
    void cleanupPage() override;

private:
};

class MultipleChoicePage : public SurveyMakerPage
{
    Q_OBJECT

public:
    MultipleChoicePage(QWidget *parent = nullptr);

    int nextId() const override;

private:
};

class SchedulePage : public SurveyMakerPage
{
    Q_OBJECT

public:
    SchedulePage(QWidget *parent = nullptr);

    int nextId() const override;

private:
};

class CourseInfoPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    CourseInfoPage(QWidget *parent = nullptr);

    int nextId() const override;
    void initializePage() override;
    void cleanupPage() override;

private:
};

class PreviewAndExportPage : public QWizardPage
{
    Q_OBJECT

public:
    PreviewAndExportPage(QWidget *parent = nullptr);

    void initializePage() override;
    int nextId() const override;

private:
    QLabel *bottomLabel = nullptr;
    QCheckBox *agreeCheckBox = nullptr;
};

#endif // SURVEYMAKERWIZARD_H
