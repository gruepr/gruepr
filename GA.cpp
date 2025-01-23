#include "GA.h"
#include <algorithm>

void GA::setGAParameters(int numRecords)
{
    int threshold = 0;
    for(const auto val : GENOMESIZETHRESHOLD) {
        if(numRecords < val) {
            populationsize = POPULATIONSIZE[threshold];
            topgenomelikelihood = TOPGENOMELIKELIHOOD[threshold];
            numgenerationsofancestors = NUMGENERATIONSOFANCESTORS[threshold];
            mutationlikelihood = MUTATIONLIKELIHOOD[threshold];
            break;
        }
        threshold++;
    }
}


//////////////////
// Clone one parent from the genepool into new genepool
//////////////////
void GA::clone(const int *const parent, const int *const ancestors, const int parentsIndex, int child[], int parentage[], const int genomeSize)
{
    for(int ID = 0; ID < genomeSize; ID++) {
        child[ID] = parent[ID];
    }
    const auto &nextGenAncestor = parentage;
    const auto &thisGenAncestor = ancestors;
    nextGenAncestor[0] = nextGenAncestor[1] = parentsIndex;   // both parents are this genome
    int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;  // parents are 0 & 1, so grandparents are 2, 3, 4, & 5
    for(int generation = 1; generation < numgenerationsofancestors; generation++) {
        //all four grandparents are this genome's parents, etc. for increasing generations
        for(int ancestor = startAncestor; ancestor < (((endAncestor - startAncestor)/2) + startAncestor); ancestor++) {
            nextGenAncestor[ancestor] = thisGenAncestor[ancestor-startAncestor+prevStartAncestor];
        }
        for(int ancestor = (((endAncestor - startAncestor)/2) + startAncestor); ancestor < endAncestor; ancestor++) {
            nextGenAncestor[ancestor] = thisGenAncestor[ancestor-(((endAncestor - startAncestor)/2) + startAncestor)+prevStartAncestor];
        }
        prevStartAncestor = startAncestor;
        startAncestor = endAncestor;
        endAncestor += (4<<generation);     //add 2^(n+1)
    }
}


