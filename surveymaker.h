#ifndef SURVEYMAKER_H
#define SURVEYMAKER_H

#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QFileInfo>
#include <QDate>
#include "customDialogs.h"
#include "gruepr_structs_and_consts.h"

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
    void on_attributeScrollBar_valueChanged(int value);
    void on_attributeTextEdit_textChanged();
    void on_attributeComboBox_currentIndexChanged(int index);
    void on_scheduleCheckBox_clicked(bool checked);
    void on_daysComboBox_currentIndexChanged(int index);
    void on_day1CheckBox_toggled(bool checked);
    void on_day2CheckBox_toggled(bool checked);
    void on_day3CheckBox_toggled(bool checked);
    void on_day4CheckBox_toggled(bool checked);
    void on_day5CheckBox_toggled(bool checked);
    void on_day6CheckBox_toggled(bool checked);
    void on_day7CheckBox_toggled(bool checked);
    void on_day1LineEdit_textChanged(const QString &arg1);
    void on_day1LineEdit_editingFinished();
    void on_day2LineEdit_textChanged(const QString &arg1);
    void on_day2LineEdit_editingFinished();
    void on_day3LineEdit_textChanged(const QString &arg1);
    void on_day3LineEdit_editingFinished();
    void on_day4LineEdit_textChanged(const QString &arg1);
    void on_day4LineEdit_editingFinished();
    void on_day5LineEdit_textChanged(const QString &arg1);
    void on_day5LineEdit_editingFinished();
    void on_day6LineEdit_textChanged(const QString &arg1);
    void on_day6LineEdit_editingFinished();
    void on_day7LineEdit_textChanged(const QString &arg1);
    void on_day7LineEdit_editingFinished();
    void on_timeStartEdit_timeChanged(const QTime &time);
    void on_timeEndEdit_timeChanged(const QTime &time);
    void on_sectionCheckBox_clicked(bool checked);
    void on_sectionNamesTextEdit_textChanged();
    void on_additionalQuestionsCheckBox_clicked(bool checked);
    void on_makeSurveyButton_clicked();
    void on_openSurveyButton_clicked();
    void on_saveSurveyButton_clicked();
    void on_helpButton_clicked();
    void on_aboutButton_clicked();

private:
    Ui::SurveyMaker *ui;
    void refreshPreview();
    void checkDays();
    bool surveyCreated = false;
    QRegularExpressionValidator *noCommas;
    QString title = "";
    bool gender = true;
    bool URM = false;
    int numAttributes = 3;
    QString attributeTexts[maxAttributes] = {"","","","","","","","",""};
    int attributeResponses[maxAttributes] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool schedule = true;
    // local day names, using the fact that 1/1/2017 is a Sunday
    const QString day1name = QDate(2017, 1, 1).toString("dddd");
    const QString day2name = QDate(2017, 1, 2).toString("dddd");
    const QString day3name = QDate(2017, 1, 3).toString("dddd");
    const QString day4name = QDate(2017, 1, 4).toString("dddd");
    const QString day5name = QDate(2017, 1, 5).toString("dddd");
    const QString day6name = QDate(2017, 1, 6).toString("dddd");
    const QString day7name = QDate(2017, 1, 7).toString("dddd");
    QString dayNames[7] = {day1name,day2name,day3name,day4name,day5name,day6name,day7name};
    int startTime = 10;
    int endTime = 17;
    bool section = false;
    QStringList sectionNames = {""};
    bool additionalQuestions = false;
    QString URL;
    QFileInfo saveFileLocation;
    const QStringList responseOptions = {
        "response options will be added later, after creating the form",
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
        "custom options, to be added after creating the form",
    };
};

#endif // SURVEYMAKER_H
