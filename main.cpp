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
//  - updated Qt to v6.9.1; updated c++ to c++20
//  - removed dark mode in windows
//  - changed handling of gender, allowing it to be multi-valued (students can select >1 in survey; set of values in the studentRecord)
//  - somewhat inconsequential mistake in GA::mate where startteam could be > endteam
//  - now correctly resizes columns in the team display tree whenever expanding an individual team
//  - now correctly reports duplicate students when the names and emails are auto-derived from Canvas roster
//  - now shows bar graphs indicating how many students selected each multiple choice response
//  - multiple choice response counts now correctly account for added / removed / edited students
//  - several bugfixes related to resorting teams
//  - Pulling in much of the ***incredible*** dissertation work by Nikhen to modernize the UI
//  - diversity criteria for multiple choice questions now optimizes for most number of values (in addition to widest range of values for ordered questions)
//
// INPROG:
/*

- move all of the connects related to ui of gender identity card (gruepr.cpp lines 299-345) into criteria/genderCriterion.cpp, using model found in scheduleCriterion.cpp
- make diverse / similar radio button filled in when selected

MELDING CHANGES FROM NIKHEN'S WORK:
-- do for urm everything in parallel to gender:
  -- update addCriteriaCard for CriteriaType::urmIdentity
  -- do for teamingOptions->urmIdentity what was done for genderIdentity, but in QMap Gender --> QString
  -- replace isolatedURMPrevented with urmIdentityRule != 1
-- install mousewheelblocker on items in groupingcards
-- All the sizecriterion->ui should be moved into sizeCriterion functions

dialogs/loaddatadialog.cpp: needs complete overhaul
            ----->  --line 651, add checkbox for manual categorization with google or canvas


dialogs/identityrulesdialog: needs complete overhaul, must accept wider variety of rules (see gruepr.cpp line 361 or so); generalize to be relevant to gender or race/ethnicity/culture identity rules (curr. just works with gender i think?)


widgets/groupingCriteriaCardWidget: too tall (when expanded)


gruepr.cpp:
- NEED TO GO THROUGH!
- ***attribute divers/similar is functional, but required/incompatible rules are not!
- geometry bug when 'minimizing' the criterioncards (add new criteria button shrinks behind lower cards)
Related to groupingCriteriaCardWidget:
- fully implement "need" vs "want"
- Make sure attribute weights are determined by card order


Removed lines marked //FROMDEV
Does teamingoptions now include which criteria are being scored?
Implement avg. grade question/criterion

*/
//
// TO DO:
//    NEW FEATURES:
//  From Nikhen's work (some disabled ones currently commented "//FROMDEV":
//  - identity rules add >, >=, <, <=
//  - graphical display of which mandatory rule failed (scoring function will need to broadcast back the source(s) of any penalty points
//  - better vocabulary / UI for "set criteria as mandatory": maybe “musts” vs “wants”
//  Pre-dating Nikhen's work
//  - add to windows installer a check on whether gruepr is currently running; provide error message and clean quit if so
//  - add ranked option as a question type (set of drop downs? select 1st, select 2nd, select 3rd, etc.)
//  - add free response number as a question type (could be done in Canvas but not in Google Form, as it requires response validation added to the API)
//  - in teammatesRules dialog, enable the 'load from teamsTab' action
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Blackboard, Qualtrics, others
//  - add motion to the LMS busy dialog so that it doesn't appear frozen (LMS.cpp line 118)
//
//    INTERNAL:
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
//      - Question options: req'd question, answer validity checks
//  - errors when trying to connect to Google on home network when IPv6 is enabled (IPv6? eero-network?)
//
//    WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
//  - preferentially mutate the lowest scoring team(s) within a genome by getGenomeScore 'outputting' the worst team (or worst location in genePool[genome])
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
    #if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
        // remove darkmode on Windows
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
