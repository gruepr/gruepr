#ifndef GRUEPR_STRUCTS_AND_CONSTS
#define GRUEPR_STRUCTS_AND_CONSTS

#define GRUEPR_VERSION_NUMBER "8.6"
#define GRUEPR_COPYRIGHT_YEAR "2019"
#define TIMESTAMP_FORMAT1 "yyyy/MM/dd h:mm:ss AP"
#define TIMESTAMP_FORMAT2 "yyyy/MM/dd h:mm:ssAP"
#define USER_REGISTRATION_URL "https://script.google.com/macros/s/AKfycbwqGejEAumqgwpxDdXrV5CJS54gm_0N_du7BweU3wHG-XORT8g/exec"

#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include "GA.h"

//schedule constants, *MUST* be coordinated with the Google Form question that collects this data
const int dailyTimeBlocks = 14;                         // how many blocks of time are in the day?
const int numTimeBlocks = 7*dailyTimeBlocks;			// how many blocks of time are in the week?
const QString dayNames[7]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const QString timeNames[dailyTimeBlocks] = {"8am", "9am", "10am", "11am", "noon", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm", "7pm", "8pm", "9pm"};


const int maxAttributes = 9;							// maximum number of skills/attitudes
const int maxStudents = maxRecords;                     // each student is a "record" in the genetic algorithm


//struct defining survey data from one student
struct studentRecord
{
    int ID;                                             // ID is assigned in order of appearance in the data file
    enum {woman, man, neither} gender;
    bool URM;                                           // true if this student is from an underrepresented minority group
    bool unavailable[numTimeBlocks] = {false};			// true if this is a busy block during week
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
    QString attributeQuestionText[maxAttributes];       // the actual attribute questions asked of the students
    QFileInfo dataFile;
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
    double attributeWeights[maxAttributes]={1,1,1,1,1,1,1,1,1};         // weights for each attribute as displayed to the user
    double scheduleWeight = 1;
    int smallerTeamsSizes[maxStudents]={0};
    int smallerTeamsNumTeams;
    int largerTeamsSizes[maxStudents]={0};
    int largerTeamsNumTeams;
};


#endif // GRUEPR_STRUCTS_AND_CONSTS
