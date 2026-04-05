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
//  - enabled a 'load from teamsTab' action in the "required" and "prevented" teammates in order to re-create or split apart an existing set of teams
//  - changed handling of gender, allowing it to be multi-valued (students can select >1 in survey; set of values saved in the studentRecord)
//  - penalty points now scale with the criterion weight so that higher-priority penalties are more impactful
//  - now correctly resizes columns in the team display tree whenever expanding an individual team
//  - now correctly reports duplicate students when the names and emails are auto-derived from Canvas roster
//  - now shows bar graphs indicating how many students selected each multiple choice response
//  - multiple choice response counts now correctly account for added / removed / edited students
//  - several bugfixes related to resorting teams
//  - added numerical attribute, ranked options, and free text response questions
//  - significantly expanded the ability to make teaming rules based on gender or racial/ethnic identity
//  - removed dark mode in windows
//  - somewhat inconsequential mistake in GA::mate where startteam could be > endteam
//  - now preferentially performs GA::mutate on the lowest scoring team within a genome
//  - added motion to the LMS busy dialog so that it doesn't appear frozen while connecting / downloading data
//  - added nicer timeout function to handle LMS connection issue
//  - fixed errors with Google connection and authorization related to IPv6 networking and PKCE authorization
//  - fixed several memory leaks and thread race conditions
//  - C++ code modernization throughout, using RAII architecture and reducing fixed size arrays
//  - updated Qt to v6.9.1; updated c++ to c++20; updated build to allow CI with GitHub Action & SignPath codesigning
//  - added to windows installer a check on whether gruepr is currently running; provide error message and clean quit if so
//  - added a linux build (untested!)
//
// TO DO:
//    NEW FEATURES:
//  - fully implement "need" vs "want" (or "requirement" vs "preference"?)
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Qualtrics, Microsoft Forms (Azure/Entra, whenever their API is published)
//
//    INTERNAL:
//  - compile for webassembly, turn into a webapp
//      - move from OpenMP to QThread or c++ threads?
//
//    NETWORK IMPLEMENTATION:
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: don't collect email, don't limit one response per user, don't show link to respond again
//      - Question options: req'd question, answer validity checks (for email & numerical input questions)
//
//    WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
//  - use multiple genepools with limited cross-breeding
//  - to get around the redundancy-of-genome issue, sort indexes w/in each team  and then each teams w/in the genome
//      - alternatively, store each genome as std::set< std::set< int > >, but that adds to data overhead
//      - could also store genepool as std::set < genome >, sorted by score
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
