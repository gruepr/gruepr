#ifndef ATTRIBUTECRITERION_H
#define ATTRIBUTECRITERION_H

#include "criterion.h"
#include "widgets/attributeWidget.h"

class AttributeCriterion : public Criterion {
    Q_OBJECT

public:
    AttributeCriterion(const DataOptions *const dataOptions, CriteriaType criteriaType, float weight = 0, bool penaltyStatus = false,
                       GroupingCriteriaCard *parent = nullptr, const int attribute = 0) :
        Criterion(criteriaType, weight, penaltyStatus, parent), dataOptions(dataOptions), attributeIndex(attribute){ };

    Criterion* clone() const override;
    QJsonObject settingsToJson() const override;
    void settingsFromJson(const QJsonObject &json) override;

    void generateCriteriaCard(TeamingOptions *const teamingOptions) override;
    void prepareForOptimization(const StudentRecord *students, int numStudents, const DataOptions *dataOptions) override;
    void calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                        QList<float> &criteriaScores, QList<float> &penaltyPoints) const override;

    QString headerLabel(const DataOptions *dataOptions) const override;
    Qt::TextElideMode headerElideMode() const override;
    QString teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QVariant teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float criterionScore, const QList<StudentRecord> &allStudents) const override;
    QString studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const override;
    QString exportTeamingOptionText(const TeamingOptions *teamingOptions, const DataOptions *dataOptions) const override;
    QString exportStudentText(const StudentRecord &student, const DataOptions *dataOptions) const override;

    const DataOptions *const dataOptions;
    const int attributeIndex;
    AttributeWidget *attributeWidget = nullptr;
    AttributeDiversity diversity = AttributeDiversity::diverse;

    bool haveAnyRequired = false;
    QList<int> requiredValues;
    bool haveAnyIncompatible = false;
    QList<QPair<int,int>> incompatibleValues;

    float targetMin = 0.0;
    float targetMax = 100.0;

private:
    static QString valToLetter(int val);
    int cachedNumAttributeLevels = 0;
    float cachedRangeAttributeLevels = 0.0f;
    float cachedOverallMean = 0.0f;
};

#endif // ATTRIBUTECRITERION_H
