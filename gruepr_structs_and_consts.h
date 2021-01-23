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
#include <QVector>
#include <set>
#include "GA.h"


const int MAX_ATTRIBUTES = 15;                          // maximum number of skills/attitudes
const int MAX_STUDENTS = MAX_RECORDS;                    // each student is a "record" in the genetic algorithm
const int MAX_TEAMS = MAX_STUDENTS/2;
const int MAX_TIMEBLOCKS = 7*24;                        // resolution of scheduling is 1 hr, and scope is weekly

const int TEAMINFO_DISPLAY_ROLE = Qt::UserRole;         // data with this role is stored in each column of the team info display tree, shown when team is collapsed
const int TEAMINFO_SORT_ROLE = Qt::UserRole + 1;        // data with this role is stored in each column of the team info display tree, used when sorting the column
const int TEAM_NUMBER_ROLE = Qt::UserRole + 2;          // data with this role is stored in column 0 of the team info display tree, used when swapping teams or teammates

//map of the "meaning" of strings that might be used in the Google Form to refer to hours of the day
const char TIME_NAMES[] {"1am,1 am,1:00,2am,2 am,2:00,3am,3 am,3:00,4am,4 am,4:00,5am,5 am,5:00,6am,6 am,6:00,7am,7 am,7:00,8am,8 am,8:00,9am,9 am,9:00,10am,10 am,10:00,"
                        "11am,11 am,11:00,12pm,12 pm,12:00,1pm,1 pm,13:00,2pm,2 pm,14:00,3pm,3 pm,15:00,4pm,4 pm,16:00,5pm,5 pm,17:00,6pm,6 pm,18:00,7pm,7 pm,19:00,8pm,8 pm,20:00,"
                        "9pm,9 pm,21:00,10pm,10 pm,22:00,11pm,11 pm,23:00,12am,12 am,0:00,noon,midnight"};
const int TIME_MEANINGS[] {1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,12,
                          13,13,13,14,14,14,15,15,15,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,22,23,23,23,0,0,0,12,0};

// Options for the team names. A name for each list of names must be given.
const char TEAMNAMECATEGORIES[] {"Arabic numbers,"
                            "Roman numerals,"
                            "Hexadecimal numbers,"
                            "Binary numbers,"
                            "English letters,"
                            "Greek letters (uppercase),"
                            "Greek letters (lowercase),"
                            "NATO phonetic alphabet,"
                            "Chemical elements,"
                            "Constellations,"
                            "Popes,"
                            "Genres of music,"
                            "Crayola crayon colors,"
                            "Shakespeare plays (RSC chron.),"
                            "Discontinued Olympic sports,"
                            "Cheeses,"
                            "Minor Simpsons characters"};

