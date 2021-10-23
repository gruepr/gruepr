#include "teamTreeWidget.h"
#include "gruepr_consts.h"
#include <QDropEvent>


//////////////////
// Tree display for teammates with swappable positions and sortable columns using hidden data
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setHeader(new TeamTreeHeaderView(this));
    header()->setSectionResizeMode(QHeaderView::Interactive);
    connect(this, &QTreeWidget::entered, this, &TeamTreeWidget::itemEntered);
    connect(this, &QTreeWidget::viewportEntered, this, [this] {leaveEvent(nullptr);});
    connect(this, &QTreeWidget::itemCollapsed, this, &TeamTreeWidget::collapseItem);
    connect(this, &QTreeWidget::itemExpanded, this, &TeamTreeWidget::expandItem);
}


void TeamTreeWidget::collapseItem(QTreeWidgetItem *item)
{
    bool itemIsStudent = (item->parent() != nullptr);
    if(itemIsStudent)    // only collapse the top level items (teams, not students on the team)
    {
        return;
    }

    QTreeWidget::collapseItem(item);

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
}


void TeamTreeWidget::collapseAll()
{
    setUpdatesEnabled(false);
    for(int i = 0; i < topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = topLevelItem(i);
        QTreeWidget::collapseItem(item);
    }
    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
    setUpdatesEnabled(true);
}


void TeamTreeWidget::expandItem(QTreeWidgetItem *item)
{
    bool itemIsStudent = (item->parent() != nullptr);
    if(itemIsStudent)    // only expand the top level items (teams, not students on the team)
    {
        return;
    }

    QTreeWidget::expandItem(item);

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
}


void TeamTreeWidget::expandAll()
{
    setUpdatesEnabled(false);
    for(int i = 0; i < topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = topLevelItem(i);
        QTreeWidget::expandItem(item);
    }

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
    setUpdatesEnabled(true);
}


void TeamTreeWidget::resetDisplay(const DataOptions *const dataOptions)
{
    QStringList headerLabels;
    headerLabels << tr("name") << tr("team\nscore");
    if(dataOptions->genderIncluded)
    {
        headerLabels << tr("gender");
    }
    if(dataOptions->URMIncluded)
    {
        headerLabels << tr("URM");
    }
    int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++)
    {
        headerLabels << tr("attribute ") + QString::number(attribute+1);
    }
    if(dataOptions->timezoneIncluded)
    {
        headerLabels << tr("timezone");
    }
    if(!dataOptions->dayNames.isEmpty())
    {
        headerLabels << tr("available\nmeeting\nhours");
    }
    headerLabels << tr("display_order");

    setColumnCount(headerLabels.size());
    for(int i = 0; i < headerLabels.size()-1; i++)
    {
        showColumn(i);
    }
    hideColumn(headerLabels.size()-1);

    auto *headerTextWithIcon = new QTreeWidgetItem;
    for(int i = 0; i < headerLabels.size(); i++)
    {
        headerTextWithIcon->setIcon(i, QIcon(":/icons/updown_arrow.png"));
        headerTextWithIcon->setText(i, headerLabels.at(i));
    }
    setHeaderItem(headerTextWithIcon);
    setSortingEnabled(false);
    header()->setDefaultAlignment(Qt::AlignCenter);
    header()->setSectionResizeMode(QHeaderView::Interactive);

    setFocus();
    clear();
}


