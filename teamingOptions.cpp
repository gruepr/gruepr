#include "teamingOptions.h"
#include <QJsonArray>

TeamingOptions::TeamingOptions()
{
    // initialize all attribute weights to 1, desires to heterogeneous, and incompatible attribute values to none
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        desireHomogeneous[i] = false;
        attributeWeights[i] = 1;
        realAttributeWeights[i] = 1;
        haveAnyRequiredAttributes[i] = false;
        requiredAttributeValues[i].clear();
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
}

TeamingOptions::TeamingOptions(const QJsonObject &jsonTeamingOptions)
{
    isolatedWomenPrevented = jsonTeamingOptions["isolatedWomenPrevented"].toBool();
    isolatedMenPrevented = jsonTeamingOptions["isolatedMenPrevented"].toBool();
    isolatedNonbinaryPrevented = jsonTeamingOptions["isolatedNonbinaryPrevented"].toBool();
    singleGenderPrevented = jsonTeamingOptions["singleGenderPrevented"].toBool();
    isolatedURMPrevented = jsonTeamingOptions["isolatedURMPrevented"].toBool();
    QJsonArray URMResponsesConsideredURArray = jsonTeamingOptions["URMResponsesConsideredUR"].toArray();
    URMResponsesConsideredUR.clear();
    URMResponsesConsideredUR.reserve(URMResponsesConsideredURArray.size());
    for(const auto &item : URMResponsesConsideredURArray) {
        URMResponsesConsideredUR << item.toString();
    }
    desiredTimeBlocksOverlap = jsonTeamingOptions["desiredTimeBlocksOverlap"].toInt();
    minTimeBlocksOverlap = jsonTeamingOptions["minTimeBlocksOverlap"].toInt();
    meetingBlockSize = jsonTeamingOptions["meetingBlockSize"].toInt();
    QJsonArray desireHomogeneousArray = jsonTeamingOptions["desireHomogeneous"].toArray();
    int i = 0;
    for(const auto &item : desireHomogeneousArray) {
        desireHomogeneous[i] = item.toBool();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        desireHomogeneous[j] = false;
    }
    QJsonArray attributeWeightsArray = jsonTeamingOptions["attributeWeights"].toArray();
    i = 0;
    for(const auto &item : attributeWeightsArray) {
        attributeWeights[i] = item.toDouble();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeWeights[j] = 1;
    }
    QJsonArray realAttributeWeightsArray = jsonTeamingOptions["realAttributeWeights"].toArray();
    i = 0;
    for(const auto &item : realAttributeWeightsArray) {
        realAttributeWeights[i] = item.toDouble();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        realAttributeWeights[j] = 1;
    }
    QJsonArray haveAnyRequiredAttributesArray = jsonTeamingOptions["haveAnyRequiredAttributes"].toArray();
    i = 0;
    for(const auto &item : haveAnyRequiredAttributesArray) {
        haveAnyRequiredAttributes[i] = item.toBool();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        haveAnyRequiredAttributes[j] = false;
    }
    QJsonArray requiredAttributeValuesArray = jsonTeamingOptions["requiredAttributeValues"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        QJsonArray requiredAttributeValuesArraySubArray = requiredAttributeValuesArray[i].toArray();
        requiredAttributeValues[i].clear();
        for (const auto &val : requiredAttributeValuesArraySubArray) {
            requiredAttributeValues[i] << val.toInt();
        }
    }
    QJsonArray haveAnyIncompatibleAttributesArray = jsonTeamingOptions["haveAnyIncompatibleAttributes"].toArray();
    i = 0;
    for(const auto &item : haveAnyIncompatibleAttributesArray) {
        haveAnyIncompatibleAttributes[i] = item.toBool();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        haveAnyIncompatibleAttributes[j] = false;
    }
    QJsonArray incompatibleAttributeValuesArray = jsonTeamingOptions["incompatibleAttributeValues"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        QJsonArray incompatibleAttributeValuesArraySubArray = incompatibleAttributeValuesArray[i].toArray();
        incompatibleAttributeValues[i].clear();
        for(const auto &pair : incompatibleAttributeValuesArraySubArray) {
            QJsonArray pairOfVals = pair.toArray();
            incompatibleAttributeValues[i].append({pairOfVals[0].toInt(), pairOfVals[1].toInt()});
        }
    }
    scheduleWeight = jsonTeamingOptions["scheduleWeight"].toDouble();
    realScheduleWeight = jsonTeamingOptions["realScheduleWeight"].toDouble();
    realNumScoringFactors = jsonTeamingOptions["realNumScoringFactors"].toInt();
    haveAnyRequiredTeammates = jsonTeamingOptions["haveAnyRequiredTeammates"].toBool();
    haveAnyPreventedTeammates = jsonTeamingOptions["haveAnyPreventedTeammates"].toBool();
    haveAnyRequestedTeammates = jsonTeamingOptions["haveAnyRequestedTeammates"].toBool();
    numberRequestedTeammatesGiven = jsonTeamingOptions["numberRequestedTeammatesGiven"].toInt();
    QJsonArray smallerTeamsSizesArray = jsonTeamingOptions["smallerTeamsSizes"].toArray();
    i = 0;
    for (const auto &val : smallerTeamsSizesArray) {
        smallerTeamsSizes[i] = val.toInt();
        i++;
    }
    for(int j = i; j < MAX_STUDENTS; j++) {
        smallerTeamsSizes[j] = 0;
    }
    smallerTeamsNumTeams = jsonTeamingOptions["smallerTeamsNumTeams"].toInt();
    QJsonArray largerTeamsSizesArray = jsonTeamingOptions["largerTeamsSizes"].toArray();
    i = 0;
    for (const auto &val : largerTeamsSizesArray) {
        largerTeamsSizes[i] = val.toInt();
        i++;
    }
    for(int j = i; j < MAX_STUDENTS; j++) {
        largerTeamsSizes[j] = 0;
    }
    largerTeamsNumTeams = jsonTeamingOptions["largerTeamsNumTeams"].toInt();
    numTeamsDesired = jsonTeamingOptions["numTeamsDesired"].toInt();
    QJsonArray teamSizesDesiredArray = jsonTeamingOptions["teamSizesDesired"].toArray();
    i = 0;
    for (const auto &val : teamSizesDesiredArray) {
        teamSizesDesired[i] = val.toInt();
        i++;
    }
    for(int j = i; j < MAX_STUDENTS; j++) {
        teamSizesDesired[j] = 0;
    }
    sectionName = jsonTeamingOptions["sectionName"].toString();
    sectionType = static_cast<SectionType>(jsonTeamingOptions["sectionType"].toInt());
    teamsetNumber = jsonTeamingOptions["teamsetNumber"].toInt();
}


