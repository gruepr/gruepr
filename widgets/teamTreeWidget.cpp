#include "teamTreeWidget.h"
#include "gruepr_globals.h"
#include "criteria/attributeCriterion.h"
#include <QDropEvent>
#include <QPainter>
#include <QTextLayout>
#include <QTimer>
#include <QToolTip>


//////////////////
// Tree display for teammates with swappable positions and sortable columns using hidden data
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    headerView = new TeamTreeHeaderView(Qt::Horizontal, this);
    setHeader(headerView);
    setStyleSheet(QString(TEAMTREEWIDGETSTYLE) + SCROLLBARSTYLE);
    setMouseTracking(true);
    setHeaderHidden(false);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(false);
    setAlternatingRowColors(true);
    setAutoScroll(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(this, &QTreeWidget::entered, this, &TeamTreeWidget::itemEntered);
    connect(this, &QTreeWidget::viewportEntered, this, [this] {leaveEvent(nullptr);});
    connect(this, &QTreeWidget::itemCollapsed, this, &TeamTreeWidget::itemCollapse);
    connect(this, &QTreeWidget::itemExpanded, this, &TeamTreeWidget::itemExpand);
}


void TeamTreeWidget::itemCollapse(QTreeWidgetItem *item)
{
    // only collapse teams (not students or sections)
    auto *newItem = dynamic_cast<TeamTreeWidgetItem*>(item);
    if(newItem == nullptr) {
        return;
    }
    const bool itemIsTeam = (newItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team);
    if(itemIsTeam) {
        for(int column = 0; column < columnCount(); column++) {
            resizeColumnToContents(column);
        }
    }
    else {
        QTreeWidget::expandItem(newItem);
    }
}


void TeamTreeWidget::itemExpand(QTreeWidgetItem *item)
{
    auto *newItem = dynamic_cast<TeamTreeWidgetItem*>(item);
    if(newItem == nullptr) {
        return;
    }
    for(int column = 0; column < columnCount(); column++) {
        resizeColumnToContents(column);
    }
}


void TeamTreeWidget::collapseAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, collapsing only the teams
    auto *item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
    while(item != nullptr) {
        const bool itemIsTeam = (item->treeItemType == TeamTreeWidgetItem::TreeItemType::team);
        if(itemIsTeam) {
            QTreeWidget::collapseItem(item);
            for(int column = 0; column < columnCount(); column++) {
                resizeColumnToContents(column);
            }
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(itemBelow(item));
    }

    setUpdatesEnabled(true);
    repaint();
}


void TeamTreeWidget::expandAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, expanding every section and team
    auto *item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
    while(item != nullptr) {
        const bool itemIsStudent = (item->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
        if(!itemIsStudent) {
            QTreeWidget::expandItem(item);
            for(int column = 0; column < columnCount(); column++) {
                resizeColumnToContents(column);
            }
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(itemBelow(item));
    }
    setUpdatesEnabled(true);
    repaint();
}


void TeamTreeWidget::resetDisplay(const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    QStringList headerLabels;
    int i = 0;
    headerLabels << tr("Name");
    headerView->setColumnElideMode(i++, Qt::ElideNone);

    // Add section column if showing all sections together (not criterion-driven, just info)
    if (teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        headerLabels << tr("Sections");
        headerView->setColumnElideMode(i++, Qt::ElideRight);
    }

    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        const auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }
        switch (criterion->criteriaType) {
            case Criterion::CriteriaType::genderIdentity:
                if (dataOptions->genderType == GenderType::pronoun) {
                    headerLabels << tr("Pronouns");
                } else {
                    headerLabels << tr("Gender");
                }
                headerView->setColumnElideMode(i++, Qt::ElideNone);
                break;
            case Criterion::CriteriaType::urmIdentity:
                headerLabels << tr("URM");
                headerView->setColumnElideMode(i++, Qt::ElideNone);
                break;
            case Criterion::CriteriaType::gradeBalance:
                headerLabels << tr("Grade");
                headerView->setColumnElideMode(i++, Qt::ElideNone);
                break;
            case Criterion::CriteriaType::attributeQuestion: {
                const auto *attrCrit = qobject_cast<const AttributeCriterion*>(criterion);
                if (attrCrit != nullptr) {
                    if (dataOptions->attributeType[attrCrit->attributeIndex] == DataOptions::AttributeType::timezone) {
                        headerLabels << tr("Timezone");
                    } else {
                        headerLabels << dataOptions->attributeQuestionText[attrCrit->attributeIndex].simplified();
                    }
                }
                headerView->setColumnElideMode(i++, Qt::ElideMiddle);
                break;
            }
            case Criterion::CriteriaType::scheduleMeetingTimes:
                headerLabels << tr("Meeting times");
                headerView->setColumnElideMode(i++, Qt::ElideMiddle);
                break;
            case Criterion::CriteriaType::requiredTeammates:
                headerLabels << tr("Required teammates");
                headerView->setColumnElideMode(i++, Qt::ElideMiddle);
                break;
            case Criterion::CriteriaType::preventedTeammates:
                headerLabels << tr("Prevented teammates");
                headerView->setColumnElideMode(i++, Qt::ElideMiddle);
                break;
            case Criterion::CriteriaType::requestedTeammates:
                headerLabels << tr("Requested teammates");
                headerView->setColumnElideMode(i++, Qt::ElideMiddle);
                break;
            default:
                break;
        }
    }

    headerLabels << tr("display_order");

    setColumnCount(int(headerLabels.size()));
    for(int i = 0; i < headerLabels.size() - 1; i++) {
        showColumn(i);
    }
    hideColumn(int(headerLabels.size()) - 1);  // don't show the sort order column (can comment this out when debugging sorting operations)

    setHeaderLabels(headerLabels);
    for(int i = 0; i < headerLabels.size(); i++) {
        headerView->setColumnIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
    }

    setSortingEnabled(false);
    setFocus();
    clear();
}


