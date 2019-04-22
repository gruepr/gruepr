/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019
// Joshua Hertz
// j.hertz@neu.edu
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
//    using version 5.12.1. These can be freely downloaded from
//    < http://qt.io/download >.

//    Icons were created by or modified from < https://icons8.com >.
//    These icons have been made available under a creative commons license.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUTURE WORK:
// - improve printing (print preview, pagination)
// - allow non-inclusion of schedule question
// - implement reading of URM status from data
// - allow swapping of teams
// - integrate with Google Drive: create form, download survey results from within the application
// - register without desktopservices use of external browser (might need to update Qt to 5.12.2!)

// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - if reached stability, continue with added mutation probability
// - change stability metric? (base the convergence metric on the population median score relative to population max)
// - maintain population diversity through prevention of inbreeding (store parent and grandparent of each genome and disallow tournament selection from genomes with a match)
// - use multiple genepools and allow limited cross-breeding
// - use parallel processing for faster operations--especially the breeding to next generation which can be completely done in parallel
// - to get around the redundancy-of-genome issue, store each genome as an unordered_set of unordered_sets. Each team is a set of IDs; each section is a set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include <QApplication>
#include <QSplashScreen>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("gruepr");
    a.setApplicationName("gruepr");
    a.setApplicationVersion(GRUEPR_VERSION_NUMBER);

    QSplashScreen *splash = new QSplashScreen;
    QPixmap pic(":/icons/splash.png");
    splash->setPixmap(pic);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\nj.hertz@neu.edu", Qt::AlignCenter);
    splash->show();
    QThread::sleep(3);

    gruepr w;
    w.show();

    splash->finish(&w);
    delete splash;

    return a.exec();
}