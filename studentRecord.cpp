#include "studentRecord.h"
#include <QRegularExpression>

StudentRecord::StudentRecord()
{
    surveyTimestamp = QDateTime::currentDateTime();

    for(int day = 0; day < MAX_DAYS; day++)
    {
        for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++)
        {
            unavailable[day][time] = true;
        }
    }
}


////////////////////////////////////////////
// Move fields read from file into student record values
////////////////////////////////////////////
void StudentRecord::parseRecordFromStringList(const QStringList &fields, const DataOptions* const dataOptions)
{
    int fieldnum = dataOptions->timestampField;
    if(fieldnum != -1)
    {
        const QString &timestampText = fields.at(fieldnum);
        QVector<Qt::DateFormat> stdTimestampFormats = {Qt::TextDate, Qt::ISODate, Qt::ISODateWithMs, Qt::SystemLocaleShortDate, Qt::SystemLocaleLongDate, Qt::RFC2822Date};

        surveyTimestamp = QDateTime::fromString(timestampText.left(timestampText.lastIndexOf(' ')), TIMESTAMP_FORMAT1); // format with direct download from Google Form
        if(surveyTimestamp.isNull())
        {
            surveyTimestamp = QDateTime::fromString(timestampText.left(timestampText.lastIndexOf(' ')), TIMESTAMP_FORMAT2); // alt format with direct download from Google Form
            if(surveyTimestamp.isNull())
            {
                surveyTimestamp = QDateTime::fromString(timestampText, TIMESTAMP_FORMAT3);
                if(surveyTimestamp.isNull())
                {
                    surveyTimestamp = QDateTime::fromString(timestampText, TIMESTAMP_FORMAT4);
                    int i = 0;
                    while(i < stdTimestampFormats.size() && surveyTimestamp.isNull())
                    {
                        surveyTimestamp = QDateTime::fromString(timestampText, stdTimestampFormats.at(i));
                        i++;
                    }
                }
            }
        }
    }
    if(surveyTimestamp.isNull())
    {
        surveyTimestamp = QDateTime::currentDateTime();
    }

    fieldnum = dataOptions->firstNameField;
    firstname = fields.at(fieldnum).toUtf8().trimmed();
    firstname[0] = firstname[0].toUpper();

    fieldnum = dataOptions->lastNameField;
    lastname = fields.at(fieldnum).toUtf8().trimmed();
    lastname[0] = lastname[0].toUpper();

    fieldnum = dataOptions->emailField;
    email = fields.at(fieldnum).toUtf8().trimmed();

    // optional gender
    if(dataOptions->genderIncluded)
    {
        fieldnum = dataOptions->genderField;
        QString field = fields.at(fieldnum).toUtf8();
        if(field.contains(QObject::tr("woman"), Qt::CaseInsensitive) || field.contains(QObject::tr("female"), Qt::CaseInsensitive))
        {
            gender = StudentRecord::woman;
        }
        else if(field.contains(QObject::tr("man"), Qt::CaseInsensitive) || field.contains(QObject::tr("male"), Qt::CaseInsensitive))
        {
            gender = StudentRecord::man;
        }
        else if((field.contains(QObject::tr("non"), Qt::CaseInsensitive) && field.contains(QObject::tr("binary"), Qt::CaseInsensitive)) ||
                 field.contains(QObject::tr("queer"), Qt::CaseInsensitive) ||
                 field.contains(QObject::tr("trans"), Qt::CaseInsensitive))
        {
            gender = StudentRecord::nonbinary;
        }
        else
        {
            gender = StudentRecord::unknown;
        }
    }
    else
    {
        gender = StudentRecord::unknown;
    }

    // optional race/ethnicity status
    if(dataOptions->URMIncluded)
    {
        fieldnum = dataOptions->URMField;
        QString field = fields.at(fieldnum).toUtf8().toLower().simplified();
        if(field == "")
        {
            field = QObject::tr("--");
        }
        URMResponse = field;
    }
    else
    {
        URM = false;
    }

    // optional attributes
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        fieldnum = dataOptions->attributeField[attribute];
        QString field = fields.at(fieldnum).toUtf8();
        field.replace("â€”","-");       // replace bad UTF-8 character representation of em-dash
        attributeResponse[attribute] = field;
    }

    // optional schedule days
    const int numDays = dataOptions->dayNames.size();
    const int numTimes = dataOptions->timeNames.size();
    int timezoneOffset = 0;
    fieldnum = dataOptions->timezoneField;
    if(fieldnum != -1)
    {
        QRegularExpression zoneFinder(".*\\[GMT(.*):(.*)\\].*");  // characters after "[GMT" are +hh:mm "]"
        QRegularExpressionMatch zone = zoneFinder.match(fields.at(fieldnum).toUtf8());
        if(zone.hasMatch())
        {
            int hours = zone.captured(1).toInt();
            float minutes = zone.captured(2).toFloat();
            timezone = hours + (hours < 0? (-minutes/60) : (minutes/60));
            if(dataOptions->homeTimezoneUsed)
            {
                timezoneOffset = std::lround(dataOptions->baseTimezone - timezone);
            }
        }
    }
    for(int day = 0; day < numDays; day++)
    {
        fieldnum = dataOptions->scheduleField[day];
        QString field = fields.at(fieldnum).toUtf8();
        QRegularExpression timename("", QRegularExpression::CaseInsensitiveOption);
        for(int time = 0; time < numTimes; time++)
        {
            timename.setPattern("\\b"+dataOptions->timeNames.at(time).toUtf8()+"\\b");
            if(dataOptions->scheduleDataIsFreetime)
            {
                // need to ignore when we're outside the bounds of the array, or we're not looking at all 7 days and this one wraps around the day
                if(((day + time + timezoneOffset) >= 0) &&
                   (((day * numTimes) + time + timezoneOffset) <= (numDays * numTimes)) &&
                   ((numDays == MAX_DAYS) || (((time + timezoneOffset) >= 0) && ((time + timezoneOffset) <= MAX_BLOCKS_PER_DAY))))
                {
                    unavailable[day][time + timezoneOffset] = !timename.match(field).hasMatch();
                }
            }
            else
            {
                // need to ignore when we're outside the bounds of the array, when we're not looking at all 7 days and this one wraps around the day,
                // or--since we asked when they're unavailable--any times that we didn't actually ask about
                if(((day + time + timezoneOffset) >= 0) &&
                   (((day * numTimes) + time + timezoneOffset) <= (numDays * numTimes)) &&
                   (time >= dataOptions->earlyHourAsked) &&
                   (time <= dataOptions->lateHourAsked) &&
                   ((numDays == MAX_DAYS) || (((time + timezoneOffset) >= 0) && ((time + timezoneOffset) <= MAX_BLOCKS_PER_DAY))))
                {
                    unavailable[day][time + timezoneOffset] = timename.match(field).hasMatch();
                }
            }
        }
    }
    if(!dataOptions->dayNames.isEmpty())
    {
        availabilityChart = QObject::tr("Availability:");
        availabilityChart += "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";
        for(int day = 0; day < numDays; day++)
        {
            availabilityChart += "<th>" + dataOptions->dayNames.at(day).toUtf8().left(3) + "</th>";   // using first 3 characters in day name as abbreviation
        }
        availabilityChart += "</tr>";
        for(int time = 0; time < numTimes; time++)
        {
            availabilityChart += "<tr><th>" + dataOptions->timeNames.at(time).toUtf8() + "</th>";
            for(int day = 0; day < numDays; day++)
            {
                availabilityChart += QString(unavailable[day][time]?
                            "<td align = center> </td>" : "<td align = center bgcolor='PaleGreen'><b>√</b></td>");
            }
            availabilityChart += "</tr>";
        }
        availabilityChart += "</table>";
    }
    ambiguousSchedule = (availabilityChart.count("√") == 0 || availabilityChart.count("√") == (numDays * numTimes));

    // optional section
    if(dataOptions->sectionIncluded)
    {
        fieldnum = dataOptions->sectionField;
        section = fields.at(fieldnum).toUtf8().trimmed();
        if(section.startsWith("section",Qt::CaseInsensitive))
        {
            section = section.right(section.size()-7).trimmed();    //removing as redundant the word "section" if at the start of the section name
        }
    }

    // optional preferred teammates
    if(dataOptions->prefTeammatesIncluded)
    {
        fieldnum = dataOptions->prefTeammatesField;
        prefTeammates = fields.at(fieldnum).toUtf8();
        prefTeammates.replace(QRegularExpression("\\s*([,;&]|(?:and))\\s*"), "\n");     // replace every [, ; & and] with new line
        prefTeammates = prefTeammates.trimmed();
    }

    // optional preferred non-teammates
    if(dataOptions->prefNonTeammatesIncluded)
    {
        fieldnum = dataOptions->prefNonTeammatesField;
        prefNonTeammates = fields.at(fieldnum).toUtf8();
        prefNonTeammates.replace(QRegularExpression("\\s*([,;&]|(?:and))\\s*"), "\n");     // replace every [, ; & and] with new line
        prefNonTeammates = prefNonTeammates.trimmed();
    }

    // optional notes
    for(int note = 0; note < dataOptions->numNotes; note++)
    {
        fieldnum = dataOptions->notesField[note];
        if(note > 0)
        {
            notes += "\n";
        }
        notes += fields.at(fieldnum).toUtf8().trimmed();     // join each one with a newline after
    }
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
        toolTip += "<br>" + QObject::tr("Timezone") + QString(":  GMT%1%2:%3").arg(timezone >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0'));
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