void TeamTreeWidget::refreshSection(TeamTreeWidgetItem *sectionItem, const QString &sectionName)
{
    if(sectionItem->treeItemType != TeamTreeWidgetItem::TreeItemType::section) {
        return;
    }
    sectionItem->setText(0, tr("Section ") + sectionName);
    sectionItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
}


void TeamTreeWidget::refreshTeam(RefreshType refreshType, TeamTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum,
                                 const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    if(teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) {
        return;
    }

    //create team items and fill in information
    int column = 0;
    teamItem->setText(column, tr("Team ") + team.name);
    teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);

    //Data items can be accessed later, in this case to determine sort order
    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, tr("Team ") + team.name);
    teamItem->setData(column, TEAMINFO_SORT_ROLE, team.name); //sort based on team name
    teamItem->setData(column, TEAM_NUMBER_ROLE, teamNum);
    teamItem->setToolTip(column, team.tooltip);
    column++;

    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        teamItem->setText(column, QString::number(team.numSections));
        teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(team.numSections));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numSections);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }

    // One column per scoring criterion
    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        const auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }

        switch (criterion->criteriaType) {
            case Criterion::CriteriaType::genderIdentity: {
                QStringList genderInitials;
                if (dataOptions->genderType == GenderType::biol) {
                    genderInitials = QString(BIOLGENDERSINITIALS).split('/');
                } else if (dataOptions->genderType == GenderType::adult) {
                    genderInitials = QString(ADULTGENDERSINITIALS).split('/');
                } else if (dataOptions->genderType == GenderType::child) {
                    genderInitials = QString(CHILDGENDERSINITIALS).split('/');
                } else {
                    genderInitials = QString(PRONOUNSINITIALS).split('/');
                }
                QString genderText;
                if (team.numWomen > 0) {
                    genderText += QString::number(team.numWomen) + genderInitials.at(static_cast<int>(Gender::woman));
                    if (team.numMen > 0 || team.numNonbinary > 0 || team.numUnknown > 0) genderText += ", ";
                }
                if (team.numMen > 0) {
                    genderText += QString::number(team.numMen) + genderInitials.at(static_cast<int>(Gender::man));
                    if (team.numNonbinary > 0 || team.numUnknown > 0) genderText += ", ";
                }
                if (team.numNonbinary > 0) {
                    genderText += QString::number(team.numNonbinary) + genderInitials.at(static_cast<int>(Gender::nonbinary));
                    if (team.numUnknown > 0) genderText += ", ";
                }
                if (team.numUnknown > 0) {
                    genderText += QString::number(team.numUnknown) + genderInitials.at(static_cast<int>(Gender::unknown));
                }
                teamItem->setText(column, genderText);
                teamItem->setTextAlignment(column, Qt::AlignCenter);
                teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, genderText);
                teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numMen - team.numWomen);
                teamItem->setToolTip(column, team.tooltip);
                const auto score = findCriterionScore(team, teamingOptions, Criterion::CriteriaType::genderIdentity, -1);
                if (score != std::nullopt) { teamItem->setBackground(column, scoreToColor(*score)); }
                column++;
                break;
            }
            case Criterion::CriteriaType::urmIdentity: {
                teamItem->setText(column, QString::number(team.numURM));
                teamItem->setTextAlignment(column, Qt::AlignCenter);
                teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(team.numURM));
                teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numURM);
                teamItem->setToolTip(column, team.tooltip);
                const auto score = findCriterionScore(team, teamingOptions, Criterion::CriteriaType::urmIdentity, -1);
                if (score != std::nullopt) { teamItem->setBackground(column, scoreToColor(*score)); }
                column++;
                break;
            }
            case Criterion::CriteriaType::gradeBalance: {
                float total = 0.0;
                for (const float grade : team.gradeVals) { total += grade; }
                const float average = team.size > 0 ? total / team.size : 0;
                teamItem->setText(column, QString::number(double(average), 'f', 2));
                teamItem->setTextAlignment(column, Qt::AlignCenter);
                teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(double(average), 'f', 2));
                teamItem->setData(column, TEAMINFO_SORT_ROLE, average);
                teamItem->setToolTip(column, team.tooltip);
                const auto score = findCriterionScore(team, teamingOptions, Criterion::CriteriaType::gradeBalance, -1);
                if (score != std::nullopt) { teamItem->setBackground(column, scoreToColor(*score)); }
                column++;
                break;
            }
            case Criterion::CriteriaType::attributeQuestion: {
                const auto *attrCrit = qobject_cast<const AttributeCriterion*>(criterion);
                if (attrCrit == nullptr) { column++; break; }
                const int attribute = attrCrit->attributeIndex;

                if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone) {
                    // Timezone column
                    const float firstVal = *(team.timezoneVals.cbegin());
                    const float lastVal = *(team.timezoneVals.crbegin());
                    QString timezoneText;
                    if (firstVal == lastVal) {
                        const int hour = int(firstVal);
                        const int minutes = 60 * (firstVal - int(firstVal));
                        timezoneText = QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs(minutes), 2, 10, QChar('0'));
                    } else {
                        const int hourF = int(firstVal), minutesF = 60 * (firstVal - int(firstVal));
                        const int hourL = int(lastVal), minutesL = 60 * (lastVal - int(lastVal));
                        timezoneText = QString("%1%2:%3").arg(hourF >= 0 ? "+" : "").arg(hourF).arg(std::abs(minutesF), 2, 10, QChar('0'))
                                       + " " + RIGHTARROW + " "
                                       + QString("%1%2:%3").arg(hourL >= 0 ? "+" : "").arg(hourL).arg(std::abs(minutesL), 2, 10, QChar('0'));
                    }
                    const int sortData = int(firstVal * 100 + lastVal);
                    teamItem->setText(column, timezoneText);
                    teamItem->setTextAlignment(column, Qt::AlignCenter);
                    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, timezoneText);
                    teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
                    teamItem->setToolTip(column, team.tooltip);
                } else {
                    // Regular attribute column
                    QString attributeText;
                    double sortData = 0;
                    auto firstTeamVal = team.attributeVals[attribute].cbegin();
                    if (firstTeamVal != team.attributeVals[attribute].cend()) {
                        if ((dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                            (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                            attributeText = QString::number(*firstTeamVal);
                            sortData = *firstTeamVal;
                            double divisor = 100;
                            auto lastTeamVal = team.attributeVals[attribute].crbegin();
                            if (*firstTeamVal != *lastTeamVal) {
                                for (auto val = std::next(firstTeamVal); val != team.attributeVals[attribute].end(); val++) {
                                    attributeText += ", " + QString::number(*val);
                                    sortData += *val / divisor;
                                    divisor *= 100;
                                }
                            }
                        } else {
                            attributeText = (*firstTeamVal <= 26 ? QString(char(*firstTeamVal - 1 + 'A'))
                                                                 : QString(char((*firstTeamVal - 1) % 26 + 'A')).repeated(1 + ((*firstTeamVal - 1) / 26)));
                            sortData = *firstTeamVal;
                            double divisor = 10;
                            for (auto val = std::next(firstTeamVal); val != team.attributeVals[attribute].end(); val++) {
                                attributeText += ", " + (*val <= 26 ? QString(char(*val - 1 + 'A'))
                                                                    : QString(char((*val - 1) % 26 + 'A')).repeated(1 + ((*val - 1) / 26)));
                                sortData += *val / divisor;
                                divisor *= 100;
                            }
                        }
                    } else {
                        attributeText = "?";
                        sortData = -1;
                    }
                    teamItem->setText(column, attributeText);
                    teamItem->setTextAlignment(column, Qt::AlignCenter);
                    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, attributeText);
                    teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
                    teamItem->setToolTip(column, team.tooltip);
                }
                const auto score = findCriterionScore(team, teamingOptions, Criterion::CriteriaType::attributeQuestion, attribute);
                if (score != std::nullopt) {
                    teamItem->setBackground(column, scoreToColor(*score));
                }
                column++;
                break;
            }
            case Criterion::CriteriaType::scheduleMeetingTimes: {
                const int numAvailTimes = team.numMeetingTimes;
                teamItem->setText(column, (team.size > 1) ? QString::number(numAvailTimes) : "  --  ");
                teamItem->setTextAlignment(column, Qt::AlignCenter);
                teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(numAvailTimes));
                teamItem->setData(column, TEAMINFO_SORT_ROLE, numAvailTimes);
                teamItem->setToolTip(column, team.tooltip);
                const auto score = findCriterionScore(team, teamingOptions, Criterion::CriteriaType::scheduleMeetingTimes, -1);
                if (score != std::nullopt) {
                    teamItem->setBackground(column, scoreToColor(*score));
                }
                column++;
                break;
            }
            case Criterion::CriteriaType::requiredTeammates:
            case Criterion::CriteriaType::preventedTeammates:
            case Criterion::CriteriaType::requestedTeammates: {
                const auto score = findCriterionScore(team, teamingOptions, criterion->criteriaType, -1);
                QString statusText = "—";
                int sortVal = 0;
                if (score != std::nullopt) {
                    if (*score > 0) {
                        statusText = "✓";
                        sortVal = 1;
                    } else {
                        statusText = "✗";
                        sortVal = -1;
                    }
                    teamItem->setBackground(column, scoreToColor(*score));
                }
                teamItem->setText(column, statusText);
                teamItem->setTextAlignment(column, Qt::AlignCenter);
                teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, statusText);
                teamItem->setData(column, TEAMINFO_SORT_ROLE, sortVal);
                teamItem->setToolTip(column, team.tooltip);
                column++;
                break;
            }
            default:
                break;
        }
    }

    // Display order column
    if(refreshType == RefreshType::newTeam) {
        teamItem->setText(column, QString::number(teamNum));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(teamNum));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, teamNum);
    }
}


