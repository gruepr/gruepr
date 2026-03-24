#include "teamingOptions.h"
#include <QJsonArray>
#include <QMetaEnum>
#include <QString>


TeamingOptions::TeamingOptions(const QJsonObject &jsonTeamingOptions)
{
    realNumScoringFactors = jsonTeamingOptions["realNumScoringFactors"].toInt();
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
    sectionName.clear();
    teamsetNumber = 1;
}

QJsonObject TeamingOptions::toJson() const
{
    QJsonArray smallerTeamsSizesArray, largerTeamsSizesArray, teamSizesDesiredArray;

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
        {"realNumScoringFactors", realNumScoringFactors},
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
