#ifndef GRUEPR_GLOBALS
#define GRUEPR_GLOBALS

#include "GA.h"
#include <QObject>


inline static const int MAX_STUDENTS = GA::MAX_RECORDS;               // each student is a "record" in the genetic algorithm
inline static const int MAX_IDS = 2 * MAX_STUDENTS;                   // since students can be removed and added yet IDs always increase, need more IDs than possible students
inline static const int MAX_TEAMS = MAX_STUDENTS/2;

inline static const int MAX_DAYS = 7;                                 // resolution of scheduling is 1 hr, and scope is weekly
inline static const int MAX_BLOCKS_PER_DAY = 24;
inline static const int MAX_ATTRIBUTES = 15;                          // maximum number of skills/attitudes in a survey
inline static const int MAX_NOTES_FIELDS = 99;                        // allowed number of notes fields in a survey
inline static const int MAX_PREFTEAMMATES = 10;

inline static const int HIGHSCHEDULEOVERLAPSCALE = 2;                 // if a team has more than the desired amount of schedule overlap, each additional overlap time is scaled by
                                                                      // the inverse of this factor (e.g., 2 means next additional hour is worth 1/2; next is worth 1/4; then 1/8, etc.)
                                                                      // DO NOT CHANGE TO ZERO--WILL CAUSE DIV. BY ZERO CRASH

inline static const int PRINTOUT_FONTSIZE = 9;

// define the unicode characters for left and right arrows and other glyphs
inline static const char16_t LEFTARROW = u'\u2190';
inline static const char16_t LEFTDOUBLEARROW = u'\u00AB';
inline static const char16_t RIGHTARROW = u'\u2192';
inline static const char16_t RIGHTARROWTOEND = u'\u21E5';
inline static const char16_t RIGHTDOUBLEARROW = u'\u00BB';
inline static const char16_t EDITPENCIL = u'\u270E';
inline static const char16_t LEFTRIGHTARROW = u'\u27F7';
inline static const char16_t BULLET = u'\u2022';

// define colors used throughout gruepr
#define TRANSPARENT "rgba(0, 0, 0, 0)"
#define STARFISHHEX "#ffd771"
#define DEEPWATERHEX "#053437"
#define OPENWATERHEX "#13a9b2"
#define AQUAHEX "#41d2ca"
#define BUBBLYHEX "#dcf2f4"
#define HIGHLIGHTYELLOWHEX "#ffff3b"
#define BOLDGREENHEX "#83f2a5"
#define BOLDPINKHEX "#f283a5"
#define LIGHTPINKHEX "#fbcfce"
#define LIGHTBLUEHEX "#ceeafb"


// define stylesheets used throughout gruepr
inline static const char TITLESTYLE[] = "font-size: 12pt; font-family: DM Sans; border-image: url(:/icons_new/surveyMakerWizardTitleBackground.png);";
inline static const char TOPLABELSTYLE[] = "color: white; font-size: 14pt; font-family: DM Sans;"
                                           "border-image: url(:/icons_new/surveyMakerWizardTopLabelBackground.png); height: 50px;";
inline static const char STDBUTTONSTYLE[] = "QPushButton {background-color: " DEEPWATERHEX "; "
                                                         "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; "
                                                         "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 10px;}";
inline static const char SMALLBUTTONSTYLE[] = "QPushButton {background-color: " DEEPWATERHEX "; "
                                                           "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; "
                                                           "color: white; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}"
                                              "QPushButton:disabled {background-color: lightGray; "
                                                                    "border-style: solid; border-width: 2px; border-radius: 5px; border-color: darkGray; "
                                                                    "color: darkGray; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}";
inline static const char SMALLBUTTONSTYLEINVERTED[] = "QPushButton {background-color: white; "
                                                                   "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; "
                                                                   "color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}"
                                                      "QPushButton:disabled {background-color: lightGray; "
                                                                            "border-style: solid; border-width: 2px; border-radius: 5px; border-color: darkGray; "
                                                                            "color: darkGray; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}";
inline static const char SMALLBUTTONSTYLETRANSPARENT[] = "QPushButton {background-color: " TRANSPARENT "; "
                                                                      "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; "
                                                                      "color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}"
                                                         "QPushButton:disabled {background-color: lightGray; "
                                                                               "border-style: solid; border-width: 2px; border-radius: 5px; border-color: darkGray; "
                                                                               "color: darkGray; font-family: 'DM Sans'; font-size: 12pt; padding: 5px;}";
inline static const char ATTRIBBUTTONONSTYLE[] = "QPushButton {background-color: " OPENWATERHEX "; "
                                                               "border-style: solid; border-width: 1px; border-color: lightGray; "
                                                               "border-top-left-radius: 0px; border-top-right-radius: 0px; "
                                                               "border-bottom-left-radius: 0px; border-bottom-right-radius: 0px; "
                                                               "color: white; font-family: 'DM Sans'; font-size: 12pt; "
                                                               "padding-top: 5px; padding-bottom: 5px; padding-left: 15px; padding-right: 15px;}";
inline static const char ATTRIBBUTTONOFFSTYLE[] = "QPushButton {background-color: white; "
                                                                "border-style: solid; border-width: 1px; border-color: lightGray; "
                                                                "border-top-left-radius: 0px; border-top-right-radius: 0px; "
                                                                "border-bottom-left-radius: 0px; border-bottom-right-radius: 0px; "
                                                                "color: " OPENWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; "
                                                                "padding-top: 5px; padding-bottom: 5px; padding-left: 15px; padding-right: 15px;}";
inline static const char GETSTARTEDBUTTONSTYLE[] = "background-color: " OPENWATERHEX "; "
                                                   "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; "
                                                   "color: white; font-family: 'DM Sans'; font-size: 14pt; padding: 12px;";
inline static const char NEXTBUTTONSTYLE[] = "QPushButton {background-color: white; "
                                                          "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; "
                                                          "color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 14pt; padding: 10px;}"
                                             "QPushButton:disabled {background-color: lightGray; "
                                                                   "border-style: solid; border-width: 2px; border-radius: 5px; border-color: darkGray; "
                                                                   "color: darkGray; font-family: 'DM Sans'; font-size: 14pt; padding: 10px;}";
inline static const char INVISBUTTONSTYLE[] = "background-color: " TRANSPARENT "; border-style: none; color: " TRANSPARENT "; font-size: 1pt; padding: 0px;";
inline static const char DELBUTTONSTYLE[] = "QPushButton {background: " TRANSPARENT "; color: " DEEPWATERHEX "; "
                                                          "font-family: 'DM Sans'; font-size: 10pt; border: none;}"
                                             "QPushButton:disabled {background: " TRANSPARENT "; color: lightGray; "
                                                                   "font-family: 'DM Sans'; font-size: 10pt; border: none;}";
