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
        int i = 0;
        while (i < allStudents.size() && allStudents[i].ID != studentID) {
            i++;
        }
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

float Criterion::scoreForOneTeamInOptimization(const StudentRecord *const students, const int teamRoster[], const int teamSize,
                                               const TeamingOptions *const teamingOptions, const DataOptions *const dataOptions,
                                               float &penaltyPoints) const
{
    // thread_local because this runs inside an OpenMP parallel region during the hill climb
    thread_local QList<float> score(1, 0.0f);
    thread_local QList<float> penalty(1, 0.0f);
    score[0] = 0.0f;
    penalty[0] = 0.0f;

    calculateScore(students, teamRoster, 1, &teamSize, teamingOptions, dataOptions, score, penalty);

    penaltyPoints = penalty[0];
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
