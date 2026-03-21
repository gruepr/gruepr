#include "attributeCriterion.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

Criterion* AttributeCriterion::clone() const {
    auto *copy = new AttributeCriterion(dataOptions, criteriaType, weight, penaltyStatus, nullptr, attributeIndex);
    copy->diversity  = diversity;
    copy->targetMin  = targetMin;
    copy->targetMax  = targetMax;
    return copy;
}

QJsonObject AttributeCriterion::settingsToJson() const {
    QJsonObject json = Criterion::settingsToJson();
    auto diversityEnum = QMetaEnum::fromType<AttributeDiversity>();
    json["diversity"] = diversityEnum.valueToKey(static_cast<int>(diversity));
    json["targetMin"] = static_cast<double>(targetMin);
    json["targetMax"] = static_cast<double>(targetMax);
    return json;
}

void AttributeCriterion::settingsFromJson(const QJsonObject &json) {
    Criterion::settingsFromJson(json);
    if(json.contains("diversity")) {
        auto diversityEnum = QMetaEnum::fromType<AttributeDiversity>();
        const int val = diversityEnum.keyToValue(qPrintable(json["diversity"].toString()));
        if(val != -1) {
            diversity = static_cast<AttributeDiversity>(val);
        }
    }
    targetMin = static_cast<float>(json["targetMin"].toDouble(0.0));
    targetMax = static_cast<float>(json["targetMax"].toDouble(100.0));
}

void AttributeCriterion::generateCriteriaCard(TeamingOptions *const teamingOptions) {
    parentCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE +
                              DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    auto *contentLayout = new QHBoxLayout();
    attributeWidget = new AttributeWidget(attributeIndex, dataOptions, teamingOptions, this, dataOptions->attributeType[attributeIndex], parentCard);
    contentLayout->addWidget(attributeWidget);
    parentCard->setContentAreaLayout(*contentLayout);
}