void TeamTreeWidget::refreshTeam(QTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum, const QString &firstStudentName, const DataOptions *const dataOptions)
{
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
    if(dataOptions->genderIncluded)
    {
        QString genderText;
        if(team.numWomen > 0)
        {
            genderText += QString::number(team.numWomen) + tr("W");
        }
        if(team.numWomen > 0 && (team.numMen > 0 || team.numNonbinary > 0 || team.numUnknown > 0))
        {
            genderText += ", ";
        }
        if(team.numMen > 0)
        {
            genderText += QString::number(team.numMen) + tr("M");
        }
        if(team.numMen > 0 && (team.numNonbinary > 0 || team.numUnknown > 0))
        {
            genderText += ", ";
        }
        if(team.numNonbinary > 0)
        {
            genderText += QString::number(team.numNonbinary) + tr("X");
        }
        if(team.numNonbinary > 0 && team.numUnknown > 0)
        {
            genderText += ", ";
        }
        if(team.numUnknown > 0)
        {
            genderText += QString::number(team.numUnknown) + tr("?");
        }
        teamItem->setText(column, genderText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, genderText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numMen - team.numWomen);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(dataOptions->URMIncluded)
    {
        teamItem->setText(column, QString::number(team.numURM));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(team.numURM));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numURM);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++)
    {
        QString attributeText;
        int sortData;
        auto firstTeamVal = team.attributeVals[attribute].begin();
        auto lastTeamVal = team.attributeVals[attribute].rbegin();
        if(dataOptions->attributeIsOrdered[attribute])
        {
            // attribute is ordered/numbered, so important info is the range of values (but ignore any "unset/unknown" values of -1)
            if(*firstTeamVal == -1)
            {
                firstTeamVal++;
            }

            if(firstTeamVal != team.attributeVals[attribute].end())
            {
                if(*firstTeamVal == *lastTeamVal)
                {
                    attributeText = QString::number(*firstTeamVal);
                }
                else
                {
                    attributeText = QString::number(*firstTeamVal) + " - " + QString::number(*lastTeamVal);
                }
                sortData = *firstTeamVal * 100 + *lastTeamVal;
            }
            else
            {
                //only attribute value was -1
                attributeText = "?";
                sortData = -1;
            }
        }
        else
        {
            // attribute is categorical, so important info is the list of values
            // if attribute has "unset/unknown" value of -1, char is nicely '?'; if attribute value is > 26, letters are repeated as needed
            attributeText = (*firstTeamVal <= 26 ? QString(char(*firstTeamVal - 1 + 'A')) : QString(char((*firstTeamVal - 1)%26 + 'A')).repeated(1+((*firstTeamVal - 1)/26)));
            for(auto val = std::next(firstTeamVal); val != team.attributeVals[attribute].end(); val++)
            {
                attributeText += ", ";
                attributeText += (*val <= 26 ? QString(char(*val - 1 + 'A')) : QString(char((*val - 1)%26 + 'A')).repeated(1+((*val - 1)/26)));
            }
            // sort by first item, then number of items, then second item
            sortData = (*firstTeamVal * 10000) + (team.attributeVals[attribute].size() * 100) + (team.attributeVals[attribute].size() > 1 ? *lastTeamVal : 0);
        }
        teamItem->setText(column, attributeText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, attributeText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(dataOptions->timezoneIncluded)
    {
        QString timezoneText;
        int sortData;
        auto firstTeamVal = team.timezoneVals.begin();
        auto lastTeamVal = team.timezoneVals.rbegin();

        if(*firstTeamVal == *lastTeamVal)
        {
            int hour = int(*firstTeamVal);
            int minutes = 60*(*firstTeamVal - int(*firstTeamVal));
            timezoneText = QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0'));;
        }
        else
        {
            int hourF = int(*firstTeamVal);
            int minutesF = 60*(*firstTeamVal - int(*firstTeamVal));
            int hourL = int(*lastTeamVal);
            int minutesL = 60*(*lastTeamVal - int(*lastTeamVal));
            timezoneText = QString("%1%2:%3").arg(hourF >= 0 ? "+" : "").arg(hourF).arg(minutesF, 2, 10, QChar('0')) + " \u2192 " +
                            QString("%1%2:%3").arg(hourL >= 0 ? "+" : "").arg(hourL).arg(minutesL, 2, 10, QChar('0'));
        }
        sortData = int(*firstTeamVal * 100 + *lastTeamVal);

        teamItem->setText(column, timezoneText);
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, timezoneText);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, sortData);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
    if(!dataOptions->dayNames.isEmpty())
    {
        int numAvailTimes = team.tooltip.count("100%");
        teamItem->setText(column, ((team.size > 1)? (QString::number(numAvailTimes)) : ("  --  ")));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(numAvailTimes));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, ((team.size > 1)? numAvailTimes : 0));
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }
}


