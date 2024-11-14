#ifndef DATAOPTIONS_H
#define DATAOPTIONS_H

#include "gruepr_globals.h"
#include <QFileInfo>
#include <QJsonObject>
#include <QStringList>
#include <map>
#include <set>

/**
 * @brief The DataOptions class defines the options set by what is found in the survey data file.
 */
class DataOptions
{
public:
    DataOptions();
    explicit DataOptions(const QJsonObject &jsonDataOptions);

    QJsonObject toJson() const;

    static bool parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &hours, float &minutes, float &offsetFromGMT);
    static bool parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &offsetFromGMT);

    inline static const int FIELDNOTPRESENT = -1;
    inline static const int DATAFROMOTHERSOURCE = 101;

    int timestampField = FIELDNOTPRESENT;
    int LMSIDField = FIELDNOTPRESENT;
    int emailField = FIELDNOTPRESENT;
    int firstNameField = FIELDNOTPRESENT;
    int lastNameField = FIELDNOTPRESENT;
    bool genderIncluded = false;                    // is gender data included in the survey?
    GenderType genderType = GenderType::adult;
    int genderField = FIELDNOTPRESENT;              // which field in surveyFile has the gender info?
    bool URMIncluded = false;                       // is URM data included in the survey?
    int URMField = FIELDNOTPRESENT;                 // which field in surveyFile has the ethnicity info?
    bool sectionIncluded = false;                   // is section data included in the survey?
    int sectionField = FIELDNOTPRESENT;             // which field in surveyFile has the section info?
    int notesField[MAX_NOTES_FIELDS];               // which field(s) in surveyFile has additional notes?
    int numNotes = 0;                               // how many notes (or other additional info) included in the survey?
    bool scheduleDataIsFreetime = false;            // was the survey set up so that students are indicating their freetime in the schedule?
    int scheduleField[MAX_DAYS];                    // which field(s) in surveyFile have schedule info?
    int numAttributes = 0;                          // how many attribute questions are in the survey?
    int attributeField[MAX_ATTRIBUTES];             // which field(s) in surveyFile have attribute info?
    bool timezoneIncluded = false;                  // is timezone data included in the survey?
    int timezoneField = FIELDNOTPRESENT;            // which field has the timezone info?
    bool homeTimezoneUsed = false;                  // whether the students' schedules refer to their own timezone
    float baseTimezone = 0;                         // offset from GMT for baseline timezone
    float earlyTimeAsked = 0;                       // earliest time asked in survey (in hours since midnight)
    float lateTimeAsked = 24;                       // latest time asked in survey (in hours since midnight)
    float scheduleResolution = 1;                   // how finely resolved the schedule is (in hours)
    enum class AttributeType {ordered, timezone, categorical, multicategorical, multiordered} attributeType[MAX_ATTRIBUTES];    // is each attribute ordered (numerical), timezone, or categorical? Are multiple values allowed?
    bool prefTeammatesIncluded = false;             // did students get to include preferred teammates?
    int numPrefTeammateQuestions = 0;
    int prefTeammatesField[MAX_PREFTEAMMATES];      // which field(s) in surveyFile has the preferred teammates info?
    bool prefNonTeammatesIncluded = false;          // did students get to include preferred non-teammates?
    int numPrefNonTeammateQuestions = 0;
    int prefNonTeammatesField[MAX_PREFTEAMMATES];   // which field(s) in surveyFile has the preferred non-teammates info?
    QStringList sectionNames;                       // all of the section names
    QStringList attributeQuestionText;              // the actual attribute questions asked of the students
    QStringList attributeQuestionResponses[MAX_ATTRIBUTES];         // the list of responses to each of the attribute questions
    std::map<QString, int> attributeQuestionResponseCounts[MAX_ATTRIBUTES];  // a count of how many students gave each response
    std::set<int> attributeVals[MAX_ATTRIBUTES];    // what values can each attribute have? There is a value corresponding to each attributeQuestionResponse; they are indexed at 1 but -1 represents "unknown"
    QStringList URMResponses;                       // the list of responses to the race/ethnicity/culture question
    QString dataSourceName;
    enum class DataSource{fromFile, fromGoogle, fromCanvas, fromPrevWork} dataSource;
    QStringList dayNames;
    QStringList timeNames;
    QString saveStateFileName;
};

#endif // DATAOPTIONS_H
