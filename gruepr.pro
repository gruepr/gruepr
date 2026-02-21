#-------------------------------------------------
#
# Project created by QtCreator 2019-02-23T11:02:15
#
#-------------------------------------------------

gruepr_version = 13.0
copyright_year = 2019-2026

QT += core gui widgets concurrent network printsupport charts networkauth designer

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

# Secrets - CI or local, whichever exists
exists(ci_secrets.pri): include(ci_secrets.pri)
exists(local_secrets.pri): include(local_secrets.pri)

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

CONFIG += c++20

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE -= -Os
# add the desired -O2 if not present
QMAKE_CXXFLAGS_RELEASE += -O2

# add OpenMP
win32: QMAKE_CXXFLAGS += -openmp
macx {
    HOMEBREW_PREFIX = $$system(brew --prefix)
    OMP_PREFIX = $$system(brew --prefix libomp)
    QMAKE_CXXFLAGS += -Xclang -fopenmp
    QMAKE_LFLAGS += -lomp
    INCLUDEPATH += $$OMP_PREFIX/include
    LIBS += -L$$OMP_PREFIX/lib -lomp
}

# Run ASan:
# win32: QMAKE_CXXFLAGS += /fsanitize=address /Zi
# win32: QMAKE_LFLAGS += /DEBUG
# macx: QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
# macx: QMAKE_LFLAGS += -fsanitize=address

SOURCES += \
        criteria/URMIdentityCriterion.cpp \
        criteria/attributeCriterion.cpp \
        criteria/genderCriterion.cpp \
        criteria/sectionCriterion.cpp \
        criteria/teammatesCriterion.cpp \
        criteria/teamsizeCriterion.cpp \
        csvfile.cpp \
        dataOptions.cpp \
        GA.cpp \
        gruepr.cpp \
        gruepr_globals.cpp \
        Levenshtein.cpp \
        main.cpp \
        studentRecord.cpp \
        surveyMakerWizard.cpp \
        teamingOptions.cpp \
        teamRecord.cpp \
        criteria/gradeBalanceCriterion.cpp \
        criteria/scheduleCriterion.cpp \
        dialogs/attributeRulesDialog.cpp \
        dialogs/baseTimeZoneDialog.cpp \
        dialogs/categorizingDialog.cpp \
        dialogs/customResponseOptionsDialog.cpp \
        dialogs/customTeamnamesDialog.cpp \
        dialogs/customTeamsizesDialog.cpp \
        dialogs/dataTypesTableDialog.cpp \
        dialogs/dayNamesDialog.cpp \
        dialogs/editOrAddStudentDialog.cpp \
        dialogs/editSectionNamesDialog.cpp \
        dialogs/findMatchingNameDialog.cpp \
        dialogs/gatherURMResponsesDialog.cpp \
        dialogs/identityRulesDialog.cpp \
        dialogs/listTableDialog.cpp \
        dialogs/loadDataDialog.cpp \
        dialogs/progressDialog.cpp \
        dialogs/registerDialog.cpp \
        dialogs/sampleQuestionsDialog.cpp \
        dialogs/startDialog.cpp \
        dialogs/teammatesRulesDialog.cpp \
        dialogs/whichFilesDialog.cpp \
        LMS/LMS.cpp \
        LMS/canvashandler.cpp \
        LMS/googlehandler.cpp \
        widgets/attributeWidget.cpp \
        widgets/boxwhiskerplot.cpp \
        widgets/comboBoxWithElidedContents.cpp \
        widgets/dropcsvframe.cpp \
        widgets/frameThatForwardsMouseClicks.cpp \
        widgets/groupingCriteriaCardWidget.cpp \
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
        criteria/URMIdentityCriterion.h \
        criteria/attributeCriterion.h \
        criteria/genderCriterion.h \
        criteria/sectionCriterion.h \
        criteria/teammatesCriterion.h \
        criteria/teamsizeCriterion.h \
        csvfile.h \
        dataOptions.h \
        GA.h \
        gruepr.h \
        gruepr_globals.h \
        Levenshtein.h \
        studentRecord.h \
        survey.h \
        surveyMakerWizard.h \
        teamingOptions.h \
        teamRecord.h \
        criteria/criterion.h \
        criteria/gradeBalanceCriterion.h \
        criteria/scheduleCriterion.h \
        dialogs/attributeRulesDialog.h \
        dialogs/baseTimeZoneDialog.h \
        dialogs/categorizingDialog.h \
        dialogs/customResponseOptionsDialog.h \
        dialogs/customTeamnamesDialog.h \
        dialogs/customTeamsizesDialog.h \
        dialogs/dataTypesTableDialog.h \
        dialogs/dayNamesDialog.h \
        dialogs/editOrAddStudentDialog.h \
        dialogs/editSectionNamesDialog.h \
        dialogs/findMatchingNameDialog.h \
        dialogs/gatherURMResponsesDialog.h \
        dialogs/identityRulesDialog.h \
        dialogs/listTableDialog.h \
        dialogs/loadDataDialog.h \
        dialogs/progressDialog.h \
        dialogs/registerDialog.h \
        dialogs/sampleQuestionsDialog.h \
        dialogs/startDialog.h \
        dialogs/teammatesRulesDialog.h \
        dialogs/whichFilesDialog.h \
        LMS/LMS.h \
        LMS/canvashandler.h \
        LMS/googlehandler.h \
        widgets/attributeWidget.h \
        widgets/boxwhiskerplot.h \
        widgets/comboBoxWithElidedContents.h \
        widgets/dropcsvframe.h \
        widgets/frameThatForwardsMouseClicks.h \
        widgets/groupingCriteriaCardWidget.h \
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
