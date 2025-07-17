#ifndef CRITERION_H
#define CRITERION_H

#include <QObject>

class Criterion : public QObject {
    Q_OBJECT

public:
    enum class AttributeDiversity {diverse, ignored, similar};    // diverse = heterogeneous (i.e., teammates have a range of values)
                                                            // similar = homogeneous (i.e., all teammates have the same value)
    Q_ENUM(AttributeDiversity)

    float weight;
    bool penaltyStatus;

    Criterion(float weight = 0, bool penaltyStatus = false, QObject *parent = nullptr);
    virtual ~Criterion() = default;
};

#endif // CRITERION_H
