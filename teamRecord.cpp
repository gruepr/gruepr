#include "teamRecord.h"
#include <QJsonArray>


TeamRecord::TeamRecord(const DataOptions *const teamSetDataOptions, const QJsonObject &jsonTeamRecord) : teamSetDataOptions(teamSetDataOptions)
{
    LMSID = jsonTeamRecord["LMSID"].toInt();
    score = jsonTeamRecord["score"].toDouble();
    size = jsonTeamRecord["size"].toInt();
    numSections = jsonTeamRecord["numSections"].toInt();
    numWomen = jsonTeamRecord["numWomen"].toInt();
    numMen = jsonTeamRecord["numMen"].toInt();
    numNonbinary = jsonTeamRecord["numNonbinary"].toInt();
    numUnknown = jsonTeamRecord["numUnknown"].toInt();
    numURM = jsonTeamRecord["numURM"].toInt();
    numStudentsWithAmbiguousSchedules = jsonTeamRecord["numStudentsWithAmbiguousSchedules"].toInt();
    numMeetingTimes = jsonTeamRecord["numMeetingTimes"].toInt();
    name = jsonTeamRecord["name"].toString();
    tooltip = jsonTeamRecord["tooltip"].toString();

    QJsonArray attributeValsArray = jsonTeamRecord["attributeVals"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        QJsonArray attributeValsArraySubArray = attributeValsArray[i].toArray();
        for (const auto &val : attributeValsArraySubArray) {
            attributeVals[i].insert(val.toInt());
        }
    }

    QJsonArray timezoneValsArray = jsonTeamRecord["timezoneVals"].toArray();
    for (const auto &val : timezoneValsArray) {
        timezoneVals.insert(val.toDouble());
    }

    QJsonArray numStudentsAvailableArray = jsonTeamRecord["numStudentsAvailable"].toArray();
    for(int i = 0; i < MAX_DAYS; i++) {
        QJsonArray numStudentsAvailableArraySubArray = numStudentsAvailableArray[i].toArray();
        for(int j = 0; j < MAX_BLOCKS_PER_DAY; j++) {
            numStudentsAvailable[i][j] = numStudentsAvailableArraySubArray[j].toInt();
        }
    }

    QJsonArray studentIndexesArray = jsonTeamRecord["studentIndexes"].toArray();
    for (const auto &val : studentIndexesArray) {
        studentIndexes << val.toInt();
    }
}