void TeamTreeWidget::refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu, const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    if(studentItem->treeItemType != TeamTreeWidgetItem::TreeItemType::student) {
        return;
    }

    int column = 0;

    // Name column
    studentItem->setText(column, stu.firstname + " " + stu.lastname);
    studentItem->setData(column, Qt::UserRole, stu.ID);
    studentItem->setToolTip(column, stu.tooltip);
    studentItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    column++;

    // Section column
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        studentItem->setText(column, stu.section);
        studentItem->setToolTip(column, stu.tooltip);
        column++;
    }

    // One column per scoring criterion
    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        const auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }

        switch (criterion->criteriaType) {
            case Criterion::CriteriaType::genderIdentity: {
                QStringList genderOptions;
                if (dataOptions->genderType == GenderType::biol) {
                    genderOptions = QString(BIOLGENDERS).split('/');
                } else if (dataOptions->genderType == GenderType::adult) {
                    genderOptions = QString(ADULTGENDERS).split('/');
                } else if (dataOptions->genderType == GenderType::child) {
                    genderOptions = QString(CHILDGENDERS).split('/');
                } else {
                    genderOptions = QString(PRONOUNS).split('/');
                }
                QString genderText;
                bool first = true;
                for (const auto gen : stu.gender) {
                    if (!first) genderText += ", ";
                    genderText += genderOptions.at(static_cast<int>(gen));
                    first = false;
                }
                studentItem->setText(column, genderText);
                studentItem->setToolTip(column, stu.tooltip);
                studentItem->setTextAlignment(column, Qt::AlignCenter);
                column++;
                break;
            }
            case Criterion::CriteriaType::urmIdentity: {
                studentItem->setText(column, stu.URM ? tr("yes") : "");
                studentItem->setToolTip(column, stu.tooltip);
                studentItem->setTextAlignment(column, Qt::AlignCenter);
                column++;
                break;
            }
            case Criterion::CriteriaType::gradeBalance: {
                studentItem->setText(column, QString::number(stu.grade));
                studentItem->setTextAlignment(column, Qt::AlignCenter);
                studentItem->setToolTip(column, stu.tooltip);
                column++;
                break;
            }
            case Criterion::CriteriaType::attributeQuestion: {
                const auto *attrCrit = qobject_cast<const AttributeCriterion*>(criterion);
                if (attrCrit == nullptr) { column++; break; }
                const int attribute = attrCrit->attributeIndex;

                if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone) {
                    const int hour = int(stu.timezone);
                    const int minutes = 60 * (stu.timezone - int(stu.timezone));
                    studentItem->setText(column, QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0')));
                } else {
                    auto value = stu.attributeVals[attribute].constBegin();
                    if (*value != -1) {
                        if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) {
                            studentItem->setText(column, QString::number(*value));
                        } else if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical) {
                            studentItem->setText(column, (*value <= 26 ? QString(char(*value - 1 + 'A'))
                                                                       : QString(char((*value - 1) % 26 + 'A')).repeated(1 + ((*value - 1) / 26))));
                        } else if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                            QString vals;
                            const auto lastVal = stu.attributeVals[attribute].constEnd();
                            while (value != lastVal) {
                                vals += (*value <= 26 ? QString(char(*value - 1 + 'A'))
                                                      : QString(char((*value - 1) % 26 + 'A')).repeated(1 + ((*value - 1) / 26)));
                                value++;
                                if (value != lastVal) vals += ", ";
                            }
                            studentItem->setText(column, vals);
                        } else if (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                            QString vals;
                            const auto lastVal = stu.attributeVals[attribute].constEnd();
                            while (value != lastVal) {
                                vals += QString::number(*value);
                                value++;
                                if (value != lastVal) vals += ", ";
                            }
                            studentItem->setText(column, vals);
                        }
                    } else {
                        studentItem->setText(column, "?");
                    }
                }
                studentItem->setToolTip(column, stu.tooltip);
                studentItem->setTextAlignment(column, Qt::AlignCenter);
                column++;
                break;
            }
            default: {
                // blank for teammate columns, schedule column
                studentItem->setText(column, "");
                studentItem->setToolTip(column, stu.tooltip);
                column++;
                break;
            }

        }
    }
}

