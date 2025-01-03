#ifndef GA_H
#define GA_H

// Code related to the Genetic Algorithm used in gruepr

#include <random>

class GA
{
public:
    void setGAParameters(int numRecords);
    void tournamentSelectParents(const int * const * const genePool, const int *const orderedIndex, const int * const * const ancestors,
                                 const int *&mom, const int *&dad, int parentage[], std::mt19937 &pRNG);
    void mate(const int *const mom, const int *const dad, const int teamSize[], const int numTeams, int child[], const long long genomeSize, std::mt19937 &pRNG);
    void mutate(int genome[], const long long genomeSize, std::mt19937 &pRNG);

    inline static const int MAX_RECORDS = 1000;             // maximum number of records to optimally partition (this might be changable, but algortihm gets pretty slow as value gets bigger)

    inline static const int NUM_ELITES = 3;                 // from each generation, this many highest scoring genomes are directly cloned into the next generation. Some suggest elitism helps speed genetic algorithms, but can lead to premature convergence. Having at least 1 elite significantly stabilizes the high score to end optimization
    inline static const int TOURNAMENTSIZE = 100;           // most of the next generation is created by mating many pairs of parent genomes, each time chosen from genomes in a randomly selected tournament in the genepool
    inline static const int MIN_GENERATIONS = 40;           // will keep optimizing for at least minGenerations
    inline static const int MAX_GENERATIONS = 500;          // will keep optimizing for at most maxGenerations
    inline static const int GENERATIONS_OF_STABILITY = 25;  // after minGenerations, if score has not improved for generationsOfStability, stop optimizing
    inline static const int MIN_SCORE_STABILITY = 100;      // will keep optimizing until scoreStability (current score divided by range of scores within generationsOfStability) exceeds this

    // working values of algorithm constants, set when beginning an optimization and the genome size is known
    int populationsize = POPULATIONSIZE[3];
    unsigned int topgenomelikelihood = TOPGENOMELIKELIHOOD[3];
    int numgenerationsofancestors = NUMGENERATIONSOFANCESTORS[3];
    unsigned int mutationlikelihood = MUTATIONLIKELIHOOD[3];

private:
    static constexpr int POPULATIONSIZE[] = {60000, 45000, 20000, 10000};// the number of genomes in each generation--larger size is slower, but each generation is more likely to have optimal result.
    static constexpr int TOPGENOMELIKELIHOOD[] = {25, 33, 66, 100};      // percent likelihood of selecting the best genome in the tournament as parent; if top is not selected, move to next best genome with same probability, and so on
    static constexpr int NUMGENERATIONSOFANCESTORS[] = {3, 3, 3, 2};     // how many generations of ancestors to look back when preventing the selection of related mates:
                                                                         //      1 = prevent if either parent is same (no siblings mating);
                                                                         //      2 = prevent if any parent or grandparent is same (no siblings or 1st cousins);
                                                                         //      3 = prevent if any parent, grandparent, or greatgrandparent is same (no siblings, 1st or 2nd cousins); etc.
    static constexpr int MUTATIONLIKELIHOOD[] = {25, 50, 50, 75};        // percent likelihood of a mutation (when mutation occurs, another chance at mutation is given with same likelihood (iteratively))
    static constexpr int GENOMESIZETHRESHOLD[] = {30, 75, 200};          // threshold values of genome size that decide which working values of the constants to use -- if genome is <= threshold 1, use more diversity; if <= threshold 2, use medium diversity
                                                                         // when the genome size gets larger, the genomes are less similar and thus selecting non-top genomes is less advantageous to maintaining genomic diversity
};


#endif // GA_H
