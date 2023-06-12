#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include "dialogs/dayNamesDialog.h"
#include "widgets/comboBoxWithElidedContents.h"
#include "widgets/surveyMakerQuestion.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>


class SurveyMakerWizard : public QWizard
{
    Q_OBJECT

public:
    SurveyMakerWizard(QWidget *parent = nullptr);

    enum Page{intro, demographics, multichoice, schedule, courseinfo, previewexport};
    static inline const QStringList pageNames = {QObject::tr("Survey Name"), QObject::tr("Demographics"), QObject::tr("Multiple Choice"),
                                                 QObject::tr("Scheduling"), QObject::tr("Course Info"), QObject::tr("Preview & Export")};
    inline static const QDateTime sunday = QDateTime(QDate(2017, 1, 1), QTime(0, 0));

    static QRegularExpressionValidator noInvalidPunctuation;
    static void badExpression(QWidget *textWidget, QString &currText, QWidget *parent = nullptr);
};


class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(SurveyMakerWizard::Page page, int numQuestions, QWidget *parent = nullptr);

protected:
    QVBoxLayout *questionLayout = nullptr;
    QList<SurveyMakerQuestionWithSwitch *> questions;
    QVBoxLayout *previewLayout = nullptr;
    QList<QWidget *> questionPreviews;
    QList<QVBoxLayout *> questionPreviewLayouts;
    QList<QLabel *> questionPreviewTopLabels;
    QList<QLabel *> questionPreviewBottomLabels;

private:
    int numQuestions;
    QGridLayout *layout = nullptr;
    QLabel *pageTitle = nullptr;
    QLabel *topLabel = nullptr;
    QWidget *questionWidget = nullptr;
    QScrollArea *questionArea = nullptr;
    QWidget *previewWidget = nullptr;
    QScrollArea *previewArea = nullptr;
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
    QLabel *bannerLeft = nullptr;
    QLabel *banner = nullptr;
    QLabel *bannerRight = nullptr;
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
    enum {firstname, lastname, email, gender, urm}; // questions in order

    QLineEdit *fn = nullptr;
    QLineEdit *ln = nullptr;
    QLineEdit *em = nullptr;
    QLabel *genderResponsesLabel = nullptr;
    QComboBox *genderResponsesComboBox = nullptr;
    QComboBox *ge = nullptr;
    QLineEdit *re = nullptr;

    void update();
};


class MultipleChoicePage : public SurveyMakerPage
{
    Q_OBJECT
    Q_PROPERTY(int numQuestions READ getNumQuestions WRITE setNumQuestions NOTIFY numQuestionsChanged)
    Q_PROPERTY(QList<QString> questionTexts READ getQuestionTexts WRITE setQuestionTexts NOTIFY questionTextsChanged)
    Q_PROPERTY(QList<QList<QString>> questionResponses READ getQuestionResponses WRITE setQuestionResponses NOTIFY questionResponsesChanged)
    Q_PROPERTY(QList<bool> questionMultis READ getQuestionMultis WRITE setQuestionMultis NOTIFY questionMultisChanged)

public:
    MultipleChoicePage(QWidget *parent = nullptr);

    void cleanupPage() override;

    void setNumQuestions(const int newNumQuestions);
    int getNumQuestions() const;
    void setQuestionTexts(const QStringList &newQuestionTexts);
    QList<QString> getQuestionTexts() const;
    void setQuestionResponses(const QList<QList<QString>> &newQuestionResponses);
    QList<QList<QString>> getQuestionResponses() const;
    void setQuestionMultis(const QList<bool> &newQuestionMultis);
    QList<bool> getQuestionMultis() const;

signals:
    void numQuestionsChanged(int newNumQuestions);
    void questionTextsChanged(QList<QString> &newQuestionTexts);
    void questionResponsesChanged(QList<QList<QString>> &newQuestionResponses);
    void questionMultisChanged(QList<bool> &newQuestionMultis);

private:
    QFrame *sampleQuestionsFrame = nullptr;
    QHBoxLayout *sampleQuestionsLayout = nullptr;
    QLabel *sampleQuestionsIcon = nullptr;
    QLabel *sampleQuestionsLabel = nullptr;
    QPushButton *sampleQuestionsButton = nullptr;
    QDialog *sampleQuestionsDialog = nullptr;
    QList<SurveyMakerMultichoiceQuestion *> multichoiceQuestions;
    QList<QSpacerItem *> spacers;
    QList<QComboBox *> qu;
    QFrame *addQuestionButtonFrame = nullptr;
    QHBoxLayout *addQuestionButtonLayout = nullptr;
    QPushButton *addQuestionButton = nullptr;
    QStringList questionTexts;
    QList<QList<QString>> questionResponses;
    QList<bool> questionMultis;