void TeamTreeWidget::setColumnHeaderIcon(int column, const QIcon &icon)
{
    headerView->setColumnIcon(column, icon);
}

void TeamTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    draggedItem = dynamic_cast<TeamTreeWidgetItem*>(currentItem());
    if(draggedItem == nullptr) {
        return;
    }
    QTreeWidget::dragEnterEvent(event);

    dragDropEventLabel = new QLabel(this);
    dragDropEventLabel->setWindowFlag(Qt::ToolTip);
    dragDropEventLabel->setTextFormat(Qt::RichText);
}


void TeamTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    QTreeWidget::dragLeaveEvent(event);

    if(dragDropEventLabel != nullptr) {
        dragDropEventLabel->hide();
        delete dragDropEventLabel;
        dragDropEventLabel = nullptr;
    }
}


void TeamTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    if(dragDropEventLabel == nullptr) {
        dragDropEventLabel = new QLabel(this);
        dragDropEventLabel->setWindowFlag(Qt::ToolTip);
        dragDropEventLabel->setTextFormat(Qt::RichText);
    }

    // get the item being dragged and ensure that the item is a TeamTreeWidgetItem
    if(draggedItem == nullptr) {
        dragDropEventLabel->hide();
        return;
    }

    // get the item currently under the cursor and ensure that it is a TeamTreeWidgetItem and not a section
    QTreeWidgetItem* itemUnderCursor = itemAt(event->position().toPoint());
    droppedItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);
    if((droppedItem == nullptr) || (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::section)) {
        dragDropEventLabel->hide();
        return;
    }

    // adjust the location and text of the tooltip
    const int iconSize = 32;
    const QString iconSizeStr = QString::number(iconSize);
    dragDropEventLabel->move(QCursor::pos() + QPoint(iconSize, iconSize));

    const bool draggedItemIsStudent = (draggedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const bool droppedItemIsStudent = (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const auto *const draggedItemParent = draggedItem->parent();
    const auto *const droppedItemParent = droppedItem->parent();

    if((draggedItem == droppedItem) || (!draggedItemIsStudent && droppedItemIsStudent) || (draggedItemParent == droppedItem)) {
        // ignore if dragging item onto self, team->student, or student->own team
        dragDropEventLabel->hide();
    }
    else if(draggedItemIsStudent && droppedItemIsStudent) {
        // dragging student->student
        // show warning if there are separated sections and dragging between different sections
        if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
            draggedItemParent->parent() != nullptr && droppedItemParent->parent() != nullptr &&
            draggedItemParent->parent() != droppedItemParent->parent()) {
                dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                            + tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + droppedItem->text(0) + "</b><br>"
                                            + tr("NOTE: these students are on teams in different sections.") + "</div>");
                dragDropEventLabel->setStyleSheet(DRAGDROPLABELWARNSTYLE);
                dragDropEventLabel->show();
                dragDropEventLabel->adjustSize();
        }
        else {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }

    }
    else if(draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging student->team
        // disallow if this is the only student left on the team (leaving team empty)
        // and warn if this team is in a differen section
        if(draggedItemParent->childCount() == 1) {
            dragDropEventLabel->setText(tr("Cannot move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto another team.<br>")
                                         + " <b>" + draggedItem->parent()->text(0) + "</b> " + tr("cannot be left empty."));
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELSTOPSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
                draggedItemParent->parent() != nullptr && draggedItemParent->parent() != droppedItemParent) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + droppedItem->text(0) + "</b><br>"
                                        + tr("NOTE: this students is on a team in different a section.") + "</div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELWARNSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();

        }
        else {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                         + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
    }
    else if(!draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging team->team
        // disallow if dragging a team to a different section
        if((draggedItemParent == nullptr) || (droppedItemParent == nullptr) ||
          ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent == droppedItemParent))) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("above") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else if ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent != droppedItemParent)) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Cannot move") + " <b>" + draggedItem->text(0) + "</b> " + tr("to a different section.") + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELSTOPSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else {
            dragDropEventLabel->hide();
        }
    }
}


