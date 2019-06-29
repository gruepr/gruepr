/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr - SurveyMaker
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
//    using version 5.12.1. These can be freely downloaded from
//    < http://qt.io/download >.

//    Icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).

//    An embedded font is used:
//    Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk)
//    released under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "surveymaker.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("gruepr");
    a.setApplicationName("surveyMaker");
    a.setApplicationVersion(GRUEPR_VERSION_NUMBER);

    SurveyMaker w;
    w.show();

    return a.exec();
}
