#include <algorithm>
#include "GA.h"

//////////////////
// Select two parents from the genepool using tournament selection
//////////////////
void GA::tournamentSelectParents(tourneyPlayer *players, int **genePool, const float scores[], int **ancestors, int *&mom, int *&dad, int parentage[])
{
    //get tournamentSize random values from 0 -> populationSize and get those index-valued genePool genomes and scores into players[]
    for(int player = 0; player < tournamentSize; player++)
    {
        int tourneyPick = rand()%populationSize;
        players[player].genome = &genePool[tourneyPick][0];
        players[player].score = scores[tourneyPick];
        players[player].ancestors = &ancestors[tourneyPick][0];
        players[player].ID = tourneyPick;
    }

    //sort tournament genomes so top genomes in tournament are at the beginning
    std::sort(players, players+tournamentSize, [](tourneyPlayer i,tourneyPlayer j){return i.score>j.score;});

    //pick first genome from tournament, most likely from the beginning so that best genomes are more likely have offspring
    int momsindex = 0;
    //choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on
    while(rand() > topGenomeLikelihood)
    {
        momsindex++;
    }

    //pick second genome from tournament in same way, but make sure to not pick the same genome
    int dadsindex = 0;
    while((rand() > topGenomeLikelihood) || (dadsindex == momsindex))
    {
        dadsindex++;
    }

    //make sure partners do not have any common ancestors going back numGenerationsOfAncestors generations
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
                    potentialMatesAreRelated |= (players[momsindex%tournamentSize].ancestors[momsAncestor] == players[dadsindex%tournamentSize].ancestors[dadsAncestor]);
                }
            }
            startAncestor = endAncestor;
            endAncestor += (4<<generation);     //add 2^(n+1)
        }
        dadsindex++;
    }
    while(potentialMatesAreRelated);
    dadsindex--;    //need to subtract off that last increment

    //return the selected genomes into mom and dad
    //using play%tournamentSize to wrap around from end of tournament back to the beginning, just in case
    mom = players[momsindex%tournamentSize].genome;
    dad = players[dadsindex%tournamentSize].genome;

    //return the parentage info
    parentage[0] = players[momsindex%tournamentSize].ID; //mom
    parentage[1] = players[dadsindex%tournamentSize].ID; //dad
    int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;
    for(int generation = 1; generation < numGenerationsOfAncestors; generation++)
    {
        for(int ancestor = startAncestor; ancestor < (((endAncestor - startAncestor)/2) + startAncestor); ancestor++)
        {
            parentage[ancestor] = players[momsindex%tournamentSize].ancestors[ancestor-startAncestor+prevStartAncestor];
        }
        for(int ancestor = (((endAncestor - startAncestor)/2) + startAncestor); ancestor < endAncestor; ancestor++)
        {
            parentage[ancestor] = players[dadsindex%tournamentSize].ancestors[ancestor-(((endAncestor - startAncestor)/2) + startAncestor)+prevStartAncestor];
        }
        prevStartAncestor = startAncestor;
        startAncestor = endAncestor;
        endAncestor += (4<<generation);     //add 2^(n+1)
    }
}


//////////////////
// Use ordered crossover to make child from mom and dad, splitting at random team boundaries within the genome
//////////////////
void GA::mate(int *mom, int *dad, const int teamSize[], int numTeams, int child[], int genomeSize)
{
    //randomly choose two team boundaries in the genome from which to cut an allele
    int endTeam = 1+rand()%numTeams, startTeam = rand()%endTeam;	//endTeam is between 1 and number of teams, startTeam is between 0 and endTeam-1

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

    return;
}


//////////////////
// Randomly swap two sites in given genome
//////////////////
void GA::mutate(int genome[], int genomeSize)
{
    int site1, site2;
    int greatestRandMultiple = RAND_MAX - (RAND_MAX % genomeSize);      // Holds the largest rand() that is a multiple of genomeSize, so that we get a uniform value 0->genomeSize
    do
    {
        site1 = rand();
    }
    while(site1 >= greatestRandMultiple);
    do
    {
        site2 = rand();
    }
    while(site2 >= greatestRandMultiple);

    std::swap(genome[site1%genomeSize], genome[site2%genomeSize]);

    return;
}
