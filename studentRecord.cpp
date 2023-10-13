#include "studentRecord.h"
#include <QJsonArray>
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

StudentRecord::StudentRecord(const QJsonObject &jsonStudentRecord)
{
    ID = jsonStudentRecord["ID"].toInt();
    LMSID = jsonStudentRecord["LMSID"].toInt();
    duplicateRecord = jsonStudentRecord["duplicateRecord"].toBool();
    gender = static_cast<Gender>(jsonStudentRecord["gender"].toInt());
    URM = jsonStudentRecord["URM"].toBool();
    timezone = jsonStudentRecord["timezone"].toDouble();
    ambiguousSchedule = jsonStudentRecord["ambiguousSchedule"].toBool();
    surveyTimestamp = QDateTime::fromString(jsonStudentRecord["surveyTimestamp"].toString(), Qt::ISODate);
    firstname = jsonStudentRecord["firstname"].toString();
    lastname = jsonStudentRecord["lastname"].toString();
    email = jsonStudentRecord["email"].toString();
    section = jsonStudentRecord["section"].toString();
    prefTeammates = jsonStudentRecord["prefTeammates"].toString();
    prefNonTeammates = jsonStudentRecord["prefNonTeammates"].toString();
    notes = jsonStudentRecord["notes"].toString();
    URMResponse = jsonStudentRecord["URMResponse"].toString();
    availabilityChart = jsonStudentRecord["availabilityChart"].toString();
    tooltip = jsonStudentRecord["tooltip"].toString();
    QJsonArray unavailableArray = jsonStudentRecord["unavailable"].toArray();;
    for(int i = 0; i < MAX_DAYS; i++) {
        QJsonArray unavailableArraySubArray = unavailableArray[i].toArray();
        for(int j = 0; j < MAX_BLOCKS_PER_DAY; j++) {
            unavailable[i][j] = unavailableArraySubArray[j].toBool();
        }
    }
    QJsonArray preventedWithArray = jsonStudentRecord["preventedWith"].toArray();
    QJsonArray requiredWithArray = jsonStudentRecord["requiredWith"].toArray();
    QJsonArray requestedWithArray = jsonStudentRecord["requestedWith"].toArray();
    for(int i = 0; i < MAX_IDS; i++) {
        preventedWith[i] = preventedWithArray[i].toBool();
        requiredWith[i] = requiredWithArray[i].toBool();
        requestedWith[i] = requestedWithArray[i].toBool();
    }
    QJsonArray attributeValsArray = jsonStudentRecord["attributeVals"].toArray();
    QJsonArray attributeResponseArray = jsonStudentRecord["attributeResponse"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeResponse[i] = attributeResponseArray[i].toString();
        QJsonArray attributeValsArraySubArray = attributeValsArray[i].toArray();
        for (const auto &val : attributeValsArraySubArray) {
            attributeVals[i] << val.toInt();
        }
    }
}


