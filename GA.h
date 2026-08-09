#ifndef GA_H
#define GA_H

// Code related to the Genetic Algorithm used in gruepr

class GA
{
public:
    void setGAParameters(int numRecords);

    void clone(const int *const parent, const int *const ancestors, const int parentsIndex,
               int child[], int parentage[], const int genomeSize) const;

    void tournamentSelectParents(const int *const *const genePool, const int *const orderedIndex, const int *const *const ancestors,
                                 const int *&mom, const int *&dad, int parentage[]) const;

    void mate(const int *const mom, const int *const dad, const int teamStartPositions[],
              const int numTeams, int child[], const long long genomeSize) const;
    void crossover(const int *const mom, const int *const dad, const unsigned int start, const unsigned int end,
                   int child[], const long long genomeSize) const;

    void mutate(int genome[], const long long genomeSize) const;
    void mutateWorstTeams(int genome[], const int teamStartPositions[], const int worstTeam, const int secondWorstTeam) const;

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
    inline static const int MUTATIONLIKELIHOOD = 50;           // percent likelihood that a genome is mutated once in a given generation

    inline static const int NUM_ELITES = 3;                 // from each generation, this many highest scoring genomes are directly cloned into the next generation. Some suggest elitism helps speed genetic algorithms, but can lead to premature convergence. Having at least 1 elite stabilizes the high score to end optimization
    inline static const int NUM_SUPER_ELITES = 1;            // of those NUM_ELITES clones, this many (the highest scoring) are also exempted from mutation, so the current best genome(s) can never be lost to a bad mutation

    // working value of the one algorithm constant that varies with class size, set when beginning an optimization and the genome size is known
    int populationsize = POPULATIONSIZE[1];

private:
    static constexpr int POPULATIONSIZE[] = {45000, 30000};  // the number of genomes in each generation--larger size is slower, but each generation is more likely to have optimal result
    static constexpr int GENOMESIZETHRESHOLD = 200;           // numRecords at or below this threshold use the larger population size; above it, the smaller one
    static constexpr int TOURNAMENTSIZE = 30;                 // most of the next generation is created by mating pairs of parent genomes, each time chosen from genomes in a randomly selected tournament in the genepool
};


#endif // GA_H
