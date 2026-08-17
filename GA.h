#ifndef GA_H
#define GA_H

#include <utility>

// Code related to the Genetic Algorithm used in gruepr

class GA
{
public:
    void setGAParameters(int numRecords);

    // static: none of clone/mate/crossover touch any GA instance state (only parameters, static
    // class constants, and GA.cpp's own thread_local RNG) -- callable without a GA instance.
    static void clone(const int *const parent, const int *const ancestors, const int parentsIndex,
                      int child[], int parentage[], const int genomeSize);

    void tournamentSelectParents(const int *const *const genePool, const int *const orderedIndex, const int *const *const ancestors,
                                 const int *&mom, const int *&dad, int parentage[]) const;

    static void mate(const int *const mom, const int *const dad, const int teamStartPositions[],
                     const int numTeams, int child[], const long long genomeSize);
    static void crossover(const int *const mom, const int *const dad, const unsigned int start, const unsigned int end,
                          int child[], const long long genomeSize);

    // Swaps a random position in [startA,endA) with a random position in [startB,endB) and returns
    // the two genome positions swapped, so a caller that needs to undo it can just std::swap them
    // back itself. A fully random, untargeted mutation is startA=startB=0, endA=endB=genomeSize;
    // confining (or splitting) the two regions to specific teams targets the mutation at just those
    // teams. Static: doesn't touch any GA instance state, only its own thread_local RNG (see
    // GA.cpp) -- callable without a GA instance. Currently unused in production (the mutation step
    // was removed after this session's sweep found it hurt convergence in a constraint-heavy
    // scenario -- see gruepr-ga-decision-record-implementation memory) -- kept for future use.
    static std::pair<int,int> mutate(int genome[], int startA, int endA, int startB, int endB);

    class GenePool {
    public:
        GenePool(const GA &ga, int genomeSize);
        ~GenePool();

        GenePool(const GenePool &) = delete;
        GenePool &operator=(const GenePool &) = delete;
        GenePool(GenePool &&o) noexcept;
        GenePool &operator=(GenePool &&o) noexcept;

        int *operator[](int genome);
        const int *operator[](int genome) const;
        int **data();
        const int *const *data() const;

        int populationSize() const;
        int genomeSize() const;

        friend void swap(GenePool &a, GenePool &b) noexcept;

    private:
        int popSize = 0;
        int genSize = 0;
        int *dataVals = nullptr;
        int **rows = nullptr;
    };

    class AncestorPool {
    public:
        AncestorPool(const GA &ga);
        ~AncestorPool();

        AncestorPool(const AncestorPool &) = delete;
        AncestorPool &operator=(const AncestorPool &) = delete;
        AncestorPool(AncestorPool &&o) noexcept;
        AncestorPool &operator=(AncestorPool &&o) noexcept;

        int *operator[](int genome);
        const int *operator[](int genome) const;
        int **data();
        const int *const *data() const;

        int populationSize() const;
        int numAncestors() const;

        friend void swap(AncestorPool &a, AncestorPool &b) noexcept;

    private:
        int popSize = 0;
        int numAncest = 0;
        int *dataVals = nullptr;
        int **rows = nullptr;
    };

    inline static const int MAX_RECORDS = 1000;             // maximum number of records to optimally partition (this might be changable, but algortihm gets pretty slow as value gets bigger)

    inline static const int MIN_GENERATIONS = 40;           // will keep optimizing for at least minGenerations
    inline static const int MAX_GENERATIONS = 800;          // will keep optimizing for at most maxGenerations
    inline static const int GENERATIONS_OF_STABILITY = 25;  // after minGenerations, if score has not improved for generationsOfStability, stop optimizing
    inline static const int MIN_SCORE_STABILITY = 100;      // will keep optimizing until scoreStability (current score divided by range of scores within generationsOfStability) exceeds this

    inline static const int TOPGENOMELIKELIHOOD = 50;         // percent likelihood of selecting the best genome in the tournament as parent; if top is not selected, move to next best genome with same probability, and so on
    inline static const int NUMGENERATIONSOFANCESTORS = 3;     // how many generations of ancestors to look back when preventing the selection of related mates:
                                                                //      1 = prevent if either parent is same (no siblings mating);
                                                                //      2 = prevent if any parent or grandparent is same (no siblings or 1st cousins);
                                                                //      3 = prevent if any parent, grandparent, or greatgrandparent is same (no siblings, 1st or 2nd cousins); etc.
    inline static const int NUM_ELITES = 3;                 // from each generation, this many highest scoring genomes are directly cloned into the next generation. Some suggest elitism helps speed genetic algorithms, but can lead to premature convergence. Having at least 1 elite stabilizes the high score to end optimization

    // working value of the one algorithm constant that varies with class size, set when beginning an optimization and the genome size is known
    int populationsize = POPULATIONSIZE[1];

private:
    static constexpr int POPULATIONSIZE[] = {45000, 30000};  // the number of genomes in each generation--larger size is slower, but each generation is more likely to have optimal result
    static constexpr int GENOMESIZETHRESHOLD = 200;           // numRecords at or below this threshold use the larger population size; above it, the smaller one
    static constexpr int TOURNAMENTSIZE = 30;                 // most of the next generation is created by mating pairs of parent genomes, each time chosen from genomes in a randomly selected tournament in the genepool
};


#endif // GA_H
