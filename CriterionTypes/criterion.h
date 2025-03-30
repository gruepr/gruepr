#ifndef CRITERION_H
#define CRITERION_H

class Criterion {
public:
    float weight;
    bool penaltyStatus;

    Criterion(float weight = 0, bool penaltyStatus = false);
    virtual ~Criterion() = default;
};

#endif // CRITERION_H
