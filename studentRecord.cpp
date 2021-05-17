#include "studentRecord.h"

StudentRecord::StudentRecord()
{
    surveyTimestamp = QDateTime::currentDateTime();
}

////////////////////////////////////////////
// Create a tooltip for a student
////////////////////////////////////////////
void StudentRecord::createTooltip(const DataOptions* const dataOptions)
{
    QString toolTip = "<html>";
    if(duplicateRecord)
    {
        toolTip += "<table><tr><td bgcolor=#ffff3b><b>" + QObject::tr("There appears to be multiple survey submissions from this student!") + "</b></td></tr></table><br>";
    }
    toolTip += firstname + " " + lastname;
    toolTip += "<br>" + email;
    if(dataOptions->genderIncluded)
    {
        toolTip += "<br>" + QObject::tr("Gender") + ":  ";
        if(gender == StudentRecord::woman)
        {
            toolTip += QObject::tr("woman");
        }
        else if(gender == StudentRecord::man)
        {
            toolTip += QObject::tr("man");
        }
        else if(gender == StudentRecord::nonbinary)
        {
            toolTip += QObject::tr("nonbinary");
        }
        else
        {
            toolTip += QObject::tr("unknown");
        }

    }
    if(dataOptions->URMIncluded)
    {
        toolTip += "<br>" + QObject::tr("Identity") + ":  ";
        toolTip += URMResponse;
    }
    int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++)
    {
        toolTip += "<br>" + QObject::tr("Attribute ") + QString::number(attribute + 1) + ":  ";
        if(attributeVal[attribute] != -1)
        {
            if(dataOptions->attributeIsOrdered[attribute])
            {
                toolTip += QString::number(attributeVal[attribute]);
            }
            else
            {
                // if attribute has "unset/unknown" value of -1, char is nicely '?'; if attribute value is > 26, letters are repeated as needed
                toolTip += (attributeVal[attribute] <= 26 ? QString(char(attributeVal[attribute]-1 + 'A')) :
                                                            QString(char((attributeVal[attribute]-1)%26 + 'A')).repeated(1+((attributeVal[attribute]-1)/26)));
            }
        }
        else
        {
            toolTip += "?";
        }
    }
    if(dataOptions->timezoneIncluded)
    {
        int hour = int(timezone);
        int minutes = 60*(timezone - int(timezone));
        toolTip += "<br>" + QObject::tr("Timezone") + QString(":  GMT %1%2:%3").arg(timezone >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0'));
    }
    if(!(availabilityChart.isEmpty()))
    {
        toolTip += "<br>--<br>" + availabilityChart;
    }
    if(dataOptions->prefTeammatesIncluded)
    {
        QString note = prefTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(dataOptions->prefNonTeammatesIncluded)
    {
        QString note = prefNonTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Non-teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(dataOptions->numNotes > 0)
    {
        QString note = notes;
        if(note.size() > SIZE_OF_NOTES_IN_TOOLTIP)
        {
            note = note.mid(0, (SIZE_OF_NOTES_IN_TOOLTIP - 3)) + "...";
        }
        toolTip += "<br>--<br>" + QObject::tr("Notes") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    toolTip += "</html>";

    tooltip = toolTip;
}