void TeamTreeWidget::refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu, const DataOptions *const dataOptions)
{
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
    if(dataOptions->genderIncluded)
    {
        if(stu.gender == StudentRecord::woman)
        {
            studentItem->setText(column,tr("woman"));

        }
        else if(stu.gender == StudentRecord::man)
        {
            studentItem->setText(column,tr("man"));
        }
        else if(stu.gender == StudentRecord::nonbinary)
        {
            studentItem->setText(column,tr("nonbinary"));
        }
        else
        {
            studentItem->setText(column,tr("unknown"));
        }

        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(dataOptions->URMIncluded)
    {
        if(stu.URM)
        {
            studentItem->setText(column,tr("yes"));

        }
        else
        {
            studentItem->setText(column,"");
        }
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    int numAttributesWOTimezone = dataOptions->numAttributes - (dataOptions->timezoneIncluded? 1 : 0);
    for(int attribute = 0; attribute < numAttributesWOTimezone; attribute++)
    {
        int value = stu.attributeVal[attribute];
        if(value != -1)
        {
            if(dataOptions->attributeIsOrdered[attribute])
            {
                studentItem->setText(column, QString::number(value));
            }
            else
            {
                studentItem->setText(column, (value <= 26 ? QString(char(value-1 + 'A')) : QString(char((value-1)%26 + 'A')).repeated(1+((value-1)/26))));
            }
        }
        else
        {
            studentItem->setText(column, "?");
        }
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(dataOptions->timezoneIncluded)
    {
        int hour = int(stu.timezone);
        int minutes = 60*(stu.timezone - int(stu.timezone));
        studentItem->setText(column, QString("%1%2:%3").arg(hour >= 0 ? "+" : "").arg(hour).arg(minutes, 2, 10, QChar('0')));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
        column++;
    }
    if(!dataOptions->dayNames.isEmpty())
    {
        int availableTimes = stu.ambiguousSchedule? 0 : stu.availabilityChart.count("√");
        studentItem->setText(column, availableTimes == 0? "--" : QString::number(availableTimes));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, Qt::AlignCenter);
    }
}


void TeamTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    draggedItem = currentItem();
    QTreeWidget::dragEnterEvent(event);

    dragDropEventLabel = new QLabel;
    dragDropEventLabel->setWindowFlag(Qt::ToolTip);
    dragDropEventLabel->setTextFormat(Qt::RichText);
}


void TeamTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    const int iconSize = 32;
    const QString iconSizeStr = QString::number(iconSize);

    QTreeWidget::dragMoveEvent(event);

    // get the item currently under the cursor and ensure that the item is a TeamTreeWidgetItem
    QTreeWidgetItem* itemUnderCursor = itemAt(event->pos());
    auto dropItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);
    if(dropItem == nullptr)
    {
        dragDropEventLabel->hide();
        return;
    }

    // adjust the location and text of the tooltip
    dragDropEventLabel->move(QCursor::pos() + QPoint(iconSize, iconSize));
    bool dragItemIsStudent = (draggedItem->parent() != nullptr), dropItemIsStudent = (dropItem->parent() != nullptr);
    if((draggedItem == dropItem) || (!dragItemIsStudent && dropItemIsStudent) || (draggedItem->parent() == dropItem))  // dragging item onto self, team->student, or student->own team
    {
        dragDropEventLabel->hide();
    }
    else if(dragItemIsStudent && dropItemIsStudent)            // dragging student->student
    {
        dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons/exchange.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                     + tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + dropItem->text(0) + "</b></div>");
        dragDropEventLabel->setStyleSheet("QLabel {background-color: #d9ffdc; color: black; border: 2px solid black;padding: 2px 2px 2px 2px;}");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
    else if(dragItemIsStudent && !dropItemIsStudent && (draggedItem->parent()->childCount() == 1))  // dragging student->team, but this is the only student left on the team
    {
        dragDropEventLabel->setText(tr("Cannot move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto another team.<br>")
                                     + " <b>" + draggedItem->parent()->text(0) + "</b> " + tr("cannot be left empty."));
        dragDropEventLabel->setStyleSheet("QLabel {background-color: #ffbdbd; color: black; border: 2px solid black;padding: 2px 2px 2px 2px;}");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
    else if(dragItemIsStudent && !dropItemIsStudent)           // dragging student->team
    {
        dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                     + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + dropItem->text(0) + "</b></div>");
        dragDropEventLabel->setStyleSheet("QLabel {background-color: #d9ffdc; color: black; border: 2px solid black;padding: 2px 2px 2px 2px;}");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
    else if(!dragItemIsStudent && !dropItemIsStudent)          // dragging team->team
    {
        dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                     + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("above") + " <b>" + dropItem->text(0) + "</b></div>");
        dragDropEventLabel->setStyleSheet("QLabel {background-color: #d9ffdc; color: black; border: 2px solid black;padding: 2px 2px 2px 2px;}");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
}


