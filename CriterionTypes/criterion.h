#ifndef CRITERION_H
#define CRITERION_H

class Criterion {
public:
    int priority;
    bool penaltyStatus;

    Criterion(int priority, bool penaltyStatus);
    virtual ~Criterion() = default;
};

#endif // CRITERION_H
