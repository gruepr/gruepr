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
//  - over 50 bugfixes:
//      - loading data
//          - Excel cells containing formulas now load as their values
//          - multi-select gender answers separated by ", " now keep every gender
//          - switching to a Canvas or Google survey no longer deletes a survey file already loaded from your computer
//          - Canvas downloads no longer hang (now cancellable, 4-minute limit), are no longer capped at 200 items, and handle quiz names with special characters
//          - a previous Canvas download's roster is no longer added to a later survey
//      - comparing to a roster
//          - choosing a close match no longer changes a different student's name or email
//          - no longer crashes when roster and survey names differ only in capitalization
//          - students already removed are no longer matched or prompted about
//      - team sizes and sections
//          - custom team sizes no longer resets to all zero, and a single team no longer hangs the optimizer
//          - a section with fewer than 4 students is now placed on one team instead of mixing into another section or losing all results
//          - section names that differ only in capitalization are now one section, and the section selector stays in sync after renaming or merging sections
//      - teammate rules
//          - the "number of requests granted" setting no longer resets to "all"
//          - Split Apart now offers students' "want to avoid" preferences
//          - students with the same name each keep their own requests
//          - the dialog box refreshes after loading or clearing data
//      - scores and assignments
//          - team compatibility scores now account for gender, identity, and teammate rules after drag-and-drop and in every section
//          - project assignments for each section now use that section's own preferences
//          - teams with all students giving identical numerical answers now scored correctly, and "average" now targets the true class average
//      - editing students and the student table
//          - editing a student no longer changes their survey timestamp
//          - a blank racial/ethnic identity no longer creates a separate category, and new genders appear in gender rules
//          - attribute response counts now reflect the selected section
//      - survey maker
//          - a survey saved without a roster no longer breaks its preferred-teammate question when reopened
//          - a failed roster upload no longer erases the roster already loaded
//
// TO DO:
//
//    NEW FEATURES:
//  - fully implement "need" vs "want" (or "requirement" vs "preference"?)
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Qualtrics, moodle, SurveyMonkey, possibly by outputting an xml survey (check QTI) and inputting csv
//  - add integration with Microsoft Forms (Azure/Entra, whenever their API is published)
//  - add a peer review system
//
//    INTERNAL:
//  - move required / separated attributes into distinct criteria?
//  - move the teammates rules from studentrecord.groupTogether & .splitApart to living in the teammatescriterion
//      - can then remove gruepr* from teammatescriterion.cpp
//
//    NETWORK IMPLEMENTATION:
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: don't collect email, don't limit one response per user, don't show link to respond again
//      - Question options: req'd question, answer validity checks (for email & numerical input questions)
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