const char TEAMNAMELISTS[]   {";"
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
                              "Andromeda,Bootes,Cassiopeia,Draco,Equuleus,Fornax,Gemini,Hydra,Indus,Leo,Musca,Norma,Orion,Perseus,Reticulum,"
                                 "Sagittarius,Taurus,Ursa Major,Virgo;"
                              "Adrian,Benedict,Clement,Damasus,Eugene,Felix,Gregory,Hilarius,Innocent,John,Leo,Martin,Nicholas,Pius,Romanus,"
                                 "Stephen,Theodore,Urban,Victor,Zosimus;"
                              "Acapella,Blues,Country,Doo-Wop,EDM,Fado,Gospel,Hip-Hop,Indie,Jazz,K-Pop,Lullaby,Mariachi,New Age,Opera,Punk,"
                                 "Qawwali,Reggae,Soundtrack,Tejano,Underground,Vocal,Western Swing,Xhosa,Yodeling,Zydeco;"
                              "Aquamarine,Burnt Sienna,Chartreuse,Dandelion,Emerald,Fuchsia,Goldenrod,Hot Magenta,Indigo,Jungle Green,Laser Lemon,Mulberry,"
                                 "Neon Carrot,Orchid,Periwinkle,Razzmatazz,Scarlet,Thistle,Ultra Orange,Vivid Tangerine,Wisteria,Yosemite Campfire,Zircon;"
                              "Taming of the Shrew,Henry VI,Two Gentlemen of Verona,Titus Andronicus,Richard III,Comedy of Errors,"
                                 "Love's Labour's Lost,Midsummer Night's Dream,Romeo and Juliet,Richard II,King John,Merchant of Venice,"
                                 "Henry IV,Much Ado about Nothing,Henry V,As You Like It,Julius Caesar,Hamlet,Merry Wives of Windsor,"
                                 "Twelfth Night,Troilus and Cressida,Othello,Measure for Measure,All's Well That Ends Well,Timon of Athens,"
                                 "King Lear,Macbeth,Antony and Cleopatra,Coriolanus,Pericles,Cymbeline,Winter's Tale,Tempest,Henry VIII,"
                                 "Two Noble Kinsmen;"
                              "Angling,Bowling,Cannon Shooting,Dog Sledding,Engraving,Fire Fighting,Gliding,Hurling,India Club Swinging,Jeu de Paume,"
                                 "Korfball,Lacrosse,Motorcycle Racing,Orchestra,Pigeon Racing,Roller Hockey,Savate,Tug of War,Vaulting,Waterskiing;"
                              "Asiago,Brie,Cheddar,Derby,Edam,Feta,Gouda,Havarti,Iberico,Jarlsberg,Kaseri,Limburger,Manchego,Neufchatel,Orla,"
                                 "Paneer,Queso Fresco,Ricotta,Siraz,Tyn Grug,Ulloa,Vignotte,Weichkaese,Xynotyro,Yorkshire Blue,Zamorano;"
                              "Artie Ziff,Brunella Pommelhorst,Cleetus,Disco Stu,Edna Krabappel,Frank 'Grimy' Grimes,Ginger Flanders,"
                                 "Helen Lovejoy,Itchy,Jebediah Springfield,Kent Brockman,Luann Van Houten,Mayor Quimby,Ned Flanders,Professor Frink,"
                                 "Queen Helvetica,Ruth Powers,Sideshow Bob,Troy McClure,Uter Zorker,Waylon Smithers,Xoxchitla,Yes Guy,Zelda"};

//struct defining survey data from one student
struct StudentRecord
{
    int ID;                                             // ID is assigned in order of appearance in the data file
    enum Gender {woman, man, nonbinary, unknown} gender = StudentRecord::unknown;
    bool URM = false;                                   // true if this student is from an underrepresented minority group
    bool unavailable[MAX_TIMEBLOCKS] = {false};			// true if this is a busy block during week
    bool ambiguousSchedule = false;                     // true if added schedule is completely full or completely empty;
    bool preventedWith[MAX_STUDENTS] = {false};			// true if this student is prevented from working with the corresponding student
    bool requiredWith[MAX_STUDENTS] = {false};			// true if this student is required to work with the corresponding student
    bool requestedWith[MAX_STUDENTS] = {false};			// true if this student desires to work with the corresponding student
    int attribute[MAX_ATTRIBUTES] = {0};                 // rating for each attribute (each rating is numerical value from 1 -> attributeLevels[attribute])
    QDateTime surveyTimestamp;                          // date/time that the survey was submitted -- see TIMESTAMP_FORMAT definition for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QString section;									// section data stored as text
    QString notes;										// any special notes for this student
    QString attributeResponse[MAX_ATTRIBUTES];           // the text of the response to each attribute question
    QString URMResponse;                                // the text of the response the the race/ethnicity/culture question
    QString availabilityChart;
};


//class defining one team
struct TeamInfo
{
    float score;
    int size;
    int numWomen;
    int numMen;
    int numNonbinary;
    int numUnknown;
    int numURM;
    std::set<int> attributeVals[MAX_ATTRIBUTES];
    int numStudentsAvailable[7][24] = {{0}};
    int numStudentsWithAmbiguousSchedules = 0;
    QVector<int> studentIDs;
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
    bool scheduleDataIsFreetime = false;                // was the survey set up so that students are indicating their freetime in the schedule?
    int numAttributes = 0;                              // how many attribute questions are in the survey?
    int attributeMin[MAX_ATTRIBUTES];                   // what is the minimum value for each attribute?
    int attributeMax[MAX_ATTRIBUTES];                   // what is the maximum value for each attribute?
    bool attributeIsOrdered[MAX_ATTRIBUTES];            // is this attribute ordered (numerical) or purely categorical?
    int numStudentsInSystem = 0;                        // total number of students in the file
    QStringList attributeQuestionText;                  // the actual attribute questions asked of the students
    QStringList attributeQuestionResponses[MAX_ATTRIBUTES];      // the list of responses to each of the attribute questions
    QStringList URMResponses;                           // the list of responses to the race/ethnicity/culture question
    QFileInfo dataFile;
    QStringList dayNames;
    QStringList timeNames;

