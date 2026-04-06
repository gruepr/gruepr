#include "teamRecord.h"
#include <QJsonArray>


TeamRecord::TeamRecord(const DataOptions *const teamSetDataOptions, const QJsonObject &jsonTeamRecord, const QList<StudentRecord> &students) :
    teamSetDataOptions(teamSetDataOptions)
{
    LMSID = jsonTeamRecord["LMSID"].toInt();
    score = jsonTeamRecord["score"].toDouble();
    size = jsonTeamRecord["size"].toInt();
    numSections = jsonTeamRecord["numSections"].toInt();
    numWomen = jsonTeamRecord["numWomen"].toInt();
    numMen = jsonTeamRecord["numMen"].toInt();
    numNonbinary = jsonTeamRecord["numNonbinary"].toInt();
    numUnknown = jsonTeamRecord["numUnknown"].toInt();
    numStudentsWithAmbiguousSchedules = jsonTeamRecord["numStudentsWithAmbiguousSchedules"].toInt();
    numMeetingTimes = jsonTeamRecord["numMeetingTimes"].toInt();
    name = jsonTeamRecord["name"].toString();
    tooltip = jsonTeamRecord["tooltip"].toString();

    const QJsonArray numStudentsAvailableArray = jsonTeamRecord["numStudentsAvailable"].toArray();
    const int numDaysInFile = numStudentsAvailableArray.size();
    const int numTimesInFile = numDaysInFile > 0 ? numStudentsAvailableArray[0].toArray().size() : 0;
    numStudentsAvailable.resize(numDaysInFile * numTimesInFile);
    for(int day = 0; day < numDaysInFile; day++) {
        const QJsonArray subArray = numStudentsAvailableArray[day].toArray();
        for(int time = 0; time < numTimesInFile; time++) {
            numStudentsAvailable[day * numTimesInFile + time] = subArray[time].toInt();
        }
    }

    if(jsonTeamRecord["studentIDs"].type() != QJsonValue::Undefined) {
        const QJsonArray studentIDsArray = jsonTeamRecord["studentIDs"].toArray();
        for (const auto &val : studentIDsArray) {
            studentIDs << val.toInteger();
        }
    }
    else {
        // this is for backwards compatability--teamRecord formerly saved the students as their indexes in the students array rather than IDs
        // In order to use work saved from prev. versions of gruepr, now must convert these indexes to the IDs
        const QJsonArray studentIndexesArray = jsonTeamRecord["studentIndexes"].toArray();
        for (const auto &val : studentIndexesArray) {
            studentIDs << students.at(val.toInt()).ID;
        }
    }
}


