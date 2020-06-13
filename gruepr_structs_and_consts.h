#ifndef GRUEPR_STRUCTS_AND_CONSTS
#define GRUEPR_STRUCTS_AND_CONSTS

#define TIMESTAMP_FORMAT1 "yyyy/MM/dd h:mm:ss AP"
#define TIMESTAMP_FORMAT2 "yyyy/MM/dd h:mm:ssAP"
#define TIMESTAMP_FORMAT3 "M/d/yyyy h:mm:ss"
#define TIMESTAMP_FORMAT4 "M/d/yyyy h:mm"

#define USER_REGISTRATION_URL "https://script.google.com/macros/s/AKfycbwqGejEAumqgwpxDdXrV5CJS54gm_0N_du7BweU3wHG-XORT8g/exec"

#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include "GA.h"

//map of the "meaning" of strings that might be used in the Google Form to refer to hours of the day
const QMap<QString, int> meaningOfTimeNames{ {"1am",1}, {"2am",2}, {"3am",3}, {"4am",4}, {"5am",5}, {"6am",6}, {"7am",7}, {"8am",8}, {"9am",9}, {"10am",10}, {"11am",11}, {"12pm",12},
                                           {"1pm",13}, {"2pm",14}, {"3pm",15}, {"4pm",16}, {"5pm",17}, {"6pm",18}, {"7pm",19}, {"8pm",20}, {"9pm",21}, {"10pm",22}, {"11pm",23}, {"12am",0},
                                           {"1:00",1}, {"2:00",2}, {"3:00",3}, {"4:00",4}, {"5:00",5}, {"6:00",6}, {"7:00",7}, {"8:00",8}, {"9:00",9}, {"10:00",10}, {"11:00",11}, {"12:00",12},
                                           {"13:00",13}, {"14:00",14}, {"15:00",15}, {"16:00",16}, {"17:00",17}, {"18:00",18}, {"19:00",19}, {"20:00",20}, {"21:00",21}, {"22:00",22}, {"23:00",23}, {"0:00",0},
                                           {"1 am",1}, {"2 am",2}, {"3 am",3}, {"4 am",4}, {"5 am",5}, {"6 am",6}, {"7 am",7}, {"8 am",8}, {"9 am",9}, {"10 am",10}, {"11 am",11}, {"12 pm",12},
                                           {"1 pm",13}, {"2 pm",14}, {"3 pm",15}, {"4 pm",16}, {"5 pm",17}, {"6 pm",18}, {"7 pm",19}, {"8 pm",20}, {"9 pm",21}, {"10 pm",22}, {"11 pm",23}, {"12 am",0},
                                           {"noon",12}, {"midnight", 0} };

const int maxAttributes = 15;							// maximum number of skills/attitudes
const int maxStudents = maxRecords;                     // each student is a "record" in the genetic algorithm
const int maxTeams = maxStudents/2;
const int maxTimeBlocks = 7*24;                         // resolution of scheduling is 1 hr, and scope is weekly

const int TeamInfoDisplay = Qt::UserRole;               // data with this role is stored in each column of the team info display tree, shown when team is collapsed
const int TeamInfoSort = Qt::UserRole + 1;              // data with this role is stored in each column of the team info display tree, used when sorting the column

// Options for the team names. A name for each list of names must be given.
const QStringList teamnameListNames = {QString("Arabic numbers,"
                                       "Roman numerals,"
                                       "Hexadecimal numbers,"
                                       "English letters,"
                                       "Greek letters,"
                                       "NATO phonetic alphabet,"
                                       "Chemical elements,"
                                       "Shakespeare plays,"
                                       "Olympic host cities")
                                       .split(",")};