    inline DataOptions(){for(int i = 0; i < MAX_ATTRIBUTES; i++) {attributeMin[i] = 1;
                                                                  attributeMax[i] = 1;
                                                                  attributeIsOrdered[i] = false;}}
};


//struct defining the teaming options set by the user
struct TeamingOptions
{
    bool isolatedWomenPrevented = false;                // if true, will prevent teams with an isolated woman
    bool isolatedMenPrevented = false;                  // if true, will prevent teams with an isolated man
    bool isolatedNonbinaryPrevented = false;            // if true, will prevent teams with an isolated nonbinary student
    bool singleGenderPrevented = false;                 // if true, will penalize teams with all men or all women
    bool isolatedURMPrevented = false;                  // if true, will prevent teams with an isolated URM student
    QStringList URMResponsesConsideredUR;               // the list of responses to the race/ethnicity/culture question that are considered underrepresented
    int desiredTimeBlocksOverlap = 8;                   // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap = 4;                       // a team is penalized if there are fewer than this many time blocks that overlap
    int meetingBlockSize = 1;                           // count available meeting times in units of 1 hour or 2 hours long
    bool desireHomogeneous[MAX_ATTRIBUTES]; 			// if true/false, tries to make all students on a team have similar/different levels of each attribute
    float attributeWeights[MAX_ATTRIBUTES];             // weights for each attribute as displayed to the user (i.e., non-normalized values)
    float realAttributeWeights[MAX_ATTRIBUTES];         // scoring weight of each attribute, normalized to total weight
    bool haveAnyIncompatibleAttributes[MAX_ATTRIBUTES];
    QVector< QPair<int,int> > incompatibleAttributeValues[MAX_ATTRIBUTES]; // for each attribute, a list of incompatible attribute value pairs
    float scheduleWeight = 1;
    float realScheduleWeight = 1;                       // scoring weight of the schedule, normalized to total weight
    int realNumScoringFactors = 1;                      // the total weight of all scoring factors, equal to the number of attributes + 1 for schedule if that is used
    bool haveAnyRequiredTeammates = false;
    bool haveAnyPreventedTeammates = false;
    bool haveAnyRequestedTeammates = false;
    int numberRequestedTeammatesGiven = 1;
    int smallerTeamsSizes[MAX_STUDENTS] = {0};
    int smallerTeamsNumTeams = 1;
    int largerTeamsSizes[MAX_STUDENTS] = {0};
    int largerTeamsNumTeams = 1;
    int numTeamsDesired = 1;
    int teamSizesDesired[MAX_STUDENTS] = {0};

    // initialize all attribute weights to 1, desires to heterogeneous, and no incompatible attribute values
    inline TeamingOptions(){for(int i = 0; i < MAX_ATTRIBUTES; i++) {desireHomogeneous[i] = false;
                                                                     attributeWeights[i] = 1;
                                                                     realAttributeWeights[i] = 1;
                                                                     haveAnyIncompatibleAttributes[i] = false;
                                                                     incompatibleAttributeValues[i].clear();}}

    // reset the variables that depend on the datafile
    inline void reset(){URMResponsesConsideredUR.clear();
                        for(int i = 0; i < MAX_ATTRIBUTES; i++) {haveAnyIncompatibleAttributes[i] = false;
                                                                 incompatibleAttributeValues[i].clear();}
                        haveAnyRequiredTeammates = false;
                        haveAnyPreventedTeammates = false;
                        haveAnyRequestedTeammates = false;
                        }
};


#endif // GRUEPR_STRUCTS_AND_CONSTS
