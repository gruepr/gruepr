#ifndef TEAMINGOPTIONS_H
#define TEAMINGOPTIONS_H

#include "criteria/criterion.h"
#include <QJsonObject>
#include <QObject>
#include <QStringList>

// A container for the general teaming criteria as well as specific values for teamsize(s) and section(s) being teamed
// (those are not stored in the criteria, since those values are used ubiquitously)

class TeamingOptions
{
public:

    TeamingOptions() {}
    explicit TeamingOptions(const QJsonObject &jsonTeamingOptions);
    void reset();

    QJsonObject toJson() const;

    QList<Criterion*> criteria;

    int idealTeamSize = 4;
    QList<int> smallerTeamsSizes;
    int smallerTeamsNumTeams = 1;
    QList<int> largerTeamsSizes;
    int largerTeamsNumTeams = 1;
    QList<int> teamSizesDesired;
    int numTeamsDesired = 1;

    QString sectionName;
    enum class SectionType {noSections, allTogether, allSeparately, oneSection} sectionType = SectionType::noSections;

    int teamsetNumber = 1;                              // which teamset are we working on now?
};

#endif // TEAMINGOPTIONS_H