inline static const char ADDBUTTONSTYLE[] = "QPushButton {background: " TRANSPARENT "; color: " OPENWATERHEX "; "
                                                         "font-family: 'DM Sans'; font-size: 12pt; text-align: left; border: none;}"
                                             "QPushButton:disabled {background: " TRANSPARENT "; color: lightGray; "
                                                                   "font-family: 'DM Sans'; font-size: 12pt; border: none;}";
inline static const char EXAMPLEBUTTONSTYLE[] = "QPushButton {background: rgba(211, 211, 211, 128); color: " DEEPWATERHEX "; "
                                                             "font-family: 'DM Sans'; font-size: 10pt; font-weight: bold; "
                                                             "border-style: solid; border-width: 1px; border-radius: 5px; padding: 10px;}";
inline static const char LABELSTYLE[] = "QLabel {color: " DEEPWATERHEX "; font-size: 10pt; font-family: DM Sans; border: none;}"
                                          "QLabel:disabled {color: darkGray; font-size: 10pt; font-family: DM Sans; border: none;}";
inline static const char LINEEDITSTYLE[] = "QLineEdit {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                       "font-family: 'DM Sans'; font-size: 12pt;}"
                                             "QLineEdit:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                  "font-family: 'DM Sans'; font-size: 12pt;}";
inline static const char PLAINTEXTEDITSTYLE[] = "QPlainTextEdit {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                                 "font-family: 'DM Sans'; font-size: 12pt;}"
                                                 "QPlainTextEdit:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                           "font-family: 'DM Sans'; font-size: 12pt;}";
inline static const char TEXTEDITSTYLE[] = "QTextEdit {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                      "font-family: 'DM Sans'; font-size: 12pt;}"
                                            "QTextEdit:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                "font-family: 'DM Sans'; font-size: 12pt;}";
inline static const char LINEEDITERRORSTYLE[] = "QLineEdit {background-color: white; color: red; border-style: solid; border-color: black; border-width: 1px; "
                                                            "font-family: 'DM Sans'; font-size: 12pt;}"
                                                 "QLineEdit:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: black; border-width: 1px; "
                                                                      "font-family: 'DM Sans'; font-size: 12pt;}";
inline static const char COMBOBOXSTYLE[] = "QComboBox {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                       "font-family: 'DM Sans'; font-size: 12pt;}"
                                             "QComboBox:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                  "font-family: 'DM Sans'; font-size: 12pt;}"
                                             "QComboBox::drop-down {border-width: 0px;}"
                                             "QComboBox::down-arrow {image: url(:/icons_new/ComboBoxButton.png); width: 14px; height: 9px; border-width: 0px;}"
                                             "QComboBox QAbstractItemView {background-color: white; color: black; selection-background-color: " DEEPWATERHEX "; selection-color: white;}";
inline static const char ERRORCOMBOBOXSTYLE[] = "QComboBox {background-color: red; color: black; border-style: solid; border-color: black; border-width: 1px;"
                                                            "font-family: 'DM Sans'; font-size: 12pt;}"
                                                 "QComboBox:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                            "font-family: 'DM Sans'; font-size: 12pt;}"
                                                 "QComboBox::drop-down {border-width: 0px;}"
                                                 "QComboBox::down-arrow {image: url(:/icons_new/ComboBoxButton.png); width: 14px; height: 9px; border-width: 0px;}";
inline static const char SPINBOXSTYLE[] = "QSpinBox {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                     "font-family: 'DM Sans'; font-size: 12pt;}"
                                            "QSpinBox:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                "font-family: 'DM Sans'; font-size: 12pt;}"
                                            "QSpinBox::down-arrow {image: url(:/icons_new/ComboBoxButton.png); width: 14px; height: 12px; border: none;}"
                                            "QSpinBox::down-button {background-color: " TRANSPARENT "; border: none;}"
                                            "QSpinBox::up-arrow {image: url(:/icons_new/SpinBoxButton.png); width: 14px; height: 12px; border: none;}"
                                            "QSpinBox::up-button {background-color: " TRANSPARENT "; border: none;}";
inline static const char DOUBLESPINBOXSTYLE[] = "QDoubleSpinBox {background-color: white; color: " DEEPWATERHEX "; border-style: solid; border-color: black; border-width: 1px; "
                                                                "font-family: 'DM Sans'; font-size: 12pt;}"
                                                  "QDoubleSpinBox:disabled {background-color: lightGray; color: darkGray; border-style: solid; border-color: darkGray; border-width: 1px; "
                                                                           "font-family: 'DM Sans'; font-size: 12pt;}"
                                                  "QDoubleSpinBox::down-arrow {image: url(:/icons_new/ComboBoxButton.png); width: 14px; height: 12px; border-width: 0px;}"
                                                  "QDoubleSpinBox::down-button {background-color: " TRANSPARENT "; border: none;}"
                                                  "QDoubleSpinBox::up-arrow {image: url(:/icons_new/SpinBoxButton.png); width: 14px; height: 12px; border-width: 0px;}"
                                                  "QDoubleSpinBox::up-button {background-color: " TRANSPARENT "; border: none;}";
inline static const char CHECKBOXSTYLE[] = "QCheckBox {background-color: " TRANSPARENT "; font-family: 'DM Sans'; font-size: 10pt;}"
                                             "QCheckBox::disabled {color: darkGray; font-family: 'DM Sans'; font-size: 10pt;}"
                                             "QCheckBox::indicator {background-color: white; width: 12px; height: 12px; border: 2px solid " DEEPWATERHEX ";}"
                                             "QCheckBox::indicator:checked {background-color: white; image: url(:/icons_new/Checkmark.png);}";
inline static const char RADIOBUTTONSTYLE[] = "QRadioButton {background-color: " TRANSPARENT "; font-family: 'DM Sans'; font-size: 10pt;}"
                                                "QRadioButton::disabled {color: darkGray; font-family: 'DM Sans'; font-size: 10pt;}"
                                                "QRadioButton::indicator {width: 16px; height: 16px;}"
                                                "QRadioButton::indicator::unchecked {image: url(:/icons_new/uncheckedradio.png);}"
                                                "QRadioButton::indicator::checked {image: url(:/icons_new/checkedradio.png);}";
inline static const char SCROLLBARSTYLE[] = "QScrollBar:vertical {border-style: none; background-color: " TRANSPARENT "; width: 15px; margin: 0 0 5px 5px;}"
                                              "QScrollBar::handle:vertical {background-color: " DEEPWATERHEX "; border-radius: 4px;}"
                                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {height: 0px;}"
                                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {height: 0px;}"
                                            "QScrollBar:horizontal {border-style: none; background-color: " TRANSPARENT "; height: 15px; margin: 5px 5px 0 0;}"
                                              "QScrollBar::handle:horizontal {background-color: " DEEPWATERHEX "; border-radius: 4px;}"
                                              "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {width: 0px;}"
                                              "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {width: 0px;}";
