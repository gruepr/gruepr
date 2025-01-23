#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include "gruepr_globals.h"
#include "survey.h"
#include "dialogs/dayNamesDialog.h"
#include "dialogs/sampleQuestionsDialog.h"
#include "widgets/comboBoxWithElidedContents.h"
#include "widgets/labelThatForwardsMouseClicks.h"
#include "widgets/labelWithInstantTooltip.h"
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


/**
 * @brief The SurveyMakerWizard class is responsible for the creation and exportation of a new Gruepr survey.
 */
class SurveyMakerWizard : public QWizard
{
    Q_OBJECT

public:
    SurveyMakerWizard(QWidget *parent = nullptr);
    ~SurveyMakerWizard() override;
    SurveyMakerWizard(const SurveyMakerWizard&) = delete;
    SurveyMakerWizard operator= (const SurveyMakerWizard&) = delete;
    SurveyMakerWizard(SurveyMakerWizard&&) = delete;
    SurveyMakerWizard& operator= (SurveyMakerWizard&&) = delete;

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

/**
 * @brief The SurveyMakerPage class is responsible for representing the fields associated with a page for the survey maker GUI.
 */
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

/**
 * @brief The IntroPage class is responsible for displaying the intropage when it comes to survey creation.
 */
class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr);

    /**
     * @brief initializePage THe first page for the intro page; concerns the survey maker part of Gruepr.
     */
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

/**
 * @brief The DemographicsPage class is responsible for displaying the demographics page portion of the survey maker part of Gruepr.
 */
class DemographicsPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    DemographicsPage(QWidget *parent = nullptr);

    /**
     * @brief initializePage Initializes the demographics page of the Gruepr survey maker portion.
     */
    void initializePage() override;

    /**
     * @brief cleanupPage
     */
    void cleanupPage() override;

private:
    enum {firstname, lastname, email, gender, urm}; // questions in order

    QLineEdit *fn = nullptr;
    QLineEdit *ln = nullptr;
    QLineEdit *em = nullptr;
    QLabel *genderResponsesLabel = nullptr;
    QComboBox *genderResponsesComboBox = nullptr;
    QCheckBox *genderResponsesAllowMulti = nullptr;
    QLabel *toplabelrb = nullptr;
    QLabel *toplabelcb = nullptr;
    QList<QRadioButton *> gerb;
    QList<QCheckBox *> gecb;
    QLineEdit *re = nullptr;

    /**
     * @brief update method is used to change the fields associated with what the user wants for the demographics page of the survey they want to create.
     */
    void update();
};

/**
 * @brief The MultipleChoicePage class responsible for
 */
class MultipleChoicePage : public SurveyMakerPage
{
    Q_OBJECT
    Q_PROPERTY(int numQuestions READ getNumQuestions WRITE setNumQuestions NOTIFY numQuestionsChanged)
    Q_PROPERTY(QList<QString> questionTexts READ getQuestionTexts WRITE setQuestionTexts NOTIFY questionTextsChanged)
    Q_PROPERTY(QList<QList<QString>> questionResponses READ getQuestionResponses WRITE setQuestionResponses NOTIFY questionResponsesChanged)
    Q_PROPERTY(QList<bool> questionMultis READ getQuestionMultis WRITE setQuestionMultis NOTIFY questionMultisChanged)

public:
    MultipleChoicePage(QWidget *parent = nullptr);

    /**
     * @brief initializePage CURRENTLY AN EMPTY METHOD.
     */
    void initializePage() override;

    /**
     * @brief cleanupPage CURRENTLY AN EMPTY METHOD.
     */
    void cleanupPage() override;

    /**
     * @brief validatePage Makes sure that the user is aware of the current state of a multiple choice page for their survey.
     * @return True or false depending on what the user wants after double checking thanks to warning displayed by this method.
     */
    bool validatePage() override;

    /**
     * @brief setNumQuestions
     * @param newNumQuestions
     */
    void setNumQuestions(const int newNumQuestions);

    /**
     * @brief getNumQuestions is a getter that returns the number of questions for the current multiple choice page.
     * @return The number of questions on this multiple choice page.
     */
    int getNumQuestions() const;

    /**
     * @brief setQuestionTexts Adds question text boxes based on the nubmer of questions the user wants on this multiple choice page.
     * @param newQuestionTexts A list of questions that the user wants on this multiple choice page.
     */
    void setQuestionTexts(const QList<QString> &newQuestionTexts);