void TeamTreeWidget::dropEvent(QDropEvent *event)
{
    if(dragDropEventLabel != nullptr) {
        dragDropEventLabel->hide();
        delete dragDropEventLabel;
        dragDropEventLabel = nullptr;
    }

    // get the item being dragged and ensure that the item is a TeamTreeWidgetItem
    if(draggedItem == nullptr) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    // get the item currently under the cursor and ensure that it is a TeamTreeWidgetItem and not a section
    QTreeWidgetItem* itemUnderCursor = itemAt(event->position().toPoint());
    droppedItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);
    if((droppedItem == nullptr) || (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::section)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    const bool draggedItemIsStudent = (draggedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const bool droppedItemIsStudent = (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const auto *const draggedItemParent = draggedItem->parent();
    const auto *const droppedItemParent = droppedItem->parent();

    if((draggedItem == droppedItem) || (!draggedItemIsStudent && droppedItemIsStudent) || (draggedItem->parent() == droppedItem) ||
        (draggedItemParent == nullptr) || (droppedItemParent == nullptr)) {
        // ignore if dragging item onto self, team->student, or student->own team, or if something went wrong looking up the team for this student
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
    else if(draggedItemIsStudent && droppedItemIsStudent) {
        // swapping two students
        // verify they want this if separated sections and dragging between different sections
        if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
            draggedItemParent->parent() != nullptr && droppedItemParent->parent() != nullptr &&
            draggedItemParent->parent() != droppedItemParent->parent()) {
                const bool okClear = grueprGlobal::warningMessage(this, "gruepr",
                                                                  tr("You are swapping students between different sections.\n"
                                                                     "Are you sure you want to continue?"),
                                                                  tr("Yes"), tr("No"));
                if(!okClear) {
                    event->setDropAction(Qt::IgnoreAction);
                    event->ignore();
                }
                else {
                    emit swapStudents({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                                      (draggedItem->data(0, Qt::UserRole)).toInt(),
                                       droppedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                                      (droppedItem->data(0, Qt::UserRole)).toInt()});
                }
        }
        else {
            emit swapStudents({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                              (draggedItem->data(0, Qt::UserRole)).toInt(),
                               droppedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                              (droppedItem->data(0, Qt::UserRole)).toInt()});
        }
    }
    else if(draggedItemIsStudent && !droppedItemIsStudent && (draggedItemParent->childCount() != 1)) {
        // dragging student onto team and not the only student left on the team
        // verify they want this if separated sections and dragging between different sections
        if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
           draggedItemParent->parent() != nullptr && draggedItemParent->parent() != droppedItemParent) {
            const bool okClear = grueprGlobal::warningMessage(this, "gruepr",
                                                              tr("You are moving a student to a team in a different section.\n"
                                                                 "Are you sure you want to continue?"),
                                                              tr("Yes"), tr("No"));
            if(!okClear) {
                event->setDropAction(Qt::IgnoreAction);
                event->ignore();
            }
            else {
                emit moveStudent({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                                 (draggedItem->data(0, Qt::UserRole)).toInt(),
                                  droppedItem->data(0, TEAM_NUMBER_ROLE).toInt()});
            }
        }
        else {
            emit moveStudent({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                             (draggedItem->data(0, Qt::UserRole)).toInt(),
                              droppedItem->data(0, TEAM_NUMBER_ROLE).toInt()});
        }
    }
    else if(!draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging team onto teams in order to reorder
        // if these are teams with separated sections, only allow dragging within the section
        if((draggedItemParent == nullptr) || (droppedItemParent == nullptr) ||
          ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent == droppedItemParent))) {
            emit reorderTeams({(draggedItem->data(0, TEAM_NUMBER_ROLE)).toInt(),
                               (droppedItem->data(0, TEAM_NUMBER_ROLE)).toInt()});
        }
        else {
            event->setDropAction(Qt::IgnoreAction);
            event->ignore();
        }
    }
    else {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
}


