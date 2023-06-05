#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include <QWizardPage>
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
};


class SurveyMakerPage : public QWizardPage
{
    Q_OBJECT
public:
    SurveyMakerPage(SurveyMakerWizard::Page page, int numQuestions, QWidget *parent = nullptr);
    ~SurveyMakerPage();

protected:
    QVBoxLayout *questionLayout = nullptr;
    SurveyMakerQuestionWithSwitch *questions = nullptr;
    QVBoxLayout *previewLayout = nullptr;
    QWidget *questionPreviews = nullptr;
    QVBoxLayout *questionPreviewLayouts = nullptr;
    QLabel *questionPreviewTopLabels = nullptr;
    QLabel *questionPreviewBottomLabels = nullptr;

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
    Q_PROPERTY(QList<int> questionList READ getQuestionList WRITE setQuestionList NOTIFY questionListChanged)

public:
    MultipleChoicePage(QWidget *parent = nullptr);

    void setQuestionList(const QList<int> &newQuestionList);
    QList<int> getQuestionList() const;

signals:
    void questionListChanged(QList<int> &newQuestionList);

private:
    QFrame *sampleQuestionsFrame = nullptr;
    QHBoxLayout *sampleQuestionsLayout = nullptr;
    QLabel *sampleQuestionsIcon = nullptr;
    QLabel *sampleQuestionsLabel = nullptr;
    QPushButton *sampleQuestionsButton = nullptr;
    QDialog *sampleQuestionsDialog = nullptr;
    QList<SurveyMakerMultichoiceQuestion *> multichoiceQuestions;
    QList<QComboBox *> qu;
    QFrame *addQuestionFrame = nullptr;
    QHBoxLayout *addQuestionLayout = nullptr;
    QPushButton *addQuestionButton = nullptr;
    QList<int> questionList;

    void addQuestion();
    void deleteAQuestion(int questionNum);
};


class SchedulePage : public SurveyMakerPage
{
    Q_OBJECT

public:
    SchedulePage(QWidget *parent = nullptr);

private:
    enum {timezone, schedule}; // questions in order

    QComboBox *tz = nullptr;
    QWidget *sc = nullptr;
    QGridLayout *scLayout = nullptr;
    enum {busy, free} busyOrFree = free;
    QLabel *busyOrFreeLabel = nullptr;
    QComboBox *busyOrFreeComboBox = nullptr;
    QStringList timeZoneNames;
    QString baseTimezone = "";
    QLabel *baseTimezoneLabel = nullptr;
    ComboBoxWithElidedContents *baseTimezoneComboBox = nullptr;
    enum TimezoneType {noneOrHome, custom=2, set=4};
    QStringList defaultDayNames;
    QStringList dayNames;
    inline static const QDateTime sunday = QDateTime(QDate(2017, 1, 1), QTime(0, 0));
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
    void sectionNamesChanged(QStringList newSectionNames);
    void studentNamesChanged(QStringList newStudentNames);

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