    /**
     * @brief getQuestionTexts Returns the list of questions the user wants for this multiple choice page (getter for field of this class).
     * @return The list of strings corresponding to questions the user wants on this multiple choice page.
     */
    QList<QString> getQuestionTexts() const;

    /**
     * @brief setQuestionResponses Sets the responses to user's desired questions for this multiple choice page.
     * @param newQuestionResponses The new question responses the user wants for this multiple choice page.
     */
    void setQuestionResponses(const QList<QList<QString>> &newQuestionResponses);

    /**
     * @brief getQuestionResponses Getter for the question responses field of this multiple choice page.
     * @return The questionResponses field.
     */
    QList<QList<QString>> getQuestionResponses() const;

    /**
     * @brief setQuestionMultis Set the number of multiple choice questions on this multiple choice page.
     * @param newQuestionMultis How many new multiple choice questions does the user want?
     */
    void setQuestionMultis(const QList<bool> &newQuestionMultis);

    /**
     * @brief getQuestionMultis Getter for the questionMultis field of this multiple choice page.
     * @return The questionMultis field of this multiple choice page.
     */
    QList<bool> getQuestionMultis() const;

signals:
    void numQuestionsChanged(int newNumQuestions);
    void questionTextsChanged(const QList<QString> &newQuestionTexts);
    void questionResponsesChanged(const QList<QList<QString>> &newQuestionResponses);
    void questionMultisChanged(const QList<bool> &newQuestionMultis);

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

/**
 * @brief The SchedulePage class is a GUI page responsible for creating questions relating to students' schedule for the survey that the user wants to create.
 */
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

    /**
     * @brief cleanupPage CURRENTLY AN EMPTY METHOD.
     */
    void cleanupPage() override;

    /**
     * @brief setDayNames Responsible for the weekdays the user wants on this page of their survey.
     * @param newDayNames A list of strings corresponding to weekday names.
     */
    void setDayNames(const QStringList &newDayNames);

    /**
     * @brief getDayNames Getter for dayNames field of this schedule page.
     * @return dayNames field of this schedule page.
     */
    QStringList getDayNames() const;

    /**
     * @brief setBaseTimezone Sets the timezone for the survey the user wants to create based on what timezone the user wants.
     * @param newBaseTimezone The timezone the user wants to change the survey to.
     */
    void setBaseTimezone(const QString &newBaseTimezone);

    /**
     * @brief getBaseTimezone Getter for the baseTimezone field of SchedulePage class.
     * @return baseTimezone field of SchedulePage class.
     */
    QString getBaseTimezone() const;

    /**
     * @brief setScheduleQuestion Sets the schedule questions for the SchedulePage section of this new survey.
     * @param newScheduleQuestion The new schedule question the user wants to include.
     */
    void setScheduleQuestion(const QString &newScheduleQuestion);

    /**
     * @brief getScheduleQuestion Getter for the scheduleQuestion field of SchedulePage class.
     * @return scheduleQuestion field of SchedulePage class.
     */
    QString getScheduleQuestion() const;

    /**
     * @brief setScheduleFrom Set the scheduleFrom field of SchedulePage class.
     * @param newScheduleFrom The scheduleFrom value the user wants.
     */
    void setScheduleFrom(const float newScheduleFrom);

    /**
     * @brief getScheduleFrom Getter for the fromComboBox field of SchedulePage class.
     * @return fromComboBox field of SchedulePage class, converted to a float.
     */
    float getScheduleFrom() const;

    /**
     * @brief setScheduleTo Setter for the toComboBox field of the SchedulePage class.
     * @param newScheduleTo The scheduleTo value the user wants.
     */
    void setScheduleTo(const float newScheduleTo);

    /**
     * @brief getScheduleTo Getter for the toComboBox field of the SchedulePage class.
     * @return toComboBox field of the SchedulePage class, converted to a float.
     */
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

    /**
     * @brief update the schedule grid of the update page of the SchedulePage based on the demands of the user creating the survey.
     */
    void update();
};

/**
 * @brief The CourseInfoPage class A CourseInfoPage for the survey creation section of Gruepr.
 */
