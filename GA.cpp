#include "GA.h"
#include <algorithm>

//////////////////
// Select two parents from the genepool using tournament selection
//////////////////
void GA::tournamentSelectParents(int **genePool, const int *orderedIndex, int **ancestors, int *&mom, int *&dad, int parentage[], std::mt19937 &pRNG)
{
    std::uniform_int_distribution<unsigned int> randProbability(1, 100);
    std::uniform_int_distribution<unsigned int> randGenome(0, populationSize-1);

    //get tournamentSize random values in the range 0 -> populationSize-1 and then sort them
    //these represent ordinal genome within the genepool (i.e., 0 = top scoring genome in genepool, 1 = 2nd highest scoring genome in genepool)
    int tourneyPick[tournamentSize];
    for(int &player : tourneyPick)
    {
        player = randGenome(pRNG);
    }
    std::sort(tourneyPick, tourneyPick+tournamentSize);

    //pick first genome from tournament, most likely from the beginning so that best genomes are more likely have offspring
    //for now, index represent which ordinal genome from the tournament is selected (i.e., 0 = top scoring genome in tournament, 1 = 2nd highest scoring genome in tournament)
    int momsindex = 0;
    //choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on
    while(randProbability(pRNG) > topGenomeLikelihood)
    {
        momsindex++;
    }

    //pick second genome from tournament in same way, but make sure to not pick the same genome
    int dadsindex = 0;
    while((randProbability(pRNG) > topGenomeLikelihood) || (dadsindex == momsindex))
    {
        dadsindex++;
    }

    //now make sure partners do not have any common ancestors going back numGenerationsOfAncestors generations
    bool potentialMatesAreRelated;
    do
    {
        potentialMatesAreRelated = false;
        int startAncestor = 0, endAncestor = 2;
        for(int generation = 0; generation < numGenerationsOfAncestors; generation++)
        {
            for(int momsAncestor = startAncestor; momsAncestor < endAncestor; momsAncestor++)
            {
                for(int dadsAncestor = startAncestor; dadsAncestor < endAncestor; dadsAncestor++)
                {
                    potentialMatesAreRelated |=
                            (ancestors[orderedIndex[tourneyPick[momsindex%tournamentSize]]][momsAncestor] == ancestors[orderedIndex[tourneyPick[dadsindex%tournamentSize]]][dadsAncestor]);
                }
            }
            startAncestor = endAncestor;
            endAncestor += (4<<generation);     //add 2^(n+1)
        }
        dadsindex++;
    }
    while(potentialMatesAreRelated);
    dadsindex--;    //need to subtract off that last increment

    //convert indexes from ordinal value within tournament to index within the genepool
    //using play%tournamentSize to wrap around from end of tournament back to the beginning, just in case
    momsindex = orderedIndex[tourneyPick[momsindex%tournamentSize]];
    dadsindex = orderedIndex[tourneyPick[dadsindex%tournamentSize]];

    //return the selected genomes into mom and dad
    mom = genePool[momsindex];
    dad = genePool[dadsindex];

    //return the parentage info
    parentage[0] = momsindex; //mom
    parentage[1] = dadsindex; //dad
    int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;
    for(int generation = 1; generation < numGenerationsOfAncestors; generation++)
    {
        //for each generation, put mom's ancestors then dad's ancestors into the parentage array one generation up
        for(int ancestor = startAncestor; ancestor < (((endAncestor - startAncestor)/2) + startAncestor); ancestor++)
        {
            parentage[ancestor] = ancestors[momsindex][ancestor-startAncestor+prevStartAncestor];
        }
        for(int ancestor = (((endAncestor - startAncestor)/2) + startAncestor); ancestor < endAncestor; ancestor++)
        {
            parentage[ancestor] = ancestors[dadsindex][ancestor-(((endAncestor - startAncestor)/2) + startAncestor)+prevStartAncestor];
        }
        prevStartAncestor = startAncestor;
        startAncestor = endAncestor;
        endAncestor += (4<<generation);     //add 2^(n+1)
    }
}


//////////////////
// Use ordered crossover to make child from mom and dad, splitting at random team boundaries within the genome
//////////////////
void GA::mate(int *mom, int *dad, const int teamSize[], int numTeams, int child[], int genomeSize, std::mt19937 &pRNG)
{

    //randomly choose two team boundaries in the genome from which to cut an allele
    std::uniform_int_distribution<unsigned int> randTeam(0, numTeams);
    int startTeam = randTeam(pRNG);
    int endTeam;
    do
    {
        endTeam = randTeam(pRNG);
    }
    while(endTeam == startTeam);

    //Now, need to find positions in genome to start and end allele--the "breaks" before startTeam and endTeam
    int end=0, start=0, team=0, position=0;
    while(team < endTeam)
    {
        if(startTeam == team)
        {
            start = position;
        }
        //increase position by number of students in this team
        position += teamSize[team];
        end = position;
        //go to next team
        team++;
    }

    //copy all of dad into child
    std::copy(dad, dad + genomeSize, child);

    //remove from the child each value in mom's allele
    for(int i = 0; i < (end-start); i++)
    {
        (void)std::remove(child, child + genomeSize, mom[start+i]);
    }

    //make room for mom's allele
    std::move_backward(child + start, child + start + genomeSize - end, child + genomeSize);

    //copy mom's allele into child
    std::copy(mom + start, mom + end, child + start);
}


//////////////////
// Randomly swap two sites in given genome
//////////////////
void GA::mutate(int genome[], int genomeSize, std::mt19937 &pRNG)
{
    std::uniform_int_distribution<unsigned int> randSite(0, genomeSize-1);
    std::swap(genome[randSite(pRNG)], genome[randSite(pRNG)]);
}
