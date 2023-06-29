/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2023
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
//    using version 6.5. These can be freely downloaded from
//    < http://qt.io/download >.
//
//    Some icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).
//    Other icons are original creations for the gruepr project by Scout
//    < https://scout.camd.northeastern.edu/ >.
//
//    Several embedded fonts are used:
//    - Oxygen Mono, (C) 2012, Vernon Adams (vern@newtypography.co.uk);
//    - DM Sans, (C) 2014-2017 Indian Type Foundry (info@indiantypefoundry.com);
//    - Paytone One, (C) 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont).
//    All fonts are licensed under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DONE:
// - moved codebase to Qt 6.5
// - moved Windows compiler to msvc
//
// INPROG:
// - updating to all new UI design!
//
// TO DO:
// - scale the sizes of everything in the startdialog depending on screen size(?)
// - BUGFIX: does not recognize when Google refreshtoken is not accepted and a re-authorization is needed
// - allow selection of which google drive account to use, remembering different refresh tokens for each
// - enable PKCE with the Google (and Canvas?) OAuth2 flows
// - create timeout function to nicely handle canvas and google connections
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

#include "gruepr_globals.h"
#include "gruepr.h"
#include "surveymaker.h"
#include "dialogs/startDialog.h"
#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    // Set up application
    QApplication a(argc, argv);
    QApplication::setOrganizationName("gruepr");
    QApplication::setApplicationName("gruepr");
    QApplication::setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");
    QFontDatabase::addApplicationFont(":/fonts/PaytoneOne-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/DMSans-Regular.ttf");

    // Get the screen size
    QRect screenGeometry = QGuiApplication::screens().at(0)->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Show splash screen
    QPixmap splashPic(":/icons_new/splash_new.png");
    auto *splash = new QSplashScreen(splashPic.scaled(screenWidth/2, screenHeight/2, Qt::KeepAspectRatio));
    const int messageSize = (25 * splash->height()) / splashPic.height();
    QFont splashFont("DM Sans");
    splashFont.setPixelSize(messageSize);
    splash->setFont(splashFont);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\n\nwww.gruepr.com", Qt::AlignCenter, QColor::fromString("#" DEEPWATERHEX));
    splash->show();

    // Create application choice (gruepr or surveymaker) window; remove splashscreen when choice window opens
    auto *startWindow = new startDialog;
    splash->finish(startWindow);
    startWindow->exec();
    int result = startWindow->result();
    delete splash;
    delete startWindow;

    // Run chosen application
    if(result == startDialog::Result::makeGroups)
    {
        gruepr w;
        w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work
        w.show();
        result = QApplication::exec();
    }
    else if(result == startDialog::Result::makeSurvey)
    {
        SurveyMaker w;
        w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work
        w.show();
        result = QApplication::exec();
    }
    else    // exit
    {
        result = 0;
    }

    return result;
}

