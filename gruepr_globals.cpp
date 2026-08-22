#include "gruepr_globals.h"
#include "dataFile.h"
#include "studentRecord.h"
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTextBrowser>
#include <QTime>
#include <QWidget>

float grueprGlobal::timeStringToHours(const QString &timeStr) {
    static const QStringList timeFormats = QString(TIMEFORMATS).split(';');
    static QString mostRecentTimeFormat = timeFormats[0];

    QTime time = QTime::fromString(timeStr, mostRecentTimeFormat);
    if(time.isValid()) {
        return time.hour() + (time.minute() / 60.0f) + (time.second()/3600.0f);
    }

    for (const auto &timeFormat : timeFormats) {
        time = QTime::fromString(timeStr, timeFormat);
        if(time.isValid()) {
            mostRecentTimeFormat = timeFormat;
            return time.hour() + (time.minute() / 60.0f) + (time.second()/3600.0f);
        }
    }

    if(timeStr.compare(QObject::tr("noon"), Qt::CaseInsensitive) == 0) {
        return 12;
    }
    if(timeStr.compare(QObject::tr("midnight"), Qt::CaseInsensitive) == 0) {
        return 0;
    }

    // Return -1.0 to indicate an invalid time string
    return -1.0;
}

bool grueprGlobal::internetIsGood() {
    //make sure we can connect to google
    auto *manager = new QNetworkAccessManager;
    manager->setTransferTimeout(8000);
    QEventLoop loop;
    QNetworkReply *networkReply = manager->head(QNetworkRequest(QUrl("http://www.google.com")));
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const bool weGotProblems = (!networkReply->isFinished() || networkReply->rawHeaderList().isEmpty());
    networkReply->deleteLater();
    manager->deleteLater();
    if(weGotProblems) {
        grueprGlobal::errorMessage(nullptr, QObject::tr("Error!"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                               "Check your network connection and try again."));
    }
    return !weGotProblems;
}

QString grueprGlobal::genderToString(const Gender gender) {
    switch (gender) {
        case Gender::woman:
            return "Woman";
        case Gender::man:
            return "Man";
        case Gender::nonbinary:
            return "Nonbinary";
        default:
            return "Unknown";
    }
}

// Convert string to enum
Gender grueprGlobal::stringToGender(const QString& genderStr) {
    if (genderStr == "Woman") {
        return Gender::woman;
    }
    if (genderStr == "Man") {
        return Gender::man;
    }
    if (genderStr == "Nonbinary") {
        return Gender::nonbinary;
    }
    return Gender::unknown; // Default to unknown if input is invalid
}

