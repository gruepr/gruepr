#include "criterion.h"

QJsonObject Criterion::settingsToJson() const {
    QJsonObject json;
    json["weight"] = weight;
    json["penaltyStatus"] = penaltyStatus;
    return json;
}

void Criterion::settingsFromJson(const QJsonObject &json) {
    weight = json["weight"].toDouble(0);
    penaltyStatus = json["penaltyStatus"].toBool(false);
}

QString Criterion::identityRuleText(const QString &identityKey, const QString &operation, int value)
{
    const QString displayKey = QString(identityKey).replace('|', tr(" or "));
    const QString students = (value == 1) ? tr("student") : tr("students");

    // "does not equal 0" and "is greater than 0" are the same rule
    if (value == 0 && (operation == "!=" || operation == ">")) {
        return tr("Every team must have at least 1 %1 student").arg(displayKey);
    }
    if (operation == "!=") {
        return tr("No team may have exactly %1 %2 %3").arg(value).arg(displayKey, students);
    }
    if (operation == "<") {
        return tr("Every team must have fewer than %1 %2 %3").arg(value).arg(displayKey, students);
    }
    if (operation == ">") {
        return tr("Every team must have more than %1 %2 %3").arg(value).arg(displayKey, students);
    }
    // No other operation exists, but a hand-edited save file could name one. Fall back to the
    // symbolic form rather than returning nothing, which would drop the rule silently from the card
    // and leave a dangling "...identity rule: " line in the export text.
    return displayKey + " " + operation + " " + QString::number(value);
}

int Criterion::resolveCriteriaTypeKey(const QMetaEnum &e, const QString &name) {
    if (name == "requiredTeammates") {
        return e.keyToValue("groupTogether");
    }
    if (name == "requestedTeammates") {
        return e.keyToValue("groupTogether");
    }
    if (name == "preventedTeammates") {
        return e.keyToValue("splitApart");
    }
    return e.keyToValue(qPrintable(name));
}

float Criterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                          const DataOptions *dataOptions, const QSet<long long> &/*allIDsBeingTeamed*/)
{
    // Build a mini-genome: find each team member's index in allStudents
    QList<int> indices;
    indices.reserve(team.size);
    for (const auto studentID : team.studentIDs) {
        const int i = grueprGlobal::findStudentIndex(allStudents, studentID);
        if (i < allStudents.size()) {
            indices.push_back(i);
        }
    }

    QList<float> score(1, 0.0f);
    QList<float> penalty(1, 0.0f);

    // Temporarily set weight to 1 to get unweighted 0-to-1 score for display.
    // Weight is only meaningful in the GA's composite scoring, not for display.
    const float savedWeight = weight;
    weight = 1.0f;

    calculateScore(allStudents.constData(), indices.data(), 1, &team.size, teamingOptions, dataOptions, score, penalty);

    weight = savedWeight;

    if (penalty[0] > 0) {
        return 0;
    }
    return score[0];
}

QColor Criterion::teamDisplayColor(float criterionScore) const
{
    if (IS_NO_SCORE(criterionScore)) {
        return Qt::transparent;
    }

    const float clamped = std::clamp(criterionScore, 0.0f, 1.0f);

    int r, g;
    if (clamped < 0.5f) {
        r = 255;
        g = static_cast<int>(255 * (clamped / 0.5f));
    } else {
        r = static_cast<int>(255 * ((1.0f - clamped) / 0.5f));
        g = 255;
    }

    return {r, g, 80, 60};
}
