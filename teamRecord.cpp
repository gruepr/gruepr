#include "teamRecord.h"


TeamRecord::TeamRecord() = default;


void TeamRecord::createTooltip(const DataOptions* const dataOptions)
{
    QString toolTip = "<html>";
    if(score <= 0)
    {
        toolTip += "<table><tr><td bgcolor=#fbcfce><b>" + QObject::tr("This team has a compatibility score &lt;=0, indicating one or more teaming requirements cannot be met."
                   "You may want to relax the constraints and try to form teams again.") + "</b></td></tr></table><br>";
    }
    toolTip += QObject::tr("Team ") + name + "<br>";
    if(dataOptions->genderIncluded)
    {
        toolTip += QObject::tr("Gender") + ":  ";
        if(numWomen > 0)
        {
            toolTip += QString::number(numWomen) + (numWomen > 1? QObject::tr(" women") : QObject::tr(" woman"));
        }
        if(numWomen > 0 && (numMen > 0 || numNonbinary > 0 || numUnknown > 0))
        {
            toolTip += ", ";
        }
        if(numMen > 0)
        {
            toolTip += QString::number(numMen) + (numMen > 1? QObject::tr(" men") : QObject::tr(" man"));
        }
        if(numMen > 0 && (numNonbinary > 0 || numUnknown > 0))
        {
            toolTip += ", ";
        }
        if(numNonbinary > 0)
        {
            toolTip += QString::number(numNonbinary) + QObject::tr(" nonbinary");
        }
        if(numNonbinary > 0 && numUnknown > 0)
        {
            toolTip += ", ";
        }
        if(numUnknown > 0)
        {
            toolTip += QString::number(numUnknown) + QObject::tr(" unknown");
        }
    }
    if(dataOptions->URMIncluded)
    {
        toolTip += "<br>" + QObject::tr("URM") + ":  " + QString::number(numURM);
    }
    int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++)
    {
        toolTip += "<br>" + QObject::tr("Attribute ") + QString::number(attribute + 1) + ":  ";
        auto teamVals = attributeVals[attribute].begin();
        if(dataOptions->attributeIsOrdered[attribute])
        {
            // attribute is ordered/numbered, so important info is the range of values (but ignore any "unset/unknown" values of -1)
            if(*teamVals == -1)
            {
                teamVals++;
            }
            if(teamVals != attributeVals[attribute].end())
            {
                if(*teamVals == *attributeVals[attribute].rbegin())
                {
                    toolTip += QString::number(*teamVals);
                }
                else
                {
                    toolTip += QString::number(*teamVals) + " - " + QString::number(*attributeVals[attribute].rbegin());
                }
            }
            else
            {
                toolTip += "?";
            }
        }
        else
        {
            // attribute is categorical, so important info is the list of values
            // if attribute has "unset/unknown" value of -1, char is nicely '?'; if attribute value is > 26, letters are repeated as needed
            toolTip += (*teamVals <= 26 ? QString(char(*teamVals - 1 + 'A')) : QString(char((*teamVals - 1)%26 + 'A')).repeated(1+((*teamVals - 1)/26)));
            for(teamVals++; teamVals != attributeVals[attribute].end(); teamVals++)
            {
                toolTip += ", " + (*teamVals <= 26 ? QString(char(*teamVals - 1 + 'A')) : QString(char((*teamVals - 1)%26 + 'A')).repeated(1+((*teamVals - 1)/26)));
            }
        }
    }
    if(dataOptions->timezoneIncluded)
    {
        float timezoneA = *timezoneVals.cbegin();
        float timezoneB = *timezoneVals.crbegin();
        int hourA = int(timezoneA);
        int hourB = int(timezoneB);
        int minutesA = 60*(timezoneA - int(timezoneA));
        int minutesB = 60*(timezoneB - int(timezoneB));
        toolTip += "<br>" + QObject::tr("Timezones") + QString(":  GMT%1%2:%3 \u2192 %4%5:%6").arg(timezoneA >= 0 ? "+" : "").arg(hourA).arg(minutesA, 2, 10, QChar('0'))
                                                                                              .arg(timezoneB >= 0 ? "+" : "").arg(hourB).arg(minutesB, 2, 10, QChar('0'));
    }
    if(!dataOptions->dayNames.isEmpty())
    {
        toolTip += "<br>--<br>" + QObject::tr("Availability:") + "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";

        for(int day = 0; day < dataOptions->dayNames.size(); day++)
        {
            // using first 3 characters in day name as abbreviation
            toolTip += "<th>" + dataOptions->dayNames.at(day).left(3) + "</th>";
        }
        toolTip += "</tr>";

        for(int time = 0; time < dataOptions->timeNames.size(); time++)
        {
            toolTip += "<tr><th>" + dataOptions->timeNames.at(time) + "</th>";
            for(int day = 0; day < dataOptions->dayNames.size(); day++)
            {
                QString percentage;
                if(size > numStudentsWithAmbiguousSchedules)
                {
                    percentage = QString::number((100*numStudentsAvailable[day][time]) / (size-numStudentsWithAmbiguousSchedules)) + "% ";
                }
                else
                {
                    percentage = "?";
                }

                if(percentage == "100% ")
                {
                    toolTip += "<td align='center' bgcolor='PaleGreen'><b>" + percentage + "</b></td>";
                }
                else
                {
                    toolTip += "<td align='center'>" + percentage + "</td>";
                }
            }
            toolTip += "</tr>";
        }
        toolTip += "</table></html>";
    }

    tooltip = toolTip;
}


void TeamRecord::refreshTeamInfo(const DataOptions* const dataOptions, const StudentRecord* const student)
{
    //re-zero values
    numWomen = 0;
    numMen = 0;
    numNonbinary = 0;
    numUnknown = 0;
    numURM = 0;
    numStudentsWithAmbiguousSchedules = 0;
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        attributeVals[attribute].clear();
    }
    for(int day = 0; day < dataOptions->dayNames.size(); day++)
    {
        for(int time = 0; time < dataOptions->timeNames.size(); time++)
        {
            numStudentsAvailable[day][time] = 0;
        }
    }

    //set values
    for(int teammate = 0; teammate < size; teammate++)
    {
        const StudentRecord &stu = student[studentIDs.at(teammate)];
        if(dataOptions->genderIncluded)
        {
            if(stu.gender == StudentRecord::woman)
            {
                numWomen++;
            }
            else if(stu.gender == StudentRecord::man)
            {
                numMen++;
            }
            else if(stu.gender == StudentRecord::nonbinary)
            {
                numNonbinary++;
            }
            else
            {
                numUnknown++;
            }
        }
        if(dataOptions->URMIncluded)
        {
            if(stu.URM)
            {
                numURM++;
            }
        }
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
        {
            attributeVals[attribute].insert(stu.attributeVal[attribute]);
        }
        if(!stu.ambiguousSchedule)
        {
            for(int day = 0; day < dataOptions->dayNames.size(); day++)
            {
                for(int time = 0; time < dataOptions->timeNames.size(); time++)
                {
                    if(!stu.unavailable[day][time])
                    {
                        numStudentsAvailable[day][time]++;
                    }
                }
            }
        }
        else
        {
            numStudentsWithAmbiguousSchedules++;
        }
        if(dataOptions->timezoneIncluded)
        {
            timezoneVals.insert(stu.timezone);
        }
    }
}
