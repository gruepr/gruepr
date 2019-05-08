#ifndef GRUEPR_STRUCTS_AND_CONSTS
#define GRUEPR_STRUCTS_AND_CONSTS

#define GRUEPR_VERSION_NUMBER "8.9"
#define GRUEPR_COPYRIGHT_YEAR "2019"
#define TIMESTAMP_FORMAT1 "yyyy/MM/dd h:mm:ss AP"
#define TIMESTAMP_FORMAT2 "yyyy/MM/dd h:mm:ssAP"
#define USER_REGISTRATION_URL "https://script.google.com/macros/s/AKfycbwqGejEAumqgwpxDdXrV5CJS54gm_0N_du7BweU3wHG-XORT8g/exec"

#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QMap>
#include "GA.h"

//map of the "meaning" of strings that might be used in the Google Form to refer to hours of the day
const QMap<QString, int> meaningOfTimeNames{ {"1am",1}, {"2am",2}, {"3am",3}, {"4am",4}, {"5am",5}, {"6am",6}, {"7am",7}, {"8am",8}, {"9am",9}, {"10am",10}, {"11am",11}, {"12pm",12},
                                           {"1pm",13}, {"2pm",14}, {"3pm",15}, {"4pm",16}, {"5pm",17}, {"6pm",18}, {"7pm",19}, {"8pm",20}, {"9pm",21}, {"10pm",22}, {"11pm",23}, {"12am",0},
                                           {"1:00",1}, {"2:00",2}, {"3:00",3}, {"4:00",4}, {"5:00",5}, {"6:00",6}, {"7:00",7}, {"8:00",8}, {"9:00",9}, {"10:00",10}, {"11:00",11}, {"12:00",12},
                                           {"13:00",13}, {"14:00",14}, {"15:00",15}, {"16:00",16}, {"17:00",17}, {"18:00",18}, {"19:00",19}, {"20:00",20}, {"21:00",21}, {"22:00",22}, {"23:00",23}, {"0:00",0},
                                           {"1 am",1}, {"2 am",2}, {"3 am",3}, {"4 am",4}, {"5 am",5}, {"6 am",6}, {"7 am",7}, {"8 am",8}, {"9 am",9}, {"10 am",10}, {"11 am",11}, {"12 pm",12},
                                           {"1 pm",13}, {"2 pm",14}, {"3 pm",15}, {"4 pm",16}, {"5 pm",17}, {"6 pm",18}, {"7 pm",19}, {"8 pm",20}, {"9 pm",21}, {"10 pm",22}, {"11 pm",23}, {"12 am",0},
                                           {"noon",12}, {"midnight", 0} };

const int maxAttributes = 9;							// maximum number of skills/attitudes
const int maxStudents = maxRecords;                     // each student is a "record" in the genetic algorithm
const int maxTeams = maxStudents/2;
const int maxTimeBlocks = 7*24;                         // resolution of scheduling is 1 hr, and scope is weekly


//struct defining survey data from one student
struct studentRecord
{
    int ID;                                             // ID is assigned in order of appearance in the data file
    enum {woman, man, neither} gender;
    bool URM;                                           // true if this student is from an underrepresented minority group
    bool unavailable[maxTimeBlocks] = {false};			// true if this is a busy block during week
    bool preventedWith[maxStudents] = {false};			// true if this student is prevented from working with the corresponding student
    bool requiredWith[maxStudents] = {false};			// true if this student is required to work with the corresponding student
    int attribute[maxAttributes] = {0};                 // rating for each attribute (each rating is numerical value from 1 -> attributeLevels[attribute])
    QDateTime surveyTimestamp;                          // date/time that the survey was submitted -- see TIMESTAMP_FORMAT definition for intepretation of timestamp in survey file
    QString firstname;
    QString lastname;
    QString email;
    QString section;									// section data stored as text
    QString notes;										// any special notes for this student
    QString availabilityChart;
};


//struct defining the options set by what is found in the survey data file
struct DataOptions
{
    bool genderIncluded;                                // is gender data included in the survey?
    bool URMIncluded;                                   // is URM data included in the survey?
    bool sectionIncluded;                               // is section data included in the survey?
    bool notesIncluded;                                 // are notes (or other additional info) included in the survey?
    int numAttributes;                                  // how many attribute questions are in the survey?
    int attributeLevels[maxAttributes]={0};             // what is the maximum value for each attribute? Max possible value is 9, due to how the value is read from the file.
    int numStudentsInSystem;                            // total number of students in the file
    QStringList attributeQuestionText;                  // the actual attribute questions asked of the students
    QFileInfo dataFile;
    QStringList dayNames;
    QStringList timeNames;
};


//struct defining the teaming options set by the user
struct TeamingOptions
{
    bool isolatedWomenPrevented;                        // if true, will prevent teams with an isolated woman
    bool isolatedMenPrevented;                          // if true, will prevent teams with an isolated man
    bool mixedGenderPreferred;                          // if true, will penalize teams with all men or all women
    bool isolatedURMPrevented;                          // if true, will prevent teams with an isolated URM student
    int desiredTimeBlocksOverlap=8;                     // want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
    int minTimeBlocksOverlap=4;                         // a team is penalized if there are fewer than this many time blocks that overlap
    int meetingBlockSize=1;                             // count available meeting times in units of 1 hour or 2 hours long
    bool desireHomogeneous[maxAttributes]; 				// if true/false, tries to make all students on a team have similar/different levels of each attribute
    float attributeWeights[maxAttributes]={1,1,1,1,1,1,1,1,1};         // weights for each attribute as displayed to the user
    float scheduleWeight = 1;
    int smallerTeamsSizes[maxStudents]={0};
    int smallerTeamsNumTeams;
    int largerTeamsSizes[maxStudents]={0};
    int largerTeamsNumTeams;
};


#endif // GRUEPR_STRUCTS_AND_CONSTS