inline static const char TABWIDGETSTYLE[] = "QTabWidget::pane {background-color: " TRANSPARENT ";}"
                                              "QTabBar {background-color: white; color: " DEEPWATERHEX "; font: 12pt 'DM Sans';}"
                                              "QTabBar::tab {background-color: white; padding: 10px;}"
                                              "QTabBar::tab:selected {background-color: white; border: 3px solid white; border-bottom-color: " OPENWATERHEX ";}"
                                              "QTabBar::tab:!selected {background-color: white; border: 3px solid white; border-bottom-color: white;}"
                                              "QTabBar::tab:hover {background-color: white; border: 3px solid white; border-bottom-color: " AQUAHEX ";}";

inline static const int DIALOG_SPACER_ROWHEIGHT = 20;
inline static const int LG_DLG_SIZE = 600;
inline static const int SM_DLG_SIZE = 300;
inline static const int XS_DLG_SIZE = 200;
inline static const int SMALL_SCREENSIZE_WIN = 900;
inline static const int SMALL_SCREENSIZE_MAC = 800;
inline static const int REDUCED_ICON_SIZE = 30;
inline static const int MSGBOX_ICON_SIZE = 40;
inline static const int UI_DISPLAY_DELAYTIME = 2000;

// data with these roles are stored in columns of the team info display tree,
inline static const int TEAMINFO_DISPLAY_ROLE = Qt::UserRole;         // shown as the team's data value for each column
inline static const int TEAMINFO_SORT_ROLE = Qt::UserRole + 1;        // used when sorting the columns
inline static const int TEAM_NUMBER_ROLE = Qt::UserRole + 2;          // column 0 of the team info display tree, used when swapping teams or teammates

enum class GenderType {biol, adult, child, pronoun};
//order of enum below MUST match order of options within the subsequent strings
enum class Gender {woman, man, nonbinary, unknown};
inline static const char BIOLGENDERS[] {"female/male/intersex or nonbinary/unknown"};
inline static const char BIOLGENDERS7CHAR[] {"Female / Male  /Nonbin./Unknown"};  // 7 character names are used for printing
inline static const char BIOLGENDERSINITIALS[] {"F/M/X/?"};                       // initials are used in table of teams
inline static const char ADULTGENDERS[] {"woman/man/nonbinary/unknown"};
inline static const char ADULTGENDERSPLURAL[] {"women/men/nonbinary/unknown"};
inline static const char ADULTGENDERS7CHAR[] {" Woman /  Man  /Nonbin./Unknown"};
inline static const char ADULTGENDERSINITIALS[] {"W/M/X/?"};
inline static const char CHILDGENDERS[] {"girl/boy/nonbinary/unknown"};
inline static const char CHILDGENDERSPLURAL[] {"girls/boys/nonbinary/unknown"};
inline static const char CHILDGENDERS7CHAR[] {" Girl  /  Boy  /Nonbin./Unknown"};
inline static const char CHILDGENDERSINITIALS[] {"G/B/X/?"};
inline static const char PRONOUNS[] {"she - her/he - him/they - them/unknown"};
inline static const char PRONOUNS7CHAR[] {"she-her/he-him /they-th/Unknown"};
inline static const char PRONOUNSINITIALS[] {"S/H/T/?"};
//next two used to replace "unknown" for the response option in the survey
inline static const char UNKNOWNVALUE[] {"unknown"};
inline static const char PREFERNOTRESPONSE[] {"prefer not to answer"};

inline static const char SURVEYINSTRUCTIONS[] {"Instructions:\n\n"
                                               "Your response to this survey will help you be on the best possible project team.\n\n"
                                               "All answers are strictly confidential, and all answers are acceptable.\n\n"
                                               "Please be as honest as possible!"};

inline static const char FIRSTNAMEQUESTION[] = "What is your first (or chosen) name?";
inline static const char LASTNAMEQUESTION[] = "What is your last name?";
inline static const char EMAILQUESTION[] = "What is your email address?";
inline static const char GENDERQUESTION[] = "With which gender do you identify most closely?";
inline static const char PRONOUNQUESTION[] = "What are your pronouns?";
inline static const char URMQUESTION[] = "How do you identify your race, ethnicity, or cultural heritage?";
inline static const char TIMEZONEQUESTION[] = "What time zone will you be based in during this class?";
enum {Sun, Mon, Tue, Wed, Thu, Fri, Sat};
inline static const char SCHEDULEQUESTION1[] = "Select the times that you are ";
inline static const char SCHEDULEQUESTION2BUSY[] = "BUSY and will be UNAVAILABLE";
inline static const char SCHEDULEQUESTION2FREE[] = "FREE and will be AVAILABLE";
inline static const char SCHEDULEQUESTION3[] = " for group work.";
inline static const char SCHEDULEQUESTION4[] = " ** Note: Times refer to ";
inline static const char SCHEDULEQUESTIONHOME[] = "your home timezone.";
inline static const char SECTIONQUESTION[] = "In which section are you enrolled?";
inline static const char PREFTEAMMATEQUESTION1TYPE[] = "Type";
inline static const char PREFTEAMMATEQUESTION1SELECT[] = "Select";
inline static const char PREFTEAMMATEQUESTION2AMATE[] = " the name of a classmate ";
inline static const char PREFTEAMMATEQUESTION2MULTIA[] = " the name(s) of up to ";
inline static const char PREFTEAMMATEQUESTION2MULTIB[] = " classmates ";
inline static const char PREFTEAMMATEQUESTION3YES[] = "you want to work with.";
inline static const char PREFTEAMMATEQUESTION3NO[] = "you want to avoid working with.";
inline static const char PREFTEAMMATEQUESTION4TYPEONE[] = " Please write their first and last name.";
inline static const char PREFTEAMMATEQUESTION4TYPEMULTI[] = " Please write their first and last name, and put a comma between each classmate.";
inline static const char SELECTONE[] = "Select one:";
inline static const char SELECTMULT[] = "Select all that apply:";

//map of the "meaning" of strings that might be used in the survey to refer to hours of the day
inline static const char TIME_NAMES[] {"1:00,1am,1 am,1:00am,1:00 am,2:00,2am,2 am,2:00am,2:00 am,3:00,3am,3 am,3:00am,3:00 am,4:00,4am,4 am,4:00am,4:00 am,"
                         "5:00,5am,5 am,5:00am,5:00 am,6:00,6am,6 am,6:00am,6:00 am,7:00,7am,7 am,7:00am,7:00 am,8:00,8am,8 am,8:00am,8:00 am,"
                         "9:00,9am,9 am,9:00am,9:00 am,10:00,10am,10 am,10:00am,10:00 am,11:00,11am,11 am,11:00am,11:00 am,12:00,12pm,12 pm,12:00pm,12:00 pm,"
                         "13:00,1pm,1 pm,1:00pm,1:00 pm,14:00,2pm,2 pm,2:00pm,2:00 pm,15:00,3pm,3 pm,3:00pm,3:00 pm,16:00,4pm,4 pm,4:00pm,4:00 pm,"
                         "17:00,5pm,5 pm,5:00pm,5:00 pm,18:00,6pm,6 pm,6:00pm,6:00 pm,19:00,7pm,7 pm,7:00pm,7:00 pm,20:00,8pm,8 pm,8:00pm,8:00 pm,"
                         "21:00,9pm,9 pm,9:00pm,9:00 pm,22:00,10pm,10 pm,10:00pm,10:00 pm,23:00,11pm,11 pm,11:00pm,11:00 pm,0:00,12am,12 am,12:00am,12:00 am,noon,midnight"};