void AttributeCriterion::calculateScore(const StudentRecord *const students, const int teammates[], const int numTeams, const int teamSizes[],
                                        const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                        std::vector<float> &criteriaScores, std::vector<int> &_penaltyPoints) const
{
    const auto type = dataOptions->attributeType[attributeIndex];
    const bool thisIsNumerical = (type == DataOptions::AttributeType::numerical);
    const bool thisIsTimezone  = (type == DataOptions::AttributeType::timezone);

    // For discrete types: derive total level count and range from DataOptions
    const int totNumAttributeLevels = thisIsNumerical || thisIsTimezone ? 0 :
                                          static_cast<int>(dataOptions->attributeVals_discrete[attributeIndex].size());

    float totRangeAttributeLevels;
    if(thisIsNumerical) {
        totRangeAttributeLevels = *dataOptions->attributeVals_continuous[attributeIndex].crbegin()
                                  - *dataOptions->attributeVals_continuous[attributeIndex].cbegin();
    }
    else if(thisIsTimezone || dataOptions->attributeVals_discrete[attributeIndex].empty()) {
        totRangeAttributeLevels = 0;
    }
    else {
        totRangeAttributeLevels = *dataOptions->attributeVals_discrete[attributeIndex].crbegin()
                                    - *dataOptions->attributeVals_discrete[attributeIndex].cbegin();
    }

    // For numerical: compute overall mean
    float overallMean = 0.0f;
    if(thisIsNumerical) {
        const auto &allVals = dataOptions->attributeVals_continuous[attributeIndex];
        if(!allVals.empty()) {
            overallMean = std::accumulate(allVals.cbegin(), allVals.cend(), 0.0f)
            / static_cast<float>(allVals.size());
        }
    }

    const bool doPenalty = this->penaltyStatus
                           || teamingOptions->haveAnyRequiredAttributes[attributeIndex]
                           || teamingOptions->haveAnyIncompatibleAttributes[attributeIndex];

    int studentNum = 0;
    for(int team = 0; team < numTeams; team++) {
        if(diversity == Criterion::AttributeDiversity::ignored) {
            criteriaScores[team] = 0.0f;
            continue;
        }

        // Gather values for this team
        std::multiset<int>   discreteLevels;
        std::multiset<float> continuousLevels;

        for(int teammate = 0; teammate < teamSizes[team]; teammate++) {
            const auto &stu = students[teammates[studentNum]];
            if(thisIsTimezone) {
                // discrete sentinel still used for unknown-detection
                discreteLevels.insert(stu.attributeVals_discrete[attributeIndex].constBegin(),
                                      stu.attributeVals_discrete[attributeIndex].constEnd());
                continuousLevels.insert(stu.timezone);
            }
            else if(thisIsNumerical) {
                continuousLevels.insert(stu.attributeVals_continuous[attributeIndex].constBegin(),
                                        stu.attributeVals_continuous[attributeIndex].constEnd());
            }
            else {
                discreteLevels.insert(stu.attributeVals_discrete[attributeIndex].constBegin(),
                                      stu.attributeVals_discrete[attributeIndex].constEnd());
            }
            studentNum++;
        }

        // ── Penalties ──────────────────────────────────────────────────────
        if(doPenalty && !thisIsNumerical) {
            if(teamingOptions->haveAnyIncompatibleAttributes[attributeIndex]) {
                for(const auto &pair : std::as_const(teamingOptions->incompatibleAttributeValues[attributeIndex])) {
                    const int n = static_cast<int>(discreteLevels.count(pair.first));
                    if(pair.first == pair.second) {
                        _penaltyPoints[team] += (n * (n - 1)) / 2;
                    }
                    else {
                        const int m = static_cast<int>(discreteLevels.count(pair.second));
                        _penaltyPoints[team] += n * m;
                    }
                }
            }
            if(teamingOptions->haveAnyRequiredAttributes[attributeIndex]) {
                for(const auto value : std::as_const(teamingOptions->requiredAttributeValues[attributeIndex])) {
                    if(discreteLevels.count(value) == 0) {
                        _penaltyPoints[team]++;
                    }
                }
            }
        }

        // Remove the unknown sentinel before scoring
        discreteLevels.erase(-1);

        // ── Scoring ────────────────────────────────────────────────────────
        if(thisIsNumerical) {
            if(continuousLevels.empty()) {
                criteriaScores[team] = 0.0f;
            }
            else {
                if(diversity == Criterion::AttributeDiversity::average) {
                    // Score = how close the team mean is to the overall mean, normalised over the observed data range.
                    float teamSum = 0.0f;
                    for(const float v : continuousLevels) {
                        teamSum += v;
                    }
                    const float teamMean = teamSum / static_cast<float>(continuousLevels.size());

                    // Determine the range to normalise against:
                    // prefer the observed range stored in DataOptions; fall back to targetMin/Max.
                    float rangeSpan = 1.0f;
                    const auto &contRange = dataOptions->attributeVals_continuous[attributeIndex];
                    if(contRange.size() >= 2) {
                        rangeSpan = *contRange.crbegin() - *contRange.cbegin();
                    }
                    else if(targetMax > targetMin) {
                        rangeSpan = targetMax - targetMin;
                    }
                    if(rangeSpan < 1e-6f) {
                        rangeSpan = 1.0f;  // guard against zero-range data
                    }

                    const float deviation = std::abs(teamMean - overallMean);
                    criteriaScores[team] = std::max(0.0f, 1.0f - (deviation / (rangeSpan * 0.5f)));
                }
                else if(diversity == Criterion::AttributeDiversity::similar || diversity == Criterion::AttributeDiversity::diverse) {
                    if(totRangeAttributeLevels <= 0.0f || continuousLevels.size() < 2) {
                        criteriaScores[team] = 0.0f;
                    }
                    else {
                        // spread: fraction of population range covered by the sample
                        const float spread = (*continuousLevels.crbegin() - *continuousLevels.cbegin()) / totRangeAttributeLevels;
                        if(continuousLevels.size() == 2) {
                            criteriaScores[team] = spread;
                        }
                        else {
                            // uniformity: 1 - Gini coefficient of gaps between consecutive sorted values
                            // compute the gaps (sample is already sorted via multiset)
                            std::vector<float> gaps;
                            gaps.reserve(continuousLevels.size() - 1);
                            auto prev = continuousLevels.cbegin();
                            for(auto it = std::next(prev); it != continuousLevels.cend(); ++it) {
                                gaps.push_back(*it - *prev);
                                prev = it;
                            }

                            // Gini = sum of |g_i - g_j| / (2 * numGaps * sumOfGaps)
                            float sumOfGaps = 0.0f;
                            for(const float g : gaps) {
                                sumOfGaps += g;
                            }
                            if(sumOfGaps <= 0.0f) {
                                // all values identical
                                criteriaScores[team] = 0.0f;
                                continue;
                            }

                            float sumAbsDiffs = 0.0f;
                            const int numGaps = static_cast<int>(gaps.size());
                            for(int i = 0; i < numGaps; i++) {
                                for(int j = i + 1; j < numGaps; j++) {
                                    sumAbsDiffs += std::abs(gaps[i] - gaps[j]);
                                }
                            }
                            sumAbsDiffs *= 2.0f;  // account for both (i,j) and (j,i)

                            const float gini = sumAbsDiffs / (2.0f * numGaps * sumOfGaps);

                            criteriaScores[team] = spread * (1.0f - gini);
                        }

                        if(diversity == Criterion::AttributeDiversity::similar) {
                            criteriaScores[team] = 1.0f - criteriaScores[team];
                        }
                    }
                }
            }
        }
        else if(thisIsTimezone) {
            if((weight > 0) && !continuousLevels.empty()) {
                const float tzRange = *continuousLevels.crbegin() - *continuousLevels.cbegin();
                // totRangeAttributeLevels is 0 for timezone — use the actual observed tz span
                // (kept consistent with pre-refactor behaviour: score = range / totRange)
                const float totTzRange = dataOptions->attributeVals_continuous[attributeIndex].size() >= 2
                                             ? *dataOptions->attributeVals_continuous[attributeIndex].crbegin()
                                                   - *dataOptions->attributeVals_continuous[attributeIndex].cbegin()
                                             : 24.0f;  // fallback: 24-hour span
                const float span = totTzRange > 0.0f ? totTzRange : 24.0f;
                criteriaScores[team] = tzRange / span;
                if(diversity == Criterion::AttributeDiversity::similar) {
                    criteriaScores[team] = 1.0f - criteriaScores[team];
                }
            }
            else {
                criteriaScores[team] = 0.0f;
            }
        }
        else {
            // Discrete types: ordered/multiordered, categorical/multicategorical
            if((weight > 0) && !discreteLevels.empty()) {
                if((type == DataOptions::AttributeType::ordered) ||
                    (type == DataOptions::AttributeType::multiordered)) {
                    // Score weighted toward range, with a lesser contribution from unique-value count
                    const int rangeOfVals = *discreteLevels.crbegin() - *discreteLevels.cbegin();
                    int numUniqueVals = 0, prevVal = -1;
                    for(const auto v : discreteLevels) {
                        if(v != prevVal) {
                            numUniqueVals++;
                        }
                        prevVal = v;
                    }
                    const float rangePart = totRangeAttributeLevels > 0 ?
                                                static_cast<float>(rangeOfVals) / totRangeAttributeLevels : 0.0f;
                    const float uniquePart = totNumAttributeLevels > 1 ?
                                                static_cast<float>(numUniqueVals - 1) / (totNumAttributeLevels - 1) : 0.0f;
                    criteriaScores[team] = 0.75f * rangePart + 0.25f * uniquePart;
                }
                else {
                    // categorical / multicategorical: maximise unique values
                    int numUniqueVals = 0, prevVal = -1;
                    for(const auto v : discreteLevels) {
                        if(v != prevVal) {
                            numUniqueVals++;
                        }
                        prevVal = v;
                    }
                    criteriaScores[team] = totNumAttributeLevels > 1
                                               ? static_cast<float>(numUniqueVals - 1) / (totNumAttributeLevels - 1)
                                               : 0.0f;
                }

                // Calculation above assumes diverse = +1, similar = 0; flip if needed
                if(diversity == Criterion::AttributeDiversity::similar) {
                    criteriaScores[team] = 1.0f - criteriaScores[team];
                }
            }
            else {
                criteriaScores[team] = 0.0f;
            }
        }

        criteriaScores[team] *= weight;
    }
}

