#include "dataOptions.h"

DataOptions::DataOptions()
{
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        attributeField[i] = -1;
        attributeMin[i] = 1;
        attributeMax[i] = 1;
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
    for(auto &attributeQuestionResponse : attributeQuestionResponses)
    {
        attributeQuestionResponse.clear();
    }
    dayNames.clear();
    timeNames.clear();
}
