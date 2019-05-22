#ifndef SURVEYMAKER_H
#define SURVEYMAKER_H

#include <QMainWindow>
#include <QValidator>

namespace Ui {
class SurveyMaker;
}

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
    void on_day2LineEdit_textChanged(const QString &arg1);
    void on_day3LineEdit_textChanged(const QString &arg1);
    void on_day4LineEdit_textChanged(const QString &arg1);
    void on_day5LineEdit_textChanged(const QString &arg1);
    void on_day6LineEdit_textChanged(const QString &arg1);
    void on_day7LineEdit_textChanged(const QString &arg1);
    void on_timeStartEdit_timeChanged(const QTime &time);
    void on_timeEndEdit_timeChanged(const QTime &time);
    void on_sectionCheckBox_clicked(bool checked);
    void on_sectionNamesTextEdit_textChanged();
    void on_additionalQuestionsCheckBox_clicked(bool checked);
    void on_pushButton_clicked();

    void on_day1LineEdit_editingFinished();

    void on_day2LineEdit_editingFinished();

    void on_day3LineEdit_editingFinished();

    void on_day4LineEdit_editingFinished();

    void on_day5LineEdit_editingFinished();

    void on_day6LineEdit_editingFinished();

    void on_day7LineEdit_editingFinished();

private:
    Ui::SurveyMaker *ui;
    void refreshPreview();
    void checkDays();
    void updateDays();
    bool surveyCreated = false;
    QRegExpValidator *noCommas;
    QString title="";
    bool gender=true;
    bool URM=false;
    int numAttributes=3;
    QString attributeTexts[9]={"","","","","","","","",""};
    QString allAttributeTexts="Question%201,Question%202,Question%203";
    bool schedule=true;
    QString dayNames[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    QString allDayNames="Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday";
    int startTime=9;
    int endTime=17;
    bool section;
    QStringList sectionNames={""};
    QString allSectionNames="";
    bool additionalQuestions;
};

#endif // SURVEYMAKER_H
