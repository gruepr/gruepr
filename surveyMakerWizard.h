#ifndef SURVEYMAKERWIZARD_H
#define SURVEYMAKERWIZARD_H

#include <QWizard>
#include <QMessageBox>
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


inline static const QString STDBUTTONSTYLE = "background-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                                             "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                                             "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
inline static const QString GETSTARTEDBUTTONSTYLE = "background-color: #" + QString(GRUEPRMEDBLUEHEX) + "; "
                                                    "border-style: outset; border-width: 2px; border-radius: 5px; border-color: white; "
                                                    "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
inline static const QString NEXTBUTTONSTYLE = "background-color: white; "
                                              "border-style: outset; border-width: 2px; border-radius: 5px; border-color: #" + QString(GRUEPRDARKBLUEHEX) + "; "
                                              "color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-family: 'DM Sans'; font-size: 14pt; padding: 15px;";
inline static const QString INVISBUTTONSTYLE = "background-color: rgba(0,0,0,0%); border-style: none; color: rgba(0,0,0,0%); font-size: 1pt; padding: 0px;";

inline static const QString QUESTIONPREVIEWHEAD = "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;";
inline static const QString QUESTIONPREVIEWTAIL = "<br></p>";
inline static const QString QUESTIONOPTIONSHEAD = "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>" + QObject::tr("options") + ": <b>{</b><i>";
inline static const QString QUESTIONOPTIONSTAIL = "</i><b>}</b></small>";
inline static const QString FIRSTNAMEQUESTION = QObject::tr("What is your first (or chosen) name?");
inline static const QString LASTNAMEQUESTION = QObject::tr("What is your last name?");
inline static const QString EMAILQUESTION = QObject::tr("What is your email address?");
inline static const QString GENDERQUESTION = QObject::tr("With which gender do you identify most closely?");
inline static const QString PRONOUNQUESTION = QObject::tr("What are your pronouns?");
inline static const QString URMQUESTION = QObject::tr("How do you identify your race, ethnicity, or cultural heritage?");
inline static const QString TIMEZONEQUESTION = QObject::tr("What time zone will you be based in during this class?");
enum {Sun, Mon, Tue, Wed, Thu, Fri, Sat};
inline static const QString SCHEDULEQUESTION1 = QObject::tr("Select the times that you are ");
inline static const QString SCHEDULEQUESTION2BUSY = QObject::tr("BUSY and will be UNAVAILABLE");
inline static const QString SCHEDULEQUESTION2FREE = QObject::tr("FREE and will be AVAILABLE");
inline static const QString SCHEDULEQUESTION3 = QObject::tr(" for group work.");
inline static const QString SCHEDULEQUESTION4 = QObject::tr("\n*Note: Times refer to ");
inline static const QString SCHEDULEQUESTIONHOME = QObject::tr("your home");
inline static const QString SCHEDULEQUESTION5 = QObject::tr(" timezone.");
inline static const QString SECTIONQUESTION = QObject::tr("In which section are you enrolled?");
inline static const QString PREF1TEAMMATEQUESTION = QObject::tr("List classmates you want to work with. Write their first and last name only.");
inline static const QString PREF1NONTEAMMATEQUESTION = QObject::tr("List classmates you want to avoid working with. Write their first and last name only.");
inline static const QString PREFMULTQUESTION1 = QObject::tr("Please list the name(s) of up to ");
inline static const QString PREFMULTQUESTION2YES = QObject::tr(" people who you would like to have on your team. Write their first and last name, and put a comma between multiple names.");
inline static const QString PREFMULTQUESTION2NO = QObject::tr(" people who you would like to NOT have on your team. Write their first and last name, and put a comma between multiple names.");
inline static const QString ADDLQUESTION = QObject::tr("Any additional things we should know about you before we form the teams?");


#endif // SURVEYMAKERWIZARD_H
