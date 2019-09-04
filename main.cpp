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
// - integrate with Google Drive: download survey results from within the application
// - remover button in student table: bug that causes crash?

// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - change stability metric? (base the convergence metric on the population median score relative to population max)
// - maintain population diversity through prevention of inbreeding (store parent and grandparent of each genome and disallow tournament selection from genomes with a match)
// - use multiple genepools and allow limited cross-breeding
// - use parallel processing for faster operations
// - to get around the redundancy-of-genome issue, store each genome as an unordered_set of unordered_sets. Each team is a set of IDs; each section is a set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("gruepr");
    a.setApplicationName("gruepr");
    a.setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");

    QSplashScreen *splash = new QSplashScreen;
    QPixmap pic(":/icons/splash.png");
    splash->setPixmap(pic);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\ngruepr@gmail.com", Qt::AlignCenter);
    splash->show();
    QThread::sleep(2);

    gruepr w;
    w.show();
    w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work

    splash->finish(&w);
    delete splash;

    return a.exec();
}
