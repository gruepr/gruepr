#include "teamTreeWidget.h"
#include "gruepr_globals.h"
#include <QDropEvent>


//////////////////
// Tree display for teammates with swappable positions and sortable columns using hidden data
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setHeader(new TeamTreeHeaderView(this));
    setStyleSheet(QString(TEAMTREEWIDGETSTYLE) + SCROLLBARSTYLE);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(false);
    header()->setStyleSheet(TEAMTREEWIDGETHEADERSTYLE);
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
    connect(this, &QTreeWidget::itemCollapsed, this, &TeamTreeWidget::itemCollapsed);
}


void TeamTreeWidget::itemCollapsed(QTreeWidgetItem *item)
{
    // only collapse teams (not students or sections)
    auto newItem = dynamic_cast<TeamTreeWidgetItem*>(item);
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


void TeamTreeWidget::collapseAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, collapsing only the teams
    auto item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
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
}


void TeamTreeWidget::expandAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, expanding every section and team
    auto item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
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
}


void TeamTreeWidget::resetDisplay(const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    QStringList headerLabels;
    headerLabels << tr("  Name  ") << tr("  Score  ");
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        headerLabels << tr("  Sections  ");
    }
    if(dataOptions->genderIncluded) {
        if(dataOptions->genderType == GenderType::pronoun) {
            headerLabels << tr("  Pronouns  ");
        }
        else {
            headerLabels << tr("  Gender  ");
        }
    }
    if(dataOptions->URMIncluded) {
        headerLabels << tr("  URM  ");
    }
    const int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++) {
        headerLabels << tr("  Q") + QString::number(attribute+1) + "  ";
    }
    if(dataOptions->timezoneIncluded) {
        headerLabels << tr("  Timezone  ");
    }
    if(!dataOptions->dayNames.isEmpty()) {
        headerLabels << tr("  Meeting  \n  times  ");
    }
    headerLabels << tr("display_order");

    setColumnCount(int(headerLabels.size()));
    for(int i = 0; i < headerLabels.size()-1; i++) {
        showColumn(i);
    }
    hideColumn(int(headerLabels.size())-1);  // don't show the sort order column (can comment this out when debugging sorting operations)

    auto *headerTextWithIcon = new QTreeWidgetItem;
    for(int i = 0; i < headerLabels.size(); i++) {
        headerTextWithIcon->setIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
        headerTextWithIcon->setText(i, headerLabels.at(i));
    }
    setHeaderItem(headerTextWithIcon);
    setSortingEnabled(false);
    header()->setDefaultAlignment(Qt::AlignCenter);
    header()->setSectionResizeMode(QHeaderView::Interactive);

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


