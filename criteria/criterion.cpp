#include "criterion.h"

//Class to keep information on weight and penalty status for each criteria that a user can order based on priority (everything excluding section and teamsize)
Criterion::Criterion(float weight, bool penaltyStatus, QObject *parent)
    : QObject(parent), weight(weight), penaltyStatus(penaltyStatus) {}
