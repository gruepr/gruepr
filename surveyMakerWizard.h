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
    QLabel *topLabel;
    QLineEdit *surveyTitle;
    QPushButton *getStartedButton;
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
    void setVisible(bool visible) override;

private slots:
    void printButtonClicked();

private:
    QLabel *bottomLabel;
    QCheckBox *agreeCheckBox;
};

#endif // SURVEYMAKERWIZARD_H