void TeamRecord::createTooltip()
{
    QString toolTipText = "<html>";
    if(score < 0) {
        toolTipText += "<table><tr><td bgcolor=" STARFISHHEX "><b>" + QObject::tr("This team has a negative compatibility score, "
                       "indicating one or more teaming requirements have not been met. "
                       "You may want to relax some constraints and try to form teams again.") + "</b></td></tr></table><br>";
    }
    toolTipText += QObject::tr("Team ") + name + "<br>";
    if(teamSetDataOptions->genderIncluded) {
        toolTipText += QObject::tr("Gender") + ":  ";
        QStringList genderSingularOptions, genderPluralOptions;
        if(teamSetDataOptions->genderType == GenderType::biol) {
            genderSingularOptions = QString(BIOLGENDERS).split('/');
            genderPluralOptions = QString(BIOLGENDERS).split('/');
        }
        else if(teamSetDataOptions->genderType == GenderType::adult) {
            genderSingularOptions = QString(ADULTGENDERS).split('/');
            genderPluralOptions = QString(ADULTGENDERSPLURAL).split('/');
        }
        else if(teamSetDataOptions->genderType == GenderType::child) {
            genderSingularOptions = QString(CHILDGENDERS).split('/');
            genderPluralOptions = QString(CHILDGENDERSPLURAL).split('/');
        }
        else { //if(teamSetDataOptions->genderType == GenderType::pronoun)
            genderSingularOptions = QString(PRONOUNS).split('/');
            genderPluralOptions = QString(PRONOUNS).split('/');
        }
        if(numWomen > 0) {
            toolTipText += QString::number(numWomen) + " " + ((numWomen == 1)? (genderSingularOptions.at(static_cast<int>(Gender::woman))) : (genderPluralOptions.at(static_cast<int>(Gender::woman))));
            if(numMen > 0 || numNonbinary > 0 || numUnknown > 0) {
                toolTipText += ", ";
            }
        }
        if(numMen > 0) {
            toolTipText += QString::number(numMen) + " " + ((numMen == 1)? (genderSingularOptions.at(static_cast<int>(Gender::man))) : (genderPluralOptions.at(static_cast<int>(Gender::man))));
            if(numNonbinary > 0 || numUnknown > 0) {
                toolTipText += ", ";
            }
        }
        if(numNonbinary > 0) {
            toolTipText += QString::number(numNonbinary) + " " + ((numNonbinary == 1)? (genderSingularOptions.at(static_cast<int>(Gender::nonbinary))) : (genderPluralOptions.at(static_cast<int>(Gender::nonbinary))));
            if(numUnknown > 0) {
                toolTipText += ", ";
            }
        }
        if(numUnknown > 0) {
            toolTipText += QString::number(numUnknown) + " " + ((numUnknown == 1)? (genderSingularOptions.at(static_cast<int>(Gender::unknown))) : (genderPluralOptions.at(static_cast<int>(Gender::unknown))));
        }
    }
    if(teamSetDataOptions->URMIncluded) {
        toolTipText += "<br>" + QObject::tr("URM") + ":  " + QString::number(numURM);
    }
    int numAttributesWOTimezone = teamSetDataOptions->numAttributes - (teamSetDataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++) {
        toolTipText += "<br>" + QObject::tr("Attribute ") + QString::number(attribute + 1) + ":  ";
        auto teamVals = attributeVals[attribute].cbegin();
        auto lastVal = attributeVals[attribute].cend();
        if((teamSetDataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
           (teamSetDataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
            // attribute is ordered/numbered, so important info is the range of values (but ignore any "unset/unknown" values of -1)
            if(*teamVals == -1) {
                teamVals++;
            }
            if(teamVals != lastVal) {
                if(*teamVals == *attributeVals[attribute].crbegin()) {
                    toolTipText += QString::number(*teamVals);
                }
                else {
                    toolTipText += QString::number(*teamVals) + " - " + QString::number(*attributeVals[attribute].crbegin());
                }
            }
            else {
                toolTipText += "?";
            }
        }
        else if((teamSetDataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical) ||
                (teamSetDataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical)) {
            // attribute is categorical, so important info is the list of values
            // if attribute has "unset/unknown" value of -1, char is nicely '?'; if attribute value is > 26, letters are repeated as needed
            toolTipText += (*teamVals <= 26 ? QString(char(*teamVals - 1 + 'A')) : QString(char((*teamVals - 1)%26 + 'A')).repeated(1+((*teamVals - 1)/26)));
            for(teamVals++; teamVals != lastVal; teamVals++) {
                toolTipText += ", " + (*teamVals <= 26 ? QString(char(*teamVals - 1 + 'A')) : QString(char((*teamVals - 1)%26 + 'A')).repeated(1+((*teamVals - 1)/26)));
            }
        }
    }
    if(teamSetDataOptions->timezoneIncluded) {
        float timezoneA = *timezoneVals.cbegin();
        float timezoneB = *timezoneVals.crbegin();
        QString timezoneText;
        if(timezoneA == timezoneB) {
            int hour = int(timezoneA);
            int minutes = int(60*(timezoneA - int(timezoneA)));
            timezoneText = QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs(minutes), 2, 10, QChar('0'));;
        }
        else {
            int hourA = int(timezoneA);
            int hourB = int(timezoneB);
            int minutesA = int(60*(timezoneA - int(timezoneA)));
            int minutesB = int(60*(timezoneB - int(timezoneB)));
            timezoneText = (QString("%1%2:%3 ") + RIGHTARROW + " %4%5:%6").arg(timezoneA >= 0 ? "+" : "").arg(hourA).arg(std::abs(minutesA), 2, 10, QChar('0'))
                                                                  .arg(timezoneB >= 0 ? "+" : "").arg(hourB).arg(std::abs(minutesB), 2, 10, QChar('0'));
        }
        toolTipText += "<br>" + QObject::tr("Timezones:  GMT") + timezoneText;
    }
    if(!teamSetDataOptions->dayNames.isEmpty()) {
        toolTipText += "<br>--<br>" + QObject::tr("Availability:") + "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";

        for(const auto &dayName : teamSetDataOptions->dayNames) {
            // using first 3 characters in day name as abbreviation
            toolTipText += "<th>" + dayName.left(3) + "</th>";
        }
        toolTipText += "</tr>";

        for(int time = 0; time < teamSetDataOptions->timeNames.size(); time++) {
            toolTipText += "<tr><th>" + teamSetDataOptions->timeNames.at(time) + "</th>";
            for(int day = 0; day < teamSetDataOptions->dayNames.size(); day++) {
                QString percentage;
                if(size > numStudentsWithAmbiguousSchedules) {
                    percentage = QString::number((100*numStudentsAvailable[day][time]) / (size-numStudentsWithAmbiguousSchedules)) + "% ";
                }
                else {
                    percentage = "?";
                }

                if(percentage == "100% ") {
                    toolTipText += "<td align='center' bgcolor='PaleGreen'><b>" + percentage + "</b></td>";
                }
                else {
                    toolTipText += "<td align='center'>" + percentage + "</td>";
                }
            }
            toolTipText += "</tr>";
        }
        toolTipText += "</table></html>";
    }

    tooltip = toolTipText;
}


void TeamRecord::refreshTeamInfo(const StudentRecord* const student, const int meetingBlockSize)
{
    //re-zero values
    numSections = 0;
    QStringList sections;
    numWomen = 0;
    numMen = 0;
    numNonbinary = 0;
    numUnknown = 0;
    numURM = 0;
    numStudentsWithAmbiguousSchedules = 0;
    numMeetingTimes = 0;
    for(int attribute = 0; attribute < teamSetDataOptions->numAttributes; attribute++) {
        attributeVals[attribute].clear();
    }
    const int numDays = teamSetDataOptions->dayNames.size();
    const int numTimes = teamSetDataOptions->timeNames.size();
    for(int day = 0; day < numDays; day++) {
        for(int time = 0; time < numTimes; time++) {
            numStudentsAvailable[day][time] = 0;
        }
    }

    //set values
    for(int teammate = 0; teammate < size; teammate++) {
        const StudentRecord &stu = student[studentIndexes.at(teammate)];
        if(!sections.contains(stu.section)) {
            sections << stu.section;
            numSections++;
        }
        if(teamSetDataOptions->genderIncluded) {
            if(stu.gender == Gender::woman) {
                numWomen++;
            }
            else if(stu.gender == Gender::man) {
                numMen++;
            }
            else if(stu.gender == Gender::nonbinary) {
                numNonbinary++;
            }
            else {
                numUnknown++;
            }
        }
        if(teamSetDataOptions->URMIncluded) {
            if(stu.URM) {
                numURM++;
            }
        }
        for(int attribute = 0; attribute < teamSetDataOptions->numAttributes; attribute++) {
            attributeVals[attribute].insert(stu.attributeVals[attribute].constBegin(), stu.attributeVals[attribute].constEnd());
        }
        if(!stu.ambiguousSchedule) {
            for(int day = 0; day < numDays; day++) {
                for(int time = 0; time < numTimes; time++) {
                    if(!stu.unavailable[day][time]) {
                        numStudentsAvailable[day][time]++;
                    }
                }
            }
        }
        else {
            numStudentsWithAmbiguousSchedules++;
        }
        if(teamSetDataOptions->timezoneIncluded) {
            timezoneVals.insert(stu.timezone);
        }
    }

    //count when there's the correct number of consecutive time blocks, but don't count wrap-around past end of 1 day!
    const int numStudentsWithoutAmbiguousSchedules = size - numStudentsWithAmbiguousSchedules;
    for(int day = 0; day < numDays; day++) {
        for(int time = 0; time < numTimes; time++) {
            int blocks = 0;
            while((numStudentsAvailable[day][time] == numStudentsWithoutAmbiguousSchedules) && (blocks < meetingBlockSize)) {
                blocks++;
                if(blocks < meetingBlockSize) {
                    time++;
                }
            }

            if((blocks == meetingBlockSize) && (blocks > 0)){
                numMeetingTimes++;
            }
        }
    }
}

QJsonObject TeamRecord::toJson() const
{
    QJsonArray attributeValsArray, timezoneValsArray, numStudentsAvailableArray, studentIndexesArray;
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        QJsonArray attributeValsArraySubArray;
        for (const auto &val : attributeVals[i]) {
            attributeValsArraySubArray.append(val);
        }
        attributeValsArray.append(attributeValsArraySubArray);
    }
    for(const auto &timezoneVal : timezoneVals) {
        timezoneValsArray.append(timezoneVal);
    }
    for(int i = 0; i < MAX_DAYS; i++) {
        QJsonArray numStudentsAvailableArraySubArray;
        for(int j = 0; j < MAX_BLOCKS_PER_DAY; j++) {
            numStudentsAvailableArraySubArray.append(numStudentsAvailable[i][j]);
        }
        numStudentsAvailableArray.append(numStudentsAvailableArraySubArray);
    }
    for(const auto &studentIndex : studentIndexes) {
        studentIndexesArray.append(studentIndex);
    }

    QJsonObject content {
        {"LMSID", LMSID},
        {"score", score},
        {"size", size},
        {"numSections", numSections},
        {"numWomen", numWomen},
        {"numMen", numMen},
        {"numNonbinary", numNonbinary},
        {"numUnknown", numUnknown},
        {"numURM", numURM},
        {"attributeVals", attributeValsArray},
        {"timezoneVals", timezoneValsArray},
        {"numStudentsAvailable", numStudentsAvailableArray},
        {"numStudentsWithAmbiguousSchedules", numStudentsWithAmbiguousSchedules},
        {"numMeetingTimes", numMeetingTimes},
        {"studentIndexes", studentIndexesArray},
        {"name", name},
        {"tooltip", tooltip},
        {"dataOptions", teamSetDataOptions->toJson()}
    };

    return content;
}
