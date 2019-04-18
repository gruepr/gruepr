#include <algorithm>
#include "GA.h"


//////////////////
// Use ordered crossover to make child from mom and dad, splitting at random team boundaries within the genome
//////////////////
void GA::mate(int mom[], int dad[], int teamSize[], int numTeams, int child[], int genomeSize)
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
        std::remove(child, child + genomeSize, mom[start+i]);
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
    do
    {
        site1 = rand();
    }
    while(site1 >= genomeSize);
    do
    {
        site2 = rand();
    }
    while((site2 >= genomeSize) || (site2 == site1));

    std::swap(genome[site1], genome[site2]);

    return;
}