////////////////////////////////////////////
// Move fields read from file into student record values
////////////////////////////////////////////
void StudentRecord::parseRecordFromStringList(const QStringList &fields, const DataOptions &dataOptions)
{
    int numFields = fields.size();
    // Timestamp
    int fieldnum = dataOptions.timestampField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        const QString &timestampText = fields.at(fieldnum);
        surveyTimestamp = QDateTime::fromString(timestampText.left(timestampText.lastIndexOf(' ')), TIMESTAMP_FORMAT1); // format with direct download from Google Form
        if(surveyTimestamp.isNull()) {
            surveyTimestamp = QDateTime::fromString(timestampText.left(timestampText.lastIndexOf(' ')), TIMESTAMP_FORMAT2); // alt format with direct download from Google Form
            if(surveyTimestamp.isNull()) {
                surveyTimestamp = QDateTime::fromString(timestampText.left(timestampText.lastIndexOf(' ')), Qt::ISODate); // format with direct download from Canvas
                if(surveyTimestamp.isNull()) {
                    surveyTimestamp = QDateTime::fromString(timestampText, TIMESTAMP_FORMAT3);
                    if(surveyTimestamp.isNull()) {
                        surveyTimestamp = QDateTime::fromString(timestampText, TIMESTAMP_FORMAT4);
                        if(surveyTimestamp.isNull()) {
                            surveyTimestamp = QLocale::system().toDateTime(timestampText, QLocale::ShortFormat);
                            if(surveyTimestamp.isNull()) {
                                surveyTimestamp = QLocale::system().toDateTime(timestampText, QLocale::LongFormat);
                                if(surveyTimestamp.isNull()) {
                                    int i = 0;
                                    QList<Qt::DateFormat> stdTimestampFormats = {Qt::TextDate, Qt::ISODate, Qt::ISODateWithMs, Qt::RFC2822Date};
                                    while(i < stdTimestampFormats.size() && surveyTimestamp.isNull()) {
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
    }
    if(surveyTimestamp.isNull()) {
        surveyTimestamp = QDateTime::currentDateTime();
    }

    // LMSID
    fieldnum = dataOptions.LMSIDField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        LMSID = fields.at(fieldnum).toUtf8().trimmed().toInt();
    }

    // First name
    fieldnum = dataOptions.firstNameField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        firstname = fields.at(fieldnum).toLatin1().trimmed();
        if(!firstname.isEmpty()) {
            firstname[0] = firstname[0].toUpper();
        }
    }

    // Last name
    fieldnum = dataOptions.lastNameField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        lastname = fields.at(fieldnum).toLatin1().trimmed();
        if(!lastname.isEmpty()) {
            lastname[0] = lastname[0].toUpper();
        }
    }

    // Email
    fieldnum = dataOptions.emailField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        email = fields.at(fieldnum).toLatin1().trimmed();
    }

    // gender
    if(dataOptions.genderIncluded) {
        fieldnum = dataOptions.genderField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString field = fields.at(fieldnum).toUtf8();
            if(field.contains(QObject::tr("female"), Qt::CaseInsensitive) ||
                field.contains(QObject::tr("woman"), Qt::CaseInsensitive) ||
                field.contains(QObject::tr("girl"), Qt::CaseInsensitive) ||
                field.contains(QObject::tr("she"), Qt::CaseInsensitive)) {
                gender = Gender::woman;
            }
            else if((field.contains(QObject::tr("non"), Qt::CaseInsensitive) && field.contains(QObject::tr("binary"), Qt::CaseInsensitive)) ||
                     field.contains(QObject::tr("queer"), Qt::CaseInsensitive) ||
                     field.contains(QObject::tr("trans"), Qt::CaseInsensitive) ||
                     field.contains(QObject::tr("they"), Qt::CaseInsensitive)) {
                gender = Gender::nonbinary;
            }
            else if(field.contains(QObject::tr("male"), Qt::CaseInsensitive) ||
                     field.contains(QObject::tr("man"), Qt::CaseInsensitive) ||
                     field.contains(QObject::tr("boy"), Qt::CaseInsensitive) ||
                     field.contains(QObject::tr("he"), Qt::CaseInsensitive)) {
                gender = Gender::man;
            }
            else {
                gender = Gender::unknown;
            }
        }
        else {
            gender = Gender::unknown;
        }
    }
    else {
        gender = Gender::unknown;
    }

    // racial/ethnic heritage
    if(dataOptions.URMIncluded) {
        fieldnum = dataOptions.URMField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString field = fields.at(fieldnum).toLatin1().toLower().trimmed();
            if(field == "") {
                field = QObject::tr("--");
            }
            URMResponse = field;
        }
        else {
            URM = false;
        }
    }
    else {
        URM = false;
    }

    // attributes
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++) {
        fieldnum = dataOptions.attributeField[attribute];
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString field = fields.at(fieldnum).toLatin1().trimmed();
            field.replace("â€”","-");       // replace bad UTF-8 character representation of em-dash
            attributeResponse[attribute] = field;
        }
    }

    // schedule
    float timezoneOffset = 0;
    fieldnum = dataOptions.timezoneField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        QString timezoneText = fields.at(fieldnum).toUtf8(), timezoneName;
        if(DataOptions::parseTimezoneInfoFromText(timezoneText, timezoneName, timezone)) {
            if(dataOptions.homeTimezoneUsed) {
                timezoneOffset = dataOptions.baseTimezone - timezone;
            }
        }
    }
    const int numDays = int(dataOptions.dayNames.size());
    const int numTimes = int(dataOptions.timeNames.size());
    for(int day = 0; day < numDays; day++) {
        fieldnum = dataOptions.scheduleField[day];
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString field = fields.at(fieldnum).toUtf8();
            static QRegularExpression timenameRegEx("", QRegularExpression::CaseInsensitiveOption);
            for(const auto &timeName : dataOptions.timeNames) {
                float time = grueprGlobal::timeStringToHours(timeName);
                // ignore this timeslot if we're not looking at all 7 days and this one wraps around the day
                if((numDays < MAX_DAYS) && (((time + timezoneOffset) < 0) || ((time + timezoneOffset) > 24))) {
                    continue;
                }

                timenameRegEx.setPattern("\\b"+timeName+"\\b");

                // determine which spot in the unavailability chart to put this date/time
                int actualday = day;
                float actualtime = time + timezoneOffset;
                // if this one wraps around the day, then adjust to the correct day/time
                if(actualtime < 0) {
                    actualtime += 24;
                    actualday--;
                    if(actualday < 0) {
                        if(numDays < MAX_DAYS) {  // less than all 7 days, so not clear where to shift this time--just ignore
                            continue;
                        }
                        actualday += MAX_DAYS;
                    }
                }
                if(actualtime >= 24) {
                    actualtime -= 24;
                    actualday++;
                    if(actualday >= numDays) {
                        if(numDays < MAX_DAYS) {  // less than all 7 days, so not clear where to shift this time--just ignore
                            continue;
                        }
                        actualday -= MAX_DAYS;
                    }
                }
                int timeindex = 0;
                while((grueprGlobal::timeStringToHours(dataOptions.timeNames.at(timeindex)) != -1) &&
                      (actualtime > grueprGlobal::timeStringToHours(dataOptions.timeNames.at(timeindex))) &&
                      timeindex < numTimes) {
                    timeindex++;
                }

                if((actualday < 0) || (actualday > MAX_DAYS) || (timeindex < 0) || (timeindex > MAX_BLOCKS_PER_DAY)) {
                    continue;   // something went wrong in figuring out where to put this value in the array!
                }

                bool &unavailabilitySpot = unavailable[actualday][timeindex];

                if(dataOptions.scheduleDataIsFreetime) {
                    unavailabilitySpot = !timenameRegEx.match(field).hasMatch();
                }
                else {
                    // since we asked when they're unavailable, ignore any times that we didn't actually ask about
                    if((time >= dataOptions.earlyTimeAsked) && (time <= dataOptions.lateTimeAsked)) {
                        unavailabilitySpot = timenameRegEx.match(field).hasMatch();
                    }
                }
            }
        }
    }
    if(!dataOptions.dayNames.isEmpty()) {
        availabilityChart = QObject::tr("Availability:");
        availabilityChart += "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";
        for(int day = 0; day < numDays; day++) {
            availabilityChart += "<th>" + dataOptions.dayNames.at(day).toUtf8().left(3) + "</th>";   // using first 3 characters in day name as abbreviation
        }
        availabilityChart += "</tr>";
        for(int time = 0; time < numTimes; time++) {
            availabilityChart += "<tr><th>" + dataOptions.timeNames.at(time).toUtf8() + "</th>";
            for(int day = 0; day < numDays; day++) {
                availabilityChart += QString(unavailable[day][time]?
                            "<td align = center> </td>" : "<td align = center bgcolor='PaleGreen'><b>√</b></td>");
            }
            availabilityChart += "</tr>";
        }
        availabilityChart += "</table>";
    }
    ambiguousSchedule = (availabilityChart.count("√") == 0 || int(availabilityChart.count("√")) == (numDays * numTimes));

    // section
    if(dataOptions.sectionIncluded) {
        QString sectionText = QObject::tr("section");
        fieldnum = dataOptions.sectionField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            section = fields.at(fieldnum).toUtf8().trimmed();
            if(section.startsWith(sectionText, Qt::CaseInsensitive)) {
                section = section.right(section.size() - sectionText.size()).trimmed();    //removing as redundant the word "section" if at the start of the section name
            }
        }
    }

    // preferred teammates
    for(int prefQ = 0; prefQ < dataOptions.numPrefTeammateQuestions; prefQ++) {
        fieldnum = dataOptions.prefTeammatesField[prefQ];
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString nextTeammate = fields.at(fieldnum).toLatin1();
            static QRegularExpression nameSeparators(R"(\s*([,;&]|(?:\sand\s))\s*)");
            nextTeammate.replace(nameSeparators, "\n");     // replace every [, ; & and] with new line
            nextTeammate = nextTeammate.trimmed();
            if(!prefTeammates.isEmpty() && !nextTeammate.isEmpty()) {
                prefTeammates += "\n" + nextTeammate;
            }
            else if(prefTeammates.isEmpty() && !nextTeammate.isEmpty()) {
                prefTeammates += nextTeammate;
            }
        }
    }

    // preferred non-teammates
    for(int prefQ = 0; prefQ < dataOptions.numPrefNonTeammateQuestions; prefQ++) {
        fieldnum = dataOptions.prefNonTeammatesField[prefQ];
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString nextTeammate = fields.at(fieldnum).toLatin1();
            static QRegularExpression nameSeparators(R"(\s*([,;&]|(?:\sand\s))\s*)");
            nextTeammate.replace(nameSeparators, "\n");     // replace every [, ; & and] with new line
            nextTeammate = nextTeammate.trimmed();
            if(!prefNonTeammates.isEmpty() && !nextTeammate.isEmpty()) {
                prefNonTeammates += "\n" + nextTeammate;
            }
            else if(prefNonTeammates.isEmpty() && !nextTeammate.isEmpty()) {
                prefNonTeammates += nextTeammate;
            }
        }
    }

    // notes
    for(int note = 0; note < dataOptions.numNotes; note++) {
        // join each one with a newline after
        fieldnum = dataOptions.notesField[note];
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString nextNote = fields.at(fieldnum).toLatin1().trimmed();
            if(!notes.isEmpty() && !nextNote.isEmpty()) {
                notes += "\n" + nextNote;
            }
            else if(notes.isEmpty() && !nextNote.isEmpty()) {
                notes += nextNote;
            }
        }
    }
}