//////////////////
// Select two parents from the genepool using tournament selection
//////////////////
void GA::tournamentSelectParents(const int *const *const genePool, const int *const orderedIndex, const int *const *const ancestors,
                                 const int *&mom, const int *&dad, int parentage[], std::mt19937 &pRNG)
{
    std::uniform_int_distribution<unsigned int> randProbability(1, 100);
    std::uniform_int_distribution<unsigned int> randGenome(0, populationsize-1);

    int momsindex = 0, dadsindex = 0;
    bool failedTournament;  // tournament fails when can't find unrelated mom and dad
    do {
        failedTournament = false;
        //get tournamentSize random values in the range 0 -> populationSize-1 and then sort them
        //these represent ordinal genome within the genepool (i.e., 0 = top scoring genome in genepool, 1 = 2nd highest scoring genome in genepool)
        unsigned int tourneyPick[TOURNAMENTSIZE];
        for(auto &player : tourneyPick) {
            player = randGenome(pRNG);
        }
        std::sort(tourneyPick, tourneyPick+TOURNAMENTSIZE);

        //pick first genome from tournament, most likely from the beginning so that best genomes are more likely have offspring
        //for now, index represent which ordinal genome from the tournament is selected (i.e., 0 = top scoring genome in tournament, 1 = 2nd highest scoring, etc.)
        //choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on
        while(randProbability(pRNG) > topgenomelikelihood) {
            momsindex++;
        }

        //pick second genome from tournament in same way, but make sure to not pick the same genome
        while((randProbability(pRNG) > topgenomelikelihood) || (dadsindex == momsindex)) {
            dadsindex++;
        }

        //convert momsindex from ordinal value within tournament to index within the genepool
        //using '%tournamentSize' to wrap around from end of tournament back to the beginning, just in case
        momsindex = orderedIndex[tourneyPick[momsindex % TOURNAMENTSIZE]];
        const auto &momsancestors = ancestors[momsindex];

        //now make sure partners do not have any common ancestors going back numgenerationsofancestors generations
        bool potentialMatesAreRelated;
        do {
            const auto &dadsancestors = ancestors[orderedIndex[tourneyPick[dadsindex % TOURNAMENTSIZE]]];
            potentialMatesAreRelated = false;
            int startAncestor = 0, endAncestor = 2;
            for(int generation = 0; generation < numgenerationsofancestors && !potentialMatesAreRelated; generation++) {
                for(int momsAncestorIndex = startAncestor; momsAncestorIndex < endAncestor && !potentialMatesAreRelated; momsAncestorIndex++) {
                    const auto &momsAncestor = momsancestors[momsAncestorIndex];
                    for(int dadsAncestorIndex = startAncestor; dadsAncestorIndex < endAncestor && !potentialMatesAreRelated; dadsAncestorIndex++) {
                        if(momsAncestor == dadsancestors[dadsAncestorIndex]) {
                            potentialMatesAreRelated = true;
                            dadsindex++;
                            if(dadsindex >= TOURNAMENTSIZE) {
                                failedTournament = true;
                            }
                        }
                    }
                }
                startAncestor = endAncestor;
                endAncestor += (4<<generation);     //add 2^(n+1)
            }
        } while(potentialMatesAreRelated && !failedTournament);

        //as done for momsindex before, convert dadsindex from ordinal value within tournament to index within the genepool
        dadsindex = orderedIndex[tourneyPick[dadsindex % TOURNAMENTSIZE]];
    } while(failedTournament);


    //return the selected genomes into mom and dad
    mom = genePool[momsindex];
    dad = genePool[dadsindex];

    //return the parentage info
    parentage[0] = momsindex; //mom
    parentage[1] = dadsindex; //dad
    auto &momsAncestors = ancestors[momsindex];
    auto &dadsAncestors = ancestors[dadsindex];
    int prevStartAncestor = 0, startAncestor = 2, endAncestor = 6;  // parents are 0 and 1, so grandparents are 2, 3, 4, 5
    for(int generation = 1; generation < numgenerationsofancestors; generation++) {
        //for each generation, put mom's ancestors then dad's ancestors into the parentage array one generation up
        for(int ancestor = startAncestor; ancestor < (((endAncestor - startAncestor)/2) + startAncestor); ancestor++) {
            parentage[ancestor] = momsAncestors[ancestor-startAncestor+prevStartAncestor];
        }
        for(int ancestor = (((endAncestor - startAncestor)/2) + startAncestor); ancestor < endAncestor; ancestor++) {
            parentage[ancestor] = dadsAncestors[ancestor-(((endAncestor - startAncestor)/2) + startAncestor)+prevStartAncestor];
        }
        prevStartAncestor = startAncestor;
        startAncestor = endAncestor;
        endAncestor += (4<<generation);     //add 2^(n+1)
    }
}


//////////////////
// Use ordered crossover to make child from mom and dad, splitting at random team boundaries within the genome
//////////////////
void GA::mate(const int *const mom, const int *const dad, const int teamSize[], const int numTeams, int child[], const long long genomeSize, std::mt19937 &pRNG)
{

    //randomly choose two team boundaries in the genome from which to cut an allele
    std::uniform_int_distribution<unsigned int> randTeam(0, numTeams);
    unsigned int startTeam = randTeam(pRNG);
    unsigned int endTeam;
    do {
        endTeam = randTeam(pRNG);
    }
    while(endTeam == startTeam);
    if(startTeam > endTeam) {
        std::swap(startTeam, endTeam);
    }

    //Now, need to find positions in genome to start and end allele--the "breaks" before startTeam and endTeam
    unsigned int end=0, start=0, team=0, position=0;
    while(team < endTeam) {
        if(startTeam == team) {
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
    for(unsigned int i = 0; i < (end-start); i++) {
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
void GA::mutate(int genome[], const long long genomeSize, std::mt19937 &pRNG)
{
    std::uniform_int_distribution<unsigned long long> randSite(0, genomeSize-1);
    std::swap(genome[randSite(pRNG)], genome[randSite(pRNG)]);
}
