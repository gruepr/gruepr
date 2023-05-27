#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include "widgets/surveyMakerPage.h"
#include "widgets/comboBoxWithElidedContents.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
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
    QComboBox *tz = nullptr;
    enum {busy, free} busyOrFree = free;
    QStringList timeZoneNames;
    QString baseTimezone = "";
    QLabel *baseTimezoneLabel = nullptr;
    ComboBoxWithElidedContents *baseTimezoneComboBox = nullptr;
    enum TimezoneType {noneOrHome, custom=2, set=4};
    QStringList defaultDayNames;
    QStringList dayNames;
    inline static const QDate sunday = QDate(2017, 1, 1);
    QLabel *daysLabel = nullptr;
    QComboBox *daysComboBox = nullptr;

    void update();
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

inline static const QString QUESTIONPREVIEWHEAD = "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;";
inline static const QString QUESTIONPREVIEWTAIL = "<br></p>";
inline static const QString QUESTIONOPTIONSHEAD = "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>" + QObject::tr("options") + ": <b>{</b><i>";
inline static const QString QUESTIONOPTIONSTAIL = "</i><b>}</b></small>";
inline static const QString FIRSTNAMEQUESTION = QObject::tr("What is your first name (or the name you prefer to be called)?");
inline static const QString LASTNAMEQUESTION = QObject::tr("What is your last name?");
inline static const QString EMAILQUESTION = QObject::tr("What is your email address?");
inline static const QString GENDERQUESTION = QObject::tr("With which gender do you identify most closely?");
inline static const QString PRONOUNQUESTION = QObject::tr("What are your pronouns?");
inline static const QString URMQUESTION = QObject::tr("How do you identify your race, ethnicity, or cultural heritage?");
inline static const QString TIMEZONEQUESTION = QObject::tr("What time zone will you be based in during this class?");
enum {Sun, Mon, Tue, Wed, Thu, Fri, Sat};
inline static const QString SCHEDULEQUESTION1 = QObject::tr("Check the times that you are ");
inline static const QString SCHEDULEQUESTION2BUSY = QObject::tr("BUSY and will be UNAVAILABLE");
inline static const QString SCHEDULEQUESTION2FREE = QObject::tr("FREE and will be AVAILABLE");
inline static const QString SCHEDULEQUESTION3 = QObject::tr(" for group work.");
inline static const QString SCHEDULEQUESTION4 = QObject::tr("\n*Note: Times refer to ");
inline static const QString SCHEDULEQUESTIONHOME = QObject::tr("your home");
inline static const QString SCHEDULEQUESTION5 = QObject::tr(" timezone.");
inline static const QString SECTIONQUESTION = QObject::tr("In which section are you enrolled?");
inline static const QString PREF1TEAMMATEQUESTION = QObject::tr("Please write the name of someone who you would like to have on your team. Write their first and last name only.");
inline static const QString PREF1NONTEAMMATEQUESTION = QObject::tr("Please write the name of someone who you would like to NOT have on your team. Write their first and last name only.");
inline static const QString PREFMULTQUESTION1 = QObject::tr("Please list the name(s) of up to ");
inline static const QString PREFMULTQUESTION2YES = QObject::tr(" people who you would like to have on your team. Write their first and last name, and put a comma between multiple names.");
inline static const QString PREFMULTQUESTION2NO = QObject::tr(" people who you would like to NOT have on your team. Write their first and last name, and put a comma between multiple names.");
inline static const QString ADDLQUESTION = QObject::tr("Any additional things we should know about you before we form the teams?");


#endif // SURVEYMAKERWIZARD_H
