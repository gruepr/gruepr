/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019
// Joshua Hertz
// gruepr@gmail.com
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see < https://www.gnu.org/licenses/ >.

//    This software incorporates code from the open source Qt libraries,
//    using version 5.12 or 5.13. These can be freely downloaded from
//    < http://qt.io/download >.

//    Icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).

//    An embedded font is used, mostly for pdf and printer output:
//    Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk)
//    released under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUTURE WORK:
// - integrate with Google Drive: download survey results from within the application; store all data in gruepr account instead of user account?
// - allowing load of external data, using levenshtein::distance to match names
// - improve preview of team files to be saved

// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - change stability metric? (base the convergence metric on the population median score relative to population max)
// - maintain population diversity through prevention of inbreeding (store parent and grandparent of each genome and disallow tournament selection from genomes with a match)
// - use multiple genepools and allow limited cross-breeding
// - to get around the redundancy-of-genome issue, store each genome as an unordered_set of unordered_sets. Each team is a set of IDs; each section is a set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include "surveymaker.h"
#include <QApplication>
#include <QSplashScreen>
#include <QMessageBox>
#include <QToolButton>
#include <QThread>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    // Set up application
    QApplication a(argc, argv);
    QApplication::setOrganizationName("gruepr");
    QApplication::setApplicationName("gruepr");
    QApplication::setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");

    // Show splash screen
    QSplashScreen *splash = new QSplashScreen;
    QPixmap pic(":/icons/fish.png");
    splash->setPixmap(pic);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\ngruepr@gmail.com", Qt::AlignCenter, Qt::white);
    splash->show();
    QThread::sleep(2);

    // Create application choice (gruepr or SurveyMaker) window
    QMessageBox *startWindow = new QMessageBox;
    QFont *boxFont = new QFont("Oxygen Mono", a.font().pointSize()+8);
    startWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    startWindow->setWindowTitle(" ");
    startWindow->setFont(*boxFont); startWindow->setText("Select an app to run:");

    // Button metrics
    boxFont->setPointSize(a.font().pointSize()+4);
    int labelWidth = (QFontMetrics(*boxFont)).boundingRect("SurveyMaker").width();
    QSize defIconSize(labelWidth-20,labelWidth-20);
    QSize defButtonSize(labelWidth+20,labelWidth+20);

    // Create and add buttons
    QToolButton *leaveButton = new QToolButton(startWindow);
    leaveButton->setIconSize(defIconSize); leaveButton->setFont(*boxFont); leaveButton->setFixedSize(defButtonSize);
    leaveButton->setIcon(QIcon(":/icons/exit.png")); leaveButton->setText("Exit"); leaveButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    QToolButton *grueprButton = new QToolButton(startWindow);
    grueprButton->setIconSize(defIconSize); grueprButton->setFont(*boxFont); grueprButton->setFixedSize(defButtonSize);
    grueprButton->setIcon(QIcon(":/icons/gruepr.png")); grueprButton->setText("gruepr"); grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    QToolButton *survMakeButton = new QToolButton(startWindow);
    survMakeButton->setIconSize(defIconSize); survMakeButton->setFont(*boxFont); survMakeButton->setFixedSize(defButtonSize);
    survMakeButton->setIcon(QIcon(":/icons/surveymaker.png")); survMakeButton->setText("SurveyMaker"); survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    //order switches for some reason mac->windows
#ifdef Q_OS_MACOS
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
#endif
#ifdef Q_OS_WIN32
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
#endif
    startWindow->setEscapeButton(leaveButton);

    // Show application choice window and delete splash
    splash->finish(startWindow);
    startWindow->exec();
    QAbstractButton *result = startWindow->clickedButton();
    delete splash;
    delete startWindow;
    delete boxFont;

    // Run chosen application
    int executionResult = 0;
    if(result == grueprButton)
    {
        gruepr w;
        w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work
        w.show();
        executionResult = a.exec();
    }
    else if(result == survMakeButton)
    {
        SurveyMaker w;
        w.setWindowTitle("gruepr: SurveyMaker [*]");         // asterisk is placeholder, shown when there is unsaved work
        w.show();
        executionResult = a.exec();
    }

    return executionResult;
}
