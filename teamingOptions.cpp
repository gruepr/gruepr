#include "teamingOptions.h"
#include <QJsonArray>
#include <QMetaEnum>
#include <QString>

TeamingOptions::TeamingOptions()
{
    // initialize all attributes to unselected, diversity to diverse (i.e., heterogeneous), weights to 1, and incompatible attribute values to none
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeSelected[i] = 0;
        attributeDiversity[i] = Criterion::AttributeDiversity::diverse;
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
    const QJsonArray URMResponsesConsideredURArray = jsonTeamingOptions["URMResponsesConsideredUR"].toArray();
    URMResponsesConsideredUR.clear();
    URMResponsesConsideredUR.reserve(URMResponsesConsideredURArray.size());
    for(const auto &item : URMResponsesConsideredURArray) {
        URMResponsesConsideredUR << item.toString();
    }
    desiredTimeBlocksOverlap = jsonTeamingOptions["desiredTimeBlocksOverlap"].toInt();
    minTimeBlocksOverlap = jsonTeamingOptions["minTimeBlocksOverlap"].toInt();
    meetingBlockSize = jsonTeamingOptions["meetingBlockSize"].toDouble();
    realMeetingBlockSize = jsonTeamingOptions["realMeetingBlockSize"].toInt();
    const QJsonArray attributeSelectedArray = jsonTeamingOptions["attributeSelected"].toArray();
    int i = 0;
    for(const auto &item : attributeSelectedArray) {
        attributeSelectedArray[i] = item.toInt();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeSelectedArray[j] = 0;
    }
    const QJsonArray attributeDiversityArray = jsonTeamingOptions["attributeDiversity"].toArray();
    auto diversity = QMetaEnum::fromType<Criterion::AttributeDiversity>();
    i = 0;
    for(const auto &item : attributeDiversityArray) {
        attributeDiversity[i] = static_cast<Criterion::AttributeDiversity>(diversity.keyToValue(qPrintable(item.toString())));
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeDiversity[j] = Criterion::AttributeDiversity::diverse;
    }
    const QJsonArray attributeWeightsArray = jsonTeamingOptions["attributeWeights"].toArray();
    i = 0;
    for(const auto &item : attributeWeightsArray) {
        attributeWeights[i] = item.toDouble();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        attributeWeights[j] = 1;
    }
    const QJsonArray realAttributeWeightsArray = jsonTeamingOptions["realAttributeWeights"].toArray();
    i = 0;
    for(const auto &item : realAttributeWeightsArray) {
        realAttributeWeights[i] = item.toDouble();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        realAttributeWeights[j] = 1;
    }
    const QJsonArray haveAnyRequiredAttributesArray = jsonTeamingOptions["haveAnyRequiredAttributes"].toArray();
    i = 0;
    for(const auto &item : haveAnyRequiredAttributesArray) {
        haveAnyRequiredAttributes[i] = item.toBool();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        haveAnyRequiredAttributes[j] = false;
    }
    const QJsonArray requiredAttributeValuesArray = jsonTeamingOptions["requiredAttributeValues"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        const QJsonArray requiredAttributeValuesArraySubArray = requiredAttributeValuesArray[i].toArray();
        requiredAttributeValues[i].clear();
        for (const auto &val : requiredAttributeValuesArraySubArray) {
            requiredAttributeValues[i] << val.toInt();
        }
    }
    const QJsonArray haveAnyIncompatibleAttributesArray = jsonTeamingOptions["haveAnyIncompatibleAttributes"].toArray();
    i = 0;
    for(const auto &item : haveAnyIncompatibleAttributesArray) {
        haveAnyIncompatibleAttributes[i] = item.toBool();
        i++;
    }
    for(int j = i; j < MAX_ATTRIBUTES; j++) {
        haveAnyIncompatibleAttributes[j] = false;
    }
    const QJsonArray incompatibleAttributeValuesArray = jsonTeamingOptions["incompatibleAttributeValues"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        const QJsonArray incompatibleAttributeValuesArraySubArray = incompatibleAttributeValuesArray[i].toArray();
        incompatibleAttributeValues[i].clear();
        for(const auto &pair : incompatibleAttributeValuesArraySubArray) {
            const QJsonArray pairOfVals = pair.toArray();
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
    const QJsonArray smallerTeamsSizesArray = jsonTeamingOptions["smallerTeamsSizes"].toArray();
    for (const auto &val : smallerTeamsSizesArray) {
        const int size = val.toInt();
        if(size > 0) {
            smallerTeamsSizes << size;
        }
    }
    smallerTeamsNumTeams = jsonTeamingOptions["smallerTeamsNumTeams"].toInt();
    const QJsonArray largerTeamsSizesArray = jsonTeamingOptions["largerTeamsSizes"].toArray();
    for (const auto &val : largerTeamsSizesArray) {
        const int size = val.toInt();
        if(size > 0) {
            largerTeamsSizes << size;
        }
    }
    largerTeamsNumTeams = jsonTeamingOptions["largerTeamsNumTeams"].toInt();
    const QJsonArray teamSizesDesiredArray = jsonTeamingOptions["teamSizesDesired"].toArray();
    for (const auto &val : teamSizesDesiredArray) {
        const int size = val.toInt();
        if(size > 0) {
            teamSizesDesired << size;
        }
    }
    numTeamsDesired = jsonTeamingOptions["numTeamsDesired"].toInt();
    sectionName = jsonTeamingOptions["sectionName"].toString();
    sectionType = static_cast<SectionType>(jsonTeamingOptions["sectionType"].toInt());
    teamsetNumber = jsonTeamingOptions["teamsetNumber"].toInt();
}


void TeamingOptions::reset()
{
    // reset the variables that depend on the datafile
    URMResponsesConsideredUR.clear();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
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
    QJsonArray attributeSelectedArray, attributeDiversityArray, attributeWeightsArray, realAttributeWeightsArray, haveAnyRequiredAttributesArray, requiredAttributeValuesArray, haveAnyIncompatibleAttributesArray,
               incompatibleAttributeValuesArray, smallerTeamsSizesArray, largerTeamsSizesArray, teamSizesDesiredArray;
    auto diversity = QMetaEnum::fromType<Criterion::AttributeDiversity>();

    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeSelectedArray.append(attributeSelected[i]);
        attributeDiversityArray.append(diversity.valueToKey(static_cast<int>(attributeDiversity[i])));
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
        for(const auto &item : incompatibleAttributeValues[i]) {
            QJsonArray pair;
            pair.append(item.first);
            pair.append(item.second);
            incompatibleAttributeValuesArraySubArray.append(pair);
        }
        incompatibleAttributeValuesArray.append(incompatibleAttributeValuesArraySubArray);
    }
    for(const auto size : smallerTeamsSizes) {
        smallerTeamsSizesArray.append(size);
    }
    for(const auto size : largerTeamsSizes) {
        largerTeamsSizesArray.append(size);
    }
    for(const auto size : teamSizesDesired) {
        teamSizesDesiredArray.append(size);
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
        {"realMeetingBlockSize", realMeetingBlockSize},
        {"attributeSelected", attributeSelectedArray},
        {"attributeDiversity", attributeDiversityArray},
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
        {"teamSizesDesired", teamSizesDesiredArray},
        {"numTeamsDesired", numTeamsDesired},
        {"sectionName", sectionName},
        {"sectionType", static_cast<int>(sectionType)},
        {"teamsetNumber", teamsetNumber}
    };

    return content;
}
