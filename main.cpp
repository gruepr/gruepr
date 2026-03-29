/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2026
// Joshua Hertz, Giovanni Assad, Nikhen Nyo
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
//    using version 6.9. These can be freely downloaded from
//    < http://qt.io/download >.
//
//    Icons and graphics are original creations for the gruepr project by Scout
//    < https://scout.camd.northeastern.edu/ >.
//
//    Several embedded fonts are used:
//    - Oxygen Mono, (C) 2012, Vernon Adams (vern@newtypography.co.uk);
//    - DM Sans, (C) 2014-2017 Indian Type Foundry (info@indiantypefoundry.com);
//    - Paytone One, (C) 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont).
//    All fonts are licensed under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DONE:
//  - Pulling in ***incredible*** dissertation work by Nikhen:
//     - Turned teaming criteria into individual 'cards' that are added, removed, and rearranged to make team formation goals
//     - Added visualization to show how well each team meets each team formation goal
//     - UI modernizations and simplifications in start dialog and data loading process
//     - diversity criteria for multiple choice questions now optimizes for most number of values (in addition to widest range of values for ordered questions)
//  - unified "required" and "requested" teammates
//  - changed handling of gender, allowing it to be multi-valued (students can select >1 in survey; set of values saved in the studentRecord)
//  - now correctly resizes columns in the team display tree whenever expanding an individual team
//  - now correctly reports duplicate students when the names and emails are auto-derived from Canvas roster
//  - now shows bar graphs indicating how many students selected each multiple choice response
//  - multiple choice response counts now correctly account for added / removed / edited students
//  - several bugfixes related to resorting teams
//  - added numerical attribute, ranked options, and free text response questions
//  - significantly expanded the ability to make teaming rules based on gender or racial/ethnic identity
//  - removed dark mode in windows
//  - somewhat inconsequential mistake in GA::mate where startteam could be > endteam
//  - now preferentially mutates the lowest scoring team(s) within a genome
//  - updated Qt to v6.9.1; updated c++ to c++20; updated build to allow CI with GitHub Action & SignPath codesigning
//
// TO DO:
//    NEW FEATURES:
//  - fully implement "need" vs "want" (or "requirement" vs "preference"?)
//  - scale the penalty points with weight?
//  - add to windows installer a check on whether gruepr is currently running; provide error message and clean quit if so
//  - in teammatesRules dialog, enable the 'load from teamsTab' action
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Blackboard, Qualtrics, others
//  - add motion to the LMS busy dialog so that it doesn't appear frozen (LMS.cpp line 118)
//
//    INTERNAL:
//  - refactor gender counts from teamRecord into genderCrierion
//  - continue removing c-style arrays, non-range-based for loops, and pointer arithmetic everywhere except in intensive optimization steps
//      - replace arrays for StudentRecord.unavailable, TeamRecord.numStudentsAvailable, EditOrAddStudentDialog.tempUnavailability
//      - much harder: replace arrays for all of the attribute-related stuff
//      - add bounds checking whenever using [], .at, .first, .constFirst, .begin, etc.
//  - analyze for memory leaks
//      - memory leak -> crash when loading large file, unloading, then repeating a few times
//  - compile for webassembly, turn into a webapp
//      - move from OpenMP to QThread or c++ threads?
//
//    NETWORK IMPLEMENTATION:
//  - create timeout function to more nicely handle LMS connections
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: accepting responses, don't collect email, don't limit one response per user, don't show link to respond again, make publicly accessible
//      - Question options: req'd question, answer validity checks (for email & numerical input questions)
//  - errors when trying to connect to Google on home network when IPv6 is enabled (IPv6? eero-network?)
//
//    WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
//  - use multiple genepools with limited cross-breeding
//  - to get around the redundancy-of-genome issue, store each genome as std::set< std::set< int > >. Each team is set of indexes to the students; each section is set of teams.
//      - would also allow sorting teams within set by ascending score and thus mutations preferentially at front
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr_globals.h"
#include "dialogs/startDialog.h"
#include "widgets/verticalspinboxstyle.h"
#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    // Set up application
    #if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
        // remove darkmode on Windows (it is removed in the plist on macOS)
        qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
    #endif
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

    #if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
        // give spinboxes vertically aligned up and down arrows (which is default on macOS)
        a.setStyle(new VerticalSpinBoxStyle(a.style()));
    #endif

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
