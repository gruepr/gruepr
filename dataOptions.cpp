#include "dataOptions.h"
#include <QRegularExpression>

DataOptions::DataOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        attributeField[i] = -1;
        attributeVals[i].clear();
        attributeQuestionResponseCounts[i].clear();
        attributeType[i] = AttributeType::categorical;
    }

    for(int &field : notesField)
    {
        field = -1;
    }

    for(int &field : scheduleField)
    {
        field = -1;
    }
}


bool DataOptions::parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &hours, float &minutes, float &offsetFromGMT)
{
    QRegularExpression timeZoneFinder(TIMEZONEREGEX);
    QRegularExpressionMatch timezoneCapture = timeZoneFinder.match(fullText);
    if(timezoneCapture.hasMatch())
    {
        timezoneName = timezoneCapture.captured(1).trimmed();
        hours = timezoneCapture.captured(2).toFloat();
        minutes = timezoneCapture.captured(3).toFloat();
        offsetFromGMT = hours + ((hours < 0)? (-minutes/60) : (minutes/60));
        return true;
    }
    return false;
}


bool DataOptions::parseTimezoneInfoFromText(const QString &fullText, QString &timezoneName, float &offsetFromGMT)
{
    float hours, minutes;
    return parseTimezoneInfoFromText(fullText, timezoneName, hours, minutes, offsetFromGMT);
}