inline static const int TIME_MEANINGS[] {1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,
                           5,5,5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,
                           9,9,9,9,9,10,10,10,10,10,11,11,11,11,11,12,12,12,12,12,
                           13,13,13,13,13,14,14,14,14,14,15,15,15,15,15,16,16,16,16,16,
                           17,17,17,17,17,18,18,18,18,18,19,19,19,19,19,20,20,20,20,20,
                           21,21,21,21,21,22,22,22,22,22,23,23,23,23,23,0,0,0,0,0,12,0};
inline static const int NUMOFTIMENAMESPERHOUR = 5;

inline static const char TIMEZONEREGEX[] {R"((.*?)\[?(?>GMT|UTC)\s?([\+\-]?\d{2}):?(\d{2}).*)"}; // capture (1) intro text,
                                                                                                 // skip "[" if present, then either "GMT" or "UTC", then any whitespace if present
                                                                                                 // capture (2) + or - if present plus 2 digits
                                                                                                 // skip ":" if present,
                                                                                                 // capture (3) 2 digits
                                                                                                 // skip rest
inline static const int STANDARDSCHEDSTARTTIME = 10;  //10 am
inline static const int STANDARDSCHEDENDTIME = 17;  //5 pm

inline static const char TIMESTAMP_FORMAT1[] {"yyyy/MM/dd h:mm:ss AP"};
inline static const char TIMESTAMP_FORMAT2[] {"yyyy/MM/dd h:mm:ssAP"};
inline static const char TIMESTAMP_FORMAT3[] {"M/d/yyyy h:mm:ss"};
inline static const char TIMESTAMP_FORMAT4[] {"M/d/yyyy h:mm"};

inline static const char TIMEZONENAMES[] {"International Date Line West [GMT-12:00];\"Samoa: Midway Island, Samoa [GMT-11:00]\";"
                            "Hawaiian: Hawaii [GMT-10:00];Alaskan: Alaska [GMT-09:00];Pacific: US and Canada, Tijuana [GMT-08:00];Mountain: US and Canada [GMT-07:00];"
                            "\"Mexico Pacific: Chihuahua, La Paz, Mazatlan [GMT-07:00]\";Central: US and Canada [GMT-06:00];Canada Central: Saskatchewan [GMT-06:00];\""
                            "Mexico Central: Guadalajara, Mexico City, Monterrey [GMT-06:00]\";Central America: Central America [GMT-06:00];Eastern: US and Canada [GMT-05:00];"
                            "\"S.A. Pacific: Bogota, Lima, Quito [GMT-05:00]\";Atlantic: Canada [GMT-04:00];\"S.A. Western: Caracas, La Paz [GMT-04:00]\";"
                            "Pacific S.A.: Santiago [GMT-04:00];Newfoundland and Labrador [GMT-03:30];E. South America: Brasilia [GMT-03:00];"
                            "\"S.A. Eastern: Buenos Aires, Georgetown [GMT-03:00]\";Greenland [GMT-03:00];Mid-Atlantic Islands [GMT-02:00];Azores [GMT-01:00];Cape Verde [GMT-01:00];"
                            "\"Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London [GMT+00:00]\";\"Greenwich: Casablanca, Monrovia [GMT+00:00]\";"
                            "\"Central Europe: Belgrade, Bratislava, Budapest, Ljubljana, Prague [GMT+01:00]\";\"Central Europe: Sarajevo, Skopje, Warsaw, Zagreb [GMT+01:00]\";"
                            "\"Romance: Brussels, Copenhagen, Madrid, Paris [GMT+01:00]\";\"W. Europe: Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna [GMT+01:00]\";"
                            "W. Central Africa: West Central Africa [GMT+01:00];E. Europe: Bucharest [GMT+02:00];Egypt: Cairo [GMT+02:00];"
                            "\"FLE: Helsinki, Kiev, Riga, Sofia, Tallinn, Vilnius [GMT+02:00]\";\"GTB: Athens, Istanbul, Minsk [GMT+02:00]\";Israel: Jerusalem [GMT+02:00];"
                            "\"South Africa: Harare, Pretoria [GMT+02:00]\";\"Russian: Moscow, St. Petersburg, Volgograd [GMT+03:00]\";\"Arab: Kuwait, Riyadh [GMT+03:00]\";"
                            "E. Africa: Nairobi [GMT+03:00];Arabic: Baghdad [GMT+03:00];Iran: Tehran [GMT+03:30];\"Arabian: Abu Dhabi, Muscat [GMT+04:00]\";"
                            "\"Caucasus: Baku, Tbilisi, Yerevan [GMT+04:00]\";Afghanistan: Kabul [GMT+04:30];Ekaterinburg [GMT+05:00];"
                            "\"West Asia: Islamabad, Karachi, Tashkent [GMT+05:00]\";\"India: Chennai, Kolkata, Mumbai, New Delhi [GMT+05:30]\";Nepal: Kathmandu [GMT+05:45];"
                            "\"Central Asia: Astana, Dhaka [GMT+06:00]\";Sri Lanka: Sri Jayawardenepura [GMT+06:00];\"N. Central Asia: Almaty, Novosibirsk [GMT+06:00]\";"
                            "Myanmar: Yangon Rangoon [GMT+06:30];\"S.E. Asia: Bangkok, Hanoi, Jakarta [GMT+07:00]\";North Asia: Krasnoyarsk [GMT+07:00];"
                            "\"China: Beijing, Chongqing, Hong Kong SAR, Urumqi [GMT+08:00]\";\"Singapore: Kuala Lumpur, Singapore [GMT+08:00]\";Taipei: Taipei [GMT+08:00];"
                            "W. Australia: Perth [GMT+08:00];\"North Asia East: Irkutsk, Ulaanbaatar [GMT+08:00]\";Korea: Seoul [GMT+09:00];\"Tokyo: Osaka, Sapporo, Tokyo [GMT+09:00]\";"
                            "Yakutsk: Yakutsk [GMT+09:00];A.U.S. Central: Darwin [GMT+09:30];Cen. Australia: Adelaide [GMT+09:30];"
                            "\"A.U.S. Eastern: Canberra, Melbourne, Sydney [GMT+10:00]\";E. Australia: Brisbane [GMT+10:00];Tasmania: Hobart [GMT+10:00];"
                            "Vladivostok: Vladivostok [GMT+10:00];\"West Pacific: Guam, Port Moresby [GMT+10:00]\";"
                            "\"Central Pacific: Magadan, Solomon Islands, New Caledonia [GMT+11:00]\";\"Fiji Islands: Fiji Islands, Kamchatka, Marshall Islands [GMT+12:00]\";"
                            "\"New Zealand: Auckland, Wellington [GMT+12:00]\";Tonga: Nuku'alofa [GMT+13:00];Line Islands: Kiribati [GMT+14:00]"};