class CourseInfoPage : public SurveyMakerPage
{
    Q_OBJECT
    Q_PROPERTY(QStringList sectionNames READ getSectionNames WRITE setSectionNames NOTIFY sectionNamesChanged)
    Q_PROPERTY(QStringList studentNames READ getStudentNames WRITE setStudentNames NOTIFY studentNamesChanged)

public:
    CourseInfoPage(QWidget *parent = nullptr);

    /**
     * @brief initializePage EMPTY METHOD
     */
    void initializePage() override;

    /**
     * @brief cleanupPage EMPTY METHOD
     */
    void cleanupPage() override;

    /**
     * @brief validatePage makes sure the user is aware of the changes they are making within the course info page of the survey creation.
     * @return True if the user is ok with the changes they are making, false otherwise.
     */
    bool validatePage() override;

    /**
     * @brief resizeEvent is used to resize the given QResizeEvent object.
     * @param event is the object to be resized.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief setSectionNames Allows the user to set the name of the different sections they want in their survey.
     * @param newSectionNames A list of the strings corresponding to the names of the new sections the user wants to add to their survey.
     */
    void setSectionNames(const QStringList &newSectionNames);

    /**
     * @brief getSectionNames Getter for sectionNames field of CourseInfoPage class.
     * @return QStringList, list of section names.
     */
    QStringList getSectionNames() const;

    /**
     * @brief setStudentNames Sets the student names string list field to the given one.
     * @param newStudentNames is the new list of student names.
     */
    void setStudentNames(const QStringList &newStudentNames);

    /**
     * @brief getStudentNames Getter for studentNames field.
     * @return studentNames field within CourseInfoPage class.
     */
    QStringList getStudentNames() const;

    /**
     * @brief generateTeammateQuestion is responsible with automatic teammate question generation based upon a few different factors.
     * @param wantToWorkWith True or false depending on whether or not the preference to work with/not work with a specific student is set.
     * @param typingNames Boolean that determines whether or not the student can select or type another student's name.
     * @param numClassmates The number of classmates that
     * @return
     */
    static QString generateTeammateQuestion(bool wantToWorkWith, bool typingNames, int numClassmates);

signals:
    void sectionNamesChanged(const QStringList &newSectionNames);
    void studentNamesChanged(const QStringList &newStudentNames);

private:
    enum {section, wantToWorkWith, wantToAvoid}; // questions in order
    QFrame *canvasSectionInfoFrame = nullptr;
    QHBoxLayout *canvasSectionInfoLayout = nullptr;
    LabelWithInstantTooltip *canvasSectionInfoIcon = nullptr;
    LabelWithInstantTooltip *canvasSectionInfoLabel = nullptr;
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

/**
 * @brief The PreviewAndExportPage class represents the page responsible for previewing and exporting the survey after its creation by the user.
 */
class PreviewAndExportPage : public SurveyMakerPage
{
    Q_OBJECT

public:
    PreviewAndExportPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

private slots:
    /**
     * @brief exportSurvey A general helper used to export the created survey to a variety of destinations.
     */
    void exportSurvey();

    // Helpers created to make exportSurvey() method more readable:

    /**
     * @brief exportSurveyDestinationGrueprFile exports the newly created survey to a Gruepr file.
     */
    void exportSurveyDestinationGrueprFile();

    /**
     * @brief exportSurveyDestinationTextFile exports the newly created survey to a text file.
     */
    void exportSurveyDestinationTextFile();

    /**
     * @brief exportSurveyDestinationGoogle exports the newly created survey to a Google forms.
     */
    void exportSurveyDestinationGoogle();

    /**
     * @brief exportSurveyDestinationCanvas exports the newly created survey to a Canvas quiz within a canvas class.
     */
    void exportSurveyDestinationCanvas();

private:
    QList<SurveyMakerPreviewSection *> section;
    QList<QSpacerItem *> preSectionSpacer;
    QWidget *schedGrid = nullptr;
    QGridLayout *schedGridLayout = nullptr;
    Survey *survey = nullptr;
    QRadioButton *destinationGoogle = nullptr;
    QRadioButton *destinationCanvas = nullptr;
    QRadioButton *destinationTextFiles = nullptr;
    QRadioButton *destinationGrueprFile = nullptr;
    bool surveyHasBeenExported = false;
};


#endif // SURVEYMAKERWIZARD_H
