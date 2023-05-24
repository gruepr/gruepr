#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include "widgets/surveyMakerPage.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>


class SurveyMakerWizard : public QWizard
{
    Q_OBJECT

public:
    SurveyMakerWizard(QWidget *parent = nullptr);
};


class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr);

    void initializePage() override;

private:
    QGridLayout *layout = nullptr;
    QLabel *pageTitle = nullptr;
    QLabel *banner = nullptr;
    QLabel *topLabel = nullptr;
    QLineEdit *surveyTitle = nullptr;
    QLabel *bottomLabel = nullptr;
    QPushButton *getStartedButton = nullptr;
};

class DemographicsPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    DemographicsPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

private:
};

class MultipleChoicePage : public SurveyMakerPage
{
    Q_OBJECT

public:
    MultipleChoicePage(QWidget *parent = nullptr);

private:
};

class SchedulePage : public SurveyMakerPage
{
    Q_OBJECT

public:
    SchedulePage(QWidget *parent = nullptr);

private:
};

class CourseInfoPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    CourseInfoPage(QWidget *parent = nullptr);

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

private:
    QLabel *bottomLabel = nullptr;
    QCheckBox *agreeCheckBox = nullptr;
};

#endif // SURVEYMAKERWIZARD_H
