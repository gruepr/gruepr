#include "surveymaker.h"
#include "ui_surveymaker.h"
#include <QRegularExpression>
#include <QMessageBox>
#include <QtNetwork>
#include <QDesktopServices>

SurveyMaker::SurveyMaker(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SurveyMaker)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/surveymaker.png"));

    QRegularExpression nc("[^,&<>]*");
    noCommas = new QRegularExpressionValidator(nc, this);

    // Create the initial survey preview. Waits 200 msec to do this--for some reason, doing this immediately prevents the embedded font from being used in every field
    QTimer::singleShot(200, [this](){ refreshPreview(); });
}

SurveyMaker::~SurveyMaker()
{
    delete noCommas;
    delete ui;
}

void SurveyMaker::refreshPreview()
{
    int currPos = ui->previewText->verticalScrollBar()->value();
    QString preview = "<h2>" + title + "</h2>";
    preview += "<h3>First, some basic information</h3>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your first name (or the name you prefer to be called)?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your last name?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your school email address?<br></p>";
    preview += gender?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;With which gender do you identify?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>woman </i><b>|</b><i> man </i><b>|</b><i> non-binary </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += URM?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Do you identify as a member of an underrepresented minority?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>yes </i><b>|</b><i> no </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += "<hr>";
    if(numAttributes > 0)
    {
        preview += "<h3>This set of questions is about your past experiences/education and teamwork preferences.</h3>";
        for(int attrib = 0; attrib < numAttributes; attrib++)
        {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;" +
                           (attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : attributeTexts[attrib])
                           + "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                           "<small>response options will be added later, after creating the form</small>";
        }
        preview += "<hr>";
    }
    if(schedule)
    {
        preview += "<h3>Please tell us about your weekly schedule.</h3>";
        preview += "<p>&nbsp;&nbsp;&nbsp;<i>grid of checkboxes:</i></p>";
        preview += "<p>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
        for(int time = startTime; time <= endTime; time++)
        {
            preview += QTime(time, 0).toString("hA") + "&nbsp;&nbsp;&nbsp;&nbsp;";
        }
        preview += "</p>";
        for(int day = 0; day < 7; day++)
        {
            if(!(dayNames[day].isEmpty()))
            preview += "<p>&nbsp;&nbsp;&nbsp;" + dayNames[day] + "</p>";
        }
        preview += "<hr>";
    }
    if(section || additionalQuestions)
    {
        preview += "<h3>Some final questions.</h3>";
        if(section)
        {
            preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;In which section are you enrolled?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                       "<small>options: <b>{ </b><i>";
            for(int sect = 0; sect < sectionNames.size(); sect++)
            {
                if(sect > 0)
                {
                    preview += " </i><b>|</b><i> ";
                }
                preview += sectionNames[sect];
            }
            preview += "</i><b> }</b></small></p>";
        }
        if(additionalQuestions)
        {
            preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Any additional things we should know about you before we form the teams?</p>";
        }
    }

    ui->previewText->setHtml(preview);
    ui->previewText->verticalScrollBar()->setValue(currPos);
}

void SurveyMaker::on_pushButton_clicked()
{
    QString URL = "https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/exec?";
    URL += "title=" + QUrl::toPercentEncoding(ui->surveyTitleLineEdit->text()) + "&";
    URL += "gend=" + QString(gender? "true" : "false") + "&";
    URL += "urm=" + QString(URM? "true" : "false") + "&";
    URL += "numattr=" + QString::number(numAttributes) + "&";
    URL += "attrtext=" + allAttributeTexts + "&";
    URL += "sched=" + QString(schedule? "true" : "false") + "&";
    URL += "start=" + QString::number(startTime) + "&end=" + QString::number(endTime) + "&days=" + allDayNames + "&";
    URL += "sect=" + QString(section? "true" : "false") + "&";
    URL += "sects=" + allSectionNames + "&";
    URL += "addl=" + QString(additionalQuestions? "true" : "false");

    //make sure we can connect to google
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QEventLoop loop;
    QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
    connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    bool weGotProblems = (networkReply->bytesAvailable() == 0);
    delete manager;

    if(weGotProblems)
    {
        QMessageBox::critical(this, tr("Error!"), tr("There does not seem to be an internet connection.\nCheck your network connection and try again.\nThe survey has NOT been created."));
        return;
    }
    else
    {
        QMessageBox createSurvey(this);
        createSurvey.setIcon(QMessageBox::Information);
        createSurvey.setWindowTitle(tr("Survey Creation"));
        createSurvey.setText(tr("The next step will open a browser window and connect to Google.\n"
                             "You may be asked first to authorize gruepr to access your Google Drive.\n"
                             "This authorization is needed so that the Google Form can be created for you.\n"
                             "The survey creation itself will take 10 - 20 seconds. During this time, the browser window will be blank.\n"
                             "A screen with additional information will be shown in your browser window as soon as the process is complete."));
        createSurvey.setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
        if(createSurvey.exec() == QMessageBox::Ok)
        {
            QDesktopServices::openUrl(QUrl(URL));
            surveyCreated = true;
        }
    }
}

