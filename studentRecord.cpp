#include "studentRecord.h"
#include <QJsonArray>
#include <QLocale>
#include <QRegularExpression>

StudentRecord::StudentRecord()
{
    surveyTimestamp = QDateTime::currentDateTime();

    for(auto &day : unavailable) {
        for(auto &time : day) {
            time = true;
        }
    }
}

StudentRecord::StudentRecord(const QJsonObject &jsonStudentRecord)
{
    deleted = jsonStudentRecord["deleted"].toBool();
    ID = jsonStudentRecord["ID"].toInt();
    LMSID = jsonStudentRecord["LMSID"].toInt();
    duplicateRecord = jsonStudentRecord["duplicateRecord"].toBool();
    if(jsonStudentRecord["genders"].type() != QJsonValue::Undefined) {
        const QJsonArray gendersArray = jsonStudentRecord["genders"].toArray();
        gender.clear();
        for (const auto &val : gendersArray) {
            gender << static_cast<Gender>(val.toInt());
        }
    }
    else {
        // this is for backwards compatability--studentRecord formerly saved a single gender
        gender = {static_cast<Gender>(jsonStudentRecord["gender"].toInt())};
    }
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
    const QJsonArray unavailableArray = jsonStudentRecord["unavailable"].toArray();;
    for(int i = 0; i < MAX_DAYS; i++) {
        QJsonArray unavailableArraySubArray = unavailableArray[i].toArray();
        for(int j = 0; j < MAX_BLOCKS_PER_DAY; j++) {
            unavailable[i][j] = unavailableArraySubArray[j].toBool();
        }
    }

    if(jsonStudentRecord["preventedWithIDs"].type() != QJsonValue::Undefined) {
        const QJsonArray preventedWithIDsArray = jsonStudentRecord["preventedWithIDs"].toArray();
        for (const auto &val : preventedWithIDsArray) {
            preventedWith << val.toInteger();
        }
    }
    else {
        // this is for backwards compatability--studentRecord formerly saved a bool array of all possible IDs.
        // In order to use work saved from prev. versions of gruepr, now must convert these indexes to the IDs
        const QJsonArray preventedWithArray = jsonStudentRecord["preventedWith"].toArray();
        const long long MAX_IDS = 2 * MAX_STUDENTS;             // since students can be removed and added yet IDs always increase, need more IDs than possible students
        for(int i = 0; i < MAX_IDS; i++) {
            if(preventedWithArray[i].toBool()) {
                preventedWith << i;
            }
        }
    }

    if(jsonStudentRecord["requiredWithIDs"].type() != QJsonValue::Undefined) {
        const QJsonArray requiredWithIDsArray = jsonStudentRecord["requiredWithIDs"].toArray();
        for (const auto &val : requiredWithIDsArray) {
            requiredWith << val.toInteger();
        }
    }
    else {
        // this is for backwards compatability--studentRecord formerly saved a bool array of all possible IDs.
        // In order to use work saved from prev. versions of gruepr, now must convert these indexes to the IDs
        const QJsonArray requiredWithArray = jsonStudentRecord["requiredWith"].toArray();
        const long long MAX_IDS = 2 * MAX_STUDENTS;             // since students can be removed and added yet IDs always increase, need more IDs than possible students
        for(int i = 0; i < MAX_IDS; i++) {
            if(requiredWithArray[i].toBool()) {
                requiredWith << i;
            }
        }
    }

    if(jsonStudentRecord["requestedWithIDs"].type() != QJsonValue::Undefined) {
        const QJsonArray requestedWithIDsArray = jsonStudentRecord["requestedWithIDs"].toArray();
        for (const auto &val : requestedWithIDsArray) {
            requestedWith << val.toInteger();
        }
    }
    else {
        // this is for backwards compatability--teamRecord formerly saved the students as their indexes in the students array rather than IDs
        // In order to use work saved from prev. versions of gruepr, now must convert these indexes to the IDs
        const QJsonArray requestedWithArray = jsonStudentRecord["requestedWith"].toArray();
        const long long MAX_IDS = 2 * MAX_STUDENTS;             // since students can be removed and added yet IDs always increase, need more IDs than possible students
        for(int i = 0; i < MAX_IDS; i++) {
            if(requestedWithArray[i].toBool()) {
                requestedWith << i;
            }
        }
    }

    const QJsonArray attributeValsArray = jsonStudentRecord["attributeVals"].toArray();
    const QJsonArray attributeResponseArray = jsonStudentRecord["attributeResponse"].toArray();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeResponse[i] = attributeResponseArray[i].toString();
        const QJsonArray attributeValsArraySubArray = attributeValsArray[i].toArray();
        for (const auto &val : attributeValsArraySubArray) {
            attributeVals[i] << val.toInt();
        }
    }
}