//////////////////
// Prompt for and open a roster file (.csv/.txt/.xlsx), let the user map its columns, and extract
// student names (and, if requested, email addresses)
//////////////////
bool grueprGlobal::loadRoster(QWidget *parent, const QString &startingDirectory, QStringList &names, QStringList *emails) {
    const QString fileName = QFileDialog::getOpenFileName(parent, QObject::tr("Open Student Roster File"), startingDirectory,
                                                            QObject::tr("Roster File") + " (*.csv *.txt *.xlsx);;All Files (*)");
    if(fileName.isEmpty()) {
        return false;
    }

    const std::unique_ptr<DataFile> rosterFile = DataFile::createForFile(fileName);
    if(!rosterFile->openExistingFile(fileName)) {
        return false;
    }

    // Read the header row
    if(!rosterFile->readHeader()) {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(parent, QObject::tr("File error."), QObject::tr("This file is empty or there is an error in its format."));
        return false;
    }

    // Ask user what the columns mean
    // Preloading the selector boxes with "unused" except first time "email" (if requested), "first name", "last name", and "name" are found
    QList<possFieldMeaning> rosterFieldOptions = {{"First Name", "((first)|(given)|(preferred)).*(name)", 1},
                                                   {"Last Name", "((last)|(sur)|(family)).*(name)", 1},
                                                   {"Full Name (First Last)", "(name)", 1},
                                                   {"Full Name (Last, First)", "(name)", 1}};
    if(emails != nullptr) {
        rosterFieldOptions.insert(2, {"Email Address", "(e).*(mail)", 1});
    }
    if(rosterFile->chooseFieldMeaningsDialog(rosterFieldOptions, parent)->exec() == QDialog::Rejected) {
        return false;
    }

    // set field values now according to user's selection of field meanings (defaulting to -1 if not chosen)
    const int emailField = int(rosterFile->fieldMeanings.indexOf("Email Address"));
    const int firstNameField = int(rosterFile->fieldMeanings.indexOf("First Name"));
    const int lastNameField = int(rosterFile->fieldMeanings.indexOf("Last Name"));
    const int firstLastNameField = int(rosterFile->fieldMeanings.indexOf("Full Name (First Last)"));
    const int lastFirstNameField = int(rosterFile->fieldMeanings.indexOf("Full Name (Last, First)"));

    // Process each row until there's an empty one. Load names (and emails, if requested)
    names.clear();
    if(emails != nullptr) {
        emails->clear();
    }
    if(rosterFile->hasHeaderRow) {
        rosterFile->readDataRow();
    }
    else {
        rosterFile->readDataRow(DataFile::ReadLocation::beginningOfFile);
    }
    do {
        QString name;
        if(firstNameField >= 0 && lastNameField >= 0) {
            // Both explicitly chosen -- the most unambiguous signal, so it takes priority over a
            // "Full Name" field that may have only been auto-guessed onto some unrelated column
            // (e.g. one asking for the names of preferred teammates, which also contains "name").
            if(firstNameField < rosterFile->fieldValues.size()) {
                name = rosterFile->fieldValues.at(firstNameField).trimmed();
            }
            name = name + " ";
            if(lastNameField < rosterFile->fieldValues.size()) {
                name = name + rosterFile->fieldValues.at(lastNameField).trimmed();
            }
        }
        else if((firstLastNameField >= 0) && (firstLastNameField < rosterFile->fieldValues.size())) {
            name = rosterFile->fieldValues.at(firstLastNameField).trimmed();
        }
        else if((lastFirstNameField >= 0) && (lastFirstNameField < rosterFile->fieldValues.size())) {
            if(rosterFile->fieldValues.at(lastFirstNameField).contains(',')) {
                const QStringList lastandfirstname = rosterFile->fieldValues.at(lastFirstNameField).split(',');
                name = QString(lastandfirstname.at(1) + " " + lastandfirstname.at(0)).trimmed();
            }
            else {
                name = rosterFile->fieldValues.at(lastFirstNameField).trimmed();
            }
        }
        else if(firstNameField >= 0 || lastNameField >= 0) {
            if((firstNameField >= 0) && (firstNameField < rosterFile->fieldValues.size())) {
                name = rosterFile->fieldValues.at(firstNameField).trimmed();
            }
            if(firstNameField >= 0 && lastNameField >= 0) {
                name = name + " ";
            }
            if((lastNameField >= 0) && (lastNameField < rosterFile->fieldValues.size())) {
                name = name + rosterFile->fieldValues.at(lastNameField).trimmed();
            }
        }
        else {
            grueprGlobal::errorMessage(parent, QObject::tr("File error."), QObject::tr("This roster does not contain student names."));
            return false;
        }

        if(!name.isEmpty() && name != " ") {
            names << name;
            if(emails != nullptr && emailField >= 0 && emailField < rosterFile->fieldValues.size()) {
                *emails << rosterFile->fieldValues.at(emailField).trimmed();
            }
        }
    }
    while(rosterFile->readDataRow());

    return true;
}

void grueprGlobal::clearLayout(QLayout *layout)
{
    while (QLayoutItem *child = layout->takeAt(0)) {
        delete child->widget();
        delete child;
    }
}

int grueprGlobal::findStudentIndex(const QList<StudentRecord> &allStudents, const long long studentID)
{
    int i = 0;
    while (i < allStudents.size() && allStudents[i].ID != studentID) {
        i++;
    }
    return i;
}

StudentRecord* grueprGlobal::findStudentFromID(QList<StudentRecord> &allStudents, const long long studentID)
{
    const int i = findStudentIndex(allStudents, studentID);
    return (i < allStudents.size()) ? &allStudents[i] : nullptr;
}

