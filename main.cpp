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
//  - SIGNIFICANT speed-up in the optimization process
//      - using faster data structures in the attribute, gender, race/ethnicity, required/prevented teammates, and schedule criteria
//      - multi-threaded the mating process that creates most of the next generation
//      - modified the genetic algorithm's mating procedure (crossover) to better maintain teams from dad's allele
//      - modified the mutation process from random to hill-climbing (meaning this is now technically a memetic, not genetic, algorithm)
//      - tweaked the genetic algorithm parameters and processes through benchmark testing
//      - modified the math for computing and sorting genome scores when they have >= 1 non-positive team score(s)
//      - added link time optimization during compiling
//  - Made export of teams more flexible, including export as Excel (xlsx) file
//  - Now alphabetizes the students / teams by last name on first display of a team set
//  - Changed the progress graph from box and whisker plots to a simpler line graph tracking the max. score
//  - Better UI associated with the "Duplicate" marker in student list
//  - Added a 2nd set of sample survey results: testdata_largeclass.csv
//  - Bugfix: autoscroll issue when drag/drop reordering the criteria cards
//  - Bugfix: project assignment criteria has a fallback when there are fewer projects than teams: options now repeat as needed
//
// INPROG:
//
// TO DO:
//  - Allow import of student data as Excel (xlsx) file
//
//    NEW FEATURES:
//  - fully implement "need" vs "want" (or "requirement" vs "preference"?)
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Qualtrics, moodle, SurveyMonkey, possibly by outputting an xml survey (check QTI) and inputting csv
//  - add integration with Microsoft Forms (Azure/Entra, whenever their API is published)
//  - add a peer review system
//
//    INTERNAL:
//  - compile for webassembly, turn into a webapp
//      - move from OpenMP to QThread or c++ threads?
//
//    NETWORK IMPLEMENTATION:
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: don't collect email, don't limit one response per user, don't show link to respond again
//      - Question options: req'd question, answer validity checks (for email & numerical input questions)
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr_globals.h"
#include "dialogs/startDialog.h"
#include "widgets/verticalspinboxstyle.h"
#include "bench/bench_mutation_sweep.h"
#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    benchMutationSweep::run(); return 0;

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