const QList<QStringList> teamNameLists{{},
                                       {},
                                       {},
                                       {QString("A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z").split(",")},
                                       {QString("Alpha,Beta,Gamma,Delta,Epsilon,Zeta,Eta,Theta,Iota,Kappa,"
                                                "Lambda,Mu,Nu,Xi,Omicron,Pi,Rho,Sigma,Tau,Upsilon,Phi,Chi,Psi,Omega").split(",")},
                                       {QString("Alfa,Bravo,Charlie,Delta,Echo,Foxtrot,Golf,Hotel,India,Juliett,Kilo,"
                                                "Lima,Mike,November,Oscar,Papa,Quebec,Romeo,Sierra,Tango,Uniform,Victor,Whiskey,X-ray,Yankee,Zulu").split(",")},
                                       {QString("Hydrogen,Helium,Lithium,Beryllium,Boron,Carbon,Nitrogen,Oxygen,Fluorine,Neon,Sodium,Magnesium,"
                                                "Aluminum,Silicon,Phosphorus,Sulfur,Chlorine,Argon,Potassium,Calcium,Scandium,Titanium,Vanadium,"
                                                "Chromium,Manganese,Iron,Cobalt,Nickel,Copper,Zinc,Gallium,Germanium,Arsenic,Selenium,Bromine,Krypton,"
                                                "Rubidium,Strontium,Yttrium,Zirconium,Niobium,Molybdenum,Technetium,Ruthenium,Rhodium,Palladium,Silver,"
                                                "Cadmium,Indium,Tin,Antimony,Tellurium,Iodine,Xenon,Cesium,Barium,Lanthanum,Cerium,Praseodymium,Neodymium,"
                                                "Promethium,Samarium,Europium,Gadolinium,Terbium,Dysprosium,Holmium,Erbium,Thulium,Ytterbium,Lutetium,"
                                                "Hafnium,Tantalum,Tungsten,Rhenium,Osmium,Iridium,Platinum,Gold,Mercury,Thallium,Lead,Bismuth,Polonium,"
                                                "Astatine,Radon,Francium,Radium,Actinium,Thorium,Protactinium,Uranium,Neptunium,Plutonium,Americium,Curium,"
                                                "Berkelium,Californium,Einsteinium,Fermium,Mendelevium,Nobelium,Lawrencium,Rutherfordium,Dubnium,Seaborgium,"
                                                "Bohrium,Hassium,Meitnerium,Darmstadtium,Roentgenium,Copernicium,Nihonium,Flerovium,Moscovium,Livermorium,"
                                                "Tennessine,Oganesson").split(",")},
                                       {QString("Taming of the Shrew,Henry VI,Two Gentlemen of Verona,Titus Andronicus,Richard III,Comedy of Errors,"
                                                "Love's Labour's Lost,Midsummer Night's Dream,Romeo and Juliet,Richard II,King John,Merchant of Venice,"
                                                "Henry IV,Much Ado about Nothing,Henry V,As You Like It,Julius Caesar,Hamlet,Merry Wives of Windsor,"
                                                "Twelfth Night,Troilus and Cressida,Othello,Measure for Measure,All's Well That Ends Well,Timon of Athens,"
                                                "King Lear,Macbeth,Antony and Cleopatra,Coriolanus,Pericles,Cymbeline,Winter's Tale,Tempest,Henry VIII,"
                                                "Two Noble Kinsmen").split(",")},
                                       {QString("Athens,Paris,St Louis,London,Stockholm,Amsterdam,Los Angeles,Berlin,Helsinki,Melbourne,Rome,Tokyo,"
                                                "Mexico City,Munich,Montreal,Moscow,Seoul,Barcelona,Atlanta,Sydney,Beijing,Rio de Janeiro").split(",")}
                                      };