//the built-in Likert scale responses offered in surveyMaker -- should not contain any of these characters: , ; < > & =
inline static const char RESPONSE_OPTIONS[] {"1. Yes / 2. No;"
                               "1. Yes / 2. Maybe / 3. No;"
                               "1. Definitely / 2. Probably / 3. Maybe / 4. Probably not / 5. Definitely not;"
                               "1. Strongly preferred / 2. Preferred / 3. Opposed / 4. Strongly opposed;"
                               "1. True / 2. False;"
                               "1. Like me / 2. Not like me;"
                               "1. Agree / 2. Disagree;"
                               "1. Strongly agree / 2. Agree / 3. Undecided / 4. Disagree / 5. Strongly disagree;"
                               "1. 4.0 - 3.75 / 2. 3.74 - 3.5 / 3. 3.49 - 3.25 / 4. 3.24 - 3.0 / 5. 2.99 - 2.75 / 6. 2.74 - 2.5 / 7. 2.49 - 2.0 / 8. Below 2.0 / 9. Not sure or prefer not to say;"
                               "1. 100 - 90 / 2. 89 - 80 / 3. 79 - 70 / 4. 69 - 60 / 5. 59 - 50 / 6. Below 50 / 7. Not sure or prefer not to say;"
                               "1. A / 2. B / 3. C / 4. D / 5. F / 6. Not sure or prefer not to say;"
                               "1. Very high / 2. Above average / 3. Average / 4. Below average / 5. Very low;"
                               "1. Excellent / 2. Very good / 3. Good / 4. Fair / 5. Poor;"
                               "1. Highly positive / 2. Somewhat positive / 3. Neutral / 4. Somewhat negative / 5. Highly negative;"
                               "1. A lot of experience / 2. Some experience / 3. Little experience / 4. No experience;"
                               "1. Extremely / 2. Very / 3. Moderately / 4. Slightly / 5. Not at all;"
                               "1. A lot / 2. Some / 3. Very Little / 4. None;"
                               "1. Much more / 2. More / 3. About the same / 4. Less / 5. Much less;"
                               "1. Most of the time / 2. Some of the time / 3. Seldom / 4. Never;"
                               "1. Available / 2. Available but prefer not to / 3. Not available;"
                               "1. Very frequently / 2. Frequently / 3. Occasionally / 4. Rarely / 5. Never;"
                               "1. Definitely will / 2. Probably will / 3. Probably won't / 4. Definitely won't;"
                               "1. Very important / 2. Important / 3. Somewhat important / 4. Not important;"
                               "1. Leader / 2. Mix of leader and follower / 3. Follower;"
                               "1. Highly confident / 2. Moderately confident / 3. Somewhat confident / 4. Not confident;"
                               "1 / 2 / 3 / 4 / 5;"
                               "1 / 2 / 3 / 4 / 5 / 6 / 7 / 8 / 9 / 10;"
                               "Custom options..."};

// Options for the team names. A name for each list of names must be given.
// If name ends with period, names are numeric and increase without end
// If name ends with asterisk, names beyond list get repeated (as in A, B, C... -> AA, BB, CC... -> AAA, BBB, CCC...)
// If name ends with tilde, names beyond list get repeated with space between (as in A, B, C... -> A A, B B, C C... -> A A A, B B B, C C C...)
// If name ends with hashtag, names beyond list get 'sequeled' (as in A, B, C... -> A 2, B 2, C 2... -> A 3, B 3, C 3...)
inline static const char TEAMNAMECATEGORIES[] {"Arabic numbers.,"
                                 "Roman numerals.,"
                                 "Hexadecimal numbers.,"
                                 "Binary numbers.,"
                                 "English letters*,"
                                 "Greek letters (uppercase)*,"
                                 "Greek letters (lowercase)*,"
                                 "NATO phonetic alphabet~,"
                                 "Chemical elements#,"
                                 "Papal names#,"
                                 "Constellations#,"
                                 "Crayola crayon colors#,"
                                 "Genres of music#,"
                                 "Cheeses#,"
                                 "Shakespeare plays (RSC chron.)#,"
                                 "Languages (globally most spoken)#,"
                                 "All time best-selling albums in US#,"
                                 "Minor Simpsons characters#,"
                                 "Bones of the human skeleton@,"
                                 "Minor league baseball teams#,"
                                 "Discontinued Olympic sports#,"
                                 "Groups of animals#,"
                                 "Obsolete units of measure@,"
                                 "Everyone's a winner!@"};