QString AttributeCriterion::headerLabel(const DataOptions *dataOptions) const {
    if(dataOptions->attributeType[attributeIndex] == DataOptions::AttributeType::timezone) {
        return tr("Timezone");
    }
    return dataOptions->attributeQuestionText[attributeIndex].simplified();
}

Qt::TextElideMode AttributeCriterion::headerElideMode() const {
    return Qt::ElideMiddle;
}

QString AttributeCriterion::teamDisplayText(const TeamRecord &team, const DataOptions *dataOptions,
                                            float /*criterionScore*/, const QList<StudentRecord> &students) const
{
    const auto type = dataOptions->attributeType[attributeIndex];

    // ── Timezone ───────────────────────────────────────────────────────────
    if(type == DataOptions::AttributeType::timezone) {
        std::set<float> tzVals;
        for(const auto id : team.studentIDs) {
            for(const auto &stu : students) {
                if(stu.ID == id) {
                    tzVals.insert(stu.timezone);
                    break;
                }
            }
        }
        if(tzVals.empty()) {
            return "?";
        }
        const float lo = *tzVals.cbegin(), hi = *tzVals.crbegin();
        auto fmtTz = [](float tz) {
            const int h = int(tz);
            const int m = std::abs(static_cast<int>(60.0f * (tz - int(tz))));
            return QString("%1%2:%3").arg(h >= 0 ? "+" : "").arg(h).arg(m, 2, 10, QChar('0'));
        };
        if(lo == hi) {
            return fmtTz(lo);
        }
        return fmtTz(lo) + " " + RIGHTARROW + " " + fmtTz(hi);
    }

    // ── Numerical ──────────────────────────────────────────────────────────
    if(type == DataOptions::AttributeType::numerical) {
        float sum = 0.0f; int count = 0;
        for(const auto id : team.studentIDs) {
            for(const auto &stu : students) {
                if(stu.ID == id) {
                    if(!stu.attributeVals_continuous[attributeIndex].isEmpty()) {
                        sum += stu.attributeVals_continuous[attributeIndex].first();
                        count++;
                    }
                    break;
                }
            }
        }
        return count > 0 ? QString::number(double(sum / count), 'f', 2) : "?";
    }

    // ── Discrete (ordered, categorical, multi-*) ───────────────────────────
    std::set<int> teamVals;
    for(const auto id : team.studentIDs) {
        for(const auto &stu : students) {
            if(stu.ID == id) {
                teamVals.insert(stu.attributeVals_discrete[attributeIndex].constBegin(),
                                stu.attributeVals_discrete[attributeIndex].constEnd());
                break;
            }
        }
    }
    teamVals.erase(-1);     // Strip the unknown sentinel before display — same as calculateScore does
    if(teamVals.empty()) {
        return "?";
    }

    auto first = teamVals.cbegin();
    if((type == DataOptions::AttributeType::ordered) ||
        (type == DataOptions::AttributeType::multiordered)) {
        QString text = QString::number(*first);
        const int last = *teamVals.crbegin();
        if(*first != last) {
            for(auto it = std::next(first); it != teamVals.end(); ++it) {
                text += ", " + QString::number(*it);
            }
        }
        return text;
    }
    QString text = valToLetter(*first);
    for(auto it = std::next(first); it != teamVals.end(); ++it) {
        text += ", " + valToLetter(*it);
    }
    return text;
}