void TeamTreeWidget::resorting(int column)
{
    auto sortOrder = headerView->sortIndicatorOrder();
    for(int i = 0; i < columnCount(); i++) {
        if(i != column) {
            headerView->setColumnIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
        }
        else if(sortOrder == Qt::AscendingOrder){
            headerView->setColumnIcon(column, QIcon(":/icons_new/downButton_white.png"));
        }
        else { // sortOrder == Qt::DescendingOrder
            headerView->setColumnIcon(column, QIcon(":/icons_new/upButton_white.png"));
        }
    }
    emit updateTeamOrder();
}


void TeamTreeWidget::itemEntered(const QModelIndex &index)
{
    // select the item cursor is hovered over
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}


void TeamTreeWidget::leaveEvent(QEvent *event)
{
    selectionModel()->clearSelection();
    if(event != nullptr) {
        QWidget::leaveEvent(event);
    }
}

QColor TeamTreeWidget::scoreToColor(float score)
{
    // Clamp to [0, 1] for color mapping (scores can exceed 1 for schedule extra credit)
    const float clamped = std::clamp(score, 0.0f, 1.0f);

    // Interpolate from red (0) through yellow (0.5) to green (1)
    int r, g;
    if (clamped < 0.5f) {
        // Red to yellow
        r = 255;
        g = static_cast<int>(255 * (clamped / 0.5f));
    } else {
        // Yellow to green
        r = static_cast<int>(255 * ((1.0f - clamped) / 0.5f));
        g = 200;
    }

    // Use a light, semi-transparent tint so text remains readable
    return QColor(r, g, 80, 60);
}

