#include "surveymaker.h"
#include "ui_surveymaker.h"
#include <QMessageBox>
#include <QtNetwork>
#include <QDesktopServices>
#include <QTextBrowser>

SurveyMaker::SurveyMaker(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SurveyMaker)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/surveymaker.png"));
    QFontDatabase::addApplicationFont(":/OxygenMono-Regular.otf");

    //Restore window geometry
    QSettings savedSettings;
    restoreGeometry(savedSettings.value("windowGeometry").toByteArray());

    noCommas = new QRegularExpressionValidator(QRegularExpression("[^,&<>]*"), this);

    refreshPreview();
}

SurveyMaker::~SurveyMaker()
{
    delete ui;
}

void SurveyMaker::refreshPreview()
{
    int currPos = ui->previewText->verticalScrollBar()->value();
    QString preview = "<h2>" + title + "</h2>";
    preview += "<h3>First, some basic information</h3>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your first name (or the name you prefer to be called)?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your last name?<br></p>"
               "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;What is your email address?<br></p>";
    preview += gender?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;With which gender do you identify?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>woman </i><b>|</b><i> man </i><b>|</b><i> non— binary </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += URM?
                "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;Do you identify as a member of an underrepresented minority?<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                "<small>options: <b>{ </b><i>yes </i><b>|</b><i> no </i><b>|</b><i> prefer not to answer</i><b> }</b></small><br></p>"
                : "";
    preview += "<hr>";
    if(numAttributes > 0)
    {
        QStringList responseOptions = {
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
            "1 / 2 / 3",
            "1 / 2 / 3 / 4",
            "1 / 2 / 3 / 4 / 5",
            "1 / 2 / 3 / 4 / 5 / 6",
            "1 / 2 / 3 / 4 / 5 / 6 / 7",
            "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8",
            "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9",
            "2 custom options, to be added after creating the form",
            "3 custom options, to be added after creating the form",
            "4 custom options, to be added after creating the form",
            "5 custom options, to be added after creating the form",
            "6 custom options, to be added after creating the form",
            "7 custom options, to be added after creating the form",
            "8 custom options, to be added after creating the form",
            "9 custom options, to be added after creating the form",
        };
        preview += "<h3>This set of questions is about your past experiences/education and teamwork preferences.</h3>";
        for(int attrib = 0; attrib < numAttributes; attrib++)
        {
                preview += "<p>&nbsp;&nbsp;&nbsp;&bull;&nbsp;" +
                           (attributeTexts[attrib].isEmpty()? "{Attribute question " + QString::number(attrib+1) + "}" : attributeTexts[attrib])
                           + "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                           "<small>options: <b>{ </b><i>" + responseOptions.at(attributeResponses[attrib]).split("/").join("</i><b>|</b><i>") + "</i><b> }</b></small>";
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
        preview += "<hr>";
    }

    URL = "https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/exec?";
    URL += "title=" + QUrl::toPercentEncoding(ui->surveyTitleLineEdit->text()) + "&";
    URL += "gend=" + QString(gender? "true" : "false") + "&";
    URL += "urm=" + QString(URM? "true" : "false") + "&";
    URL += "numattr=" + QString::number(numAttributes) + "&";
    URL += "attrtext=" + allAttributeTexts + "&";
    URL += "attrresps=" + allAttributeResponses + "&";
    URL += "sched=" + QString(schedule? "true" : "false") + "&";
    URL += "start=" + QString::number(startTime) + "&end=" + QString::number(endTime) + "&days=" + allDayNames + "&";
    URL += "sect=" + QString(section? "true" : "false") + "&";
    URL += "sects=" + allSectionNames + "&";
    URL += "addl=" + QString(additionalQuestions? "true" : "false");

    //preview += "<br><br><small>URL preview: " + URL + "</small><hr>";

    ui->previewText->setHtml(preview);
    ui->previewText->verticalScrollBar()->setValue(currPos);
}

void SurveyMaker::on_makeSurveyButton_clicked()
{
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
    allAttributeTexts.clear();
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

void SurveyMaker::on_attributeScrollBar_valueChanged(int value)
{
    ui->attributeTextEdit->setPlainText(attributeTexts[value]);
    ui->attributeTextEdit->setPlaceholderText(tr("Enter attribute question ") + QString::number(value+1) + ".");
    ui->attributeComboBox->setCurrentIndex(attributeResponses[value]);
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
    allAttributeTexts.clear();
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

void SurveyMaker::on_attributeComboBox_currentIndexChanged(int index)
{
    attributeResponses[ui->attributeScrollBar->value()] = index;
    allAttributeResponses.clear();
    for(int attrib = 0; attrib < numAttributes; attrib++)
    {
        if(attrib != 0)
        {
            allAttributeResponses += ",";
        }
        allAttributeResponses += QString::number(attributeResponses[attrib]);
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

    // split the input at every newline, then remove any blanks (including just spaces)
    sectionNames = ui->sectionNamesTextEdit->toPlainText().split("\n");
    for (int line = 0; line < sectionNames.size(); line++)
    {
        sectionNames[line] = sectionNames.at(line).trimmed();
    }
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

void SurveyMaker::on_helpButton_clicked()
{
    QFile helpFile(":/help.html");
    if (!helpFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QDialog helpWindow(this);
    helpWindow.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    helpWindow.setSizeGripEnabled(true);
    helpWindow.setWindowTitle("Help");
    QGridLayout theGrid(&helpWindow);
    QTextBrowser helpContents(&helpWindow);
    helpContents.setHtml(tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr: SurveyMaker " GRUEPR_VERSION_NUMBER "</h1>"
                            "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                            "<p>Joshua Hertz <a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                            "<p>Project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>"));
    helpContents.append(helpFile.readAll());
    helpFile.close();
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(600,600);
    helpWindow.exec();
}

void SurveyMaker::on_aboutButton_clicked()
{
    QMessageBox::about(this, tr("About gruepr: SurveyMaker"),
                       tr("<h1 style=\"font-family:'Oxygen Mono';\">gruepr: SurveyMaker " GRUEPR_VERSION_NUMBER "</h1>"
                          "<p>Copyright &copy; " GRUEPR_COPYRIGHT_YEAR
                          "<br>Joshua Hertz<br><a href = mailto:gruepr@gmail.com>gruepr@gmail.com</a>"
                          "<p>gruepr is an open source project. The source code is freely available at"
                          "<br>the project homepage: <a href = http://bit.ly/Gruepr>http://bit.ly/Gruepr</a>."
                          "<p>gruepr incorporates:"
                              "<ul><li>Code libraries from <a href = http://qt.io>Qt, v 5.12.1</a>, released under the GNU Lesser General Public License version 3</li>"
                              "<li>Icons from <a href = https://icons8.com>Icons8</a>, released under Creative Commons license \"Attribution-NoDerivs 3.0 Unported\"</li>"
                              "<li><span style=\"font-family:'Oxygen Mono';\">The font <a href = https://www.fontsquirrel.com/fonts/oxygen-mono>"
                                                                    "Oxygen Mono</a>, Copyright &copy; 2012, Vernon Adams (vern@newtypography.co.uk),"
                                                                    " released under SIL OPEN FONT LICENSE V1.1.</span></li></ul>"
                          "<h3>Disclaimer</h3>"
                          "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
                          "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                          "<p>This program is free software: you can redistribute it and/or modify it under the terms of the <a href = https://www.gnu.org/licenses/gpl.html>"
                          "GNU General Public License</a> as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."));
}

//////////////////
// Before closing the application window, save the window geometry for next time and then, if the user never created a survey, ask them if they really want to leave
//////////////////
void SurveyMaker::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    savedSettings.setValue("windowGeometry", saveGeometry());

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