QVariant AttributeCriterion::teamSortValue(const TeamRecord &team,
                                           const DataOptions *dataOptions,
                                           float /*criterionScore*/,
                                           const QList<StudentRecord> &students) const
{
    const auto type = dataOptions->attributeType[attributeIndex];

    // ── Timezone ───────────────────────────────────────────────────────────
    if(type == DataOptions::AttributeType::timezone) {
        std::set<float> tzVals;
        for(const auto sid : team.studentIDs) {
            for(const auto &stu : students) {
                if(stu.ID == sid) {
                    tzVals.insert(stu.timezone); break;
                }
            }
        }
        if(tzVals.empty()) {
            return -1;
        }
        // Encode as single int: lo * 100 + hi (same encoding as before)
        return static_cast<int>(*tzVals.cbegin() * 100 + *tzVals.crbegin());
    }

    // ── Numerical ──────────────────────────────────────────────────────────
    if(type == DataOptions::AttributeType::numerical) {
        float sum = 0.0f; int count = 0;
        for(const auto sid : team.studentIDs) {
            for(const auto &stu : students) {
                if(stu.ID == sid) {
                    if(!stu.attributeVals_continuous[attributeIndex].isEmpty()) {
                        sum += stu.attributeVals_continuous[attributeIndex].first();
                        count++;
                    }
                    break;
                }
            }
        }
        return count > 0 ? static_cast<double>(sum / count) : -1.0;
    }

    // ── Discrete ───────────────────────────────────────────────────────────
    std::set<int> teamVals;
    for(const auto sid : team.studentIDs) {
        for(const auto &stu : students) {
            if(stu.ID == sid) {
                teamVals.insert(stu.attributeVals_discrete[attributeIndex].constBegin(),
                                stu.attributeVals_discrete[attributeIndex].constEnd());
                break;
            }
        }
    }
    // Strip unknown sentinel — sort value should reflect real data only
    teamVals.erase(-1);
    if(teamVals.empty()) {
        return -1;
    }

    auto first = teamVals.cbegin();
    double sortData = *first;
    if((type == DataOptions::AttributeType::ordered) ||
        (type == DataOptions::AttributeType::multiordered)) {
        double divisor = 100.0;
        for(auto it = std::next(first); it != teamVals.end(); ++it) {
            sortData += *it / divisor;
            divisor *= 100.0;
        }
    }
    else {
        double divisor = 10.0;
        for(auto it = std::next(first); it != teamVals.end(); ++it) {
            sortData += *it / divisor;
            divisor *= 100.0;
        }
    }
    return sortData;
}