std::optional<float> TeamTreeWidget::findCriterionScore(const TeamRecord &team, const TeamingOptions *teamingOptions,
                                                        Criterion::CriteriaType type, int attributeIndex)
{
    for (int i = 0; i < teamingOptions->realNumScoringFactors; i++) {
        const auto *criterion = teamingOptions->criteria[i];
        if (criterion == nullptr || criterion->criteriaType != type) {
            continue;
        }
        if (type == Criterion::CriteriaType::attributeQuestion) {
            const auto *attrCrit = qobject_cast<const AttributeCriterion*>(criterion);
            if (attrCrit == nullptr || attrCrit->attributeIndex != attributeIndex) {
                continue;
            }
        }
        // qDebug() << "findCriterionScore: type=" << (int)type
        //          << "attrIdx=" << attributeIndex
        //          << "found at i=" << i
        //          << "score=" << team.criteriaScores[i];
        return team.criteriaScores[i];
    }
    return std::nullopt;
}

///////////////////////////////////////////////////////////////////////

TeamTreeHeaderView::TeamTreeHeaderView(Qt::Orientation orientation, TeamTreeWidget *parent)
    :QHeaderView(orientation, parent), m_elideMode(Qt::ElideMiddle), m_lineCount(1), m_iconSize(ICONSIZE, ICONSIZE)
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setSectionResizeMode(QHeaderView::Interactive);
    setDefaultAlignment(Qt::AlignCenter);
    setStretchLastSection(false);
    setStyleSheet(TEAMTREEWIDGETHEADERSTYLE);
    setMaximumSectionSize(MAX_SECTION_WIDTH);

    connect(this, &TeamTreeHeaderView::sectionClicked, parent, &TeamTreeWidget::resorting);
    connect(this, &TeamTreeHeaderView::sectionResized, this, &TeamTreeHeaderView::updateHeaderHeight);
}

void TeamTreeHeaderView::setElideMode(Qt::TextElideMode mode) {
    m_elideMode = mode;
    viewport()->update();
}

void TeamTreeHeaderView::setColumnElideMode(int column, Qt::TextElideMode mode) {
    m_columnElideModes[column] = mode;
    viewport()->update();
}

void TeamTreeHeaderView::setColumnIcon(int column, const QIcon &icon) {
    m_columnIcons[column] = icon;
    viewport()->update();
}

void TeamTreeHeaderView::setIconSize(const QSize &size) {
    m_iconSize = size;
    updateHeaderHeight();
    viewport()->update();
}

void TeamTreeHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid())
        return;

    // Get the full text
    const QString fullText = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    m_fullTexts[logicalIndex] = fullText;

    // Prepare style option
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = rect;
    opt.section = logicalIndex;

    // Check if we have an icon
    const bool hasIcon = m_columnIcons.contains(logicalIndex) && !m_columnIcons[logicalIndex].isNull();

    // Calculate available text width
    int textWidth = rect.width() - 4; // padding
    if (hasIcon) {
        textWidth -= (m_iconSize.width() + 4);
    }

    // Get the elide mode for this section
    Qt::TextElideMode columnElideMode = m_elideMode;
    if (m_columnElideModes.contains(logicalIndex)) {
        columnElideMode = m_columnElideModes[logicalIndex];
    }

    // Check if text would be elided
    const QFontMetrics fm(font());
    const bool wouldElide = fm.horizontalAdvance(fullText) > textWidth;

    // For middle eliding, switch to word wrap if text would be elided
    if (wouldElide) {
        const int prevLineCount = m_lineCountPerColumn.value(logicalIndex, 1);
        const QString wrappedText = wrapText(logicalIndex, fullText, textWidth, fm);
        opt.text = wrappedText;

        if (m_lineCountPerColumn.value(logicalIndex, 1) != prevLineCount) {
            QTimer::singleShot(0, const_cast<TeamTreeHeaderView*>(this),
                               &TeamTreeHeaderView::updateHeaderHeight);
        }
    }
    else {
        // Use normal eliding for other modes
        m_lineCountPerColumn[logicalIndex] = 1;
        opt.text = fm.elidedText(fullText, columnElideMode, textWidth);
    }
    opt.textAlignment = Qt::AlignLeft | Qt::AlignVCenter;

    // Set icon in the option if we have one
    if (hasIcon) {
        opt.icon = m_columnIcons[logicalIndex];
        opt.iconAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    // Let the style draw everything including the icon
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
}

