#include "attributeCriterion.h"
#include "widgets/groupingCriteriaCardWidget.h"

void AttributeCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions)
{
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

    auto *attributeContentLayout = new QHBoxLayout();
    attributeWidget = new AttributeWidget(attributeIndex, dataOptions, teamingOptions, parentCard);
    attributeContentLayout->addWidget(attributeWidget);

    parentCard->setContentAreaLayout(*attributeContentLayout);
}

void AttributeCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                        std::vector<float> &criteriaScores, std::vector<int> &_penaltyPoints) const
{
    const bool thisIsTimezone = dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone;
    const bool penaltyStatus = this->penaltyStatus || teamingOptions->haveAnyRequiredAttributes[attributeIndex] ||
                               teamingOptions->haveAnyIncompatibleAttributes[attributeIndex];
    const int totNumAttributeLevels = int(dataOptions->attributeVals[attributeIndex].size());
    const int totRangeAttributeLevels = *(dataOptions->attributeVals[attributeIndex].crbegin()) - *(dataOptions->attributeVals[attributeIndex].cbegin());

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        // gather all the attribute values on the team
        std::multiset<int> attributeLevelsInTeam;
        std::multiset<float> timezoneLevelsInTeam;
        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            attributeLevelsInTeam.insert(students[teammates[studentNum]].attributeVals[attributeIndex].constBegin(),
                                         students[teammates[studentNum]].attributeVals[attributeIndex].constEnd());
            if(thisIsTimezone) {
                timezoneLevelsInTeam.insert(students[teammates[studentNum]].timezone);
            }
            studentNum++;
        }

        if (penaltyStatus){
            /*if((criterion->weight > 0) && (!attributeLevelsInTeam.empty())) {
                //get the values of all, put a penalty for each
                if (_teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::similar){ //homogenous
                    if(thisIsTimezone) { //similar, penalize if uniqueItems > 1 (we can only have 1 unique)
                        std::set<int> uniqueItems(timezoneLevelsInTeam.begin(), timezoneLevelsInTeam.end());
                        int duplicatedValues = timezoneLevelsInTeam.size() - uniqueItems.size();  // Count unique values
                        int uniqueCount = uniqueItems.size();
                        if (uniqueCount > 1) {
                            _penaltyPoints[team] += uniqueCount - 1; // Penalize for extra unique values
                        }
                    } else { //if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                        //(_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)){
                        std::set<int> uniqueItems(attributeLevelsInTeam.begin(), attributeLevelsInTeam.end());
                        int duplicatedValues = attributeLevelsInTeam.size() - uniqueItems.size();  // Count unique values
                        int uniqueCount = uniqueItems.size();
                        if (uniqueCount > 1) {
                            _penaltyPoints[team] += uniqueCount - 1; // Penalize for extra unique values
                        }
                    }
                } else { //heterogenous, penalize same values
                    if(thisIsTimezone) {
                        std::set<int> uniqueItems(timezoneLevelsInTeam.begin(), timezoneLevelsInTeam.end());
                        int duplicatedValues = std::max(0, int(timezoneLevelsInTeam.size() - uniqueItems.size()));
                        _penaltyPoints[team] += duplicatedValues;

                    } else { //if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                        //(_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)){
                        std::set<int> uniqueItems(attributeLevelsInTeam.begin(), attributeLevelsInTeam.end());
                        int duplicatedValues = std::max(0, int(attributeLevelsInTeam.size() - uniqueItems.size()));
                        _penaltyPoints[team] += duplicatedValues;

                    }
                }
            }*/

            if(teamingOptions->haveAnyIncompatibleAttributes[attributeIndex]) {
                // go through each pair found in teamingOptions->incompatibleAttributeValues[attribute] list and see if both are found in attributeLevelsInTeam
                for(const auto &pair : std::as_const(teamingOptions->incompatibleAttributeValues[attributeIndex])) {
                    //getting the attribute level count for each incompatible attribute value
                    const int n = int(attributeLevelsInTeam.count(pair.first));
                    if(pair.first == pair.second) {
                        _penaltyPoints[team] += (n * (n-1))/ 2;  // number of incompatible pairings is the sum 1 -> n-1 (0 if n == 0 or n == 1)
                    }
                    else {
                        const int m = int(attributeLevelsInTeam.count(pair.second));
                        _penaltyPoints[team] += n * m;           // number of incompatible pairings is the # of n -> m interactions (0 if n == 0 or m == 0)
                    }
                }
            }

            // Add a penalty per required attribute response not found
            if(teamingOptions->haveAnyRequiredAttributes[attributeIndex]) {
                // go through each value found in teamingOptions->requiredAttributeValues[attrib] list and see whether it's found in attributeLevelsInTeam
                for(const auto value : std::as_const(teamingOptions->requiredAttributeValues[attributeIndex])) {
                    if(attributeLevelsInTeam.count(value) == 0) {
                        _penaltyPoints[team]++;
                    }
                }
            }
        }

        // Remove attribute values of -1 (unknown/not set) and then determine attribute scores
        attributeLevelsInTeam.erase(-1);

        //calculating score from homogeneity/heterogeneity
        if((weight > 0) && (!attributeLevelsInTeam.empty())) {
            if(thisIsTimezone) {
                // "attribute" is timezone, so use range of timezone values
                criteriaScores[team] = (*timezoneLevelsInTeam.crbegin() - *timezoneLevelsInTeam.cbegin()) / totRangeAttributeLevels;
            }
            else if((dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::ordered) ||
                     (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::multiordered)) {
                // attribute has meaningful ordering/numerical values--diverse range means mostly
                // max. spread between max and min values BUT ALSO, to a lesser extent, the number of unique values
                const int rangeOfVals = *attributeLevelsInTeam.crbegin() - *attributeLevelsInTeam.cbegin();  // crbegin is last = largest val; cbegin is 1st = smallest
                int numUniqueVals = 0;
                int prevVal = -1;
                for(const auto currVal : attributeLevelsInTeam) {
                    if(currVal != prevVal) {
                        numUniqueVals++;
                    }
                    prevVal = currVal;
                }
                criteriaScores[team] = (0.75 * rangeOfVals / totRangeAttributeLevels) + (0.25 * (numUniqueVals - 1) / (totNumAttributeLevels - 1));
            }
            else {
                // attribute is categorical or multicategorical--diverse range means maximum number of unique values
                int numUniqueVals = 0;
                int prevVal = -1;
                for(const auto currVal : attributeLevelsInTeam) {
                    if(currVal != prevVal) {
                        numUniqueVals++;
                    }
                    prevVal = currVal;
                }
                criteriaScores[team] = (numUniqueVals - 1.0f) / (totNumAttributeLevels - 1.0f);
            }

            //Calculation assumes diverse (i.e., 0 if team is fully similar and +1 if fully diverse); flip it if want similar
            if(teamingOptions->attributeDiversity[attributeIndex] == Criterion::AttributeDiversity::similar) {
                criteriaScores[team] = 1 - criteriaScores[team];
            }
            else if(teamingOptions->attributeDiversity[attributeIndex] == Criterion::AttributeDiversity::ignored) {
                criteriaScores[team] = 0;
            }
        }
        criteriaScores[team] *= weight;
    }
}

