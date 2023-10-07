#include "dataOptions.h"
#include <QJsonArray>
#include <QRegularExpression>

DataOptions::DataOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        attributeField[i] = -1;
        attributeVals[i].clear();
        attributeQuestionResponseCounts[i].clear();
        attributeType[i] = AttributeType::categorical;
    }

    for(int &field : notesField)
    {
        field = -1;
    }

    for(int &field : prefTeammatesField)
    {
        field = -1;
    }

    for(int &field : prefNonTeammatesField)
    {
        field = -1;
    }

    for(int &field : scheduleField)
    {
        field = -1;
    }
}

DataOptions::DataOptions(const QJsonObject &jsonDataOptions)
{
    timestampField = jsonDataOptions["timestampField"].toInt();
    LMSIDField = jsonDataOptions["LMSIDField"].toInt();
    emailField = jsonDataOptions["emailField"].toInt();
    firstNameField = jsonDataOptions["firstNameField"].toInt();
    lastNameField = jsonDataOptions["lastNameField"].toInt();
    genderIncluded = jsonDataOptions["genderIncluded"].toBool();
    genderType = static_cast<GenderType>(jsonDataOptions["genderType"].toInt());
    genderField = jsonDataOptions["genderField"].toInt();
    URMIncluded = jsonDataOptions["URMIncluded"].toBool();
    URMField = jsonDataOptions["URMField"].toInt();
    sectionIncluded = jsonDataOptions["sectionIncluded"].toBool();
    sectionField = jsonDataOptions["sectionField"].toInt();
    numNotes = jsonDataOptions["numNotes"].toInt();
    scheduleDataIsFreetime = jsonDataOptions["scheduleDataIsFreetime"].toBool();
    numAttributes = jsonDataOptions["numAttributes"].toInt();
    timezoneIncluded = jsonDataOptions["timezoneIncluded"].toBool();
    timezoneField = jsonDataOptions["timezoneField"].toInt();
    homeTimezoneUsed = jsonDataOptions["homeTimezoneUsed"].toBool();
    baseTimezone = jsonDataOptions["baseTimezone"].toDouble();
    earlyTimeAsked = jsonDataOptions["earlyTimeAsked"].toDouble();
    lateTimeAsked = jsonDataOptions["lateTimeAsked"].toDouble();
    prefTeammatesIncluded = jsonDataOptions["prefTeammatesIncluded"].toBool();
    numPrefTeammateQuestions = jsonDataOptions["numPrefTeammateQuestions"].toInt();
    prefNonTeammatesIncluded = jsonDataOptions["prefNonTeammatesIncluded"].toBool();
    numPrefNonTeammateQuestions = jsonDataOptions["numPrefNonTeammateQuestions"].toInt();
    numStudentsInSystem = jsonDataOptions["numStudentsInSystem"].toInt();
    dataSourceName = jsonDataOptions["dataSourceName"].toString();
    dataSource = static_cast<DataSource>(jsonDataOptions["dataSource"].toInt());
    saveStateFileName = jsonDataOptions["saveStateFileName"].toString();

    QJsonArray notesFieldArray = jsonDataOptions["notesField"].toArray();
    int i = 0;
    for(const auto &item : notesFieldArray) {
        notesField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_NOTES_FIELDS; j++) {
        notesField[i] = -1;
    }

    QJsonArray scheduleFieldArray = jsonDataOptions["scheduleField"].toArray();
    i = 0;
    for(const auto &item : scheduleFieldArray) {
        scheduleField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_DAYS; j++) {
        scheduleField[i] = -1;
    }

    QJsonArray attributeFieldArray = jsonDataOptions["attributeField"].toArray();
    i = 0;
    for(const auto &item : attributeFieldArray) {
        attributeField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeField[j] = -1;
    }

    QJsonArray attributeTypeArray = jsonDataOptions["attributeType"].toArray();
    i = 0;
    for(const auto &item : attributeTypeArray) {
        attributeType[i] = static_cast<AttributeType>(item.toInt());
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeType[j] = AttributeType::categorical;
    }

    QJsonArray prefTeammatesFieldArray = jsonDataOptions["prefTeammatesField"].toArray();
    i = 0;
    for(const auto &item : prefTeammatesFieldArray) {
        prefTeammatesField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_PREFTEAMMATES; j++) {
        prefTeammatesField[j] = -1;
    }

    QJsonArray prefNonTeammatesFieldArray = jsonDataOptions["prefNonTeammatesField"].toArray();
    i = 0;
    for(const auto &item : prefNonTeammatesFieldArray) {
        prefNonTeammatesField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_PREFTEAMMATES; j++) {
        prefNonTeammatesField[j] = -1;
    }

    QJsonArray sectionNamesArray = jsonDataOptions["sectionNames"].toArray();
    sectionNames.reserve(sectionNamesArray.size());
    for(const auto &item : sectionNamesArray) {
        sectionNames << item.toString();
    }

    QJsonArray attributeQuestionTextArray = jsonDataOptions["attributeQuestionText"].toArray();
    attributeQuestionText.reserve(attributeQuestionTextArray.size());
    for(const auto &item : attributeQuestionTextArray) {
        attributeQuestionText << item.toString();
    }

    QJsonArray attributeQuestionResponsesArray = jsonDataOptions["attributeQuestionResponses"].toArray();
    i = 0;
    for(const auto &item : attributeQuestionResponsesArray) {
        QJsonArray attributeQuestionResponsesArraySubarray = item.toArray();
        attributeQuestionResponses[i].reserve(attributeQuestionResponsesArraySubarray.size());
        for(const auto &response : attributeQuestionResponsesArraySubarray) {
            attributeQuestionResponses[i] << response.toString();
        }
        i++;
    }

    QJsonArray attributeQuestionResponseCountsArray = jsonDataOptions["attributeQuestionResponseCounts"].toArray();
    i = 0;
    for(const auto &item : attributeQuestionResponseCountsArray) {
        attributeQuestionResponseCounts[i].clear();
        QVariantMap attributeQuestionResponseCountsArraySubobject = item.toObject().toVariantMap();
        for(const auto &key : attributeQuestionResponseCountsArraySubobject.keys()) {
            attributeQuestionResponseCounts[i].emplace(key, attributeQuestionResponseCountsArraySubobject.value(key).toInt());
        }
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeQuestionResponseCounts[j].clear();
    }

    QJsonArray attributeValsArray = jsonDataOptions["attributeVals"].toArray();
    i = 0;
    for(const auto &item : attributeValsArray) {
        attributeVals[i].clear();
        QJsonArray attributeValsArraySubarray = item.toArray();
        for(const auto &val : attributeValsArraySubarray) {
            attributeVals[i].insert(val.toInt());
        }
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeVals[j].clear();
    }

    QJsonArray URMResponsesArray = jsonDataOptions["URMResponses"].toArray();
    URMResponses.reserve(URMResponsesArray.size());
    for(const auto &item : URMResponsesArray) {
        URMResponses << item.toString();
    }

    QJsonArray dayNamesArray = jsonDataOptions["dayNames"].toArray();
    dayNames.reserve(dayNamesArray.size());
    for(const auto &item : dayNamesArray) {
        dayNames << item.toString();
    }

    QJsonArray timeNamesArray = jsonDataOptions["timeNames"].toArray();
    timeNames.reserve(timeNamesArray.size());
    for(const auto &item : timeNamesArray) {
        timeNames << item.toString();
    }
}