    int numQuestions = 0;
    void addQuestion();
    void deleteAQuestion(int questionNum);
};


class SchedulePage : public SurveyMakerPage
{
    Q_OBJECT
    Q_PROPERTY(QStringList dayNames READ getDayNames WRITE setDayNames NOTIFY dayNamesChanged)
    Q_PROPERTY(QString scheduleQuestion READ getScheduleQuestion WRITE setScheduleQuestion NOTIFY scheduleQuestionChanged)

public:
    SchedulePage(QWidget *parent = nullptr);

    void cleanupPage() override;

    void setDayNames(const QStringList &newDayNames);
    QStringList getDayNames() const;
    void setScheduleQuestion(const QString &newScheduleQuestion);
    QString getScheduleQuestion() const;

signals:
    void dayNamesChanged(const QStringList &newDayNames);
    void scheduleQuestionChanged(const QString &newScheduleQuestion);

private slots:
    void daysComboBox_activated(int index);
    void day_CheckBox_toggled(bool checked, QLineEdit *dayLineEdit, const QString &dayname);
    void day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname);

private:
    enum {timezone, schedule}; // questions in order

    QComboBox *tz = nullptr;
    QWidget *sc = nullptr;
    QGridLayout *scLayout = nullptr;
    QString scheduleQuestion;
    enum {busy, free} busyOrFree = free;
    QLabel *busyOrFreeLabel = nullptr;
    QComboBox *busyOrFreeComboBox = nullptr;
    QStringList timeZoneNames;
    QString baseTimezone = "";
    QLabel *baseTimezoneLabel = nullptr;
    QLineEdit *customBaseTimezone = nullptr;
    ComboBoxWithElidedContents *baseTimezoneComboBox = nullptr;
    QStringList defaultDayNames;
    QStringList dayNames;
    dayNamesDialog *daysWindow = nullptr;
    QList<QLineEdit *> dayLineEdits;
    QList<QCheckBox *> dayCheckBoxes;
    void checkDays();
    QLabel *timespanLabel = nullptr;
    QComboBox *daysComboBox = nullptr;
    QLabel *fromLabel = nullptr;
    QComboBox *fromComboBox = nullptr;
    QLabel *toLabel = nullptr;
    QComboBox *toComboBox = nullptr;

    void update();
};


class CourseInfoPage : public SurveyMakerPage
{
    Q_OBJECT
    Q_PROPERTY(QStringList sectionNames READ getSectionNames WRITE setSectionNames NOTIFY sectionNamesChanged)
    Q_PROPERTY(QStringList studentNames READ getStudentNames WRITE setStudentNames NOTIFY studentNamesChanged)

public:
    CourseInfoPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

    void setSectionNames(const QStringList &newSectionNames);
    QStringList getSectionNames() const;
    void setStudentNames(const QStringList &newStudentNames);
    QStringList getStudentNames() const;

signals:
    void sectionNamesChanged(const QStringList &newSectionNames);
    void studentNamesChanged(const QStringList &newStudentNames);

private:
    enum {section, wantToWorkWith, wantToAvoid, selectFromList}; // questions in order
    QComboBox *sc = nullptr;
    QList<QLineEdit *> sectionLineEdits;
    QList<QPushButton *> deleteSectionButtons;
    QPushButton *addSectionButton = nullptr;
    QStringList sectionNames;
    QStringList studentNames;
    QLineEdit *ww = nullptr;
    QComboBox *wwc = nullptr;
    QLineEdit *wa = nullptr;
    QComboBox *wac = nullptr;
    QLabel *uploadExplainer = nullptr;
    QPushButton *uploadButton = nullptr;

    void update();
    void deleteASection(int sectionNum);
    void addASection();
    void uploadRoster();
};


class PreviewAndExportPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PreviewAndExportPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

    bool saveSurvey();
    bool exportSurvey();

private:
    QList<SurveyMakerPreviewSection *> section;
    QList<QSpacerItem *> preSectionSpacer;
    QWidget *schedGrid = nullptr;
    QGridLayout *schedGridLayout = nullptr;
};


#endif // SURVEYMAKERWIZARD_H
