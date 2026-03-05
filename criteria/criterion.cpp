#include "criterion.h"

int Criterion::resolveCriteriaTypeKey(const QMetaEnum &e, const QString &name) {
    if (name == "requiredTeammates")  return e.keyToValue("groupTogether");
    if (name == "requestedTeammates") return e.keyToValue("groupTogether");
    if (name == "preventedTeammates") return e.keyToValue("splitApart");
    return e.keyToValue(qPrintable(name));
}

float Criterion::scoreForOneTeamInDisplay(const QList<StudentRecord> &allStudents, const TeamRecord &team, const TeamingOptions *teamingOptions,
                                          const DataOptions *dataOptions, const QSet<long long> &/*allIDsBeingTeamed*/)
{
    // Build a mini-genome: find each team member's index in allStudents
    std::vector<int> indices;
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

    std::vector<float> score(1, 0.0f);
    std::vector<int> penalty(1, 0);

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

    return QColor(r, g, 80, 60);
}