bool DataOptions::parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &hours, float &minutes, float &offsetFromGMT)
{
    static QRegularExpression timeZoneFinder(TIMEZONEREGEX);
    QRegularExpressionMatch timezoneCapture = timeZoneFinder.match(fullText);
    if(timezoneCapture.hasMatch())
    {
        timezoneName = timezoneCapture.captured(1).trimmed();
        hours = timezoneCapture.captured(2).toFloat();
        minutes = timezoneCapture.captured(3).toFloat();
        offsetFromGMT = hours + ((hours < 0)? (-minutes/60) : (minutes/60));
        return true;
    }
    return false;
}


bool DataOptions::parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &offsetFromGMT)
{
    float hours, minutes;
    return parseTimezoneInfoFromText(fullText, timezoneName, hours, minutes, offsetFromGMT);
}


QJsonObject DataOptions::toJson() const
{
    QJsonArray notesFieldArray, scheduleFieldArray, attributeFieldArray, attributeTypeArray, prefTeammatesFieldArray, prefNonTeammatesFieldArray,
        attributeQuestionResponsesArray, attributeQuestionResponseCountsArray, attributeValsArray;

    for(int i = 0; i < MAX_NOTES_FIELDS; i++) {
        notesFieldArray.append(notesField[i]);
    }
    for(int i = 0; i < MAX_DAYS; i++) {
        scheduleFieldArray.append(scheduleField[i]);
    }
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeFieldArray.append(attributeField[i]);
        attributeTypeArray.append(static_cast<int>(attributeType[i]));
        attributeQuestionResponsesArray.append(QJsonArray::fromStringList(attributeQuestionResponses[i]));
        QJsonObject attributeQuestionResponseCountsObj;
        for(const auto &item : attributeQuestionResponseCounts[i])
        {
            attributeQuestionResponseCountsObj.insert(item.first, item.second);
        }
        attributeQuestionResponseCountsArray.append(attributeQuestionResponseCountsObj);
        QJsonArray attributeValsArraySubArray;
        for (const auto &val : attributeVals[i]) {
            attributeValsArraySubArray.append(val);
        }
        attributeValsArray.append(attributeValsArraySubArray);
    }
    for(int i = 0; i < MAX_PREFTEAMMATES; i++) {
        prefTeammatesFieldArray.append(prefTeammatesField[i]);
        prefNonTeammatesFieldArray.append(prefNonTeammatesField[i]);
    }

    QJsonObject content {
        {"timestampField", timestampField},
        {"LMSIDField", LMSIDField},
        {"emailField", emailField},
        {"firstNameField", firstNameField},
        {"lastNameField", lastNameField},
        {"genderIncluded", genderIncluded},
        {"genderType", static_cast<int>(genderType)},
        {"genderField", genderField},
        {"URMIncluded", URMIncluded},
        {"URMField", URMField},
        {"sectionIncluded", sectionIncluded},
        {"sectionField", sectionField},
        {"notesField", notesFieldArray},
        {"numNotes", numNotes},
        {"scheduleDataIsFreetime", scheduleDataIsFreetime},
        {"scheduleField", scheduleFieldArray},
        {"numAttributes", numAttributes},
        {"attributeField", attributeFieldArray},
        {"timezoneIncluded", timezoneIncluded},
        {"timezoneField", timezoneField},
        {"homeTimezoneUsed", homeTimezoneUsed},
        {"baseTimezone", baseTimezone},
        {"earlyTimeAsked", earlyTimeAsked},
        {"lateTimeAsked", lateTimeAsked},
        {"attributeType", attributeTypeArray},
        {"prefTeammatesIncluded", prefTeammatesIncluded},
        {"numPrefTeammateQuestions", numPrefTeammateQuestions},
        {"prefTeammatesField", prefTeammatesFieldArray},
        {"prefNonTeammatesIncluded", prefNonTeammatesIncluded},
        {"numPrefNonTeammateQuestions", numPrefNonTeammateQuestions},
        {"prefNonTeammatesField", prefNonTeammatesFieldArray},
        {"numStudentsInSystem", numStudentsInSystem},
        {"sectionNames", QJsonArray::fromStringList(sectionNames)},
        {"attributeQuestionText", QJsonArray::fromStringList(attributeQuestionText)},
        {"attributeQuestionResponses", attributeQuestionResponsesArray},
        {"attributeQuestionResponseCounts", attributeQuestionResponseCountsArray},
        {"attributeVals", attributeValsArray},
        {"URMResponses", QJsonArray::fromStringList(URMResponses)},
        {"dataSourceName", dataSourceName},
        {"dataSource", static_cast<int>(dataSource)},
        {"dayNames", QJsonArray::fromStringList(dayNames)},
        {"timeNames", QJsonArray::fromStringList(timeNames)},
        {"saveStateFileName", saveStateFileName}
    };

    return content;
}
