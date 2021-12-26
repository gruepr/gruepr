#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 10.3
copyright_year = 2019-2021

QT       += core gui widgets concurrent network printsupport charts
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

CONFIG += c++11

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE -= -Os
# add the desired -O2 if not present
QMAKE_CXXFLAGS_RELEASE += -O2

# add OpenMP
win32: QMAKE_CXXFLAGS += -fopenmp #use -fopenmp for mingw, -openmp for msvc
win32: LIBS += -fopenmp
macx: QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -I/usr/local/include
macx: LIBS += -L /usr/local/lib /usr/local/Cellar/llvm/9.0.1/lib/libomp.dylib

# static builds
QTPREFIX=$$[QT_INSTALL_PREFIX]
equals(QTPREFIX, "C:/Qt/5.15.2/mingw81_64_static"){
    message("--STATIC BUILD--")
    CONFIG += qt static
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
} else {
    message("--NON-STATIC BUILD--")
}

SOURCES += \
        Levenshtein.cpp \
        boxwhiskerplot.cpp \
        csvfile.cpp \
        dataOptions.cpp \
        main.cpp \
        gruepr.cpp \
        GA.cpp \
        studentRecord.cpp \
        surveymaker.cpp \
        teamRecord.cpp \
        teamingOptions.cpp \
        dialogs\baseTimeZoneDialog.cpp \
        dialogs\customTeamnamesDialog.cpp \
        dialogs\customTeamsizesDialog.cpp \
        dialogs\dayNamesDialog.cpp \
        dialogs\editOrAddStudentDialog.cpp \
        dialogs\findMatchingNameDialog.cpp \
        dialogs\gatherAttributeValuesDialog.cpp \
        dialogs\gatherTeammatesDialog.cpp \
        dialogs\gatherURMResponsesDialog.cpp \
        dialogs\listTableDialog.cpp \
        dialogs\progressDialog.cpp \
        dialogs\registerDialog.cpp \
        dialogs\whichFilesDialog.cpp \
        widgets\attributeTabItem.cpp \
        widgets\categoricalSpinBox.cpp \
        widgets\comboBoxWithElidedContents.cpp \
        widgets\pushButtonWithMouseEnter.cpp \
        widgets\sortableTableWidgetItem.cpp \
        widgets\studentTableWidget.cpp \
        widgets\teamTreeWidget.cpp

HEADERS += \
        Levenshtein.h \
        boxwhiskerplot.h \
        csvfile.h \
        dataOptions.h \
        gruepr.h \
        GA.h \
        gruepr_consts.h \
        studentRecord.h \
        surveymaker.h \
        teamRecord.h \
        teamingOptions.h \
        dialogs\baseTimeZoneDialog.h \
        dialogs\customTeamnamesDialog.h \
        dialogs\customTeamsizesDialog.h \
        dialogs\dayNamesDialog.h \
        dialogs\editOrAddStudentDialog.h \
        dialogs\findMatchingNameDialog.h \
        dialogs\gatherAttributeValuesDialog.h \
        dialogs\gatherTeammatesDialog.h \
        dialogs\gatherURMResponsesDialog.h \
        dialogs\listTableDialog.h \
        dialogs\progressDialog.h \
        dialogs\registerDialog.h \
        dialogs\whichFilesDialog.h \
        widgets\attributeTabItem.h \
        widgets\categoricalSpinBox.h \
        widgets\comboBoxWithElidedContents.h \
        widgets\pushButtonWithMouseEnter.h \
        widgets\sortableTableWidgetItem.h \
        widgets\studentTableWidget.h \
        widgets\teamTreeWidget.h

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