QSize TeamTreeHeaderView::sectionSizeFromContents(int logicalIndex) const {
    QSize size = QHeaderView::sectionSizeFromContents(logicalIndex);
    if (m_lineCountPerColumn.contains(logicalIndex) && m_lineCountPerColumn[logicalIndex] > 1) {
        const int height = (fontMetrics().height() * m_lineCountPerColumn[logicalIndex]) + 12;
        size.setHeight(qMax(size.height(), height));
    }
    return size;
}

bool TeamTreeHeaderView::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        const auto *const helpEvent = static_cast<QHelpEvent *>(e);
        const int column = logicalIndexAt(helpEvent->pos());

        if (column >= 0 && m_fullTexts.contains(column)) {
            const QString fullText = m_fullTexts[column];

            // Only show tooltip if text is elided
            const QFontMetrics fm(font());
            int sectionWidth = sectionSize(column) - 4;
            if (m_columnIcons.contains(column)) {
                sectionWidth -= m_iconSize.width() + 4;
            }

            if (fm.horizontalAdvance(fullText) > sectionWidth) {
                QToolTip::showText(helpEvent->globalPos(), fullText);
                return true;
            }
        }
        QToolTip::hideText();
        return true;
    }
    return QHeaderView::event(e);
}

void TeamTreeHeaderView::resizeEvent(QResizeEvent *e)
{
    QHeaderView::resizeEvent(e);
    updateHeaderHeight();
}

void TeamTreeHeaderView::updateHeaderHeight()
{
    if (!model())
        return;

    int maxLines = 1;
    for (const auto lines : std::as_const(m_lineCountPerColumn)) {
        maxLines = std::max(maxLines, lines);
    }
    const int maxHeight = (fontMetrics().height() * maxLines) + 12;

    setMinimumHeight(maxHeight);
    updateGeometry();
}

QString TeamTreeHeaderView::wrapText(int logicalIndex, const QString &text, int availableWidth, const QFontMetrics &fm) const
{
    const QStringList words = text.split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty()) {
        m_lineCountPerColumn[logicalIndex] = 1;
        return text;
    }

    QStringList lines;
    QString currentLine = words.first();

    for (int i = 1; i < words.size(); i++) {
        const QString candidate = currentLine + " " + words[i];
        if (fm.horizontalAdvance(candidate) <= availableWidth) {
            currentLine = candidate;
        } else {
            lines << currentLine;
            currentLine = words[i];
        }
    }
    lines << currentLine;

    // Elide any lines that are still too wide (single long words)
    for (auto &line : lines) {
        if (fm.horizontalAdvance(line) > availableWidth) {
            line = fm.elidedText(line, Qt::ElideRight, availableWidth);
        }
    }

    m_lineCountPerColumn[logicalIndex] = lines.size();
    return lines.join("\n");
}


///////////////////////////////////////////////////////////////////////

TeamTreeWidgetItem::TeamTreeWidgetItem(TreeItemType type, int columns, float teamScore)
{
    treeItemType = type;
    if(treeItemType == TreeItemType::team && columns > 0) {
        for(int col = 0; col < columns; col++) {
            setForeground(col, Qt::black);
        }
        setScoreColor(teamScore);
    }
    if(treeItemType == TreeItemType::section) {
        //sections are fixed--they cannot be dragged or dropped onto
        setFlags(flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }
}

void TeamTreeWidgetItem::setScoreColor(float teamScore)
{
    // if(teamScore < 0) {
    //     setBackground(1, QColor::fromString(STARFISHHEX));
    // }
    // else {
    setBackground(1, background(0));
    //}
}


bool TeamTreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    if(treeItemType != TreeItemType::team) {      // only sort the teams
        return false;
    }

    const int sortColumn = treeWidget()->sortColumn();

    // sort using sortorder data in column, and use existing order to break ties
    return((data(sortColumn, TEAMINFO_SORT_ROLE).toDouble() != other.data(sortColumn, TEAMINFO_SORT_ROLE).toDouble()) ?
                (data(sortColumn, TEAMINFO_SORT_ROLE).toDouble() < other.data(sortColumn, TEAMINFO_SORT_ROLE).toDouble()) :
                (data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt() < other.data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt()));
}
