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
//    Other icons and graphics are original creations for the gruepr project by Scout
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
//   NEW FEATURES:
// - gruepr now auto-saves, including the current set of teaming options, the student database, and all team sets
// - more granular scheduling option, down to the 15 minute level; use survey->schedTimeNames in googlehandler and canvashandler
// - better UI on when close button is hit in surveymaker, confirming close if survey has not yet been exported
//
//   BUGFIXES:
// - now handles (by skipping) blank names in the student roster csv file used in surveymaker
//
// TO DO:
//   BUGFIXES:
// - does not recognize when Google refreshtoken is not accepted and a re-authorization is needed
// - drag-and-drop issues in teams tab?
//
//   NEW FEATURES:
// - add ranked option as a question type (set of drop downs? select 1st, select 2nd, select 3rd, etc.
// - in teammatesRules dialog, enable the 'load from teamsTab' action
// - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
// - adjust getGrueprDataDialog lines 844-881 to pad the
//
//   C++ MODERNIZATION:
// - modernize use of pointers throughout to C++17 style; check for memory leaks
// - remove c-style arrays and update to range-based for-loops except in intense optimization
// - create an LMS class and then subclass Canvas, Google
//
//   NETWORK IMPLEMENTATION:
// - enable PKCE with the Google (and Canvas?) OAuth2 flows
// - auto-shorten URL for Google Form (using Google's firebase API?)
// - create timeout function to nicely handle canvas and google connections
// - allow selection of which google drive account to use, remembering different refresh tokens for each
// - add integration with Blackboard, Qualtrics, others
// - enable in Google Forms various options -- must wait on new API functionality from Google
//     - Form options: accepting responses, don't collect email, don't limit one response per user, don't show link to respond again
//     - Question options: req'd question, answer validity checks
//   WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - use multiple genepools with limited cross-breeding
// - to get around the redundancy-of-genome issue, store each genome as unordered_set of unordered_set. Each team is set of IDs; each section is set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr_globals.h"
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
    auto *splash = new QSplashScreen(splashPic.scaled(screenWidth/2, screenHeight/2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    splash->setAttribute(Qt::WA_DeleteOnClose);
    const int messageSize = (25 * splash->height()) / splashPic.height();
    QFont splashFont("DM Sans");
    splashFont.setPixelSize(messageSize);
    splash->setFont(splashFont);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\n" GRUEPRHOMEPAGE, Qt::AlignCenter, QColor::fromString(DEEPWATERHEX));
    splash->show();

    // Create application choice (gruepr or surveymaker) window; remove splashscreen when choice window opens
    auto *startWindow = new StartDialog;
    splash->finish(startWindow);
    QEventLoop loop;
    QAction::connect(startWindow, &QDialog::finished, &loop, &QEventLoop::quit);
    startWindow->show();
    loop.exec();

    return 0;
}