inline static const char TEAMNAMELISTS[]   {";"
                              ";"
                              ";"
                              ";"
                              "A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z;"
                              "Α,Β,Γ,Δ,Ε,Ζ,Η,Θ,Ι,Κ,Λ,Μ,Ν,Ξ,Ο,Π,Ρ,Σ,Τ,ϒ,Φ,Χ,Ψ,Ω;"
                              "α,β,γ,δ,ε,ζ,η,θ,ι,κ,λ,μ,ν,ξ,ο,π,ρ,σ,τ,υ,φ,χ,ψ,ω;"
                              "Alfa,Bravo,Charlie,Delta,Echo,Foxtrot,Golf,Hotel,India,Juliett,Kilo,Lima,Mike,"
                                 "November,Oscar,Papa,Quebec,Romeo,Sierra,Tango,Uniform,Victor,Whiskey,X-ray,Yankee,Zulu;"
                              "Hydrogen,Helium,Lithium,Beryllium,Boron,Carbon,Nitrogen,Oxygen,Fluorine,Neon,Sodium,Magnesium,"
                                 "Aluminum,Silicon,Phosphorus,Sulfur,Chlorine,Argon,Potassium,Calcium,Scandium,Titanium,Vanadium,"
                                 "Chromium,Manganese,Iron,Cobalt,Nickel,Copper,Zinc,Gallium,Germanium,Arsenic,Selenium,Bromine,Krypton,"
                                 "Rubidium,Strontium,Yttrium,Zirconium,Niobium,Molybdenum,Technetium,Ruthenium,Rhodium,Palladium,Silver,"
                                 "Cadmium,Indium,Tin,Antimony,Tellurium,Iodine,Xenon,Cesium,Barium,Lanthanum,Cerium,Praseodymium,Neodymium,"
                                 "Promethium,Samarium,Europium,Gadolinium,Terbium,Dysprosium,Holmium,Erbium,Thulium,Ytterbium,Lutetium,"
                                 "Hafnium,Tantalum,Tungsten,Rhenium,Osmium,Iridium,Platinum,Gold,Mercury,Thallium,Lead,Bismuth,Polonium,"
                                 "Astatine,Radon,Francium,Radium,Actinium,Thorium,Protactinium,Uranium,Neptunium,Plutonium,Americium,Curium,"
                                 "Berkelium,Californium,Einsteinium,Fermium,Mendelevium,Nobelium,Lawrencium,Rutherfordium,Dubnium,Seaborgium,"
                                 "Bohrium,Hassium,Meitnerium,Darmstadtium,Roentgenium,Copernicium,Nihonium,Flerovium,Moscovium,Livermorium,"
                                 "Tennessine,Oganesson;"
                              "Adrian,Benedict,Clement,Damasus,Eugene,Felix,Gregory,Hilarius,Innocent,John,Leo,Martin,Nicholas,Pius,Romanus,"
                                 "Stephen,Theodore,Urban,Victor,Zosimus,Alexander,Boniface,Celestine,Dionysius,Evaristus,Fabian,Gelasius,Honorius,"
                                 "Julius,Lucius,Marcellus,Pelagius,Sixtus,Telesphorus,Valentine,Zephyrinus;"
                              "Andromeda,Bootes,Cassiopeia,Draco,Equuleus,Fornax,Gemini,Hydra,Indus,Leo,Musca,Norma,Orion,Perseus,Reticulum,"
                                 "Sagittarius,Taurus,Ursa Major,Virgo;"
                              "Aquamarine,Burnt Sienna,Chartreuse,Dandelion,Emerald,Fuchsia,Goldenrod,Heat Wave,Indigo,Jungle Green,Laser Lemon,Mulberry,"
                                 "Neon Carrot,Orchid,Periwinkle,Razzmatazz,Scarlet,Thistle,Ultra Orange,Vivid Tangerine,Wisteria,Yosemite Campfire,Zircon;"
                              "Acapella,Blues,Country,Doo-Wop,EDM,Fado,Gospel,Hip-Hop,Indie,Jazz,K-Pop,Lullaby,Mariachi,New Age,Opera,Punk,"
                                 "Qawwali,Reggae,Soundtrack,Tejano,Underground,Vocal,Western Swing,Xhosa,Yodeling,Zydeco;"
                              "Asiago,Brie,Cheddar,Derby,Edam,Feta,Gouda,Havarti,Iberico,Jarlsberg,Kaseri,Limburger,Manchego,Neufchatel,Orla,"
                                 "Paneer,Queso Fresco,Ricotta,Siraz,Tyn Grug,Ulloa,Vignotte,Weichkaese,Xynotyro,Yorkshire Blue,Zamorano;"
                              "Taming of the Shrew,Henry VI,Two Gentlemen of Verona,Titus Andronicus,Richard III,Comedy of Errors,"
                                 "Love's Labour's Lost,Midsummer Night's Dream,Romeo and Juliet,Richard II,King John,Merchant of Venice,"
                                 "Henry IV,Much Ado about Nothing,Henry V,As You Like It,Julius Caesar,Hamlet,Merry Wives of Windsor,"
                                 "Twelfth Night,Troilus and Cressida,Othello,Measure for Measure,All's Well That Ends Well,Timon of Athens,"
                                 "King Lear,Macbeth,Antony and Cleopatra,Coriolanus,Pericles,Cymbeline,Winter's Tale,Tempest,Henry VIII,"
                                 "Two Noble Kinsmen;"
                              "Mandarin,Spanish,English,Hindi,Arabic,Portugese,Bengali,Russian,Japanese,Punjabi,German,Javanese,Wu,Malay,Telugu,Vietnamese,Korean,"
                                 "French,Marathi,Tamil,Urdu,Turkish,Italian,Yue,Thai,Gujarati,Jin,Southern Min,Persian,Polish,Pashto,Kannada,Xiang,Malayalam,Sundanese,"
                                 "Hausa,Odia,Burmese,Hakka,Ukranian,Bhojpuri,Tagalog,Yoruba,Maithili,Uzbek,Sindhi,Amharic,Fula,Romanian,Oromo,Igbo,Azerbaijani,Awadhi,Gan,"
                                 "Cebuano,Dutch,Kurdish,Serbo-Croatian,Malagasy,Saraiki,Nepali,Sinhala,Chittagonian,Zhuang,Khmer,Turkmen,Assamese,Madurese,Somali,Marwari,"
                                 "Magahi,Haryanvi,Hungarian,Chhattisgarhi,Greek,Chewa,Deccan,Akan,Kazakh,Northern Min,Sylethi,Zulu,Czech,Kinyarwanda,Dhundhari,Haitian Creole,"
                                 "Eastern Min,Ilocano,Quechua,Kirundi,Swedish,Hmong,Shona,Uyghur,Hiligaynon,Mossi,Xhosa,Belarusian,Balochi,Konkani;"
                              "Thriller,Eagles' Greatest Hits,Come On Over,Rumours,The Bodyguard,Back In Black,The Dark Side of the Moon,"
                                 "Saturday Night Fever,Bat Out of Hell,Bad,Led Zeppelin IV,21,Jagged Little Pill,1,ABBA's Greatest Hits,"
                                 "Appetite for Destruction,Hotel California,Supernatural,Metallica,The Immaculate Collection,Falling Into You,"
                                 "Born in the U.S.A.,Let's Talk About Love,The Wall,Sgt. Pepper's Lonely Hearts Club Band,Titanic,"
                                 "Dirty Dancing,Brothers in Arms,Nevermind,Dangerous,Abbey Road,Grease,Goodbye Yellow Brick Road,"
                                 "Slippery When Wet!,Music Box,Hybrid Theory,The Eminem Show,Come Away With Me,Unplugged,...Baby One More Time,Legend,"
                                 "Tapestry,No Jacket Required,Queen's Greatest Hits,True Blue,Bridge Over Troubled Water,The Joshua Tree,Purple Rain,"
                                 "Faith,Elvis' Christmas Album;"
                              "Artie Ziff,Brunella Pommelhorst,Cleetus,Disco Stu,Edna Krabappel,Frank 'Grimy' Grimes,Ginger Flanders,"
                                 "Helen Lovejoy,Itchy,Jebediah Springfield,Kent Brockman,Luann Van Houten,Mayor Quimby,Ned Flanders,Professor Frink,"
                                 "Queen Helvetica,Ruth Powers,Sideshow Bob,Troy McClure,Uter Zorker,Waylon Smithers,Xoxchitla,Yes Guy,Zelda;"
                              "Femur,Patella,Mandible,Hip,Metacarpal,Ulna,Humerus,Lacrimal,Distal Phalange,Cuboid,Trapezium,Coccyx,Zygomatic,Sternum,"
                                 "Ethmoid,Pisiform,Maxiallary,Lumbar Vertebrae,Stapes,Scapula,Navicular,Hamate,Rib,Hyoid,Occipital,Talus,Malleus,"
                                 "Triquetrum,Incus,Clavicle,Fibula,Proximal Phalange,Tibia,Lunate,Frontal,Palatine,Parietal,Medial Cuneiform,Vomer,"
                                 "Thoracic Vertebrae,Nasal,Capitate,Inferior Nasal Concha,Scaphoid,Sacrum,Temporal,Middle Cuneiform,Sphenoid,Calcaneus,"
                                 "Lateral Cuneiform,Radius,Cervical Vertebrae,Trapezoid;"
                              "Aviators,Blue Rocks,Crawdads,Drillers,Emeralds,Fireflies,GreenJackets,Hammerheads,Isotopes,Jumbo Shrimp,"
                                 "Knights,Lugnuts,Mighty Mussels,Nuts,Pelicans,Quakes,Rumble Ponies,Skeeters,Trash Pandas,Wood Ducks,Yard Goats,"
                                 "AquaSox,Bats,Curve,Drive,Express,Flying Squirrels,Grasshoppers,Hops,IronPigs,Blue Jays,Kernels,Lookouts,"
                                 "Mudcats,Naturals,Patriots,RubberDucks,Sod Poodles,TinCaps,Whitecaps,Baysox,Hot Rods,Marauders,SeaWolves,"
                                 "Cyclones,Bisons,RiverDogs,Threshers,Clippers,Hooks,Dragons,Tortugas,Shorebirds,Bulls,Chihuahuas,Woodpeckers,"
                                 "IronBirds,Nationals,Grizzlies,RoughRiders,Stripers,Senators,Snappers,Shuckers,Hillcats,Redbirds,RockHounds,"
                                 "Renegades,66ers,Cubs,BlueClaws,Cannon Ballers,Captains,Storm,Flying Tigers,Loons,Travelers,Tourists,Biscuits,"
                                 "Sounds,Fisher Cats,Tides,Barons,Dodgers,Storm Chasers,Cardinals,Blue Wahoos,Sea Dogs,River Bandits,Fightin Phils,"
                                 "Aces,Red Wings,River Cats,Red Sox,Lake Bees,Missions,Giants,RailRiders,Mets,Saints,Ports,Rainiers,Tarpons,Smokies,"
                                 "Mud Hens,Dust Devils,Canadians,Rawhide,Wind Surge,Dash,Timber Rattlers;"
                              "Angling,Bowling,Cannon Shooting,Dog Sledding,Engraving,Fire Fighting,Gliding,Hurling,India Club Swinging,Jeu de Paume,"
                                 "Korfball,Lacrosse,Motorcycle Racing,Orchestra,Pigeon Racing,Roller Hockey,Savate,Tug of War,Vaulting,Waterskiing,"
                                 "Ballooning,Croquet,Dueling Pistol,Kaatsen,Life Saving,Plunge Distance Diving,Rope Climb,Solo Synchronized Swimming,"
                                 "Standing High Jump,Polo;"
                              "Congregation of Alligators,Fluffle of Bunnies,Murder of Crows,Drove of Donkeys,Convocation of Eagles,School of Fish,Gaggle of Geese,"
                                 "Cackle of Hyenas,Mess of Iguanas,Shadow of Jaguars,Mob of Kangaroos,Pride of Lions,Labor of Moles,"
                                 "Blessing of Narwhals,Parliament of Owls,Pod of Porpoises,Bevy of Quail,Unkindness of Ravens,Shiver of Sharks,"
                                 "Rafter of Turkeys,Glory of Unicorns,Nest of Vipers,Confusion of Wildebeest,Herd of Yaks,Dazzle of Zebras,"
                                 "Army of Ants,Swarm of Bees,Brood of Chickens,Dule of Doves,Gang of Elk,Skulk of Foxes,Cloud of Gnats,"
                                 "Bloat of Hippopotamuses,Smuck of Jellyfish,Litter of Kittens,Plague of Locusts,Brace of Mallards,Watch of Nightingales,"
                                 "Bed of Oysters,String of Ponies,Flock of Quetzals,Crash of Rhinos,Dray of Squirrels,Bale of Turtles,Herd of Urchin,"
                                 "Committee of Vultures,Colony of Weasels;"
                              "Bunarium,Oxgang,Sthène,Poncelet,Jow,Cubit,Oka,Zentner,Buddam,Keel,Esterling,Slug,Hogshead,Masu,Omer,League,Perch,Pièze,Rood,"
                                 "Scruple,Morgen,Grain,Plethron,Congius,Ephah,Chungah,Ell,Pood,Funt,Homer,Grzywna,Zolotnik,Barleycorn,Gill,Quire;"
                              "A1,Gold,Blue Ribbon,Alpha,Numero Uno,First Place,Crème de la crème,Jewel in the crown,Pole Position,Elite,"
                                 "Superlative,Prime,Pick of the litter,Best of the bunch,Head of the line,Superior,Tip-top,Peak,First class,Flawless,"
                                 "Preeminent,Ultimate"};