void SurveyMaker::on_surveyTitleLineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->surveyTitleLineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the survey title:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    title = ui->surveyTitleLineEdit->text().trimmed();
    refreshPreview();
}

void SurveyMaker::on_genderCheckBox_clicked(bool checked)
{
    gender = checked;
    refreshPreview();
}

void SurveyMaker::on_URMCheckBox_clicked(bool checked)
{
    URM = checked;
    refreshPreview();
}

void SurveyMaker::on_attributeCountSpinBox_valueChanged(int arg1)
{
    numAttributes = arg1;
    ui->attributeScrollBar->setMaximum(std::max(arg1-1,0));
    ui->attributeScrollBar->setEnabled(numAttributes > 0);
    ui->attributeTextEdit->setEnabled(numAttributes > 0);
    refreshPreview();
}

void SurveyMaker::on_attributeScrollBar_valueChanged(int value)
{
    ui->attributeTextEdit->setPlainText(attributeTexts[value]);
    ui->attributeTextEdit->setPlaceholderText(tr("Enter the text of attribute question ") + QString::number(value+1) + ".");
}

void SurveyMaker::on_attributeTextEdit_textChanged()
{
    //validate entry
    QString currText = ui->attributeTextEdit->toPlainText();
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->attributeTextEdit->setPlainText(ui->attributeTextEdit->toPlainText().remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the question text:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    attributeTexts[ui->attributeScrollBar->value()] = ui->attributeTextEdit->toPlainText().simplified();
    allAttributeTexts = "";
    for(int attrib = 0; attrib < numAttributes; attrib++)
    {
        if(attrib != 0)
        {
            allAttributeTexts += ",";
        }

        if(!(attributeTexts[attrib].isEmpty()))
        {
            allAttributeTexts += QUrl::toPercentEncoding(attributeTexts[attrib]);
        }
        else
        {
            allAttributeTexts += QUrl::toPercentEncoding(tr("Question ") + QString::number(attrib+1));
        }
    }
    refreshPreview();
}

void SurveyMaker::on_scheduleCheckBox_clicked(bool checked)
{
    schedule = checked;
    ui->daysComboBox->setEnabled(checked);
    ui->day1CheckBox->setEnabled(checked);
    ui->day1LineEdit->setEnabled(checked);
    ui->day2CheckBox->setEnabled(checked);
    ui->day2LineEdit->setEnabled(checked);
    ui->day3CheckBox->setEnabled(checked);
    ui->day3LineEdit->setEnabled(checked);
    ui->day4CheckBox->setEnabled(checked);
    ui->day4LineEdit->setEnabled(checked);
    ui->day5CheckBox->setEnabled(checked);
    ui->day5LineEdit->setEnabled(checked);
    ui->day6CheckBox->setEnabled(checked);
    ui->day6LineEdit->setEnabled(checked);
    ui->day7CheckBox->setEnabled(checked);
    ui->day7LineEdit->setEnabled(checked);
    ui->timeStartEdit->setEnabled(checked);
    ui->timeEndEdit->setEnabled(checked);
    refreshPreview();
}

void SurveyMaker::on_daysComboBox_currentIndexChanged(int index)
{
    if(index == 0)
    {
        //All Days
        ui->day1CheckBox->setChecked(true);
        ui->day2CheckBox->setChecked(true);
        ui->day3CheckBox->setChecked(true);
        ui->day4CheckBox->setChecked(true);
        ui->day5CheckBox->setChecked(true);
        ui->day6CheckBox->setChecked(true);
        ui->day7CheckBox->setChecked(true);
    }
    else if(index == 1)
    {
        //Weekdays
        ui->day1CheckBox->setChecked(false);
        ui->day2CheckBox->setChecked(true);
        ui->day3CheckBox->setChecked(true);
        ui->day4CheckBox->setChecked(true);
        ui->day5CheckBox->setChecked(true);
        ui->day6CheckBox->setChecked(true);
        ui->day7CheckBox->setChecked(false);

    }
    else if(index == 2)
    {
        //Weekends
        ui->day1CheckBox->setChecked(true);
        ui->day2CheckBox->setChecked(false);
        ui->day3CheckBox->setChecked(false);
        ui->day4CheckBox->setChecked(false);
        ui->day5CheckBox->setChecked(false);
        ui->day6CheckBox->setChecked(false);
        ui->day7CheckBox->setChecked(true);
    }
    else
    {
        //Custom Days
    }
}

void SurveyMaker::on_day1CheckBox_toggled(bool checked)
{
    ui->day1LineEdit->setText(checked? tr("Sunday") : "");
    ui->day1LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day2CheckBox_toggled(bool checked)
{
    ui->day2LineEdit->setText(checked? tr("Monday") : "");
    ui->day2LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day3CheckBox_toggled(bool checked)
{
    ui->day3LineEdit->setText(checked? tr("Tuesday") : "");
    ui->day3LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day4CheckBox_toggled(bool checked)
{
    ui->day4LineEdit->setText(checked? tr("Wednesday") : "");
    ui->day4LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day5CheckBox_toggled(bool checked)
{
    ui->day5LineEdit->setText(checked? tr("Thursday") : "");
    ui->day5LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day6CheckBox_toggled(bool checked)
{
    ui->day6LineEdit->setText(checked? tr("Friday") : "");
    ui->day6LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::on_day7CheckBox_toggled(bool checked)
{
    ui->day7LineEdit->setText(checked? tr("Saturday") : "");
    ui->day7LineEdit->setEnabled(checked);
    checkDays();
}

void SurveyMaker::checkDays()
{
    bool weekends = ui->day1CheckBox->isChecked() && ui->day7CheckBox->isChecked();
    bool noWeekends = !(ui->day1CheckBox->isChecked() || ui->day7CheckBox->isChecked());
    bool weekdays = ui->day2CheckBox->isChecked() && ui->day3CheckBox->isChecked() &&
                    ui->day4CheckBox->isChecked() && ui->day5CheckBox->isChecked() && ui->day6CheckBox->isChecked();
    bool noWeekdays = !(ui->day2CheckBox->isChecked() || ui->day3CheckBox->isChecked() ||
                        ui->day4CheckBox->isChecked() || ui->day5CheckBox->isChecked() || ui->day6CheckBox->isChecked());
    if(weekends && weekdays)
    {
        ui->daysComboBox->setCurrentIndex(0);
    }
    else if(weekdays && noWeekends)
    {
        ui->daysComboBox->setCurrentIndex(1);
    }
    else if(weekends && noWeekdays)
    {
        ui->daysComboBox->setCurrentIndex(2);
    }
    else
    {
        ui->daysComboBox->setCurrentIndex(3);
    }
}

void SurveyMaker::on_day1LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day1LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[0] = ui->day1LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day1LineEdit_editingFinished()
{
    if((dayNames[0].isEmpty()))
    {
        ui->day1CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day2LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day2LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[1] = ui->day2LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day2LineEdit_editingFinished()
{
    if((dayNames[1].isEmpty()))
    {
        ui->day2CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day3LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day3LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[2] = ui->day3LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day3LineEdit_editingFinished()
{
    if((dayNames[2].isEmpty()))
    {
        ui->day3CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day4LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day4LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[3] = ui->day4LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day4LineEdit_editingFinished()
{
    if((dayNames[3].isEmpty()))
    {
        ui->day4CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day5LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day5LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[4] = ui->day5LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day5LineEdit_editingFinished()
{
    if((dayNames[4].isEmpty()))
    {
        ui->day5CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day6LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day6LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[5] = ui->day6LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day6LineEdit_editingFinished()
{
    if((dayNames[5].isEmpty()))
    {
        ui->day6CheckBox->setChecked(false);
    }
}

void SurveyMaker::on_day7LineEdit_textChanged(const QString &arg1)
{
    //validate entry
    QString currText = arg1;
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->day7LineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the day name:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    dayNames[6] = ui->day7LineEdit->text().trimmed();
    updateDays();
    refreshPreview();
}

void SurveyMaker::on_day7LineEdit_editingFinished()
{
    if((dayNames[6].isEmpty()))
    {
        ui->day7CheckBox->setChecked(false);
    }
}

void SurveyMaker::updateDays()
{
    allDayNames = "";
    bool firstOne = true;
    for(int day = 0; day < 7; day++)
    {
        if(!(dayNames[day].isEmpty()))
        {
            if(!firstOne)
            {
                allDayNames += ",";
            }
            allDayNames += QUrl::toPercentEncoding(dayNames[day]);
            firstOne = false;
        }
    }
}

void SurveyMaker::on_timeStartEdit_timeChanged(const QTime &time)
{
    startTime = time.hour();
    if(ui->timeEndEdit->time() <= time)
        ui->timeEndEdit->setTime(QTime(time.hour(), 0));
    refreshPreview();
}

void SurveyMaker::on_timeEndEdit_timeChanged(const QTime &time)
{
    endTime = time.hour();
    if(ui->timeStartEdit->time() >= time)
        ui->timeStartEdit->setTime(QTime(time.hour(), 0));
    refreshPreview();
}

void SurveyMaker::on_sectionCheckBox_clicked(bool checked)
{
    section = checked;
    ui->sectionNamesTextEdit->setEnabled(checked);
    refreshPreview();
}

void SurveyMaker::on_sectionNamesTextEdit_textChanged()
{
    //validate entry
    QString currText = ui->sectionNamesTextEdit->toPlainText();
    int currPos = 0;
    if(noCommas->validate(currText, currPos) != QValidator::Acceptable)
    {
        ui->sectionNamesTextEdit->setPlainText(ui->sectionNamesTextEdit->toPlainText().remove(',').remove('&').remove('<').remove('>'));
        QApplication::beep();
        QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation marks are not allowed in the section names:\n    ,  &  <  >\nOther punctuation is allowed."));
    }

    sectionNames = ui->sectionNamesTextEdit->toPlainText().trimmed().split("\n");
    sectionNames.removeAll(QString(""));

    refreshPreview();
    allSectionNames = "";
    if(section)
    {
        for(int sect = 0; sect < sectionNames.size(); sect++)
        {
            if(sect != 0)
                allSectionNames += ",";
            allSectionNames += QUrl::toPercentEncoding(sectionNames[sect]);
        }
    }
}

void SurveyMaker::on_additionalQuestionsCheckBox_clicked(bool checked)
{
    additionalQuestions = checked;
    refreshPreview();
}

//////////////////
// Before closing the application window, if the user never created a survey, ask them if they really want to leave
//////////////////
void SurveyMaker::closeEvent(QCloseEvent *event)
{
    bool actuallyExit = surveyCreated;
    if(!surveyCreated)
    {
        QMessageBox exitNoSurveyBox(this);
        exitNoSurveyBox.setIcon(QMessageBox::Warning);
        exitNoSurveyBox.setWindowTitle(tr("Are you sure you want to exit?"));
        exitNoSurveyBox.setText(tr("You have not created a survey yet. Are you sure you want to exit?"));
        exitNoSurveyBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
        QAbstractButton* pButtonYes = exitNoSurveyBox.button(QMessageBox::Yes);
        pButtonYes->setText(tr("Yes, exit without creating a survey"));
        QAbstractButton* pButtonNo = exitNoSurveyBox.button(QMessageBox::No);
        pButtonNo->setText(tr("No, please take me back to SurveyMaker"));

        exitNoSurveyBox.exec();

        actuallyExit = (exitNoSurveyBox.clickedButton() == pButtonYes);
    }

    if(actuallyExit)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
