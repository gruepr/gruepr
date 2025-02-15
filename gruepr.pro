#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 12.8
copyright_year = 2019-2025

QT       += core gui widgets concurrent network printsupport charts networkauth

TARGET = gruepr
TEMPLATE = app
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0
macx: QMAKE_APPLE_DEVICE_ARCHS = x86_64

DEFINES += GRUEPR_VERSION_NUMBER='\\"$$gruepr_version\\"'
DEFINES += GRUEPR_COPYRIGHT_YEAR='\\"$$copyright_year\\"'
DEFINES += NUMBER_VERSION_FIELDS=4  # Allowing for version numbers 4 levels deep (i.e., 0.0.0.0)
DEFINES += NUMBER_VERSION_PRECISION=100 # Allowing for version values up to but not including 100 (i.e., 0.0.0.0 -> 99.99.99.99)

DEFINES += VERSION_CHECK_URL='\\"https://api.github.com/repos/gruepr/gruepr/releases/latest\\"'
DEFINES += USER_REGISTRATION_URL='\\"https://script.google.com/macros/s/AKfycbzuMivfe02aRIf6hLJQAuhAvaunOOmAK8RAaUyySFOBOKYI9LXNemFbt_uMrunoNVmq/exec\\"'
DEFINES += GRUEPRHOMEPAGE='\\"gruepr.com\\"'
DEFINES += GRUEPRDOWNLOADSUBPAGE='\\"Download\\"'   # Need to add hash between homepage and this, but cannot include "#" in the define here
DEFINES += BUGREPORTPAGE='\\"https://github.com/gruepr/gruepr/issues\\"'
DEFINES += GRUEPRHELPEMAIL='\\"info@gruepr.com\\"'

#Set Config
CONFIG += no_moc_predefs #Added to avoid file name conflicts

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

DEFINES += QT_DISABLE_DEPRECATED_UP_TO=0x060500

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
macx: LIBS += -L /usr/local/lib
macx: LIBS += -L /usr/local/Cellar/llvm/9.0.1/lib/libomp.dylib

SOURCES += \
        csvfile.cpp \
        dataOptions.cpp \
        gruepr_globals.cpp \
        gruepr.cpp \
        GA.cpp \
        Levenshtein.cpp \
        main.cpp \
        studentRecord.cpp \
        surveyMakerWizard.cpp \
        teamRecord.cpp \
        teamingOptions.cpp \
        dialogs/attributeRulesDialog.cpp \
        dialogs/baseTimeZoneDialog.cpp \
        dialogs/customResponseOptionsDialog.cpp \
        dialogs/customTeamnamesDialog.cpp \
        dialogs/customTeamsizesDialog.cpp \
        dialogs/dayNamesDialog.cpp \
        dialogs/editOrAddStudentDialog.cpp \
        dialogs/editSectionNamesDialog.cpp \
        dialogs/findMatchingNameDialog.cpp \
        dialogs/gatherURMResponsesDialog.cpp \
        dialogs/getGrueprDataDialog.cpp \
        dialogs/listTableDialog.cpp \
        dialogs/progressDialog.cpp \
        dialogs/registerDialog.cpp \
        dialogs/sampleQuestionsDialog.cpp \
        dialogs/startDialog.cpp \
        dialogs/teammatesRulesDialog.cpp \
        dialogs/whichFilesDialog.cpp \
        LMS/LMS.cpp \
        LMS/canvashandler.cpp \
        LMS/googlehandler.cpp \
        widgets/attributeDiversitySlider.cpp \
        widgets/attributeWidget.cpp \
        widgets/boxwhiskerplot.cpp \
        widgets/comboBoxWithElidedContents.cpp \
        widgets/labelThatForwardsMouseClicks.cpp \
        widgets/labelWithInstantTooltip.cpp \
        widgets/pushButtonWithMouseEnter.cpp \
        widgets/sortableTableWidgetItem.cpp \
        widgets/studentTableWidget.cpp \
        widgets/surveyMakerQuestion.cpp \
        widgets/switchButton.cpp \
        widgets/teamsTabItem.cpp \
        widgets/teamTreeWidget.cpp

HEADERS += \
        csvfile.h \
        dataOptions.h \
        gruepr.h \
        GA.h \
        gruepr_globals.h \
        Levenshtein.h \
        studentRecord.h \
        survey.h \
        surveyMakerWizard.h \
        teamRecord.h \
        teamingOptions.h \
        dialogs/attributeRulesDialog.h \
        dialogs/baseTimeZoneDialog.h \
        dialogs/customResponseOptionsDialog.h \
        dialogs/customTeamnamesDialog.h \
        dialogs/customTeamsizesDialog.h \
        dialogs/dayNamesDialog.h \
        dialogs/editOrAddStudentDialog.h \
        dialogs/editSectionNamesDialog.h \
        dialogs/findMatchingNameDialog.h \
        dialogs/gatherURMResponsesDialog.h \
        dialogs/getGrueprDataDialog.h \
        dialogs/listTableDialog.h \
        dialogs/progressDialog.h \
        dialogs/registerDialog.h \
        dialogs/sampleQuestionsDialog.h \
        dialogs/startDialog.h \
        dialogs/teammatesRulesDialog.h \
        dialogs/whichFilesDialog.h \
        LMS/LMS.h \
        LMS/canvashandler.h \
        LMS/googlehandler.h \
        widgets/attributeDiversitySlider.h \
        widgets/attributeWidget.h \
        widgets/boxwhiskerplot.h \
        widgets/comboBoxWithElidedContents.h \
        widgets/labelThatForwardsMouseClicks.h \
        widgets/labelWithInstantTooltip.h \
        widgets/pushButtonWithMouseEnter.h \
        widgets/sortableTableWidgetItem.h \
        widgets/studentTableWidget.h \
        widgets/surveyMakerQuestion.h \
        widgets/switchButton.h \
        widgets/teamsTabItem.h \
        widgets/teamTreeWidget.h

FORMS += \
      dialogs/attributeRulesDialog.ui \
      dialogs/getGrueprDataDialog.ui \
      dialogs/sampleQuestionsDialog.ui \
      dialogs/teammatesRulesDialog.ui \
      dialogs/whichFilesDialog.ui \
      gruepr.ui

RESOURCES += \
          gruepr.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    macOS/MyAppInfo.plist