bool internetIsGood();
//void testFunction();

inline static const char GRUEPRDOWNLOADPAGE[] {"https://www." GRUEPRHOMEPAGE "/#/" GRUEPRDOWNLOADSUBPAGE};

void aboutWindow(QWidget *parent);
inline static const char ABOUTWINDOWCONTENT[] {"<h1 style=\"font-family:'Paytone One';\">gruepr</h1>"
                                               "<p>v" GRUEPR_VERSION_NUMBER " &copy; " GRUEPR_COPYRIGHT_YEAR
                                               "<br>Joshua Hertz<br><a href=\"mailto:" GRUEPRHELPEMAIL "\">" GRUEPRHELPEMAIL "</a>"
                                               "<p>gruepr is an open source project. The source code is freely available at "
                                               "the project homepage: <a href=\"https://" GRUEPRHOMEPAGE "\">" GRUEPRHOMEPAGE "</a>"
                                               "<br><br>gruepr incorporates:"
                                               "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                                 "<li>UI and UX designs by <a href=\"https://scout.camd.northeastern.edu/\">Scout</a></li>"
                                                 "<li>Code libraries from <a href=\"http://qt.io\">Qt, v 6.5</a>, released under the GNU Lesser General Public License version 3</li>"
                                                 "<li>Icons from <a href=\"https://icons8.com\">Icons8</a>, released under Creative Commons license "
                                                      "\"Attribution-NoDerivs 3.0 Unported\"</li>"
                                                 "<li>The following fonts, all released under SIL OPEN FONT LICENSE V1.1:"
                                                 "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                                   "<li><span style=\"font-family:'Paytone One';\">"
                                                   "Paytone One</span> &copy; 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont)</li>"
                                                   "<li><span style=\"font-family:'DM Sans';\">"
                                                   "DM Sans</span> &copy; 2014-2017 Indian Type Foundry (info@indiantypefoundry.com)</li>"
                                                   "<li><span style=\"font-family:'Oxygen Mono';\">"
                                                   "Oxygen Mono</span> &copy; 2012 Vernon Adams (vern@newtypography.co.uk)</li>"
                                                 "</ul></li>"
                                               "</ul>"
                                               "<h3>Disclaimer</h3>"
                                               "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of "
                                               "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details."
                                               "<p>This program is free software: you can redistribute it and/or modify it under the terms of the "
                                               "<a href = https://www.gnu.org/licenses/gpl.html>GNU General Public License</a> "
                                               "as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version."};

