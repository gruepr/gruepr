#include "dataOptions.h"
#include <QJsonArray>
#include <QRegularExpression>

DataOptions::DataOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeField[i] = FIELDNOTPRESENT;
        attributeVals[i].clear();
        attributeQuestionResponseCounts[i].clear();
        attributeType[i] = AttributeType::categorical;
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
    scheduleDataIsFreetime = jsonDataOptions["scheduleDataIsFreetime"].toBool();
    earlyTimeAsked = jsonDataOptions["earlyTimeAsked"].toDouble();
    lateTimeAsked = jsonDataOptions["lateTimeAsked"].toDouble();
    scheduleResolution = jsonDataOptions["scheduleResolution"].toDouble();
    numAttributes = jsonDataOptions["numAttributes"].toInt();
    timezoneIncluded = jsonDataOptions["timezoneIncluded"].toBool();
    timezoneField = jsonDataOptions["timezoneField"].toInt();
    homeTimezoneUsed = jsonDataOptions["homeTimezoneUsed"].toBool();
    baseTimezone = jsonDataOptions["baseTimezone"].toDouble();
    dataSourceName = jsonDataOptions["dataSourceName"].toString();
    dataSource = static_cast<DataSource>(jsonDataOptions["dataSource"].toInt());
    saveStateFileName = jsonDataOptions["saveStateFileName"].toString();
    gradeIncluded = jsonDataOptions["gradeIncluded"].toBool();
    gradeField = jsonDataOptions["gradeField"].toInt();

    const QJsonArray notesFieldArray = jsonDataOptions["notesField"].toArray();
    notesFields.reserve(notesFieldArray.size());
    for(const auto &item : notesFieldArray) {
        const int fieldNum = item.toInt();
        if(fieldNum != FIELDNOTPRESENT) {
            notesFields << fieldNum;
        }
    }

    const QJsonArray scheduleFieldArray = jsonDataOptions["scheduleField"].toArray();
    scheduleField.reserve(scheduleFieldArray.size());
    for(const auto &item : scheduleFieldArray) {
        const int fieldNum = item.toInt();
        if(fieldNum != FIELDNOTPRESENT) {
            scheduleField << fieldNum;
        }
    }

    const QJsonArray attributeFieldArray = jsonDataOptions["attributeField"].toArray();
    int i = 0;
    for(const auto &item : attributeFieldArray) {
        attributeField[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeField[j] = FIELDNOTPRESENT;
    }

    const QJsonArray attributeTypeArray = jsonDataOptions["attributeType"].toArray();
    i = 0;
    for(const auto &item : attributeTypeArray) {
        attributeType[i] = static_cast<AttributeType>(item.toInt());
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeType[j] = AttributeType::categorical;
    }

    const QJsonArray prefTeammatesFieldArray = jsonDataOptions["prefTeammatesField"].toArray();
    prefTeammatesField.reserve(prefTeammatesFieldArray.size());
    for(const auto &item : prefTeammatesFieldArray) {
        const int fieldNum = item.toInt();
        if(fieldNum != FIELDNOTPRESENT) {
            prefTeammatesField << fieldNum;
        }
    }

    const QJsonArray prefNonTeammatesFieldArray = jsonDataOptions["prefNonTeammatesField"].toArray();
    prefNonTeammatesField.reserve(prefNonTeammatesFieldArray.size());
    for(const auto &item : prefNonTeammatesFieldArray) {
        const int fieldNum = item.toInt();
        if(fieldNum != FIELDNOTPRESENT) {
            prefNonTeammatesField << fieldNum;
        }
    }

    const QJsonArray sectionNamesArray = jsonDataOptions["sectionNames"].toArray();
    sectionNames.reserve(sectionNamesArray.size());
    for(const auto &item : sectionNamesArray) {
        sectionNames << item.toString();
    }

    const QJsonArray attributeQuestionTextArray = jsonDataOptions["attributeQuestionText"].toArray();
    attributeQuestionText.reserve(attributeQuestionTextArray.size());
    for(const auto &item : attributeQuestionTextArray) {
        attributeQuestionText << item.toString();
    }

    const QJsonArray attributeQuestionResponsesArray = jsonDataOptions["attributeQuestionResponses"].toArray();
    i = 0;
    for(const auto &item : attributeQuestionResponsesArray) {
        const QJsonArray attributeQuestionResponsesArraySubarray = item.toArray();
        attributeQuestionResponses[i].reserve(attributeQuestionResponsesArraySubarray.size());
        for(const auto &response : attributeQuestionResponsesArraySubarray) {
            attributeQuestionResponses[i] << response.toString();
        }
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeQuestionResponses[j].clear();
    }

    const QJsonArray attributeQuestionResponseCountsArray = jsonDataOptions["attributeQuestionResponseCounts"].toArray();
    i = 0;
    for(const auto &item : attributeQuestionResponseCountsArray) {
        attributeQuestionResponseCounts[i].clear();
        const QVariantMap attributeQuestionResponseCountsArraySubobject = item.toObject().toVariantMap();
        for(auto iter = attributeQuestionResponseCountsArraySubobject.cbegin(),
                 end = attributeQuestionResponseCountsArraySubobject.cend(); iter != end; ++iter) {
            attributeQuestionResponseCounts[i].emplace(iter.key(), iter.value().toInt());
        }
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeQuestionResponseCounts[j].clear();
    }

    const QJsonArray attributeValsArray = jsonDataOptions["attributeVals"].toArray();
    i = 0;
    for(const auto &item : attributeValsArray) {
        attributeVals[i].clear();
        const QJsonArray attributeValsArraySubarray = item.toArray();
        for(const auto &val : attributeValsArraySubarray) {
            attributeVals[i].insert(val.toInt());
        }
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeVals[j].clear();
    }

    const QJsonArray GendersArray = jsonDataOptions["Genders"].toArray();
    genderValues.reserve(GendersArray.size());
    for(const auto &item : GendersArray) {
        genderValues << grueprGlobal::stringToGender(item.toString());
    }

    const QJsonArray URMResponsesArray = jsonDataOptions["URMResponses"].toArray();
    URMResponses.reserve(URMResponsesArray.size());
    for(const auto &item : URMResponsesArray) {
        URMResponses << item.toString();
    }

    const QJsonArray dayNamesArray = jsonDataOptions["dayNames"].toArray();
    dayNames.reserve(dayNamesArray.size());
    for(const auto &item : dayNamesArray) {
        dayNames << item.toString();
    }

    const QJsonArray timeNamesArray = jsonDataOptions["timeNames"].toArray();
    timeNames.reserve(timeNamesArray.size());
    for(const auto &item : timeNamesArray) {
        timeNames << item.toString();
    }
}


