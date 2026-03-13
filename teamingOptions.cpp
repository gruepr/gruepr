#include "teamingOptions.h"
#include <QJsonArray>
#include <QMetaEnum>
#include <QString>

TeamingOptions::TeamingOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        haveAnyRequiredAttributes[i] = false;
        requiredAttributeValues[i].clear();
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
}

TeamingOptions::TeamingOptions(const QJsonObject &jsonTeamingOptions)
{
    desiredTimeBlocksOverlap = jsonTeamingOptions["desiredTimeBlocksOverlap"].toInt();
    minTimeBlocksOverlap = jsonTeamingOptions["minTimeBlocksOverlap"].toInt();
    meetingBlockSize = jsonTeamingOptions["meetingBlockSize"].toDouble();
    realMeetingBlockSize = jsonTeamingOptions["realMeetingBlockSize"].toInt();
    targetMinimumGroupGradeAverage = jsonTeamingOptions["targetMinimumGroupGradeAverage"].toDouble();
    targetMaximumGroupGradeAverage = jsonTeamingOptions["targetMaximumGroupGradeAverage"].toDouble();
    const QJsonArray haveAnyRequiredAttributesArray = jsonTeamingOptions["haveAnyRequiredAttributes"].toArray();
    int i = 0;
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
    realNumScoringFactors = jsonTeamingOptions["realNumScoringFactors"].toInt();
    haveAnyGroupTogethers = jsonTeamingOptions.contains("haveAnyGroupTogethers") ?
                                jsonTeamingOptions["haveAnyGroupTogethers"].toBool()
                                : jsonTeamingOptions["haveAnyRequiredTeammates"].toBool() ||        // old Terminology
                                  jsonTeamingOptions["haveAnyRequestedTeammates"].toBool();
    haveAnySplitAparts = jsonTeamingOptions.contains("haveAnySplitAparts") ?
                            jsonTeamingOptions["haveAnySplitAparts"].toBool()
                            : jsonTeamingOptions["haveAnyPreventedTeammates"].toBool();             // old Terminology
    numberGroupTogethersGiven = jsonTeamingOptions.contains("numberGroupTogethersGiven") ?
                                    jsonTeamingOptions["numberGroupTogethersGiven"].toInt()
                                    : jsonTeamingOptions["numberRequestedTeammatesGiven"].toInt();  // old Terminology
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
    idealTeamSize = jsonTeamingOptions["idealTeamSize"].toInt(4);
    sectionName = jsonTeamingOptions["sectionName"].toString();
    sectionType = static_cast<SectionType>(jsonTeamingOptions["sectionType"].toInt());
    teamsetNumber = jsonTeamingOptions["teamsetNumber"].toInt();
}


void TeamingOptions::reset()
{
    // reset the variables that depend on the datafile
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        haveAnyRequiredAttributes[i] = false;
        requiredAttributeValues[i].clear();
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
    haveAnyGroupTogethers = false;
    haveAnySplitAparts = false;
    sectionName.clear();
    teamsetNumber = 1;
}

QJsonObject TeamingOptions::toJson() const
{
    QJsonArray haveAnyRequiredAttributesArray, requiredAttributeValuesArray, haveAnyIncompatibleAttributesArray,
               incompatibleAttributeValuesArray, smallerTeamsSizesArray, largerTeamsSizesArray, teamSizesDesiredArray;

    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
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
        {"desiredTimeBlocksOverlap", desiredTimeBlocksOverlap},
        {"minTimeBlocksOverlap", minTimeBlocksOverlap},
        {"meetingBlockSize", meetingBlockSize},
        {"realMeetingBlockSize", realMeetingBlockSize},
        {"haveAnyRequiredAttributes", haveAnyRequiredAttributesArray},
        {"requiredAttributeValues", requiredAttributeValuesArray},
        {"haveAnyIncompatibleAttributes", haveAnyIncompatibleAttributesArray},
        {"incompatibleAttributeValues", incompatibleAttributeValuesArray},
        {"realNumScoringFactors", realNumScoringFactors},
        {"haveAnyGroupTogethers", haveAnyGroupTogethers},
        {"haveAnySplitAparts", haveAnySplitAparts},
        {"numberGroupTogethersGiven", numberGroupTogethersGiven},
        {"idealTeamSize", idealTeamSize},
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
