#ifndef GA_H
#define GA_H

#include <utility>

// Code related to the Genetic Algorithm used in gruepr
// Could probably be converted to a namespace instead of a class (all functions static, no instance data)

class GA
{
public:
    static void clone(const int *const parent, const int *const ancestors, const int parentsIndex,
                      int child[], int parentage[], const int genomeSize);

    static void tournamentSelectParents(const int *const *const genePool, const int *const orderedIndex, const int *const *const ancestors,
                                        const int *&mom, const int *&dad, int parentage[]);

    static void mate(const int *const mom, const int *const dad, const int teamStartPositions[],
                     const int numTeams, int child[], const long long genomeSize);
    static void crossover(const int *const mom, const int *const dad, const unsigned int start, const unsigned int end,
                          int child[], const long long genomeSize);

    // Swaps a random position in [startA,endA) with a random position in [startB,endB) and returns
    // the two genome positions swapped, so a caller that needs to undo it can just std::swap them
    // back itself. A fully random, untargeted mutation is startA=startB=0, endA=endB=genomeSize;
    // confining (or splitting) the two regions to specific teams targets the mutation at just those
    // teams. Currently unused in production (the mutation step was removed after a sweep found it
    // hurt convergence in a constraint-heavy scenario
    static std::pair<int,int> mutate(int genome[], int startA, int endA, int startB, int endB);

    class GenePool {
    public:
        explicit GenePool(int populationSize, int genomeSize);
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
        explicit AncestorPool(int populationSize);
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

    static constexpr int MAX_RECORDS = 1000;            // maximum number of records to optimally partition (this might be changable, but algortihm gets pretty slow as value gets bigger)

    static constexpr int MIN_GENERATIONS = 30;          // will keep optimizing for at least minGenerations
    static constexpr int MAX_GENERATIONS = 1000;        // will keep optimizing for at most maxGenerations
    static constexpr int GENERATIONS_OF_STABILITY = 15; // after minGenerations, if score has not improved for generationsOfStability, stop optimizing
    static constexpr int MIN_SCORE_STABILITY = 100;     // will keep optimizing until scoreStability (current score divided by range of scores within generationsOfStability) exceeds this

    static constexpr int NUMGENERATIONSOFANCESTORS = 3; // how many generations of ancestors to look back when preventing the selection of related mates:
                                                        //    1 = prevent if either parent is same (no siblings mating);
                                                        //    2 = prevent if any parent or grandparent is same (no siblings or 1st cousins);
                                                        //    3 = prevent if any parent, grandparent, or greatgrandparent is same (no siblings, 1st or 2nd cousins); etc.

    static constexpr int POPULATIONSIZE = 50000;        // how many genomes are in the genepool

    static constexpr int NUM_ELITES = 3;                // From each generation, this many highest scoring genomes are directly cloned into the next generation.
                                                        //    Having at least 1 elite stabilizes the high score to end optimization

    static constexpr int TOURNAMENTSIZE = 30;           // Most of the next generation is created by mating pairs of parent genomes,
                                                        //   each time chosen from genomes in a randomly selected tournament in the genepool
    static constexpr int TOPGENOMELIKELIHOOD = 50;      // percent likelihood of selecting the best genome in the tournament as parent;
                                                        //    if top is not selected, move to next best genome with same probability, and so on
};


#endif // GA_H
