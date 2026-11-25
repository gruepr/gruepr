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
//  - added peer review, a third button on the start screen, to ask the members of each team for feedback about each other
//      - teams come from a saved gruepr session, a spreadsheet, or a Canvas group set
//      - peer feedback can be solicited via Google Form or a Canvas quiz (a separate quiz for each team)
//      - questions can be rating scales, number scales, contribution to the work, unscored choices, numbers, or written answers
//      - start from a default set of questions, a contribution-based set, a saved question set, or an earlier peer review
//      - each question can be set as a self assessment, peer assessment, self and peer assessment, or standalone question
//      - each question's responses can be private to the instructor, shared with teammates anonymously, or shared with teammates with name attached
//      - results show who has responded, each student's ratings and how much teammates agree, the answers to each question, contribution, and flags
//      - each student's feedback report can be previewed and edited, then delivered as PDFs, a mail merge file, Google Docs, or posted as a Canvas comment
//  - added a confirmation dialog when closing a teamset tab
//  - switched the session save file from QFile to QSaveFile for crash-proof saves
//  - switched saved authorization tokens to encrypted storage
//  - added a way to delete saved team forming sessions
//  - moved Windows Store to be msix based, for auto-maintained updates
//  - fixed accented letters showing incorrectly in csv files
//  - added progress bars to the slower Canvas actions: downloading survey results, creating a survey or teams in Canvas, and the peer review quizzes
//  - added feature to remind students who haven't submitted a survey or a peer review: copy their email addresses, send Canvas messages, or make mail merge file with personalized messages
//
// TO DO:
//
//    NEW FEATURES:
//  - fully implement "need" vs "want" (or "requirement" vs "preference"?)
//  - add an option to specify 'characteristics' of the off-sized teams (low or high value of attribute; particular student on it)
//  - add integration with Qualtrics, moodle, SurveyMonkey, possibly by outputting an xml survey (check QTI) and inputting csv
//  - add integration with Microsoft Forms (Azure/Entra, whenever their API is published)
//
//    INTERNAL:
//  - move required / separated attributes into distinct criteria?
//  - move the teammates rules from studentrecord.groupTogether & .splitApart to living in the teammatescriterion
//      - can then remove gruepr* from teammatescriterion.cpp
//
//    NETWORK IMPLEMENTATION:
//  - enable in Google Forms various options -- must wait on new API functionality from Google
//      - Form options: don't collect email, don't limit one response per user, don't show link to respond again
//      - Question options: answer validity checks (for email & numerical input questions)
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
