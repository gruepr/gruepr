#include "teamingOptions.h"


TeamingOptions::TeamingOptions()
{
    // initialize all attribute weights to 1, desires to heterogeneous, and incompatible attribute values to none
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        desireHomogeneous[i] = false;
        attributeWeights[i] = 1;
        realAttributeWeights[i] = 1;
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
}


void TeamingOptions::reset()
{
    // reset the variables that depend on the datafile
    URMResponsesConsideredUR.clear();
    for(int i = 0; i < MAX_ATTRIBUTES; i++)
    {
        haveAnyIncompatibleAttributes[i] = false;
        incompatibleAttributeValues[i].clear();
    }
    haveAnyRequiredTeammates = false;
    haveAnyPreventedTeammates = false;
    haveAnyRequestedTeammates = false;
    sectionName.clear();
    teamsetNumber = 1;
}
