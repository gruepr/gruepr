#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 10.1
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
        attributeTabItem.cpp \
        baseTimeZoneDialog.cpp \
        boxwhiskerplot.cpp \
        categoricalSpinBox.cpp \
        comboBoxWithElidedContents.cpp \
        csvfile.cpp \
        customTeamnamesDialog.cpp \
        customTeamsizesDialog.cpp \
        dataOptions.cpp \
        dayNamesDialog.cpp \
        editOrAddStuentDialog.cpp \
        findMatchingNameDialog.cpp \
        gatherAttributeValuesDialog.cpp \
        gatherTeammatesDialog.cpp \
        gatherURMResponsesDialog.cpp \
        listTableDialog.cpp \
        main.cpp \
        gruepr.cpp \
        GA.cpp \
        progressDialog.cpp \
        pushButtonWithMouseEnter.cpp \
        registerDialog.cpp \
        sortableTableWidgetItem.cpp \
        studentRecord.cpp \
        studentTableWidget.cpp \
        surveymaker.cpp \
        teamRecord.cpp \
        teamTreeWidget.cpp \
        teamingOptions.cpp \
        whichFilesDialog.cpp

HEADERS += \
        Levenshtein.h \
        attributeTabItem.h \
        baseTimeZoneDialog.h \
        boxwhiskerplot.h \
        categoricalSpinBox.h \
        comboBoxWithElidedContents.h \
        csvfile.h \
        customTeamnamesDialog.h \
        customTeamsizesDialog.h \
        dataOptions.h \
        dayNamesDialog.h \
        editOrAddStuentDialog.h \
        findMatchingNameDialog.h \
        gatherAttributeValuesDialog.h \
        gatherTeammatesDialog.h \
        gatherURMResponsesDialog.h \
        gruepr.h \
        GA.h \
        gruepr_consts.h \
        listTableDialog.h \
        progressDialog.h \
        pushButtonWithMouseEnter.h \
        registerDialog.h \
        sortableTableWidgetItem.h \
        studentRecord.h \
        studentTableWidget.h \
        surveymaker.h \
        teamRecord.h \
        teamTreeWidget.h \
        teamingOptions.h \
        whichFilesDialog.h

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
