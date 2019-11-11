#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 9.0
copyright_year = 2019

QT       += core gui widgets concurrent network printsupport
win32: QT += winextras

TARGET = gruepr
TEMPLATE = app

DEFINES += GRUEPR_VERSION_NUMBER='\\"$$gruepr_version\\"'
DEFINES += GRUEPR_COPYRIGHT_YEAR='\\"$$copyright_year\\"'

# set application properties
VERSION = $$gruepr_version
QMAKE_TARGET_COPYRIGHT = $$copyright_year
QMAKE_TARGET_COMPANY = gruepr
QMAKE_TARGET_PRODUCT = gruepr

# set application icon
win32: RC_ICONS = icons\gruepr.ico
macx: ICON = icons\gruepr.icns


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -Os

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE += -O3

# add OpenMP
win32: QMAKE_CXXFLAGS += -fopenmp
win32: LIBS += -fopenmp
macx: QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -I/usr/local/include
macx: LIBS += -L /usr/local/lib /usr/local/lib/libomp.dylib

SOURCES += \
        customWidgets.cpp \
        main.cpp \
        gruepr.cpp \
        GA.cpp \
        customDialogs.cpp \
        surveymaker.cpp

HEADERS += \
        customWidgets.h \
        gruepr.h \
        GA.h \
        customDialogs.h \
        gruepr_structs_and_consts.h \
        surveymaker.h

FORMS += \
      gruepr.ui \
      surveymaker.ui

RESOURCES += \
          gruepr.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