void TeamingOptions::reset()
{
    // reset the variables that depend on the datafile
    URMResponsesConsideredUR.clear();
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        haveAnyRequiredAttributes[i] = false;
        requiredAttributeValues[i].clear();
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
    haveAnyRequiredTeammates = false;
    haveAnyPreventedTeammates = false;
    haveAnyRequestedTeammates = false;
    sectionName.clear();
    teamsetNumber = 1;
}

QJsonObject TeamingOptions::toJson() const
{
    QJsonArray desireHomogeneousArray, attributeWeightsArray, realAttributeWeightsArray, haveAnyRequiredAttributesArray, requiredAttributeValuesArray, haveAnyIncompatibleAttributesArray,
               incompatibleAttributeValuesArray, smallerTeamsSizesArray, largerTeamsSizesArray, teamSizesDesiredArray;

    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        desireHomogeneousArray.append(desireHomogeneous[i]);
        attributeWeightsArray.append(attributeWeights[i]);
        realAttributeWeightsArray.append(realAttributeWeights[i]);
        haveAnyRequiredAttributesArray.append(haveAnyRequiredAttributes[i]);
        QJsonArray requiredAttributeValuesArraySubArray;
        for (const auto &val : requiredAttributeValues[i]) {
            requiredAttributeValuesArraySubArray.append(val);
        }
        requiredAttributeValuesArray.append(requiredAttributeValuesArraySubArray);
        haveAnyIncompatibleAttributesArray.append(haveAnyIncompatibleAttributes[i]);
        QJsonArray incompatibleAttributeValuesArraySubArray;
        for(const auto &item : incompatibleAttributeValues[i])
        {
            QJsonArray pair;
            pair.append(item.first);
            pair.append(item.second);
            incompatibleAttributeValuesArraySubArray.append(pair);
        }
        incompatibleAttributeValuesArray.append(incompatibleAttributeValuesArraySubArray);
    }
    for(int i = 0; i < MAX_STUDENTS; i++) {
        smallerTeamsSizesArray.append(smallerTeamsSizes[i]);
        largerTeamsSizesArray.append(largerTeamsSizes[i]);
        teamSizesDesiredArray.append(teamSizesDesired[i]);
    }

    QJsonObject content {
        {"isolatedWomenPrevented", isolatedWomenPrevented},
        {"isolatedMenPrevented", isolatedMenPrevented},
        {"isolatedNonbinaryPrevented", isolatedNonbinaryPrevented},
        {"singleGenderPrevented", singleGenderPrevented},
        {"isolatedURMPrevented", isolatedURMPrevented},
        {"URMResponsesConsideredUR", QJsonArray::fromStringList(URMResponsesConsideredUR)},
        {"desiredTimeBlocksOverlap", desiredTimeBlocksOverlap},
        {"minTimeBlocksOverlap", minTimeBlocksOverlap},
        {"meetingBlockSize", meetingBlockSize},
        {"desireHomogeneous", desireHomogeneousArray},
        {"attributeWeights", attributeWeightsArray},
        {"realAttributeWeights", realAttributeWeightsArray},
        {"haveAnyRequiredAttributes", haveAnyRequiredAttributesArray},
        {"requiredAttributeValues", requiredAttributeValuesArray},
        {"haveAnyIncompatibleAttributes", haveAnyIncompatibleAttributesArray},
        {"incompatibleAttributeValues", incompatibleAttributeValuesArray},
        {"scheduleWeight", scheduleWeight},
        {"realScheduleWeight", realScheduleWeight},
        {"realNumScoringFactors", realNumScoringFactors},
        {"haveAnyRequiredTeammates", haveAnyRequiredTeammates},
        {"haveAnyPreventedTeammates", haveAnyPreventedTeammates},
        {"haveAnyRequestedTeammates", haveAnyRequestedTeammates},
        {"numberRequestedTeammatesGiven", numberRequestedTeammatesGiven},
        {"smallerTeamsSizes", smallerTeamsSizesArray},
        {"smallerTeamsNumTeams", smallerTeamsNumTeams},
        {"largerTeamsSizes", largerTeamsSizesArray},
        {"largerTeamsNumTeams", largerTeamsNumTeams},
        {"numTeamsDesired", numTeamsDesired},
        {"teamSizesDesired", teamSizesDesiredArray},
        {"sectionName", sectionName},
        {"sectionType", static_cast<int>(sectionType)},
        {"teamsetNumber", teamsetNumber}
    };

    return content;
}