QString AttributeCriterion::studentDisplayText(const StudentRecord &student,
                                               const DataOptions *dataOptions) const
{
    const auto type = dataOptions->attributeType[attributeIndex];

    if(type == DataOptions::AttributeType::timezone) {
        const int h = int(student.timezone);
        const int m = std::abs(static_cast<int>(60.0f * (student.timezone - int(student.timezone))));
        return QString("%1%2:%3").arg(h >= 0 ? "+" : "").arg(h).arg(m, 2, 10, QChar('0'));
    }

    if(type == DataOptions::AttributeType::numerical) {
        if(student.attributeVals_continuous[attributeIndex].isEmpty()) {
            return "?";
        }
        return QString::number(double(student.attributeVals_continuous[attributeIndex].first()), 'f', 2);
    }

    // Discrete types
    auto value = student.attributeVals_discrete[attributeIndex].constBegin();
    if(student.attributeVals_discrete[attributeIndex].isEmpty() || *value == -1) {
        return "?";
    }

    if(type == DataOptions::AttributeType::ordered) {
        return QString::number(*value);
    }
    if(type == DataOptions::AttributeType::categorical) {
        return valToLetter(*value);
    }
    if(type == DataOptions::AttributeType::multicategorical) {
        QString text;
        const auto end = student.attributeVals_discrete[attributeIndex].constEnd();
        while(value != end) {
            text += valToLetter(*value);
            if(++value != end) {
                text += ", ";
            }
        }
        return text;
    }
    if(type == DataOptions::AttributeType::multiordered) {
        QString text;
        const auto end = student.attributeVals_discrete[attributeIndex].constEnd();
        while(value != end) {
            text += QString::number(*value);
            if(++value != end) {
                text += ", ";
            }
        }
        return text;
    }
    return "?";
}