void helpWindow(QWidget *parent);
inline static const char HELPWINDOWCONTENT[] {"<h1 style=\"font-family:'Paytone One';\">gruepr</h1>"
                                              "<p>v" GRUEPR_VERSION_NUMBER " &copy; " GRUEPR_COPYRIGHT_YEAR
                                              "<br>Joshua Hertz<br><a href=\"mailto:" GRUEPRHELPEMAIL "\">" GRUEPRHELPEMAIL "</a>"
                                              "<p>Project homepage: <a href=\"https://" GRUEPRHOMEPAGE "\">" GRUEPRHOMEPAGE "</a>"
                                              "<p>&nbsp; &nbsp;gruepr is a program for splitting a section of 4-500 students into optimized teams. It was originally based on "
                                                 "CATME's team forming routine as described in "
                                                 "<a href=\"http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf\">this paper</a>. "
                                                 "The student data is read from a file (typically directly downloaded within the app), and the students are split into teams "
                                                 "of any desired size(s). The algorithm is highly flexible, allowing the instructor to create and use customized surveys and "
                                                 "define in a highly tailored way the characteristics of an optimal team.</p>"
                                              "<h3>Obtaining student data</h3>"
                                              "<p>&nbsp; &nbsp;A customized Google Form is generally used to collect data from the students, and you can use gruepr to customize "
                                                 "and create this form. After collecting responses, you can download the results from within gruepr or you can open the Google Form "
                                                 "on your Google Drive, go the \"Responses\" tab, and click the three-dot icon to select \"Download responses (.csv)\". "
                                                 "Alternative means can be used to collect this data, since gruepr can use any data saved using the file format described below.</p>"
                                              "<h3>How the teams are optimized and displayed</h3>"
                                              "<p>&nbsp; &nbsp;To optimize the teams, a genetic algorithm is used. A good distribution of students into teams is determined by a "
                                                 "numerical \"compatibility score\". The instructor uses gruepr to determine how this score is calculated. Options include:</p>"
                                              "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                                "<li>Preventing isolated women, isolated men, and/or isolated non-binary persons;</li>"
                                                "<li>Requiring mixed gender teams;</li>"
                                                "<li>Preventing isolated students from underrepresented racial/ethnic groups</li>"
                                                "<li>Achieving within each team either homogeneity or heterogeneity of the student's responses to up to 15 multiple choice questions. "
                                                   "These questions might represent skills assessments, work preferences, attitudes, major, GPA, or any other "
                                                   "categorical/Likert question</li>"
                                                "<li>Preventing students with incompatible responses to a multiple choice question from being on the same team</li>"
                                                "<li>Requiring each team to have at least one student with a particular responses to a multiple choice question</li>"
                                                "<li>Achieving a high degree of overlap in schedule free time (with timezone awareness)</li>"
                                                "<li>Preventing any particular students from being on the same team</li>"
                                                "<li>Requiring any particular students to be on the same team</li>"
                                                "<li>Requiring at least a certain number of students from within a given list to be on the same team</li>"
                                              "</ul>"
                                              "<p>&nbsp; &nbsp;After optimizing the teams for some time, the best set of teams found is shown on the screen. The teams are displayed "
                                                 "showing the summarized survey data from each team's students. Each team's compatibility score is also shown. "
                                                 "You can choose whether to save this teamset, adjust this teamset by moving around individual students or teams, or throw away "
                                                 "the teamset entirely and start over. If you save the teamset, three files can be printed or saved as text or pdf files: "
                                                 "1) the full output including demographic data on each student; 2) the same screen output but without listing any of the team scores "
                                                 "or student attributes, which is useful for distributing to the students; and 3) the section, team, and student info in a spreadsheet "
                                                 "format.</p>"
                                              "<hr>"
                                              "<h3>Further details on the genetic optimization algorithm</h3>"
                                              "<p>&nbsp; &nbsp;A population of 30,000 random teamings (each is a \"genome\") is created and then refined over multiple generations. "
                                                 "In each generation, a few of the highest scoring \"elite\" genomes are directly copied (cloned) into the next generation, and then "
                                                 "the remainder are created by mating tournament-selected parent genomes using ordered crossover. Once the next generation's genepool "
                                                 "is created, each genome has 1 or more potential mutations, implementd as a swapping of two random locations on the genome. A genome's "
                                                 "net score is, generally, the harmonic mean of the compatibility score for each team. Evolution proceeds for at least a minimum number "
                                                 "of generations. It automatically stops (though the user can choose to keep it going indefinitely) after a maximum number of have been "
                                                 "reached or when the best score has remained +/- 1% for a set number of generations in a row.</p>"
                                              "<hr>"
                                              "<h3>Further details on the data file format</h3>"
                                              "<p>&nbsp; &nbsp;The datafile of student information must be a comma-separated-values (.csv), and it is easiest if the content is as "
                                                 "described below. Using the Google Form or other survey instrument created by gruepr allows you to download the data into exactly this "
                                                 "format without modification. The datafile contents: </p>"
                                              "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                                 "<li>a header row with the question texts, with specific wording needed if you want gruepr to auto-recognize each question's meaning</li>"
                                                 "<li>each student's response to the questions on a separate row, starting at the row immediately after the header</li>"
                                                 "<li>within each row, the order of comma-separated questions/responses should be:</li>"
                                                 "<ul style=\"margin-top: 2px; margin-bottom: 0px; margin-left: 15px; margin-right: 0px; -qt-list-indent: 0;\">"
                                                    "<li>timestamp</li>"
                                                    "<li>first name</li>"
                                                    "<li>last name</li>"
                                                    "<li>email address</li>"
                                                    "<li>gender, using nomenclature based on biological sex, adult identity, child identity, or pronouns</li>"
                                                    "<li>student's self-identified racial / ethnic / cultural identity</li>"
                                                    "<li>[0 to 15 values] text responses to the multiple choice questions, each in its own field</li>"
                                                    "<li>timezone</li>"
                                                    "<li>[0 to 7 values] semicolon-separated list of times within each day that the student is either available or unavailable to work</li>"
                                                    "<li>course section</li>"
                                                    "<li>student's preferred teammated</li>"
                                                    "<li>student's preferred non-teammates</li>"
                                                    "<li>any additional notes for the student (additional columns can be included for your own use but ignored by gruepr)</li>"
                                                 "</ul>"
                                              "</ul>"};

class MouseWheelBlocker : public QObject
{
    Q_OBJECT
public:
    explicit MouseWheelBlocker(QObject *parent);
protected:
    bool eventFilter(QObject* o, QEvent* e) override;
};

#endif // GRUEPR_GLOBALS