void StudentRecord::clear() {
    deleted = false;
    ID = -1;
    LMSID = -1;
    duplicateRecord = false;
    gender = {Gender::unknown};
    URM = false;
    for(auto &day : unavailable) {
        for(auto &time : day) {
            time = true;
        }
    }
    timezone = 0;
    ambiguousSchedule = false;
    preventedWith.clear();
    requiredWith.clear();
    requestedWith.clear();
    for(int i = 0; i < MAX_ATTRIBUTES; i++) {
        attributeVals[i].clear();
        attributeResponse[i].clear();
    }
    surveyTimestamp = QDateTime::currentDateTime();
    firstname.clear();
    lastname.clear();
    email.clear();
    section.clear();
    prefTeammates.clear();
    prefNonTeammates.clear();
    notes.clear();
    URMResponse.clear();
    availabilityChart.clear();
    tooltip.clear();
}

////////////////////////////////////////////
// Move fields read from file into student record values
////////////////////////////////////////////
void StudentRecord::parseRecordFromStringList(const QStringList &fields, const DataOptions &dataOptions)
{
    qDebug() << fields;
    const int numFields = fields.size();

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
                                    const QList<Qt::DateFormat> stdTimestampFormats = {Qt::TextDate, Qt::ISODate, Qt::ISODateWithMs, Qt::RFC2822Date};
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
        LMSID = fields.at(fieldnum).trimmed().toInt();
    }

    // First name
    fieldnum = dataOptions.firstNameField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        firstname = fields.at(fieldnum).trimmed();
        if(!firstname.isEmpty()) {
            firstname[0] = firstname[0].toUpper();
        }
    }

    // Last name
    fieldnum = dataOptions.lastNameField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        lastname = fields.at(fieldnum).trimmed();
        if(!lastname.isEmpty()) {
            lastname[0] = lastname[0].toUpper();
        }
    }

    // Email
    fieldnum = dataOptions.emailField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        email = fields.at(fieldnum).trimmed();
    }

    // gender
    if(dataOptions.genderIncluded) {
        gender.clear();
        QList<QStringList> genderOptions;
        genderOptions << QString(BIOLGENDERS).split('/');
        genderOptions << QString(ADULTGENDERS).split('/');
        genderOptions << QString(CHILDGENDERS).split('/');
        genderOptions << QString(PRONOUNS).split('/');
                fieldnum = dataOptions.genderField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            const QStringList field = fields.at(fieldnum).simplified().split(',');
            for(const auto &options : qAsConst(genderOptions)) {
                if(field.contains(options.at(static_cast<int>(Gender::woman)), Qt::CaseInsensitive)) {
                    gender << Gender::woman;
                }
                if(field.contains(options.at(static_cast<int>(Gender::nonbinary)), Qt::CaseInsensitive)) {
                    gender << Gender::nonbinary;
                }
                if(field.contains(options.at(static_cast<int>(Gender::man)), Qt::CaseInsensitive)) {
                    gender << Gender::man;
                }
            }
            if(gender.isEmpty()) {
                gender = {Gender::unknown};
            }
        }
        else {
            gender = {Gender::unknown};
        }
    }
    else {
        gender = {Gender::unknown};
    }

    // racial/ethnic heritage
    if(dataOptions.URMIncluded) {
        fieldnum = dataOptions.URMField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString field = fields.at(fieldnum).toLower().trimmed();
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
            QString field = fields.at(fieldnum).trimmed();
            field.replace("â€”","-");       // replace bad UTF-8 character representation of em-dash
            attributeResponse[attribute] = field;
        }
    }

    // schedule
    float timezoneOffset = 0;
    fieldnum = dataOptions.timezoneField;
    if((fieldnum >= 0) && (fieldnum < numFields)) {
        QString timezoneName;
        if(DataOptions::parseTimezoneInfoFromText(fields.at(fieldnum), timezoneName, timezone)) {
            if(dataOptions.homeTimezoneUsed) {
                timezoneOffset = dataOptions.baseTimezone - timezone;
            }
        }
    }
    const int numDays = int(dataOptions.dayNames.size());
    const int numTimes = int(dataOptions.timeNames.size());
    //build a map of the hour value for each timename (e.g., "9:15am" --> 9.25)
    QMap<float, QString> hoursForEachTimeName;
    for(const auto &timeName : dataOptions.timeNames) {
        hoursForEachTimeName[grueprGlobal::timeStringToHours(timeName)] = timeName;
    }
    int day = 0;
    for(const int fieldnum : dataOptions.scheduleField) {
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            const QString &field = fields.at(fieldnum);
            static QRegularExpression timenameRegEx("", QRegularExpression::CaseInsensitiveOption);
            for(const auto &timeName : dataOptions.timeNames) {
                const float time = grueprGlobal::timeStringToHours(timeName);
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
                while((actualtime > hoursForEachTimeName.key(dataOptions.timeNames.at(timeindex))) && (timeindex < numTimes)) {
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
        day++;
    }
    if(!dataOptions.dayNames.isEmpty()) {
        availabilityChart = QObject::tr("Availability:");
        availabilityChart += "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";
        for(int day = 0; day < numDays; day++) {
            availabilityChart += "<th>" + dataOptions.dayNames.at(day).left(3) + "</th>";   // using first 3 characters in day name as abbreviation
        }
        availabilityChart += "</tr>";
        for(int time = 0; time < numTimes; time++) {
            availabilityChart += "<tr><th>" + dataOptions.timeNames.at(time) + "</th>";
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
        const QString sectionText = QObject::tr("section");
        fieldnum = dataOptions.sectionField;
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            section = fields.at(fieldnum).trimmed();
            if(section.startsWith(sectionText, Qt::CaseInsensitive)) {
                section = section.right(section.size() - sectionText.size()).trimmed();    //removing redundant "section" if at the start of the section name
            }
        }
    }

    // preferred teammates
    for(const int fieldnum : dataOptions.prefTeammatesField) {
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString nextTeammate = fields.at(fieldnum);
            static const QRegularExpression nameSeparators(R"(\s*([,;&]|(?:\sand\s))\s*)");
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
    for(const int fieldnum : dataOptions.prefNonTeammatesField) {
        if((fieldnum >= 0) && (fieldnum < numFields)) {
            QString nextTeammate = fields.at(fieldnum);
            static const QRegularExpression nameSeparators(R"(\s*([,;&]|(?:\sand\s))\s*)");
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
    for(const auto notesField : dataOptions.notesFields) {
        // join each one with a newline after
        if((notesField >= 0) && (notesField < numFields)) {
            const QString nextNote = fields.at(notesField).trimmed();
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
    if(duplicateRecord) {
        toolTip += "<table><tr><td bgcolor=" STARFISHHEX "><b>" +
                   QObject::tr("There appears to be multiple survey submissions from this student!") +
                   "</b></td></tr></table><br>";
    }
    toolTip += firstname + " " + lastname;
    if(dataOptions.emailField != DataOptions::FIELDNOTPRESENT) {
        toolTip += "<br>" + email;
    }
    if(dataOptions.genderIncluded) {
        toolTip += "<br>";
        QStringList genderOptions;
        if(dataOptions.genderType == GenderType::biol) {
            toolTip += QObject::tr("Gender") + ":  ";
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions.genderType == GenderType::adult) {
            toolTip += QObject::tr("Gender") + ":  ";
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions.genderType == GenderType::child) {
            toolTip += QObject::tr("Gender") + ":  ";
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else { //if(dataOptions.genderType == GenderType::pronoun)
            toolTip += QObject::tr("Pronouns") + ":  ";
            genderOptions = QString(PRONOUNS).split('/');
        }
        bool firstGender = true;
        for(const auto gen : qAsConst(gender)) {
            if(!firstGender) {
                toolTip += ", ";
            }
            toolTip += genderOptions.at(static_cast<int>(gen));
            firstGender = false;
        }
    }
    if(dataOptions.URMIncluded) {
        toolTip += "<br>" + QObject::tr("Identity") + ":  ";
        toolTip += URMResponse;
    }
    for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++) {
        if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone) {
            continue;
        }
        toolTip += "<br>" + QObject::tr("Multiple choice Q") + QString::number(attribute + 1) + ":  ";
        auto value = attributeVals[attribute].constBegin();
        if(*value != -1) {
            if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::ordered) {
                toolTip += QString::number(*value);
            }
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::categorical) {
                // if attribute value is > 26, letters are repeated as needed
                toolTip += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) :
                                             QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
            }
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                const auto lastVal = attributeVals[attribute].constEnd();
                while(value != lastVal) {
                    toolTip += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) :
                                                 QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
                    value++;
                    if(value != lastVal) {
                        toolTip += ",";
                    }
                }
            }
            else if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                const auto lastVal = attributeVals[attribute].constEnd();
                while(value != lastVal) {
                    toolTip += QString::number(*value);
                    value++;
                    if(value != lastVal) {
                        toolTip += ",";
                    }
                }
            }
        }
        else {
            toolTip += "?";
        }
    }
    if(dataOptions.timezoneIncluded) {
        toolTip += "<br>" + QObject::tr("Timezone:  ");
        //find the timezone as attribute value so that -1 can show as unknown timezone
        for(int attribute = 0; attribute < dataOptions.numAttributes; attribute++) {
            if(dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone) {
                if(*attributeVals[attribute].constBegin() != -1) {
                    const int hour = int(timezone);
                    const int minutes = 60*(timezone - int(timezone));
                    toolTip += QString("GMT%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs((minutes)), 2, 10, QChar('0'));
                }
                else {
                    toolTip += "?";
                }
            }
        }
    }
    if(!(availabilityChart.isEmpty())) {
        toolTip += "<br>--<br>" + availabilityChart;
    }
    if(!dataOptions.prefTeammatesField.empty()) {
        QString note = prefTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(!dataOptions.prefNonTeammatesField.empty()) {
        QString note = prefNonTeammates;
        toolTip += "<br>--<br>" + QObject::tr("Preferred Non-teammates") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    if(!dataOptions.notesFields.empty()) {
        QString note = notes;
        if(note.size() > SIZE_OF_NOTES_IN_TOOLTIP) {
            note = note.mid(0, (SIZE_OF_NOTES_IN_TOOLTIP - 3)) + "...";
        }
        toolTip += "<br>--<br>" + QObject::tr("Notes") + ":<br>" + (note.isEmpty()? ("<i>" + QObject::tr("none") + "</i>") : note.replace("\n","<br>"));
    }
    toolTip += "</html>";

    tooltip = toolTip;
}

QJsonObject StudentRecord::toJson() const
{
    QJsonArray gendersArray, unavailableArray, preventedWithArray, requiredWithArray, requestedWithArray, attributeValsArray, attributeResponseArray;
    for(const auto &unavailableDay : unavailable) {
        QJsonArray unavailableArraySubArray;
        for(const auto unavailableTime : unavailableDay) {
            unavailableArraySubArray.append(unavailableTime);
        }
        unavailableArray.append(unavailableArraySubArray);
    }
    for(const auto g : gender) {
        gendersArray.append(static_cast<int>(g));
    }
    for(const auto id : preventedWith) {
        preventedWithArray.append(id);
    }
    for(const auto id : requiredWith) {
        requiredWithArray.append(id);
    }
    for(const auto id : requestedWith) {
        requestedWithArray.append(id);
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
        {"deleted", deleted},
        {"ID", ID},
        {"LMSID", LMSID},
        {"duplicateRecord", duplicateRecord},
        {"genders", gendersArray},
        {"URM", URM},
        {"unavailable", unavailableArray},
        {"timezone", timezone},
        {"ambiguousSchedule", ambiguousSchedule},
        {"preventedWithIDs", preventedWithArray},
        {"requiredWithIDs", requiredWithArray},
        {"requestedWithIDs", requestedWithArray},
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
