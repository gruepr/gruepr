#include "studentRecord.h"
#include <QLocale>
#include <QRegularExpression>

StudentRecord::StudentRecord()
{
    surveyTimestamp = QDateTime::currentDateTime();

    for(auto &day : unavailable)
    {
        for(auto &time : day)
        {
            time = true;
        }
    }
}


////////////////////////////////////////////
// Move fields read from file into student record values
////////////////////////////////////////////
void StudentRecord::parseRecordFromStringList(const QStringList &fields, const DataOptions* const dataOptions)
{
    // Timestamp
    int fieldnum = dataOptions->timestampField;
    if(fieldnum != -1)
    {
        const QString &timestampText = fields.at(fieldnum);
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
                    if(surveyTimestamp.isNull())
                    {
                        surveyTimestamp = QLocale::system().toDateTime(timestampText, QLocale::ShortFormat);
                        if(surveyTimestamp.isNull())
                        {
                            surveyTimestamp = QLocale::system().toDateTime(timestampText, QLocale::LongFormat);
                            if(surveyTimestamp.isNull())
                            {
                                int i = 0;
                                QVector<Qt::DateFormat> stdTimestampFormats = {Qt::TextDate, Qt::ISODate, Qt::ISODateWithMs, Qt::RFC2822Date};
                                while(i < stdTimestampFormats.size() && surveyTimestamp.isNull())
                                {
                                    surveyTimestamp = QDateTime::fromString(timestampText, stdTimestampFormats.at(i));
                                    i++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(surveyTimestamp.isNull())
    {
        surveyTimestamp = QDateTime::currentDateTime();
    }

    // First name
    fieldnum = dataOptions->firstNameField;
    if(fieldnum != -1)
    {
        firstname = fields.at(fieldnum).toLatin1().trimmed();
        firstname[0] = firstname[0].toUpper();
    }

    // Last name
    fieldnum = dataOptions->lastNameField;
    if(fieldnum != -1)
    {
        lastname = fields.at(fieldnum).toLatin1().trimmed();
        lastname[0] = lastname[0].toUpper();
    }

    // Email
    fieldnum = dataOptions->emailField;
    if(fieldnum != -1)
    {
        email = fields.at(fieldnum).toLatin1().trimmed();
    }

    // gender
    if(dataOptions->genderIncluded)
    {
        fieldnum = dataOptions->genderField;
        QString field = fields.at(fieldnum).toUtf8();
        if(field.contains(QObject::tr("female"), Qt::CaseInsensitive) || field.contains(QObject::tr("woman"), Qt::CaseInsensitive)  || field.contains(QObject::tr("girl"), Qt::CaseInsensitive)
                 || field.contains(QObject::tr("she"), Qt::CaseInsensitive))
        {
            gender = Gender::woman;
        }
        else if((field.contains(QObject::tr("non"), Qt::CaseInsensitive) && field.contains(QObject::tr("binary"), Qt::CaseInsensitive)) ||
                 field.contains(QObject::tr("queer"), Qt::CaseInsensitive) || field.contains(QObject::tr("trans"), Qt::CaseInsensitive)  || field.contains(QObject::tr("they"), Qt::CaseInsensitive))
        {
            gender = Gender::nonbinary;
        }
        else if(field.contains(QObject::tr("male"), Qt::CaseInsensitive) || field.contains(QObject::tr("man"), Qt::CaseInsensitive) || field.contains(QObject::tr("boy"), Qt::CaseInsensitive)
                 || field.contains(QObject::tr("he"), Qt::CaseInsensitive))
        {
            gender = Gender::man;
        }
        else
        {
            gender = Gender::unknown;
        }
    }
    else
    {
        gender = Gender::unknown;
    }

    // racial/ethnic heritage
    if(dataOptions->URMIncluded)
    {
        fieldnum = dataOptions->URMField;
        QString field = fields.at(fieldnum).toLatin1().toLower().simplified();
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

    // attributes
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        fieldnum = dataOptions->attributeField[attribute];
        QString field = fields.at(fieldnum).toLatin1();
        field.replace("â€”","-");       // replace bad UTF-8 character representation of em-dash
        attributeResponse[attribute] = field;
    }

    // schedule days
    const int numDays = dataOptions->dayNames.size();
    const int numTimes = dataOptions->timeNames.size();
    int timezoneOffset = 0;
    fieldnum = dataOptions->timezoneField;
    if(fieldnum != -1)
    {
        QString timezoneText = fields.at(fieldnum).toUtf8(), timezoneName;
        if(DataOptions::parseTimezoneInfoFromText(timezoneText, timezoneName, timezone))
        {
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

    // section
    if(dataOptions->sectionIncluded)
    {
        QString sectionText = QObject::tr("section");
        fieldnum = dataOptions->sectionField;
        section = fields.at(fieldnum).toUtf8().trimmed();
        if(section.startsWith(sectionText, Qt::CaseInsensitive))
        {
            section = section.right(section.size() - sectionText.size()).trimmed();    //removing as redundant the word "section" if at the start of the section name
        }
    }

    // preferred teammates
    if(dataOptions->prefTeammatesIncluded)
    {
        fieldnum = dataOptions->prefTeammatesField;
        prefTeammates = fields.at(fieldnum).toLatin1();
        prefTeammates.replace(QRegularExpression("\\s*([,;&]|(?:and))\\s*"), "\n");     // replace every [, ; & and] with new line
        prefTeammates = prefTeammates.trimmed();
    }

    // preferred non-teammates
    if(dataOptions->prefNonTeammatesIncluded)
    {
        fieldnum = dataOptions->prefNonTeammatesField;
        prefNonTeammates = fields.at(fieldnum).toLatin1();
        prefNonTeammates.replace(QRegularExpression("\\s*([,;&]|(?:and))\\s*"), "\n");     // replace every [, ; & and] with new line
        prefNonTeammates = prefNonTeammates.trimmed();
    }

    // notes
    for(int note = 0; note < dataOptions->numNotes; note++)
    {
        fieldnum = dataOptions->notesField[note];
        if(note > 0)
        {
            notes += "\n";
        }
        notes += fields.at(fieldnum).toLatin1().trimmed();     // join each one with a newline after
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
    if(dataOptions->emailField != -1)
    {
        toolTip += "<br>" + email;
    }
    if(dataOptions->genderIncluded)
    {
        toolTip += "<br>";
        QStringList genderOptions;
        if(dataOptions->genderType == GenderType::biol)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::adult)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::child)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else //if(dataOptions->genderType == GenderType::pronoun)
        {
            toolTip += QObject::tr("Pronouns");
            genderOptions = QString(PRONOUNS).split('/');
        }
        toolTip += ":  " + genderOptions.at(static_cast<int>(gender));
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
        const auto *value = attributeVals[attribute].constBegin();
        if(*value != -1)
        {
            if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered)
            {
                toolTip += QString::number(*value);
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical)
            {
                // if attribute value is > 26, letters are repeated as needed
                toolTip += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) :
                                             QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical)
            {
                const auto *lastVal = attributeVals[attribute].constEnd();
                while(value != lastVal)
                {
                    toolTip += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) :
                                                 QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
                    value++;
                    if(value != lastVal)
                    {
                        toolTip += ",";
                    }
                }
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)
            {
                const auto *lastVal = attributeVals[attribute].constEnd();
                while(value != lastVal)
                {
                    toolTip += QString::number(*value);

                    value++;
                    if(value != lastVal)
                    {
                        toolTip += ",";
                    }
                }
            }
        }
        else
        {
            toolTip += "?";
        }
    }
    if(dataOptions->timezoneIncluded)
    {
        toolTip += "<br>" + QObject::tr("Timezone:  ");
        //find the timezone as attribute value so that -1 can show as unknown timezone
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
        {
            if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone)
            {
                const auto *value = attributeVals[attribute].constBegin();
                if(*value != -1)
                {
                    int hour = int(timezone);
                    int minutes = 60*(timezone - int(timezone));
                    toolTip += QString("GMT%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs((minutes)), 2, 10, QChar('0'));
                }
                else
                {
                    toolTip += "?";
                }
            }
        }
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
