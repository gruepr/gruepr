#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include "canvashandler.h"
#include "googlehandler.h"
#include "gruepr_globals.h"
#include "survey.h"
#include "dialogs/dayNamesDialog.h"
#include "dialogs/sampleQuestionsDialog.h"
#include "widgets/comboBoxWithElidedContents.h"
#include "widgets/labelThatForwardsMouseClicks.h"
#include "widgets/surveyMakerQuestion.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>


class SurveyMakerWizard : public QWizard
{
    Q_OBJECT

public:
    SurveyMakerWizard(QWidget *parent = nullptr);
    ~SurveyMakerWizard() override;

    static inline const int numPages = 6;
    enum Page{introtitle, demographics, multichoice, schedule, courseinfo, previewexport};
    static inline const QList<int> numOfQuestionsInPage = {0, 5, MAX_ATTRIBUTES, 2, 3, 0};
    static inline const QStringList pageNames = {QObject::tr("Survey Name"), QObject::tr("Demographics"), QObject::tr("Multiple Choice"),
                                                 QObject::tr("Scheduling"), QObject::tr("Course Info"), QObject::tr("Preview & Export")};

    static inline const auto sundayMidnight = QDateTime(QDate(2017, 1, 1), QTime(0, 0));
    static inline const auto locale = QLocale::system();
    static inline const QStringList defaultDayNames = {locale.toString(SurveyMakerWizard::sundayMidnight.addDays(0), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(1), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(2), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(3), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(4), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(5), "dddd"),
                                                       locale.toString(SurveyMakerWizard::sundayMidnight.addDays(6), "dddd")};

    static inline const QStringList timezoneNames = [](){QStringList names = QString(TIMEZONENAMES).split(";");
                                                         for(auto &name : names){
                                                             name = name.remove('"').trimmed();
                                                         }
                                                         return names;}();

    //RegEx for punctuation not allowed within a URL, and a function to handle problem cases
    static inline const QRegularExpressionValidator noInvalidPunctuation = QRegularExpressionValidator(QRegularExpression("[^,&<>/]*"));
    static void invalidExpression(QWidget *textWidget, QString &currText, QWidget *parent = nullptr);

    QFileInfo saveFileLocation;
    bool previewPageVisited = false;

public slots:
    void loadSurvey(int customButton);
};


class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(SurveyMakerWizard::Page page, QWidget *parent = nullptr);

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
    QList<QRadioButton *> ge;
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

    void initializePage() override;
    void cleanupPage() override;

    void setNumQuestions(const int newNumQuestions);
    int getNumQuestions() const;
    void setQuestionTexts(const QList<QString> &newQuestionTexts);
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
    SampleQuestionsDialog *sampleQuestionsDialog = nullptr;
    QList<SurveyMakerMultichoiceQuestion *> multichoiceQuestions;
    QList<QSpacerItem *> spacers;
    QList<QFrame *> previewSeparators;
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
    Q_PROPERTY(QString baseTimezone READ getBaseTimezone WRITE setBaseTimezone NOTIFY baseTimezoneChanged)
    Q_PROPERTY(QString scheduleQuestion READ getScheduleQuestion WRITE setScheduleQuestion NOTIFY scheduleQuestionChanged)
    Q_PROPERTY(float scheduleFrom READ getScheduleFrom WRITE setScheduleFrom NOTIFY scheduleFromChanged)
    Q_PROPERTY(float scheduleTo READ getScheduleTo WRITE setScheduleTo NOTIFY scheduleToChanged)

public:
    SchedulePage(QWidget *parent = nullptr);

    void cleanupPage() override;

    void setDayNames(const QStringList &newDayNames);
    QStringList getDayNames() const;
    void setBaseTimezone(const QString &newBaseTimezone);
    QString getBaseTimezone() const;
    void setScheduleQuestion(const QString &newScheduleQuestion);
    QString getScheduleQuestion() const;
    void setScheduleFrom(const float newScheduleFrom);
    float getScheduleFrom() const;
    void setScheduleTo(const float newScheduleTo);
    float getScheduleTo() const;

    enum scheduleType {busy, free};

    static inline const QList<QPair<QString, int>> resolutionValues = {{QObject::tr("2 hr"), 120/MIN_SCHEDULE_RESOLUTION},
                                                                       {QObject::tr("1 hr"), 60/MIN_SCHEDULE_RESOLUTION},
                                                                       {QObject::tr("30 min"), 30/MIN_SCHEDULE_RESOLUTION},
                                                                       {QObject::tr("15 min"), 15/MIN_SCHEDULE_RESOLUTION}};  // int value is # of time blocks
    static inline const QList<QPair<QString, QString>> timeFormats = [](){const QStringList formats = QString(TIMEFORMATS).split(';');
                                                                          const QStringList examples = QString(TIMEFORMATEXAMPLES).split(';');
                                                                          QList<QPair<QString, QString>> x;
                                                                          x.reserve(formats.size());
                                                                          for(int i = 0; i < formats.size(); i++) {
                                                                              x << qMakePair(formats.at(i), examples.at(i));
                                                                          }
                                                                          return(x);}();

    static QString generateScheduleQuestion(bool scheduleAsBusy, bool timezoneOn, const QString &baseTimezone);