QString AttributeCriterion::headerLabel(const DataOptions *dataOptions) const {
    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone) {
        return tr("Timezone");
    }
    return dataOptions->attributeQuestionText[attributeIndex].simplified();
}

Qt::TextElideMode AttributeCriterion::headerElideMode() const {
    return Qt::ElideMiddle;
}

QString AttributeCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions, float /*criterionScore*/) const {
    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone) {
        const float firstVal = *(team.timezoneVals.cbegin());
        const float lastVal = *(team.timezoneVals.crbegin());
        if (firstVal == lastVal) {
            const int hour = int(firstVal);
            const int minutes = 60 * (firstVal - int(firstVal));
            return QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs(minutes), 2, 10, QChar('0'));
        }
        const int hourF = int(firstVal), minutesF = 60 * (firstVal - int(firstVal));
        const int hourL = int(lastVal), minutesL = 60 * (lastVal - int(lastVal));
        return QString("%1%2:%3").arg(hourF >= 0 ? "+" : "").arg(hourF).arg(std::abs(minutesF), 2, 10, QChar('0'))
               + " " + RIGHTARROW + " "
               + QString("%1%2:%3").arg(hourL >= 0 ? "+" : "").arg(hourL).arg(std::abs(minutesL), 2, 10, QChar('0'));
    }

    auto firstTeamVal = team.attributeVals[attributeIndex].cbegin();
    if (firstTeamVal == team.attributeVals[attributeIndex].cend()) {
        return "?";
    }

    QString text;
    if ((dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::ordered) ||
        (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::multiordered)) {
        text = QString::number(*firstTeamVal);
        auto lastTeamVal = team.attributeVals[attributeIndex].crbegin();
        if (*firstTeamVal != *lastTeamVal) {
            for (auto val = std::next(firstTeamVal); val != team.attributeVals[attributeIndex].end(); val++) {
                text += ", " + QString::number(*val);
            }
        }
    } else {
        text = valToLetter(*firstTeamVal);
        for (auto val = std::next(firstTeamVal); val != team.attributeVals[attributeIndex].end(); val++) {
            text += ", " + valToLetter(*val);
        }
    }
    return text;
}