void TeamTreeWidget::refreshTeam(TeamTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum, const QString &firstStudentName,
                                 const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    if(teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) {
        return;
    }

    //create team items and fill in information
    int column = 0;
    teamItem->setText(column, tr("Team ") + team.name);
    teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, tr("Team ") + team.name);
    teamItem->setData(column, TEAMINFO_SORT_ROLE, firstStudentName);
    teamItem->setData(column, TEAM_NUMBER_ROLE, teamNum);
    teamItem->setToolTip(column, team.tooltip);
    column++;
    teamItem->setText(column, ((team.size > 1)? (QString::number(double(team.score), 'f', 2)) : ("  --  ")));
    teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(double(team.score), 'f', 2));
    teamItem->setData(column, TEAMINFO_SORT_ROLE, team.score);
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
    if(dataOptions->genderIncluded) {
        QStringList genderInitials;
        if(dataOptions->genderType == GenderType::biol) {
            genderInitials = QString(BIOLGENDERSINITIALS).split('/');
        }
        else if(dataOptions->genderType == GenderType::adult) {
            genderInitials = QString(ADULTGENDERSINITIALS).split('/');
        }
        else if(dataOptions->genderType == GenderType::child) {
            genderInitials = QString(CHILDGENDERSINITIALS).split('/');
        }
        else {          //if(dataOptions->genderResponses == GenderType::pronoun)
            genderInitials = QString(PRONOUNSINITIALS).split('/');
        }
        QString genderText;
        if(team.numWomen > 0) {
            genderText += QString::number(team.numWomen) + genderInitials.at(static_cast<int>(Gender::woman));
            if(team.numMen > 0 || team.numNonbinary > 0 || team.numUnknown > 0) {
                genderText += ", ";
            }
        }
        if(team.numMen > 0) {
            genderText += QString::number(team.numMen) + genderInitials.at(static_cast<int>(Gender::man));
            if(team.numNonbinary > 0 || team.numUnknown > 0) {
                genderText += ", ";
            }
        }
        if(team.numNonbinary > 0) {
            genderText += QString::number(team.numNonbinary) + genderInitials.at(static_cast<int>(Gender::nonbinary));
            if(team.numUnknown > 0) {
                genderText += ", ";
            }
        }
        if(team.numUnknown > 0) {
            genderText += QString::number(team.numUnknown) + genderInitials.at(static_cast<int>(Gender::unknown));
        }
        teamItem->setText(column, genderText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, genderText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numMen - team.numWomen);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(dataOptions->URMIncluded) {
        teamItem->setText(column, QString::number(team.numURM));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(team.numURM));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numURM);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    const int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++) {
        QString attributeText;
        int sortData;
        auto firstTeamVal = team.attributeVals[attribute].cbegin();
        auto lastTeamVal = team.attributeVals[attribute].crbegin();
        if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
            (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
            // attribute is ordered/numbered, so important info is the range of values (but ignore any "unset/unknown" values of -1)
            if(*firstTeamVal == -1) {
                firstTeamVal++;
            }

            if(firstTeamVal != team.attributeVals[attribute].cend()) {
                if(*firstTeamVal == *lastTeamVal) {
                    attributeText = QString::number(*firstTeamVal);
                }
                else {
                    attributeText = QString::number(*firstTeamVal) + " - " + QString::number(*lastTeamVal);
                }
                sortData = *firstTeamVal * 100 + *lastTeamVal;
            }
            else {
                //only attribute value was -1
                attributeText = "?";
                sortData = -1;
            }
        }
        else {
            // attribute is categorical or multicategorical, so important info is the list of values
            // if attribute has "unset/unknown" value of -1, char is nicely '?'; if attribute value is > 26, letters are repeated as needed
            attributeText = (*firstTeamVal <= 26 ? QString(char(*firstTeamVal - 1 + 'A')) : QString(char((*firstTeamVal - 1)%26 + 'A')).repeated(1+((*firstTeamVal - 1)/26)));
            for(auto val = std::next(firstTeamVal); val != team.attributeVals[attribute].end(); val++) {
                attributeText += ", ";
                attributeText += (*val <= 26 ? QString(char(*val - 1 + 'A')) : QString(char((*val - 1)%26 + 'A')).repeated(1+((*val - 1)/26)));
            }
            // sort by first item, then number of items, then second item
            sortData = (*firstTeamVal * 10000) + (int(team.attributeVals[attribute].size()) * 100) + (int(team.attributeVals[attribute].size()) > 1 ? *lastTeamVal : 0);
        }
        teamItem->setText(column, attributeText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, attributeText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(dataOptions->timezoneIncluded) {
        const float firstTeamVal = *(team.timezoneVals.cbegin());
        const float lastTeamVal = *(team.timezoneVals.crbegin());
        QString timezoneText;
        if(firstTeamVal == lastTeamVal) {
            const int hour = int(firstTeamVal);
            const int minutes = 60*(firstTeamVal - int(firstTeamVal));
            timezoneText = QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(std::abs(minutes), 2, 10, QChar('0'));;
        }
        else {
            const int hourF = int(firstTeamVal);
            const int minutesF = 60*(firstTeamVal - int(firstTeamVal));
            const int hourL = int(lastTeamVal);
            const int minutesL = 60*(lastTeamVal - int(lastTeamVal));
            timezoneText = QString("%1%2:%3").arg(hourF >= 0 ? "+" : "").arg(hourF).arg(std::abs(minutesF), 2, 10, QChar('0')) + " " + RIGHTARROW + " " +
                            QString("%1%2:%3").arg(hourL >= 0 ? "+" : "").arg(hourL).arg(std::abs(minutesL), 2, 10, QChar('0'));
        }
        const int sortData = int(firstTeamVal * 100 + lastTeamVal);

        teamItem->setText(column, timezoneText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, timezoneText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(!dataOptions->dayNames.isEmpty()) {
        const int numAvailTimes = team.numMeetingTimes;
        teamItem->setText(column, ((team.size > 1)? (QString::number(numAvailTimes)) : ("  --  ")));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(numAvailTimes));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, numAvailTimes);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
}


void TeamTreeWidget::refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu, const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    if(studentItem->treeItemType != TeamTreeWidgetItem::TreeItemType::student) {
        return;
    }

    int column = 0;
    studentItem->setText(column, stu.firstname + " " + stu.lastname);
    studentItem->setData(column, Qt::UserRole, stu.ID);
    studentItem->setToolTip(column, stu.tooltip);
    studentItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    column++;
    // blank teamscore column, but show a tooltip if hovered
    studentItem->setText(column, " ");
    studentItem->setToolTip(column, stu.tooltip);
    column++;
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        studentItem->setText(column, stu.section);
        studentItem->setToolTip(column, stu.tooltip);
        column++;
    }
    if(dataOptions->genderIncluded) {
        QStringList genderOptions;
        if(dataOptions->genderType == GenderType::biol) {
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::adult) {
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::child) {
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else {         //if(dataOptions->genderType == GenderType::pronoun)
            genderOptions = QString(PRONOUNS).split('/');
        }
        studentItem->setText(column, genderOptions.at(static_cast<int>(stu.gender)));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(dataOptions->URMIncluded) {
        if(stu.URM) {
            studentItem->setText(column,tr("yes"));

        }
        else {
            studentItem->setText(column,"");
        }
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    const int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++) {
        auto value = stu.attributeVals[attribute].constBegin();
        if(*value != -1) {
            if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) {
                studentItem->setText(column, QString::number(*value));
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical) {
                studentItem->setText(column, ((*value) <= 26 ? QString(char((*value)-1 + 'A')) : QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26))));
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                QString studentsVals;
                const auto lastVal = stu.attributeVals[attribute].constEnd();
                while(value != lastVal) {
                    studentsVals += ((*value) <= 26 ? QString(char((*value)-1 + 'A')) : QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)));
                    value++;
                    if(value != lastVal) {
                        studentsVals += ", ";
                    }
                }
                studentItem->setText(column, studentsVals);
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                QString studentsVals;
                const auto lastVal = stu.attributeVals[attribute].constEnd();
                while(value != lastVal) {
                    studentsVals += QString::number(*value);
                    value++;
                    if(value != lastVal) {
                        studentsVals += ", ";
                    }
                }
                studentItem->setText(column, studentsVals);
            }
        }
        else {
            studentItem->setText(column, "?");
        }
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(dataOptions->timezoneIncluded) {
        const int hour = int(stu.timezone);
        const int minutes = 60*(stu.timezone - int(stu.timezone));
        studentItem->setText(column, QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0')));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(!dataOptions->dayNames.isEmpty()) {
        const int availableTimes = stu.ambiguousSchedule? 0 : int(stu.availabilityChart.count("√"));
        studentItem->setText(column, availableTimes == 0? "--" : QString::number(availableTimes));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
    }
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
    const auto draggedItemParent = draggedItem->parent();
    const auto droppedItemParent = droppedItem->parent();

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
    const auto draggedItemParent = draggedItem->parent();
    const auto droppedItemParent = droppedItem->parent();

    if((draggedItem == droppedItem) || (!draggedItemIsStudent && droppedItemIsStudent) || (draggedItem->parent() == droppedItem)) {
        // ignore if dragging item onto self, team->student, or student->own team
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
    for(int i = 0; i < columnCount(); i++) {
        if(i != column) {
            headerItem()->setIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
        }
        else {
            headerItem()->setIcon(column, QIcon(":/icons_new/blank_arrow.png"));
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
    if(teamScore < 0) {
        setBackground(1, QColor::fromString(STARFISHHEX));
    }
    else {
        setBackground(1, background(0));
    }
}


bool TeamTreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    if(treeItemType != TreeItemType::team) {      // only sort the teams
        return false;
    }

    const int sortColumn = treeWidget()->sortColumn();

    if(sortColumn == 0) {
        return (data(sortColumn, TEAMINFO_SORT_ROLE).toString() < other.data(sortColumn, TEAMINFO_SORT_ROLE).toString());
    }

    // sort using sortorder data in column, and use existing order to break ties
    return((data(sortColumn, TEAMINFO_SORT_ROLE).toInt() != other.data(sortColumn, TEAMINFO_SORT_ROLE).toInt()) ?
               (data(sortColumn, TEAMINFO_SORT_ROLE).toInt() < other.data(sortColumn, TEAMINFO_SORT_ROLE).toInt()) :
               (data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt() < other.data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt()));
}

///////////////////////////////////////////////////////////////////////

TeamTreeHeaderView::TeamTreeHeaderView(TeamTreeWidget *parent)
    :QHeaderView(Qt::Horizontal, parent)
{
    connect(this, &TeamTreeHeaderView::sectionClicked, parent, &TeamTreeWidget::resorting);
}