QString AttributeCriterion::exportTeamingOptionText(const TeamingOptions * /*teamingOptions*/,
                                                    const DataOptions *dataOptions) const
{
    const auto type = dataOptions->attributeType[attributeIndex];
    QString text = "\n" + tr("Question: ") +
                   tr("weight") + " = " + QString::number(double(weight));

    if(type == DataOptions::AttributeType::numerical) {
        text += ", " + tr("numerical balance");
        text += "\n" + dataOptions->attributeQuestionText.at(attributeIndex);
        text += "\n" + tr("Target group average range: ") +
                QString::number(double(targetMin), 'f', 2) + " - " +
                QString::number(double(targetMax), 'f', 2);
    }
    else {
        if(diversity == Criterion::AttributeDiversity::similar) {
            text += ", " + tr("similar");
        }
        else if(diversity == Criterion::AttributeDiversity::diverse) {
            text += ", " + tr("diverse");
        }
        else {
            text += ", " + tr("ignored");
        }

        text += "\n" + dataOptions->attributeQuestionText.at(attributeIndex) +
                "\n" + tr("Responses:");

        for(int r = 0; r < dataOptions->attributeQuestionResponses[attributeIndex].size(); r++) {
            if((type == DataOptions::AttributeType::ordered) ||
                (type == DataOptions::AttributeType::multiordered) ||
                (type == DataOptions::AttributeType::timezone)) {
                text += "\n\t" + dataOptions->attributeQuestionResponses[attributeIndex].at(r);
            }
            else if((type == DataOptions::AttributeType::categorical) ||
                     (type == DataOptions::AttributeType::multicategorical)) {
                text += "\n\t" + (r < 26 ? QString(char(r + 'A'))
                                         : QString(char(r % 26 + 'A')).repeated(1 + r / 26));
                text += ". " + dataOptions->attributeQuestionResponses[attributeIndex].at(r);
            }
        }
    }
    text += "\n";
    return text;
}

QString AttributeCriterion::exportStudentText(const StudentRecord &student,
                                              const DataOptions *dataOptions) const
{
    const auto type = dataOptions->attributeType[attributeIndex];

    if(type == DataOptions::AttributeType::timezone) {
        return QString::number(student.timezone).leftJustified(5);
    }

    if(type == DataOptions::AttributeType::numerical) {
        if(student.attributeVals_continuous[attributeIndex].isEmpty()) {
            return QString("?").leftJustified(5);
        }
        return QString::number(double(student.attributeVals_continuous[attributeIndex].first()), 'f', 2)
            .leftJustified(5);
    }

    auto value = student.attributeVals_discrete[attributeIndex].constBegin();
    if(student.attributeVals_discrete[attributeIndex].isEmpty() || *value == -1) {
        return QString("?").leftJustified(3);
    }

    if(type == DataOptions::AttributeType::ordered) {
        return QString::number(*value).leftJustified(3);
    }
    if(type == DataOptions::AttributeType::categorical) {
        return valToLetter(*value).leftJustified(3);
    }
    if(type == DataOptions::AttributeType::multicategorical) {
        QString text;
        const auto end = student.attributeVals_discrete[attributeIndex].constEnd();
        while(value != end) {
            text += valToLetter(*value);
            if(++value != end) {
                text += ",";
            }
        }
        return text.leftJustified(3);
    }
    if(type == DataOptions::AttributeType::multiordered) {
        QString text;
        const auto end = student.attributeVals_discrete[attributeIndex].constEnd();
        while(value != end) {
            text += QString::number(*value);
            if(++value != end) {
                text += ",";
            }
        }
        return text.leftJustified(3);
    }
    return QString("?").leftJustified(3);
}

QString AttributeCriterion::valToLetter(int val) {
    if(val <= 0)  {
        return "?";
    }
    if(val <= 26) {
        return {char(val - 1 + 'A')};
    }
    return QString(char((val - 1) % 26 + 'A')).repeated(1 + (val - 1) / 26);
}