bool DataOptions::parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &hours, float &minutes, float &offsetFromGMT)
{
    static const QRegularExpression timeZoneFinder(TIMEZONEREGEX);
    const QRegularExpressionMatch timezoneCapture = timeZoneFinder.match(fullText);
    if(timezoneCapture.hasMatch()) {
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
        attributeQuestionResponsesArray, attributeQuestionResponseCountsArray, attributeValsArray, gendersArray;

    for (const int field : notesFields) {
        notesFieldArray.append(field);
    }
    for (const int field : scheduleField) {
        scheduleFieldArray.append(field);
    }
    for (const int field : prefTeammatesField) {
        prefTeammatesFieldArray.append(field);
    }
    for (const int field : prefNonTeammatesField) {
        prefNonTeammatesFieldArray.append(field);
    }
    for (int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeFieldArray.append(attributeField[i]);
        attributeTypeArray.append(static_cast<int>(attributeType[i]));
        attributeQuestionResponsesArray.append(QJsonArray::fromStringList(attributeQuestionResponses[i]));
        QJsonObject attributeQuestionResponseCountsObj;
        for(const auto &item : attributeQuestionResponseCounts[i]) {
            attributeQuestionResponseCountsObj.insert(item.first, item.second);
        }
        attributeQuestionResponseCountsArray.append(attributeQuestionResponseCountsObj);
        QJsonArray attributeValsArraySubArray;
        for (const auto &val : attributeVals[i]) {
            attributeValsArraySubArray.append(val);
        }
        attributeValsArray.append(attributeValsArraySubArray);
    }
    for (const Gender genderValue : genderValues) {
        gendersArray.append(grueprGlobal::genderToString(genderValue));
    }
    if (gendersArray.isEmpty()) {
        gendersArray.append("unknown");
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
        {"scheduleDataIsFreetime", scheduleDataIsFreetime},
        {"scheduleField", scheduleFieldArray},
        {"earlyTimeAsked", earlyTimeAsked},
        {"lateTimeAsked", lateTimeAsked},
        {"scheduleResolution", scheduleResolution},
        {"numAttributes", numAttributes},
        {"attributeField", attributeFieldArray},
        {"timezoneIncluded", timezoneIncluded},
        {"timezoneField", timezoneField},
        {"homeTimezoneUsed", homeTimezoneUsed},
        {"baseTimezone", baseTimezone},
        {"gradeIncluded", gradeIncluded},
        {"gradeField", gradeField},
        {"attributeType", attributeTypeArray},
        {"prefTeammatesField", prefTeammatesFieldArray},
        {"prefNonTeammatesField", prefNonTeammatesFieldArray},
        {"sectionNames", QJsonArray::fromStringList(sectionNames)},
        {"attributeQuestionText", QJsonArray::fromStringList(attributeQuestionText)},
        {"attributeQuestionResponses", attributeQuestionResponsesArray},
        {"attributeQuestionResponseCounts", attributeQuestionResponseCountsArray},
        {"attributeVals", attributeValsArray},
        {"URMResponses", QJsonArray::fromStringList(URMResponses)},
        {"Genders", gendersArray},
        {"dataSourceName", dataSourceName},
        {"dataSource", static_cast<int>(dataSource)},
        {"dayNames", QJsonArray::fromStringList(dayNames)},
        {"timeNames", QJsonArray::fromStringList(timeNames)},
        {"saveStateFileName", saveStateFileName}
    };

    return content;
}
