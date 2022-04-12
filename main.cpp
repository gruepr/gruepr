/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2022
// Joshua Hertz
// info@gruepr.com
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see < https://www.gnu.org/licenses/ >.
//
//    This software incorporates code from the open source Qt libraries,
//    using version 5.15. These can be freely downloaded from
//    < http://qt.io/download >.
//
//    Icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).
//
//    An embedded font is used, mostly for pdf and printer output:
//    Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk)
//    released under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DONE:
// - improved UI in start window, with message confirming user has the latest version and is registered
// - adjusted scoring when team has more-than-desired schedule overlap to prevent runaway scores with high overlap--now reduces worth of each added timeslot by half
//      (i.e., 1st additional is work 1/2 of the previous one, the 2nd additional is worth 1/4, the 3rd additional is worth 1/8, etc...)
// - increased max number of students to 500; works fast enough with 500 students, but gets very slow when Required Teammates or Non-Teammates are present
// - improved UI in graph of optimization progress--now colors plot blue/pink when there is/is not at least one genome that meets all the rules
// - add which student listed the name that needs to be matched in select-name-dialogue
// - code modernization
//
// TO DO:
// - made the "Create Teams" button more emphasized/obvious
// - in gatherteammates dialog, enable the 'load from teamsTab' action
// - more granular scheduling option, down to the 15 minute level at least
// - Sync with Canvas to load the teams into the groups part of Canvas (under the people tab)
// - integrate with Google Drive: download survey results from within the application; expand to Canvas, Qualtrics, and other OAuth2 integration
//
// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - use multiple genepools with limited cross-breeding
// - to get around the redundancy-of-genome issue, store each genome as unordered_set of unordered_set. Each team is set of IDs; each section is set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include "dialogs/registerDialog.h"
#include "surveymaker.h"
#include <algorithm>
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>
#include <QSplashScreen>
#include <QStringList>
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
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\ninfo@gruepr.com", Qt::AlignCenter, Qt::white);
    splash->show();

    // check the latest version available for download and compare to this current version
    auto *manager = new QNetworkAccessManager(splash);
    QNetworkRequest request((QUrl(VERSION_CHECK_URL)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *networkReply = manager->get(request);
    QEventLoop loop;
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QString latestVersionString = (networkReply->bytesAvailable() != 0) ? networkReply->readAll() : "0";
    delete manager;
    QStringList latestVersion = latestVersionString.split('.');
    QStringList thisVersion = QString(GRUEPR_VERSION_NUMBER).split('.');
    // pad fields out to NUMBER_VERSION_FIELDS in size (e.g., 5.2 --> 5.2.0.0)
    for(int field = latestVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersion << QString("0");
    }
    for(int field = thisVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
    {
        thisVersion << QString("0");
    }
    unsigned long long int latestVersionAsInt = 0, thisVersionAsInt = 0;
    for(int field = 0; field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersionAsInt = (latestVersionAsInt*100) + latestVersion.at(field).toInt();
        thisVersionAsInt = (thisVersionAsInt*100) + thisVersion.at(field).toInt();
    }
    const bool upgradeAvailable = latestVersionAsInt > thisVersionAsInt;
    const bool haveBetaVersion = thisVersionAsInt > latestVersionAsInt;

    // check to see if this copy of gruepr has been registered
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    const bool registered = (!registeredUser.isEmpty() && (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex())));

    // Create application choice (gruepr or SurveyMaker) window
    auto *startWindow = new QMessageBox;
    auto *boxFont = new QFont("Oxygen Mono", QApplication::font().pointSize() + gruepr::MAINWINDOWFONT);
    startWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    startWindow->setWindowTitle("gruepr");
    startWindow->setFont(*boxFont);
    startWindow->setText(QObject::tr("Select an app to run:"));

    // Create buttons and add to window
    auto *leaveButton = new QToolButton(startWindow);
    auto *grueprButton = new QToolButton(startWindow);
    auto *survMakeButton = new QToolButton(startWindow);
    auto *upgradeButton = new QToolButton(startWindow);
    upgradeButton->setStyleSheet(QString("background-color: #") + BOLDGREENHEX);
    upgradeButton->setToolTip("<html>" + QObject::tr("There is a newer version of gruepr available. You have version ") +
                              GRUEPR_VERSION_NUMBER + QObject::tr(" and you can now download version ") +
                              latestVersionString + ".</html>");
    auto *registerUserButton = new QToolButton(startWindow);
    registerUserButton->setStyleSheet(QString("background-color: #") + BOLDPINKHEX);
    registerUserButton->setToolTip("<html>" + QObject::tr("Please register your copy of gruepr. "
                                   "Doing so helps me support the community of educators that use it.") + "</html>");
    QList<QToolButton *> buttons = {survMakeButton, grueprButton, leaveButton, registerUserButton, upgradeButton};
    QStringList buttonTexts = {"SurveyMaker", "gruepr", QObject::tr("Exit"), QObject::tr("Register")+" gruepr", QObject::tr("New version!")};
    QStringList buttonIcons = {"surveymaker", "gruepr", "exit", "license", "website"};
    //order reverses for some reason mac->windows
#ifdef Q_OS_MACOS
    std::reverse(buttons.begin(), buttons.end());
    std::reverse(buttonTexts.begin(), buttonTexts.end());
    std::reverse(buttonIcons.begin(), buttonIcons.end());
#endif
    //set button metrics
    boxFont->setPointSize(QApplication::font().pointSize() + gruepr::MAINWINDOWBUTTONFONT);
    const int labelWidth = (QFontMetrics(*boxFont)).boundingRect(QObject::tr("Register")+" gruepr").width();
    const QSize defIconSize(labelWidth - gruepr::MAINWINDOWPADDING,labelWidth - gruepr::MAINWINDOWPADDING);
    const QSize defButtonSize(labelWidth + gruepr::MAINWINDOWPADDING,labelWidth);
    for(int i = 0; i < buttonTexts.size(); i++)
    {
        auto &button = buttons[i];
        button->setIconSize(defIconSize);
        button->setFont(*boxFont);
        button->setFixedSize(defButtonSize);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setText(buttonTexts.at(i));
        button->setIcon(QIcon(":/icons/" + buttonIcons.at(i) + ".png"));
        startWindow->addButton(button, QMessageBox::YesRole);
    }
    startWindow->setEscapeButton(leaveButton);

    // Create status bar to show when registered and/or latest version installed
    auto *statusLabel = new QLabel(startWindow);
    QString statusMessage;
    if(!upgradeAvailable)
    {
        upgradeButton->hide();
        if(latestVersionAsInt != 0)     // couldn't connect to the online check, so even though we're hiding the button, we shouldn't add an affirmation of having latest version
        {
            if(haveBetaVersion)
            {
                statusMessage = QObject::tr("You have a pre-release version of gruepr. ");
            }
            else
            {
                statusMessage = QObject::tr("You have the latest version of gruepr. ");
            }
        }
    }
    if(registered)
    {
        registerUserButton->hide();
        statusMessage += QObject::tr("Thank you for being a registered user.");
    }
    if(!registered && upgradeAvailable)
    {
        statusLabel->hide();
    }
    else
    {
        statusLabel->setText(statusMessage);
        statusLabel->setFont(QApplication::font());
        auto *grid = qobject_cast<QGridLayout*>(startWindow->layout());
        if(grid != nullptr)
        {
            grid->setRowMinimumHeight(grid->rowCount(), gruepr::MAINWINDOWPADDING);
            grid->addWidget(statusLabel, grid->rowCount(), 0, -1, -1, Qt::AlignLeft);
        }
    }

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
        else if(result == registerUserButton)
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
                QMessageBox::critical(startWindow, QObject::tr("No Internet Connection"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                                                      "Please register at another time."));
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
                    if(QDesktopServices::openUrl(QUrl(QString(USER_REGISTRATION_URL)+
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
                        QMessageBox::critical(startWindow, QObject::tr("No Internet Connection"),
                                              QObject::tr("There seems to be a problem with submitting your registration.\n"
                                                          "Please try again at another time or contact <info@gruepr.com>."));
                    }
                }
                delete manager;
                delete window;
            }
            startWindow->exec();
            result = startWindow->clickedButton();
        }
        else if(result == upgradeButton)
        {
            QDesktopServices::openUrl(QUrl("http://gruepr.com"));
            startWindow->exec();
            result = startWindow->clickedButton();
        }
    }

    delete startWindow;
    return executionResult;
}
