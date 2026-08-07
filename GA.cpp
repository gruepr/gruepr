#include "GA.h"
#include <algorithm>
#include <limits>
#include <random>
#include <vector>

namespace {
// One generator per thread, seeded on first use. std::mt19937 is not thread-safe, so a shared one
// would race; thread_local removes the sharing entirely and keeps the call sites free of any
// per-thread bookkeeping. Seeded from random_device, so the streams are not reproducible -- see the
// note in GA.h.
thread_local std::mt19937 threadRNG{std::random_device{}()};
}

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
void GA::clone(const int *const parent, const int *const ancestors, const int parentsIndex, int child[], int parentage[], const int genomeSize) const
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
                                 const int *&mom, const int *&dad, int parentage[]) const
{
    std::mt19937 &pRNG = threadRNG;
    std::uniform_int_distribution<unsigned int> randProbability(1, 100);
    std::uniform_int_distribution<unsigned int> randGenome(0, populationsize-1);

    int momsindex, dadsindex;
    bool failedTournament;  // tournament fails when can't find unrelated mom and dad
    do {
        failedTournament = false;
        momsindex = 0;
        dadsindex = 0;
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
void GA::mate(const int *const mom, const int *const dad, const int teamStartPositions[],
              const int numTeams, int child[], const long long genomeSize) const
{
    //randomly choose two team boundaries in the genome from which to cut an allele
    std::uniform_int_distribution<unsigned int> randTeam(0, numTeams);
    unsigned int startTeam = randTeam(threadRNG);
    unsigned int endTeam;
    do {
        endTeam = randTeam(threadRNG);
    }
    while(endTeam == startTeam);
    if(startTeam > endTeam) {
        std::swap(startTeam, endTeam);
    }

    //Now, need to find positions in genome to start and end allele--the "breaks" before startTeam and endTeam
    crossover(mom, dad, teamStartPositions[startTeam], teamStartPositions[endTeam], child, genomeSize);
}


//////////////////
// Ordered crossover with the cut points already chosen
//////////////////
void GA::crossover(const int *const mom, const int *const dad, const unsigned int start, const unsigned int end,
                   int child[], const long long genomeSize) const
{
    //copy mom's allele directly into the child at the same positions
    std::copy(mom + start, mom + end, child + start);

    //Mark every value in mom's allele in a lookup indexed by the value itself, so that testing one of
    //dad's values for membership below is a single array read.
    //Genome values are indices into the students array, which is sparse whenever students are deleted
    //or only one section is being teamed, so the lookup grows to the largest value actually present;
    //any value past the end is by definition not in the allele. The stamp increments once per call, so
    //the lookup never has to be cleared -- only reset in the (practically unreachable) overflow case.
    thread_local std::vector<int> valueIsInMomsAllele;
    thread_local int momsAlleleStamp = 0;
    if(momsAlleleStamp == std::numeric_limits<int>::max()) {
        std::fill(valueIsInMomsAllele.begin(), valueIsInMomsAllele.end(), 0);
        momsAlleleStamp = 0;
    }
    momsAlleleStamp++;
    for(unsigned int i = start; i < end; i++) {
        const int value = mom[i];
        if(value < 0) {
            continue;   //cannot happen for a well-formed genome, but must not be used as an index
        }
        if(value >= int(valueIsInMomsAllele.size())) {
            valueIsInMomsAllele.resize(size_t(value) + 1, 0);
        }
        valueIsInMomsAllele[value] = momsAlleleStamp;
    }

    //fill the rest of the child with dad's values, in dad's relative order, skipping over
    //mom's allele (already placed above) and any value dad has that's already in mom's allele
    long long writePos = 0;
    for(long long i = 0; i < genomeSize; i++) {
        if(writePos == start) {
            writePos = end;
        }
        const int value = dad[i];
        const bool inMomsAllele = (value >= 0) && (value < int(valueIsInMomsAllele.size())) &&
                                  (valueIsInMomsAllele[value] == momsAlleleStamp);
        if(!inMomsAllele) {
            child[writePos] = value;
            writePos++;
        }
    }
}


//////////////////
// Randomly swap two sites in given genome
//////////////////
void GA::mutate(int genome[], const long long genomeSize) const
{
    std::uniform_int_distribution<unsigned long long> randSite(0, genomeSize-1);
    std::swap(genome[randSite(threadRNG)], genome[randSite(threadRNG)]);
}

//////////////////
// Swap a random student from the worst-scoring team with a random student from any other team
//////////////////
void GA::mutateWorstTeam(int genome[], const int teamStartPositions[], const int worstTeam, const long long genomeSize) const
{
    std::mt19937 &pRNG = threadRNG;
    const int worstTeamStart = teamStartPositions[worstTeam];
    const int worstTeamEnd = teamStartPositions[worstTeam + 1];
    const int worstTeamSize = worstTeamEnd - worstTeamStart;

    // If the worst team is the only team, there is no other team to swap in to
    if(worstTeamSize >= genomeSize) {
        return;
    }

    // pick a random student in the worst team
    std::uniform_int_distribution<int> randWorstSite(worstTeamStart, worstTeamEnd - 1);
    const int siteA = randWorstSite(pRNG);

    // pick a random student NOT in the worst team
    std::uniform_int_distribution<long long> randOtherSite(0, genomeSize - worstTeamSize - 1);
    long long siteB = randOtherSite(pRNG);
    if(siteB >= worstTeamStart) {
        siteB += worstTeamSize;
    }

    std::swap(genome[siteA], genome[siteB]);
}


//////////////////
// GenePool RAII wrapper
//////////////////
GA::GenePool::GenePool(const GA &ga, int genomeSize)
    : popSize(ga.populationsize), genSize(genomeSize)
    , dataVals(new int[static_cast<size_t>(popSize) * genSize]), rows(new int*[popSize])
{
    for(int i = 0; i < popSize; ++i) {
        rows[i] = dataVals + (static_cast<size_t>(i) * genSize);
    }
}

GA::GenePool::~GenePool()
{
    delete[] rows;
    delete[] dataVals;
}

GA::GenePool::GenePool(GenePool &&o) noexcept
    : popSize(o.popSize), genSize(o.genSize)
    , dataVals(o.dataVals), rows(o.rows)
{
    o.popSize = 0;
    o.genSize = 0;
    o.dataVals = nullptr;
    o.rows = nullptr;
}

GA::GenePool &GA::GenePool::operator=(GenePool &&o) noexcept
{
    if(this != &o) {
        delete[] rows;
        delete[] dataVals;
        popSize = o.popSize;
        genSize = o.genSize;
        dataVals = o.dataVals;
        rows = o.rows;
        o.popSize = 0;
        o.genSize = 0;
        o.dataVals = nullptr;
        o.rows = nullptr;
    }
    return *this;
}

int *GA::GenePool::operator[](int genome) { return rows[genome]; }
const int *GA::GenePool::operator[](int genome) const { return rows[genome]; }
int **GA::GenePool::data() { return rows; }
const int *const *GA::GenePool::data() const { return rows; }
int GA::GenePool::populationSize() const { return popSize; }
int GA::GenePool::genomeSize() const { return genSize; }

void swap(GA::GenePool &a, GA::GenePool &b) noexcept
{
    std::swap(a.popSize, b.popSize);
    std::swap(a.genSize, b.genSize);
    std::swap(a.dataVals, b.dataVals);
    std::swap(a.rows, b.rows);
}


//////////////////
// AncestorPool RAII wrapper
//////////////////
GA::AncestorPool::AncestorPool(const GA &ga)
    : popSize(ga.populationsize), numAncest(2)   // always track mom & dad
    , dataVals(nullptr), rows(nullptr)
{
    for(int generation = 0; generation < ga.numgenerationsofancestors; ++generation) {
        numAncest += (4 << generation);   // add 2^(n+1) for each level of (great)grandparents
    }
    dataVals = new int[static_cast<size_t>(popSize) * numAncest];
    rows = new int*[popSize];
    for(int i = 0; i < popSize; ++i) {
        rows[i] = dataVals + (static_cast<size_t>(i) * numAncest);
    }
}

GA::AncestorPool::~AncestorPool()
{
    delete[] rows;
    delete[] dataVals;
}

GA::AncestorPool::AncestorPool(AncestorPool &&o) noexcept
    : popSize(o.popSize), numAncest(o.numAncest)
    , dataVals(o.dataVals), rows(o.rows)
{
    o.popSize = 0;
    o.numAncest = 0;
    o.dataVals = nullptr;
    o.rows = nullptr;
}

GA::AncestorPool &GA::AncestorPool::operator=(AncestorPool &&o) noexcept
{
    if(this != &o) {
        delete[] rows;
        delete[] dataVals;
        popSize = o.popSize;
        numAncest = o.numAncest;
        dataVals = o.dataVals;
        rows = o.rows;
        o.popSize = 0;
        o.numAncest = 0;
        o.dataVals = nullptr;
        o.rows = nullptr;
    }
    return *this;
}

int *GA::AncestorPool::operator[](int genome) { return rows[genome]; }
const int *GA::AncestorPool::operator[](int genome) const { return rows[genome]; }
int **GA::AncestorPool::data() { return rows; }
const int *const *GA::AncestorPool::data() const { return rows; }
int GA::AncestorPool::populationSize() const { return popSize; }
int GA::AncestorPool::numAncestors() const { return numAncest; }

void swap(GA::AncestorPool &a, GA::AncestorPool &b) noexcept
{
    std::swap(a.popSize, b.popSize);
    std::swap(a.numAncest, b.numAncest);
    std::swap(a.dataVals, b.dataVals);
    std::swap(a.rows, b.rows);
}
