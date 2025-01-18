/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2025
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
//    using version 6.7. These can be freely downloaded from
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
// DONE:
//  - changed handling of gender, allowing it to be multi-valued (check one or more in survey; set of values in the studentRecord)
//  - somewhat inconsequential mistake in GA::mate where startteam could be > endteam
//  - now correctly resizes columns in the team display tree whenever expanding an individual team
//  - now correctly reports duplicate students when the names and emails are auto-derived from Canvas roster
//  - attribute response counts now correctly account for added / removed / edited students
//  - several bugfixes related to resorting teams
//
// INPROG:
//  - export of teams should include the section number if that's being displayed (having trouble re-creating)
//  - swapping students between teams reorders teams (TeamsTabItem line 580ish?)
//
// TO DO:
//    BUGFIXES:
//
//    NEW FEATURES:
//  - add motion to the LMS busy dialog so that it doesn't appear frozen
//  - add ranked option as a question type (set of drop downs? select 1st, select 2nd, select 3rd, etc.)
//  - add free response number as a question type (could be done in Canvas but not in Google Form, as it requires response validation added to the API)
//  - in teammatesRules dialog, enable the 'load from teamsTab' action
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Blackboard, Qualtrics, others
//
//    INTERNAL:
//  - continue removing c-style arrays, non-range-based for loops, and pointer arithmetic everywhere except in intensive optimization steps
//      - replace arrays for StudentRecord.unavailable, TeamRecord.numStudentsAvailable, EditOrAddStudentDialog.tempUnavailability
//      - much harder: replace arrays for all of the attribute-related stuff
//      - add bounds checking whenever using [], .at, .first, .constFirst, .begin, etc.
//  - analyze for memory leaks
//      - memory leak -> crash when loading large file, unloading, then repeating a few times
//  - compile for webassembly, turn into a webapp
//      - move from OpenMP to QThread?
//
//    NETWORK IMPLEMENTATION:
//  - create timeout function to more nicely handle LMS connections
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: accepting responses, don't collect email, don't limit one response per user, don't show link to respond again, make publicly accessible
//      - Question options: req'd question, answer validity checks
//  - errors when trying to connect to Google on home network when IPv6 is enabled (IPv6? eero-network?)
//
//    WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
//  - preferentially mutate the lowest scoring team(s) within a genome
//  - use multiple genepools with limited cross-breeding
//  - to get around the redundancy-of-genome issue, store each genome as std::set< std::set< int > >. Each team is set of indexes to the students; each section is set of teams.
//      - would also allow sorting teams within set by ascending score and thus mutations preferentially at front
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr_globals.h"
#include "dialogs/startDialog.h"
#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    // Set up application
    const QApplication a(argc, argv);
    QApplication::setOrganizationName("gruepr");
    QApplication::setApplicationName("gruepr");
    QApplication::setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");
    QFontDatabase::addApplicationFont(":/fonts/PaytoneOne-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/DMSans-Regular.ttf");

    const QRect screenGeometry = QGuiApplication::screens().at(0)->availableGeometry();
    qApp->setProperty("_SCREENWIDTH", screenGeometry.width());
    qApp->setProperty("_SCREENHEIGHT", screenGeometry.height());

    // Show splash screen
    const QPixmap splashPic(":/icons_new/splash_new.png");
    auto *splash = new QSplashScreen(splashPic.scaled(SCREENWIDTH/2, SCREENHEIGHT/2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    splash->deleteLater();
    loop.exec();
    startWindow->deleteLater();

    return 0;
}