signals:
    void dayNamesChanged(const QStringList &newDayNames);
    void baseTimezoneChanged(const QString &newBaseTimezone);
    void scheduleQuestionChanged(const QString &newScheduleQuestion);
    void scheduleFromChanged(const float newScheduleFrom);
    void scheduleToChanged(const float newScheduleTo);

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
    QLabel *busyOrFreeLabel = nullptr;
    QComboBox *busyOrFreeComboBox = nullptr;
    QString baseTimezone = "";
    QLabel *baseTimezoneLabel = nullptr;
    QLineEdit *customBaseTimezone = nullptr;
    ComboBoxWithElidedContents *baseTimezoneComboBox = nullptr;
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
    QLabel *resolutionLabel = nullptr;
    QComboBox *resolutionComboBox = nullptr;
    QLabel *formatLabel = nullptr;
    QComboBox *formatComboBox = nullptr;
    const int DEFAULTTIMEFORMAT = 7;
    const int DEFAULTSCHEDSTARTTIME = 10;  //10 am
    const int DEFAULTSCHEDENDTIME = 17;  //5 pm
    const int DEFAULTSCHEDRESOLUTION = 60;  //1 hr


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
    void resizeEvent(QResizeEvent *event) override;

    void setSectionNames(const QStringList &newSectionNames);
    QStringList getSectionNames() const;
    void setStudentNames(const QStringList &newStudentNames);
    QStringList getStudentNames() const;

    static QString generateTeammateQuestion(bool wantToWorkWith, bool typingNames, int numClassmates);

signals:
    void sectionNamesChanged(const QStringList &newSectionNames);
    void studentNamesChanged(const QStringList &newStudentNames);

private:
    enum {section, wantToWorkWith, wantToAvoid}; // questions in order
    QList<QLineEdit *> sectionLineEdits;
    QList<int> visibleSectionLineEdits;
    QVBoxLayout *sectionsPreviewLayout = nullptr;
    QList<QRadioButton *> sc;
    QList<QPushButton *> deleteSectionButtons;
    QPushButton *addSectionButton = nullptr;
    const int MAX_SECTIONSEXPECTED = 10;
    QStringList sectionNames;
    int numSectionsEntered = 0;
    QPlainTextEdit *ww = nullptr;
    QList<QComboBox *> wwc;
    QPlainTextEdit *wa = nullptr;
    QList<QComboBox *> wac;
    QLabel *numPrefTeammatesExplainer = nullptr;
    QSpinBox *numPrefTeammatesSpinBox = nullptr;
    int numPrefTeammates = 1;
    LabelThatForwardsMouseClicks *selectFromRosterLabel = nullptr;
    SwitchButton *selectFromRosterSwitch = nullptr;
    QLabel *uploadExplainer = nullptr;
    QPushButton *uploadButton = nullptr;
    QStringList studentNames;

    void update();
    void resizeQuestionPlainTextEdit(QPlainTextEdit *const questionPlainTextEdit);
    void deleteASection(int sectionNum, bool pauseVisualUpdate = false);
    void addASection(bool pauseVisualUpdate = false);
    bool uploadRoster();
};


class PreviewAndExportPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PreviewAndExportPage(QWidget *parent = nullptr);
    ~PreviewAndExportPage() override;

    void initializePage() override;
    void cleanupPage() override;

private slots:
    void exportSurvey();

private:
    QList<SurveyMakerPreviewSection *> section;
    QList<QSpacerItem *> preSectionSpacer;
    QWidget *schedGrid = nullptr;
    QGridLayout *schedGridLayout = nullptr;
    Survey *survey = nullptr;
    GoogleHandler *google = nullptr;
    CanvasHandler *canvas = nullptr;
    QRadioButton *destinationGoogle = nullptr;
    QRadioButton *destinationCanvas = nullptr;
    QRadioButton *destinationTextFiles = nullptr;
    QRadioButton *destinationGrueprFile = nullptr;
    bool surveyHasBeenExported = false;
};


#endif // SURVEYMAKERWIZARD_H