void grueprGlobal::errorMessage(QWidget *parent, const QString &windowTitle, const QString &message) {
    auto *win = new QMessageBox(parent);
    win->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    win->setStyleSheet(LABEL10PTSTYLE);
    win->setIconPixmap(QPixmap(":/icons_new/error.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    win->setWindowTitle(windowTitle.isEmpty()? "gruepr" : windowTitle);
    win->setText(message.isEmpty()? QObject::tr("There was an unspecified error.") : message);
    win->setStandardButtons(QMessageBox::Ok);
    win->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    win->exec();
    win->deleteLater();
}

bool grueprGlobal::warningMessage(QWidget *parent, const QString &windowTitle, const QString &message, const QString &OKtext, const QString &cancelText) {
    auto *win = new QMessageBox(parent);
    win->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    win->setStyleSheet(LABEL10PTSTYLE);
    win->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    win->setWindowTitle(windowTitle);
    win->setText(message);
    auto *okButton = new QPushButton(OKtext, win);
    okButton->setStyleSheet(SMALLBUTTONSTYLE);
    win->addButton(okButton, QMessageBox::AcceptRole);
    if(!cancelText.isEmpty()) {
        auto *cancelButton = new QPushButton(cancelText, win);
        cancelButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        win->addButton(cancelButton, QMessageBox::RejectRole);
    }
    win->exec();
    const bool result = (win->clickedButton() == okButton);
    win->deleteLater();
    return result;
}

void grueprGlobal::aboutWindow(QWidget *parent) {
    const QSettings savedSettings;
    const QString registeredUser = savedSettings.value("registeredUser", "").toString();
    const QString user = registeredUser.isEmpty()? QObject::tr("UNREGISTERED") : (QObject::tr("registered to ") + registeredUser);

    static const char ABOUTWINDOWCONTENT[] {"<h1 style=\"font-family:'Paytone One';\">gruepr</h1>"
                                              "<p>v" GRUEPR_VERSION_NUMBER " &copy; " GRUEPR_COPYRIGHT_YEAR
                                              "<br>Joshua Hertz, Giovanni Assad, Nikhen Nyo<br>"
                                              "<a href=\"mailto:" GRUEPRHELPEMAIL "\">" GRUEPRHELPEMAIL "</a>"
                                              "<p>gruepr is an open source project. The source code is freely available at "
                                              "the project homepage: <a href=\"https://" GRUEPRHOMEPAGE "\">" GRUEPRHOMEPAGE "</a></p>"
                                              "<br><br>gruepr incorporates:"
                                              "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                              "<li>UI, UX, and graphical designs by <a href=\"https://scout.camd.northeastern.edu/\">Scout</a></li>"
                                              "<li>Code libraries from <a href=\"http://qt.io\">Qt, v 6.9</a>, released under the GNU General Public License version 3</li>"
                                              "<li>The following fonts, all released under SIL OPEN FONT LICENSE V1.1:"
                                              "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                              "<li><span style=\"font-family:'Paytone One';\">"
                                              "Paytone One</span> &copy; 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont)</li>"
                                              "<li><span style=\"font-family:'DM Sans';\">"
                                              "DM Sans</span> &copy; 2014-2017 Indian Type Foundry (info@indiantypefoundry.com)</li>"
                                              "<li><span style=\"font-family:'Oxygen Mono';\">"
                                              "Oxygen Mono</span> &copy; 2012 Vernon Adams (vern@newtypography.co.uk)</li>"
                                              "</ul></li>"
                                              "</ul>"
                                              "<h3>SPONSOR:</h3>"
                                              "<p>Free code signing on Windows provided by SignPath.io, certificate by SignPath Foundation.</p>"
                                              "<h3>Disclaimer</h3>"
                                              "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of "
                                              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                                              "<p>This program is free software: you can redistribute it and/or modify it under the terms of the "
                                              "<a href = https://www.gnu.org/licenses/gpl.html>GNU General Public License</a> "
                                              "as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.</p>"};


    QMessageBox::about(parent, QObject::tr("About gruepr"), ABOUTWINDOWCONTENT + QObject::tr("<p><b>This copy of gruepr is ") + user + "</b>.");
}

void grueprGlobal::helpWindow(QWidget *parent) {
    QDialog helpWindow(parent);
    helpWindow.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    helpWindow.setSizeGripEnabled(true);
    helpWindow.setWindowTitle("Help");
    QGridLayout theGrid(&helpWindow);
    QTextBrowser helpContents(&helpWindow);

    static const char HELPWINDOWCONTENT[] {"<h1 style=\"font-family:'Paytone One';\">gruepr</h1>"
                                             "<p>v" GRUEPR_VERSION_NUMBER " &copy; " GRUEPR_COPYRIGHT_YEAR
                                             "<br>Joshua Hertz, Giovanni Assad, Nikhen Nyo<br><a href=\"mailto:" GRUEPRHELPEMAIL "\">" GRUEPRHELPEMAIL "</a>"
                                             "<p>Project homepage: <a href=\"https://" GRUEPRHOMEPAGE "\">" GRUEPRHOMEPAGE "</a>"
                                             "<p>&nbsp; &nbsp;gruepr is a program for splitting a section of 4-1000 students into optimized teams. It was originally based on "
                                             "CATME's team forming routine as described in "
                                             "<a href=\"http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf\">this paper</a>. "
                                             "The student data is read from a file (typically directly downloaded within the app), and the students are split into teams "
                                             "of any desired size(s). The algorithm is highly flexible, allowing the instructor to create and use customized surveys and "
                                             "define in a highly tailored way the characteristics of an optimal team.</p>"
                                             "<h3>Obtaining student data</h3>"
                                             "<p>&nbsp; &nbsp;A customized Google Form is generally used to collect data from the students, and you can use gruepr to customize "
                                             "and create this form. After collecting responses, you can download the results from within gruepr or you can open the Google Form "
                                             "on your Google Drive, go the \"Responses\" tab, and click the three-dot icon to select \"Download responses (.csv)\". "
                                             "Alternative means can be used to collect this data, since gruepr can use any data saved using the file format described below.</p>"
                                             "<h3>How the teams are optimized and displayed</h3>"
                                             "<p>&nbsp; &nbsp;To optimize the teams, a genetic algorithm is used. A good distribution of students into teams is determined by a "
                                             "numerical \"compatibility score\". The instructor uses gruepr to determine how this score is calculated. Options include:</p>"
                                             "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                             "<li>Preventing isolated women, isolated men, and/or isolated nonbinary persons;</li>"
                                             "<li>Preventing isolated students from selected racial/ethnic/cultural identity groups</li>"
                                             "<li>Achieving within each team either homogeneity or heterogeneity of the student's responses to up to 15 \"attribute\" questions. "
                                             "These questions might represent skills assessments, work preferences, attitudes, major, GPA, or any other question with a "
                                             "multiple choice or numerical value response</li>"
                                             "<li>Achieving a high degree of overlap in schedule free time (with timezone awareness)</li>"
                                             "<li>Separating particular students onto different teams</li>"
                                             "<li>Requiring particular students to be on the same team</li>"
                                             "</ul>"
                                             "<p>&nbsp; &nbsp;After optimizing the teams for some time, the best set of teams found is shown on the screen. You can choose whether "
                                             "to save this teamset, adjust this teamset by moving around individual students or teams, or throw away "
                                             "the teamset entirely and start over. The teamset can be saved in text, pdf, or spreadsheet format.</p>"
                                             "<hr>"
                                             "<h3>Further details on the genetic optimization algorithm</h3>"
                                             "<p>&nbsp; &nbsp;A population of 10's of thousands of random teamings (each is a \"genome\") is created and then refined over "
                                             "multiple generations. In each generation, a few of the highest scoring \"elite\" genomes are directly copied (cloned) into the next "
                                             "generation, and then the remainder are created by mating tournament-selected parent genomes using ordered crossover. "
                                             "A genome's net score is, generally, the harmonic mean of the compatibility score for each team. Evolution proceeds for at least a "
                                             "minimum number of generations. It automatically stops (though the user can choose to keep it going indefinitely) after a maximum "
                                             "number of have been reached or when the best score has remained +/- 1% for a set number of generations in a row.</p>"
                                             "<hr>"
                                             "<h3>Further details on the data file format</h3>"
                                             "<p>&nbsp; &nbsp;The datafile of student information must be a comma-separated-values (.csv), and it is easiest if the content is as "
                                             "described below. Using the Google Form or other survey instrument created by gruepr allows you to download the data into exactly this "
                                             "format without modification. The datafile contents: </p>"
                                             "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                             "<li>a header row with the question texts, with specific wording needed if you want gruepr to auto-recognize each question's meaning</li>"
                                             "<li>each student's response to the questions on a separate row, starting at the row immediately after the header</li>"
                                             "<li>within each row, the order of comma-separated questions/responses should be:</li>"
                                             "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                             "<li>timestamp</li>"
                                             "<li>first name</li>"
                                             "<li>last name</li>"
                                             "<li>email address</li>"
                                             "<li>gender, using nomenclature based on biological sex, adult identity, child identity, or pronouns</li>"
                                             "<li>student's self-identified racial / ethnic / cultural identity</li>"
                                             "<li>[0 to 15 values] text responses to the multiple choice questions, each in its own field</li>"
                                             "<li>timezone</li>"
                                             "<li>[0 to 7 values] semicolon-separated list of times within each day that the student is either available or unavailable to work</li>"
                                             "<li>course section</li>"
                                             "<li>student's preferred teammated</li>"
                                             "<li>student's preferred non-teammates</li>"
                                             "<li>any additional notes for the student (additional columns can be included for your own use but ignored by gruepr)</li>"
                                             "</ul>"
                                             "</ul>"};

    helpContents.setHtml(HELPWINDOWCONTENT);
    helpContents.setFont(QFont("DM Sans"));
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    helpContents.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    helpContents.verticalScrollBar()->setStyleSheet(SCROLLBARSTYLE);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(LG_DLG_SIZE, LG_DLG_SIZE);
    helpWindow.exec();
}

MouseWheelBlocker::MouseWheelBlocker(QObject *parent) : QObject(parent) { }
bool MouseWheelBlocker::eventFilter(QObject *o, QEvent *e) {
    const QWidget* widget = static_cast<QWidget*>(o);
    if (e->type() == QEvent::Wheel && (widget != nullptr) && !widget->hasFocus()) {
        e->ignore();
        return true;
    }

    return QObject::eventFilter(o, e);
}
