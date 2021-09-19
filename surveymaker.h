#ifndef SURVEYMAKER_H
#define SURVEYMAKER_H

#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QFileInfo>
#include <QDate>
#include "widgets\attributeTabItem.h"
#include "widgets\comboBoxWithElidedContents.h"
#include "dialogs\gatherTeammatesDialog.h"
#include "dialogs\dayNamesDialog.h"
#include "gruepr_consts.h"

namespace Ui {class SurveyMaker;}

class SurveyMaker : public QMainWindow
{
    Q_OBJECT

public:
    explicit SurveyMaker(QWidget *parent = nullptr);
    ~SurveyMaker();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_surveyTitleLineEdit_textChanged(const QString &arg1);
    void on_genderCheckBox_clicked(bool checked);
    void on_URMCheckBox_clicked(bool checked);
    void on_attributeCountSpinBox_valueChanged(int arg1);
    void attributeTextChanged(int currAttribute);
    void on_timezoneCheckBox_clicked(bool checked);
    void on_scheduleCheckBox_clicked(bool checked);
    void checkTimezoneAndSchedule();
    void on_busyFreeComboBox_currentIndexChanged(const QString &arg1);
    void baseTimezoneComboBox_currentIndexChanged(int arg1);
    void on_baseTimezoneLineEdit_textChanged();
    void on_daysComboBox_activated(int index);
    void day_CheckBox_toggled(bool checked, QLineEdit *dayLineEdit, const QString &dayname);
    void day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname);
    void on_timeStartEdit_timeChanged(QTime time);
    void on_timeEndEdit_timeChanged(QTime time);
    void on_sectionCheckBox_clicked(bool checked);
    void on_sectionNamesTextEdit_textChanged();
    void on_preferredTeammatesCheckBox_clicked(bool checked);
    void on_preferredNonTeammatesCheckBox_clicked(bool checked);
    void on_numAllowedSpinBox_valueChanged(int arg1);
    void on_additionalQuestionsCheckBox_clicked(bool checked);
    void on_makeSurveyButton_clicked();
    void on_surveyDestinationBox_currentIndexChanged(const QString &arg1);
    void openSurvey();
    void saveSurvey();
    void settingsWindow();
    void helpWindow();
    void aboutWindow();

private:
    Ui::SurveyMaker *ui;
    QVector<attributeTabItem*> attributeTab;
    void refreshPreview();
    void checkDays();
    bool surveyCreated = false;
    QRegularExpressionValidator *noInvalidPunctuation;
    QString title = "";
    bool gender = true;
    bool URM = false;
    int numAttributes = 3;
    QString attributeTexts[MAX_ATTRIBUTES] = {""};
    int attributeResponses[MAX_ATTRIBUTES] = {0};
    bool schedule = true;
    enum {busy, free} busyOrFree = free;
    bool timezone = false;
    QString baseTimezone = "";
    ComboBoxWithElidedContents *baseTimezoneComboBox = nullptr;
    enum TimezoneType {noneOrHome, custom=2, set=4};
    QStringList defaultDayNames;
    QString dayNames[MAX_DAYS];
    const QDate sunday = QDate(2017, 1, 1);
    QLineEdit *dayLineEdits[MAX_DAYS] = {nullptr};
    QCheckBox *dayCheckBoxes[MAX_DAYS] = {nullptr};
    dayNamesDialog *daysWindow = nullptr;
    int startTime = 10;
    int endTime = 17;
    bool section = false;
    QStringList sectionNames = {""};
    bool preferredTeammates = false;
    bool preferredNonTeammates = false;
    int numPreferredAllowed = 1;
    bool additionalQuestions = false;
    static void postGoogleURL(SurveyMaker *survey = nullptr);
    static void createFiles(SurveyMaker *survey = nullptr);
    void (*generateSurvey)(SurveyMaker *survey) = SurveyMaker::postGoogleURL;
    QFileInfo saveFileLocation;
    QStringList responseOptions = {
        "Yes / No",
        "Yes / Maybe / No",
        "Definitely / Probably / Maybe / Probably not / Definitely not",
        "Strongly preferred / Preferred / Opposed / Strongly opposed",
        "True / False",
        "Like me / Not like me",
        "Agree / Disagree",
        "Strongly agree / Agree / Undecided / Disagree / Strongly disagree",
        "4.0— 3.75 / 3.74— 3.5 / 3.49— 3.25 / 3.24— 3.0 / 2.99— 2.75 / 2.74— 2.5 / 2.49— 2.0 / Below 2.0 / Not sure, or prefer not to say",
        "100— 90 / 89— 80 / 79— 70 / 69— 60 / 59— 50 / Below 50 / Not sure, or prefer not to say",
        "A / B / C / D / F / Not sure, or prefer not to say",
        "Very high / Above average / Average / Below average / Very low",
        "Excellent / Very good / Good / Fair / Poor",
        "Highly positive / Somewhat positive / Neutral / Somewhat negative / Highly negative",
        "A lot of experience / Some experience / Little experience / No experience",
        "Extremely / Very / Moderately / Slightly / Not at all",
        "A lot / Some / Very Little / None",
        "Much more / More / About the same / Less / Much less",
        "Most of the time / Some of the time / Seldom / Never",
        "Available / Available, but prefer not to / Not available",
        "Very frequently / Frequently / Occasionally / Rarely / Never",
        "Definitely will / Probably will / Probably won't / Definitely won't",
        "Very important / Important / Somewhat important / Not important",
        "Leader / Mix of leader and follower / Follower",
        "Highly confident / Moderately confident / Somewhat confident / Not confident",
        "1 / 2 / 3 / 4",
        "1 / 2 / 3 / 4 / 5",
        "1 / 2 / 3 / 4 / 5 / 6",
        "1 / 2 / 3 / 4 / 5 / 6 / 7",
        "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8",
        "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9",
        "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9 / 10",
        "custom options, to be added after creating the form",};
    static const int LAST_LIKERT_RESPONSE = 25;
    static const int TIMEZONE_RESPONSE_OPTION = 101;
};

#endif // SURVEYMAKER_H
