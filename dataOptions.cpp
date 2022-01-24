#include "dataOptions.h"

DataOptions::DataOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        attributeField[i] = -1;
        attributeVals[i].clear();
        attributeQuestionResponseCounts[i].clear();
        attributeType[i] = categorical;
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

void DataOptions::reset()
{
    latestStudentID = 0;
    numStudentsInSystem = 0;
    numAttributes = 0;
    attributeQuestionText.clear();
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        attributeQuestionResponses[i].clear();
        attributeQuestionResponseCounts[i].clear();
        attributeVals[i].clear();
    }
    dayNames.clear();
    timeNames.clear();
}