////////////////////////////////////////////
// Create a tooltip for a student
////////////////////////////////////////////
void StudentRecord::createTooltip(const DataOptions &dataOptions)
{
    QString toolTip = "<html>";
    if(duplicateRecord)
    {
        toolTip += "<table><tr><td bgcolor=" STARFISHHEX "><b>" + QObject::tr("There appears to be multiple survey submissions from this student!") + "</b></td></tr></table><br>";
    }
    toolTip += firstname + " " + lastname;
    if(dataOptions.emailField != -1)
    {
        toolTip += "<br>" + email;
    }
    if(dataOptions.genderIncluded)
    {
        toolTip += "<br>";
        QStringList genderOptions;
        if(dataOptions.genderType == GenderType::biol)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions.genderType == GenderType::adult)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions.genderType == GenderType::child)
        {
            toolTip += QObject::tr("Gender");
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else //if(dataOptions.genderType == GenderType::pronoun)
        {
            toolTip += QObject::tr("Pronouns");
            genderOptions = QString(PRONOUNS).split('/');
        }
        toolTip += ":  " + genderOptions.at(static_cast<int>(gender));
    }
    if(dataOptions.URMIncluded)
    {
        toolTip += "<br>" + QObject::tr("Identity") + ":  ";
        toolTip += URMResponse;
    }
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
    {
        if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone)
        {
            continue;
        }
        toolTip += "<br>" + QObject::tr("Attribute ") + QString::number(attribute + 1) + ":  ";
        auto value = attributeVals[attribute].constBegin();
        if(*value != -1)
        {
            if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::ordered)
            {
                toolTip += QString::number(*value);
            }
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::categorical)
            {
                // if attribute value is > 26, letters are repeated as needed
                toolTip += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) :
                                             QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
            }
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::multicategorical)
            {
                const auto lastVal = attributeVals[attribute].constEnd();
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
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::multiordered)
            {
                const auto lastVal = attributeVals[attribute].constEnd();
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
    if(dataOptions.timezoneIncluded)
    {
        toolTip += "<br>" + QObject::tr("Timezone:  ");
        //find the timezone as attribute value so that -1 can show as unknown timezone
        for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++)
        {
            if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone)
            {
                if(*attributeVals[attribute].constBegin() != -1)
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
    if(dataOptions.prefTeammatesIncluded)
    {
        QString note = prefTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(dataOptions.prefNonTeammatesIncluded)
    {
        QString note = prefNonTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Non-teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(dataOptions.numNotes > 0)
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

QJsonObject StudentRecord::toJson() const
{
    QJsonArray unavailableArray, preventedWithArray, requiredWithArray, requestedWithArray, attributeValsArray, attributeResponseArray;
    for(int i = 0; i < MAX_DAYS; i++) {
        QJsonArray unavailableArraySubArray;
        for(int j = 0; j < MAX_BLOCKS_PER_DAY; j++) {
            unavailableArraySubArray.append(unavailable[i][j]);
        }
        unavailableArray.append(unavailableArraySubArray);
    }
    for(int i = 0; i < MAX_IDS; i++) {
        preventedWithArray.append(preventedWith[i]);
        requiredWithArray.append(requiredWith[i]);
        requestedWithArray.append(requestedWith[i]);
    }
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeResponseArray.append(attributeResponse[i]);
        QJsonArray attributeValsArraySubArray;
        for (const auto &val : attributeVals[i]) {
            attributeValsArraySubArray.append(val);
        }
        attributeValsArray.append(attributeValsArraySubArray);
    }

    QJsonObject content {
        {"ID", ID},
        {"LMSID", LMSID},
        {"duplicateRecord", duplicateRecord},
        {"gender", static_cast<int>(gender)},
        {"URM", URM},
        {"unavailable", unavailableArray},
        {"timezone", timezone},
        {"ambiguousSchedule", ambiguousSchedule},
        {"preventedWith", preventedWithArray},
        {"requiredWith", requiredWithArray},
        {"requestedWith", requestedWithArray},
        {"attributeVals", attributeValsArray},
        {"surveyTimestamp", surveyTimestamp.toString(Qt::ISODate)},
        {"firstname", firstname},
        {"lastname", lastname},
        {"email", email},
        {"section", section},
        {"prefTeammates", prefTeammates},
        {"prefNonTeammates", prefNonTeammates},
        {"notes", notes},
        {"attributeResponse", attributeResponseArray},
        {"URMResponse", URMResponse},
        {"availabilityChart", availabilityChart},
        {"tooltip", tooltip}
    };

    return content;
}
