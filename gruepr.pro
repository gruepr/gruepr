#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 12.0
copyright_year = 2019-2023

QT       += core gui widgets concurrent network printsupport charts networkauth webenginewidgets

TARGET = gruepr
TEMPLATE = app
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0

DEFINES += GRUEPR_VERSION_NUMBER='\\"$$gruepr_version\\"'
DEFINES += GRUEPR_COPYRIGHT_YEAR='\\"$$copyright_year\\"'
DEFINES += NUMBER_VERSION_FIELDS=4  # Allowing for version numbers 4 levels deep (i.e., 0.0.0.0)
DEFINES += NUMBER_VERSION_PRECISION=100 # Allowing for version values up to but not including 100 (i.e., 0.0.0.0 -> 99.99.99.99)

# set application properties
VERSION = $$gruepr_version
QMAKE_TARGET_COPYRIGHT = $$copyright_year
QMAKE_TARGET_COMPANY = gruepr
QMAKE_TARGET_PRODUCT = gruepr

# set application icon
win32: RC_ICONS = icons_new\icons.ico
macx: ICON = icons_new\gruepr.icns

# set mac info.plist and bundle
macx: QMAKE_INFO_PLIST = macOS\MyAppInfo.plist
macx: QMAKE_TARGET_BUNDLE_PREFIX = com.gruepr

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE -= -Os
# add the desired -O2 if not present
QMAKE_CXXFLAGS_RELEASE += -O2

# add OpenMP
win32: QMAKE_CXXFLAGS += -openmp #use -fopenmp for mingw, -openmp for msvc
#win32: LIBS += -openmp          #needed for mingw (?)
win32: LIBS += -L"C:\msys64\home\jhertz\openssl-1.1.1d\dist\bin"
macx: QMAKE_CXXFLAGS += -Xpreprocessor -fopenmp -I/usr/local/include
macx: LIBS += -L /usr/local/lib /usr/local/Cellar/llvm/9.0.1/lib/libomp.dylib

SOURCES += \
        Levenshtein.cpp \
        boxwhiskerplot.cpp \
        canvashandler.cpp \
        csvfile.cpp \
        dataOptions.cpp \
        googlehandler.cpp \
        gruepr_globals.cpp \
        main.cpp \
        gruepr.cpp \
        GA.cpp \
        studentRecord.cpp \
        survey.cpp \
        surveyMakerWizard.cpp \
        surveymaker.cpp \
        teamRecord.cpp \
        teamingOptions.cpp \
        dialogs/baseTimeZoneDialog.cpp \
        dialogs/customResponseOptionsDialog.cpp \
        dialogs/customTeamnamesDialog.cpp \
        dialogs/customTeamsizesDialog.cpp \
        dialogs/dayNamesDialog.cpp \
        dialogs/editOrAddStudentDialog.cpp \
        dialogs/findMatchingNameDialog.cpp \
        dialogs/gatherAttributeValuesDialog.cpp \
        dialogs/gatherTeammatesDialog.cpp \
        dialogs/gatherURMResponsesDialog.cpp \
        dialogs/listTableDialog.cpp \
        dialogs/progressDialog.cpp \
        dialogs/registerDialog.cpp \
        dialogs/startDialog.cpp \
        dialogs/whichFilesDialog.cpp \
        widgets/comboBoxThatPassesScrollwheel.cpp \
        widgets/surveyMakerPage.cpp \
        widgets/surveyMakerQuestionWithSwitch.cpp \
        widgets/switchButton.cpp \
        widgets/teamsTabItem.cpp \
        widgets/attributeTabItem.cpp \
        widgets/comboBoxWithElidedContents.cpp \
        widgets/pushButtonWithMouseEnter.cpp \
        widgets/sortableTableWidgetItem.cpp \
        widgets/studentTableWidget.cpp \
        widgets/teamTreeWidget.cpp

HEADERS += \
        Levenshtein.h \
        boxwhiskerplot.h \
        canvashandler.h \
        csvfile.h \
        dataOptions.h \
        googlehandler.h \
        gruepr.h \
        GA.h \
        gruepr_globals.h \
        studentRecord.h \
        survey.h \
        surveyMakerWizard.h \
        surveymaker.h \
        teamRecord.h \
        teamingOptions.h \
        dialogs/baseTimeZoneDialog.h \
        dialogs/customResponseOptionsDialog.h \
        dialogs/customTeamnamesDialog.h \
        dialogs/customTeamsizesDialog.h \
        dialogs/dayNamesDialog.h \
        dialogs/editOrAddStudentDialog.h \
        dialogs/findMatchingNameDialog.h \
        dialogs/gatherAttributeValuesDialog.h \
        dialogs/gatherTeammatesDialog.h \
        dialogs/gatherURMResponsesDialog.h \
        dialogs/listTableDialog.h \
        dialogs/progressDialog.h \
        dialogs/registerDialog.h \
        dialogs/startDialog.h \
        dialogs/whichFilesDialog.h \
        widgets/comboBoxThatPassesScrollwheel.h \
        widgets/surveyMakerPage.h \
        widgets/surveyMakerQuestionWithSwitch.h \
        widgets/switchButton.h \
        widgets/teamsTabItem.h \
        widgets/attributeTabItem.h \
        widgets/comboBoxWithElidedContents.h \
        widgets/pushButtonWithMouseEnter.h \
        widgets/sortableTableWidgetItem.h \
        widgets/studentTableWidget.h \
        widgets/teamTreeWidget.h

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