QVariant AttributeCriterion::teamSortValue(const TeamRecord &team, const DataOptions *dataOptions, float /*criterionScore*/) const {
    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone) {
        const float firstVal = *(team.timezoneVals.cbegin());
        const float lastVal = *(team.timezoneVals.crbegin());
        return int(firstVal * 100 + lastVal);
    }

    auto firstTeamVal = team.attributeVals[attributeIndex].cbegin();
    if (firstTeamVal == team.attributeVals[attributeIndex].cend()) {
        return -1;
    }

    double sortData = *firstTeamVal;
    if ((dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::ordered) ||
        (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::multiordered)) {
        double divisor = 100;
        for (auto val = std::next(firstTeamVal); val != team.attributeVals[attributeIndex].end(); val++) {
            sortData += *val / divisor;
            divisor *= 100;
        }
    } else {
        double divisor = 10;
        for (auto val = std::next(firstTeamVal); val != team.attributeVals[attributeIndex].end(); val++) {
            sortData += *val / divisor;
            divisor *= 100;
        }
    }
    return sortData;
}

QString AttributeCriterion::studentDisplayText(const StudentRecord &student, const DataOptions *dataOptions) const {
    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone) {
        const int hour = int(student.timezone);
        const int minutes = 60 * (student.timezone - int(student.timezone));
        return QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0'));
    }

    auto value = student.attributeVals[attributeIndex].constBegin();
    if (*value == -1) {
        return "?";
    }

    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::ordered) {
        return QString::number(*value);
    }

    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::categorical) {
        return valToLetter(*value);
    }

    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::multicategorical) {
        QString text;
        const auto lastVal = student.attributeVals[attributeIndex].constEnd();
        while (value != lastVal) {
            text += valToLetter(*value);
            value++;
            if (value != lastVal) text += ", ";
        }
        return text;
    }

    if (dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::multiordered) {
        QString text;
        const auto lastVal = student.attributeVals[attributeIndex].constEnd();
        while (value != lastVal) {
            text += QString::number(*value);
            value++;
            if (value != lastVal) text += ", ";
        }
        return text;
    }

    return "?";
}

QString AttributeCriterion::valToLetter(int val) {
    if (val <= 26) {
        return QString(char(val - 1 + 'A'));
    }
    return QString(char((val - 1) % 26 + 'A')).repeated(1 + ((val - 1) / 26));
}