void TeamTreeWidget::dropEvent(QDropEvent *event)
{
    dragDropEventLabel->hide();
    delete dragDropEventLabel;

    droppedItem = itemAt(event->pos());
    QModelIndex droppedIndex = indexFromItem(droppedItem);
    if( !droppedIndex.isValid() )
    {
        return;
    }

    // in the tree view, students have a parent (the team number) but teams do not.
    bool dragItemIsStudent = (draggedItem->parent() != nullptr), dropItemIsStudent = (droppedItem->parent() != nullptr);
    if(dragItemIsStudent && dropItemIsStudent)          // two students
    {
        // UserRole data stored in the item is the studentRecord.ID; TeamNumber data stored in the parent's column 0 is the team number
        emit swapChildren((draggedItem->parent()->data(0,TEAM_NUMBER_ROLE)).toInt(), (draggedItem->data(0,Qt::UserRole)).toInt(),
                          (droppedItem->parent()->data(0,TEAM_NUMBER_ROLE)).toInt(), (droppedItem->data(0,Qt::UserRole)).toInt());
    }
    else if(!dragItemIsStudent && !dropItemIsStudent)   // two teams
    {
        emit reorderParents((draggedItem->data(0,TEAM_NUMBER_ROLE)).toInt(), (droppedItem->data(0,TEAM_NUMBER_ROLE)).toInt());
    }
    else if(dragItemIsStudent && !dropItemIsStudent && (draggedItem->parent()->childCount() != 1))  // dragging student onto team and not the only student left on the team
    {
        emit moveChild((draggedItem->parent()->data(0,TEAM_NUMBER_ROLE)).toInt(), (draggedItem->data(0,Qt::UserRole)).toInt(), (droppedItem->data(0,TEAM_NUMBER_ROLE)).toInt());
    }
    else
    {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
}


void TeamTreeWidget::resorting(int column)
{
    for(int i = 0; i < columnCount(); i++)
    {
        if(i != column)
        {
            headerItem()->setIcon(i, QIcon(":/icons/updown_arrow.png"));
        }
        else
        {
            headerItem()->setIcon(column, QIcon(":/icons/blank_arrow.png"));
        }
    }
    emit updateTeamOrder();
}


void TeamTreeWidget::itemEntered(const QModelIndex &index)
{
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}


void TeamTreeWidget::leaveEvent(QEvent *event)
{
    selectionModel()->clearSelection();
    if(event != nullptr)
    {
        QWidget::leaveEvent(event);
    }
}

///////////////////////////////////////////////////////////////////////

TeamTreeWidgetItem::TeamTreeWidgetItem(TreeItemType type, int columns, bool teamHasBadScore)
{
    if(type == team && columns > 0)
    {
        QFont teamFont = this->font(0);
        teamFont.setBold(true);
        QBrush teamColor;
        if(teamHasBadScore)
        {
            teamColor = QColor(0xfb, 0xcf, 0xce);
        }
        else
        {
            teamColor = QColor(0xce, 0xea, 0xfb);
        }

        for(int col = 0; col < columns; col++)
        {
            setBackground(col, teamColor);
            setForeground(col, Qt::black);
            setFont(col, teamFont);
        }
    }
}


bool TeamTreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    if(parent() != nullptr)      // don't sort the students, only the teams
    {
        return false;
    }

    int sortColumn = treeWidget()->sortColumn();

    if(sortColumn == 0)
    {
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
