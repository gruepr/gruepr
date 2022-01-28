#ifndef SURVEYMAKER_H
#define SURVEYMAKER_H

#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QFileInfo>
#include <QDate>
#include "widgets/attributeTabItem.h"
#include "widgets/comboBoxWithElidedContents.h"
#include "dialogs/gatherTeammatesDialog.h"
#include "dialogs/dayNamesDialog.h"
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
    void attributeTabClose(int index);
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
    bool attributeAllowMultipleResponses[MAX_ATTRIBUTES] = {false};
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
    QStringList responseOptions;
    static const int LAST_LIKERT_RESPONSE = 25;
    static const int TIMEZONE_RESPONSE_OPTION = 101;
    const QSize TABCLOSEICONSIZE = {8,8};
    enum {Sun, Mon, Tue, Wed, Thu, Fri, Sat};
};

#endif // SURVEYMAKER_H