void TeamRecord::createTooltip(const QList<StudentRecord> &students)
{
    QString toolTipText = "<html>";
    toolTipText += QObject::tr("Team ") + name + "<br>" +
                   (score <= 0 ? "<span style=\"color: red; font-weight: bold;\">" : "") + QString::number(score, 'f', 1) + " % compatibility score" +
                   (score <= 0 ? "</span>" : "") + "<br>";
    if(!assignedOption.isEmpty()) {
        toolTipText += QObject::tr("Assignment: ") + "<b>" + assignedOption + "</b><br>";
    }
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
    const int numAttributesWOTimezone = teamSetDataOptions->numAttributes - (teamSetDataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++) {
        const auto type = teamSetDataOptions->attributeType[attribute];
        toolTipText += "<br>" + QObject::tr("Q") + QString::number(attribute + 1) + ":  ";

        if(type == DataOptions::AttributeType::numerical) {
            // Show team mean
            float sum = 0; int count = 0;
            for(const auto &studentID : std::as_const(studentIDs)) {
                for(const auto &student : students) {
                    if(student.ID == studentID && !student.attributeVals_continuous[attribute].isEmpty()) {
                        sum += student.attributeVals_continuous[attribute].front();
                        count++;
                        break;
                    }
                }
            }
            toolTipText += count > 0 ? QString::number(double(sum / count), 'f', 2) : "?";
            continue;
        }

        // Collect discrete values from team members into a set
        std::set<int> teamDiscreteVals;
        for(const auto &id : std::as_const(studentIDs)) {
            for(const auto &student : students) {
                if(student.ID == id) {
                    teamDiscreteVals.insert(student.attributeVals_discrete[attribute].constBegin(),
                                            student.attributeVals_discrete[attribute].constEnd());
                    break;
                }
            }
        }

        auto teamVals = teamDiscreteVals.cbegin();
        auto lastVal  = teamDiscreteVals.cend();

        if(type == DataOptions::AttributeType::ordered ||
           type == DataOptions::AttributeType::multiordered) {
            if(teamVals != lastVal && *teamVals == -1) {
                teamVals++; // skip unknown sentinel
            }
            if(teamVals != lastVal) {
                const int lo = *teamVals;
                const int hi = *teamDiscreteVals.crbegin();
                toolTipText += (lo == hi) ? QString::number(lo)
                                           : QString::number(lo) + " - " + QString::number(hi);
            } else {
                toolTipText += "?";
            }
        }
        else { // categorical / multicategorical
            if(teamVals != lastVal) {
                auto fmt = [](int v) {
                    return v <= 0  ? QString("?")
                         : v <= 26 ? QString(char(v - 1 + 'A'))
                                   : QString(char((v-1)%26 + 'A')).repeated(1 + (v-1)/26);
                };
                toolTipText += fmt(*teamVals);
                for(++teamVals; teamVals != lastVal; ++teamVals) {
                    toolTipText += ", " + fmt(*teamVals);
                }
            }
            else {
                toolTipText += "?";
            }
        }
    }

    // Timezone row
    if(teamSetDataOptions->timezoneIncluded) {
        std::set<float> tzVals;
        for(const auto &studentID : std::as_const(studentIDs)) {
            for(const auto &student : students) {
                if(student.ID == studentID) { tzVals.insert(student.timezone); break; }
            }
        }
        if(!tzVals.empty()) {
            const float tzA = *tzVals.cbegin(), tzB = *tzVals.crbegin();
            auto fmtTz = [](float tz) {
                const int h = int(tz), m = int(60*(tz - int(tz)));
                return QString("%1%2:%3").arg(h >= 0 ? "+" : "").arg(h)
                                         .arg(std::abs(m), 2, 10, QChar('0'));
            };
            toolTipText += "<br>" + QObject::tr("Timezones:  GMT");
            toolTipText += (tzA == tzB) ? fmtTz(tzA)
                                        : fmtTz(tzA) + " " + RIGHTARROW + " " + fmtTz(tzB);
        }
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
                    percentage = QString::number((100*numStudentsAvailable[day * teamSetDataOptions->timeNames.size() + time]) /
                                                 (size-numStudentsWithAmbiguousSchedules)) + "% ";
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


void TeamRecord::refreshTeamInfo(const QList<StudentRecord> &students, const int meetingBlockSize)
{
    //re-zero values
    numSections = 0;
    QStringList sections;
    numWomen = 0;
    numMen = 0;
    numNonbinary = 0;
    numUnknown = 0;
    numStudentsWithAmbiguousSchedules = 0;
    numMeetingTimes = 0;
    const qsizetype numDays = teamSetDataOptions->dayNames.size();
    const qsizetype numTimes = teamSetDataOptions->timeNames.size();
    numStudentsAvailable.fill(0, numDays * numTimes);

    //set values
    for(const auto studentID : std::as_const(studentIDs)) {
        const StudentRecord* student = nullptr;
        for(const auto &thisStudent : std::as_const(students)) {
            if(thisStudent.ID == studentID) {
                student = &thisStudent;
                break;
            }
        }
        if(student == nullptr) {
            continue;
        }

        if(!sections.contains(student->section)) {
            sections << student->section;
            numSections++;
        }
        if(teamSetDataOptions->genderIncluded) {
            if(student->gender.contains(Gender::woman)) {
                numWomen++;
            }
            if(student->gender.contains(Gender::man)) {
                numMen++;
            }
            if(student->gender.contains(Gender::nonbinary)) {
                numNonbinary++;
            }
            if (student->gender.contains(Gender::unknown)) {
                numUnknown++;
            }
        }

        if(!student->ambiguousSchedule) {
            for(int day = 0; day < numDays; day++) {
                for(int time = 0; time < numTimes; time++) {
                    if(!student->unavailable[day * numTimes + time]) {
                        numStudentsAvailable[day * numTimes + time]++;
                    }
                }
            }
        }
        else {
            numStudentsWithAmbiguousSchedules++;
        }
    }

    //count when there's the correct number of consecutive time blocks, but don't count wrap-around past end of 1 day!
    const int numStudentsWithoutAmbiguousSchedules = size - numStudentsWithAmbiguousSchedules;
    for(int day = 0; day < numDays; day++) {
        for(int time = 0; time < numTimes; time++) {
            int blocks = 0;
            while((numStudentsAvailable[day * numTimes + time] == numStudentsWithoutAmbiguousSchedules) && (blocks < meetingBlockSize)) {
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
    QJsonArray numStudentsAvailableArray, studentIDsArray;
    const int numDays = teamSetDataOptions->dayNames.size();
    const int numTimes = teamSetDataOptions->timeNames.size();
    for(int day = 0; day < numDays; day++) {
        QJsonArray subArray;
        for(int time = 0; time < numTimes; time++) {
            subArray.append(numStudentsAvailable[day * numTimes + time]);
        }
        numStudentsAvailableArray.append(subArray);
    }
    for(const auto &studentID : studentIDs) {
        studentIDsArray.append(studentID);
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
        {"numStudentsAvailable", numStudentsAvailableArray},
        {"numStudentsWithAmbiguousSchedules", numStudentsWithAmbiguousSchedules},
        {"numMeetingTimes", numMeetingTimes},
        {"studentIDs", studentIDsArray},
        {"name", name},
        {"tooltip", tooltip},
        {"dataOptions", teamSetDataOptions->toJson()}
    };

    return content;
}