//struct defining survey data from one student
struct studentRecord
{
    int ID;                                             // ID is assigned in order of appearance in the data file
    enum Gender {woman, man, neither} gender = studentRecord::neither;
    bool URM = false;                                   // true if this student is from an underrepresented minority group
    bool unavailable[maxTimeBlocks] = {false};			// true if this is a busy block during week
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    bool preventedWith[maxStudents] = {false};			// true if this student is prevented from working with the corresponding student
    bool requiredWith[maxStudents] = {false};			// true if this student is required to work with the corresponding student
    bool requestedWith[maxStudents] = {false};			// true if this student desires to work with the corresponding student
    int attribute[maxAttributes] = {0};                 // rating for each attribute (each rating is numerical value from 1 -> attributeLevels[attribute])
    QDateTime surveyTimestamp;                          // date/time that the survey was submitted -- see TIMESTAMP_FORMAT definition for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QString section;									// section data stored as text
    QString notes;										// any special notes for this student
    QString attributeResponse[maxAttributes];           // the text of the response to each attribute question
    QString URMResponse;                                // the text of the response the the race/ethnicity/culture question
    QString availabilityChart;
};


//class defining one team
struct teamInfo
{
    float score;
    int size;
    int numWomen;
    int numMen;
    int numNeither;
    int numURM;
    QList<int> attributeVals[maxAttributes];
    int numStudentsAvailable[7][24] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    QList<int> studentIDs;
    QString name;
    QString availabilityChart;
    QString tooltip;
};


//struct defining the options set by what is found in the survey data file
struct DataOptions
{
    bool genderIncluded = false;                        // is gender data included in the survey?
    bool URMIncluded = false;                           // is URM data included in the survey?
    bool sectionIncluded = false;                       // is section data included in the survey?
    bool notesIncluded = false;                         // are notes (or other additional info) included in the survey?
    int numAttributes = 0;                              // how many attribute questions are in the survey?
    int attributeMin[maxAttributes];                    // what is the minimum value for each attribute?
    int attributeMax[maxAttributes];                    // what is the maximum value for each attribute?
    bool attributeIsOrdered[maxAttributes];             // is this attribute ordered (numerical) or purely categorical?
    int numStudentsInSystem = 0;                        // total number of students in the file
    QStringList attributeQuestionText;                  // the actual attribute questions asked of the students
    QStringList attributeQuestionResponses[maxAttributes];      // the list of responses to each of the attribute questions
    QStringList URMResponses;                           // the list of responses to the race/ethnicity/culture question
    QFileInfo dataFile;
    QStringList dayNames;
    QStringList timeNames;
    inline DataOptions(){for(int i = 0; i < maxAttributes; i++) {attributeMin[i] = 1; attributeMax[i] = 1; attributeIsOrdered[i] = true;}}
};


//struct defining the teaming options set by the user
struct TeamingOptions
{
    bool isolatedWomenPrevented = false;                // if true, will prevent teams with an isolated woman
    bool isolatedMenPrevented = false;                  // if true, will prevent teams with an isolated man
    bool mixedGenderPreferred = false;                  // if true, will penalize teams with all men or all women
    bool isolatedURMPrevented = false;                  // if true, will prevent teams with an isolated URM student
    QStringList URMResponsesConsideredUR;               // the list of responses to the race/ethnicity/culture question that are considered underrepresented
    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    int meetingBlockSize = 1;                           // count available meeting times in units of 1 hour or 2 hours long
    bool desireHomogeneous[maxAttributes]; 				// if true/false, tries to make all students on a team have similar/different levels of each attribute
    float attributeWeights[maxAttributes];              // weights for each attribute as displayed to the user (i.e., non-normalized values)
    QList< QPair<int,int> > incompatibleAttributeValues[maxAttributes]; // for each attribute, a list of incompatible attribute value pairs
    float scheduleWeight = 1;
    int numberRequestedTeammatesGiven = 1;
    int smallerTeamsSizes[maxStudents] = {0};
    int smallerTeamsNumTeams = 1;
    int largerTeamsSizes[maxStudents] = {0};
    int largerTeamsNumTeams = 1;
    // initialize all attribute weights to 1, desires to heterogeneous, and no incompatible attribute values
    inline TeamingOptions(){for(int i = 0; i < maxAttributes; i++) {desireHomogeneous[i] = false; attributeWeights[i] = 1;}}
};


#endif // GRUEPR_STRUCTS_AND_CONSTS
