/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2021
// Joshua Hertz
// gruepr@gmail.com
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see < https://www.gnu.org/licenses/ >.

//    This software incorporates code from the open source Qt libraries,
//    using version 5.15. These can be freely downloaded from
//    < http://qt.io/download >.

//    Icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).

//    An embedded font is used, mostly for pdf and printer output:
//    Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk)
//    released under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DONE:
// - bugfix: newlines within a csv field are now handled properly
// - bugfix: save file after adding or editing a student mishandled attribute values
// - bugfix: Gather URM Response dialog crash in some cases
// - attribute response texts now always show in attribute value selection box in add/edit student dialog; better GUI behavior on those boxes
// - now distinguishes nonbinary from unknown gender in instructor's save file
// - expansion of what gets recognized for gender response: woman OR female; man OR male; anything that includes "trans", "queer", or any combination of "non" and "binary"
// - doesn't mark students as duplicate just because they have blank email adresses
// - An option in the survey to have students put in preferred teammates / non-teammates and allow to import those as required/requested/prevented
// - create Levenshtein dialog to replace repeated code in gather teammates dialog
// - change incompatible response dialog / behavior to add incompatible same responses (1 with 1)
// - add penalty point per incompatible pair found
// - moved studentRecord to a separate class, with tooltip creation as member function
// - ancestors of elites now created (using elite genome as mom and dad)
// - code modernization and organization
// - moved code to Qt v5.15 standards
// - add "load roster" option to populate student table or compare with existing data
//
// TO DO:
// - bugfix: removing a student disrupts all of the saved required/prevented/requested teammates values because the IDs have changed
// - change the ID behavior to a dataOption value that always increments as a student is added and where student.ID is set to that value when created and then never changes
// - check if pairing is both (required or requested) and prevented
// - create csv import dialog box to select columns
// - allow for section question later in the "additional questions"
// - add select-multiple attribute question option
// - more granular scheduling option, down to the 15 minute level at least
// - Sync with Canvas to load the teams into the groups part of Canvas (under the people tab)
// - integrate with Google Drive: download survey results from within the application; expand to Canvas, Qualtrics, and other OAuth2 integration
//
// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - to get around the redundancy-of-genome issue, store each genome as unordered_set of unordered_set. Each team is set of IDs; each section is set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include "surveymaker.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>
#include <QSplashScreen>
#include <QThread>
#include <QToolButton>
#include <QtNetwork>

int main(int argc, char *argv[])
{
    // Set up application
    QApplication a(argc, argv);
    QApplication::setOrganizationName("gruepr");
    QApplication::setApplicationName("gruepr");
    QApplication::setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");

    // Show splash screen
    auto *splash = new QSplashScreen;
    QPixmap pic(":/icons/fish.png");
    splash->setPixmap(pic);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\ngruepr@gmail.com", Qt::AlignCenter, Qt::white);
    splash->show();

    // Create application choice (gruepr or SurveyMaker) window
    auto *startWindow = new QMessageBox;
    auto *boxFont = new QFont("Oxygen Mono", QApplication::font().pointSize()+8);
    startWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    startWindow->setWindowTitle("gruepr");
    startWindow->setFont(*boxFont); startWindow->setText("Select an app to run:");

    // Button metrics
    boxFont->setPointSize(QApplication::font().pointSize()+4);
    int labelWidth = (QFontMetrics(*boxFont)).boundingRect("Register gruepr").width();
    QSize defIconSize(labelWidth-20,labelWidth-20);
    QSize defButtonSize(labelWidth+20,labelWidth);

    // Create and add buttons
    auto *leaveButton = new QToolButton(startWindow);
    leaveButton->setIconSize(defIconSize); leaveButton->setFont(*boxFont); leaveButton->setFixedSize(defButtonSize);
    leaveButton->setIcon(QIcon(":/icons/exit.png")); leaveButton->setText("Exit");
    leaveButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto *grueprButton = new QToolButton(startWindow);
    grueprButton->setIconSize(defIconSize); grueprButton->setFont(*boxFont); grueprButton->setFixedSize(defButtonSize);
    grueprButton->setIcon(QIcon(":/icons/gruepr.png")); grueprButton->setText("gruepr");
    grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto *survMakeButton = new QToolButton(startWindow);
    survMakeButton->setIconSize(defIconSize); survMakeButton->setFont(*boxFont); survMakeButton->setFixedSize(defButtonSize);
    survMakeButton->setIcon(QIcon(":/icons/surveymaker.png")); survMakeButton->setText("SurveyMaker");
    survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    //Add registration button only if unregistered
    QToolButton *registerUserButton = nullptr;
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    bool registered = (!registeredUser.isEmpty() && (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex())));

    //order switches for some reason mac->windows
