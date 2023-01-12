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
//
// INPROG:
// - modifying behind the scenes to make the app submittable to apple and windows app stores
//
// TO DO:
// - BUGFIX: does not recognize when Google refreshtoken is not accepted and a re-authorization is needed
// - create timeout function to nicely handle canvas and google connections
// - enable PKCE with the Google (and Canvas?) OAuth2 flows
// - more granular scheduling option, down to the 15 minute level at least
// - add integration with Blackboard, Qualtrics, others
// - modernize use of pointers throughout to C++17 style; check for memory leaks
// - enable in Google Forms various options -- must wait on new API functionality from Google
//     - Form options: accepting responses, don't collect email, don't limit one response per user, don't show link to respond again
//     - Question options: req'd question, answer validity checks
// - make the "Create Teams" button more emphasized/obvious
// - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
// - create an LMS class and then subclass Canvas, Google
// - auto-shorten URL for Google Form (using Google's firebase API?)
// - in gatherteammates dialog, enable the 'load from teamsTab' action
//
// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - use multiple genepools with limited cross-breeding
// - to get around the redundancy-of-genome issue, store each genome as unordered_set of unordered_set. Each team is set of IDs; each section is set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include "gruepr_globals.h"
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

    // check github for the latest version available for download and compare to this current version
    auto *manager = new QNetworkAccessManager(splash);
    auto *request = new QNetworkRequest(QUrl(VERSION_CHECK_URL));
    request->setSslConfiguration(QSslConfiguration::defaultConfiguration());
    request->setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = manager->get(*request);
    QString latestVersionString;
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0)
    {
        latestVersionString = "0";
    }
    else
    {
        QRegularExpression versionNum(R"(\"tag_name\":\"v([\d*.]{1,})\")");
        QRegularExpressionMatch match = versionNum.match(reply->readAll());
        latestVersionString = (match.hasMatch() ? match.captured(1) : ("0"));
    }
    delete reply;
    delete request;
    delete manager;

    QStringList latestVersion = latestVersionString.split('.');
    QStringList thisVersion = QString(GRUEPR_VERSION_NUMBER).split('.');
    // pad fields out to NUMBER_VERSION_FIELDS in size (e.g., 5.2 --> 5.2.0.0)
    for(int field = latestVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersion << "0";
    }
    for(int field = thisVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
    {
        thisVersion << "0";
    }
    // convert to single integer
    unsigned long long int latestVersionAsInt = 0, thisVersionAsInt = 0;
    for(int field = 0; field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersionAsInt = (latestVersionAsInt*NUMBER_VERSION_PRECISION) + latestVersion.at(field).toInt();
        thisVersionAsInt = (thisVersionAsInt*NUMBER_VERSION_PRECISION) + thisVersion.at(field).toInt();
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
    startWindow->setText(QObject::tr("What would you like to do?"));

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
    QStringList buttonTexts = {"Make a survey", "Form teams", QObject::tr("Exit"), QObject::tr("Register")+" gruepr", QObject::tr("New version!")};
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
                statusMessage = QObject::tr("You have a pre-release version of gruepr! ");
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
            if(internetIsGood())
            {
                //we can connect, so gather name, institution, and email address for submission
                auto *registerWin = new registerDialog(startWindow);
                if(registerWin->exec() == QDialog::Accepted)
                {
                    //If user clicks OK, add to saved settings
                    registerWin->show();
                    auto *box = new QHBoxLayout;
                    auto *icon = new QLabel;
                    auto *message = new QLabel;
                    box->addWidget(icon);
                    box->addWidget(message, 0, Qt::AlignLeft);
                    (qobject_cast<QBoxLayout *>(registerWin->layout()))->addLayout(box);
                    icon->setPixmap(QPixmap(":/icons/wait.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                    message->setText(QObject::tr("Communicating..."));

                    manager = new QNetworkAccessManager(startWindow);
                    request = new QNetworkRequest(QUrl(USER_REGISTRATION_URL));
                    request->setSslConfiguration(QSslConfiguration::defaultConfiguration());
                    request->setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
                    request->setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
                    QJsonObject data;
                    data["name"] = registerWin->name->text();
                    data["institution"] = registerWin->institution->text();
                    data["email"] = registerWin->email->text();
                    QJsonDocument doc(data);
                    QByteArray postData = doc.toJson();
                    reply = manager->post(*request, postData);
                    QEventLoop loop;
                    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                    loop.exec();
                    QString replyBody = (reply->bytesAvailable() == 0 ? "" : reply->readAll());
                    if(replyBody.contains("Registration successful"))
                    {
                        registeredUser = registerWin->name->text();
                        QSettings savedSettings;
                        savedSettings.setValue("registeredUser", registeredUser);
                        savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex()));
                        startWindow->removeButton(registerUserButton);
                        statusLabel->setText(statusLabel->text() + QObject::tr("Thank you for being a registered user."));
                        icon->setPixmap(QPixmap(":/icons/ok.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                        message->setText(QObject::tr("Success!"));
                    }
                    else
                    {
                        icon->setPixmap(QPixmap(":/icons/delete.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                        message->setText(QObject::tr("Error. Please try again later or contact <info@gruepr.com>."));
                    }
                    QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
                    loop.exec();
                    registerWin->hide();
                }
                delete registerWin;
            }
            delete reply;
            delete request;
            delete manager;
            startWindow->exec();
            result = startWindow->clickedButton();
        }
        else if(result == upgradeButton)
        {
            QDesktopServices::openUrl(QUrl(GRUEPRHOMEPAGE));
            startWindow->exec();
            result = startWindow->clickedButton();
        }
    }

    delete startWindow;
    return executionResult;
}