#ifdef Q_OS_MACOS
    if(!registered)
    {
        registerUserButton = new QToolButton(startWindow);
        registerUserButton->setIconSize(defIconSize); registerUserButton->setFont(*boxFont); registerUserButton->setFixedSize(defButtonSize);
        registerUserButton->setIcon(QIcon(":/icons/license.png")); registerUserButton->setText("Register gruepr");
        registerUserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        registerUserButton->setStyleSheet("background-color: #f283a5");
        startWindow->addButton(registerUserButton, QMessageBox::YesRole);
    }
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
#endif
#ifdef Q_OS_WIN32
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
    if(!registered)
    {
        registerUserButton = new QToolButton(startWindow);
        registerUserButton->setIconSize(defIconSize); registerUserButton->setFont(*boxFont); registerUserButton->setFixedSize(defButtonSize);
        registerUserButton->setIcon(QIcon(":/icons/license.png")); registerUserButton->setText("Register gruepr");
        registerUserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        registerUserButton->setStyleSheet("background-color: #f283a5");
        startWindow->addButton(registerUserButton, QMessageBox::YesRole);
    }
#endif
    startWindow->setEscapeButton(leaveButton);

    // Show application choice window and delete splash
    splash->finish(startWindow);
    startWindow->exec();
    QAbstractButton *result = startWindow->clickedButton();
    delete splash;
    delete boxFont;

    // Run chosen application
    int executionResult = 0;
    while(result != leaveButton)
    {
        if(result == grueprButton)
        {
            gruepr w;
            w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work
            w.show();
            executionResult = QApplication::exec();
            result = leaveButton;
        }
        else if(result == survMakeButton)
        {
            SurveyMaker w;
            w.setWindowTitle("gruepr: SurveyMaker [*]");         // asterisk is placeholder, shown when there is unsaved work
            w.show();
            executionResult = QApplication::exec();
            result = leaveButton;
        }
        else
        {
            //make sure we can connect to google
            auto *manager = new QNetworkAccessManager(startWindow);
            QEventLoop loop;
            QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
            QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if(networkReply->bytesAvailable() == 0)
            {
                //no internet right now
                QMessageBox::critical(startWindow, "No Internet Connection", "There does not seem to be an internet connection.\n"
                                                                             "Please register at another time.");
                delete manager;
            }
            else
            {
                //we can connect, so gather name, institution, and email address for submission
                auto *window = new registerDialog(startWindow);
                int reply = window->exec();
                //If user clicks OK, email registration info and add to saved settings
                if(reply == QDialog::Accepted)
                {
                    // using DesktopServices (i.e., user's browser) because access to Google Script is via https, and ssl is tough in Qt
                    if(QDesktopServices::openUrl(QUrl(USER_REGISTRATION_URL
                                                      "?name="+QUrl::toPercentEncoding(window->name->text())+
                                                      "&institution="+QUrl::toPercentEncoding(window->institution->text())+
                                                      "&email="+QUrl::toPercentEncoding(window->email->text()))))
                    {
                        registeredUser = window->name->text();
                        QSettings savedSettings;
                        savedSettings.setValue("registeredUser", registeredUser);
                        savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()),QCryptographicHash::Md5).toHex()));
                        startWindow->removeButton(registerUserButton);
                    }
                    else
                    {
                        QMessageBox::critical(startWindow, "No Connection",
                                              "There seems to be a problem with submitting your registration.\n"
                                              "Please try again at another time or contact <gruepr@gmail.com>.");
                    }
                }
                delete manager;
                delete window;
            }
            startWindow->exec();
            result = startWindow->clickedButton();
        }
    }

    delete startWindow;
    return executionResult;
}
