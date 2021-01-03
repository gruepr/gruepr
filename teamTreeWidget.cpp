#include "teamTreeWidget.h"
#include "gruepr_structs_and_consts.h"
#include <QDropEvent>


//////////////////
// Tree display for teammates with swappable positions and sortable columns using hidden data
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setStyleSheet("QHeaderView::section{border-top:0px solid #D8D8D8;border-left:0px solid #D8D8D8;border-right:1px solid black;"
                  "border-bottom: 1px solid black;background-color:Gainsboro;padding:4px;font-weight:bold;}"
                  "QHeaderView::down-arrow{image: url(:/icons/down_arrow.png);width:18px;subcontrol-origin:padding;subcontrol-position:bottom left;}"
                  "QHeaderView::up-arrow{image: url(:/icons/up_arrow.png);width:18px;subcontrol-origin:padding;subcontrol-position:top left;}"
                  "QTreeWidget::item:selected{color: black;background-color: #85cbf8;}"
                  "QTreeWidget::item:hover{color: black;background-color: #85cbf8;}"
                  "QTreeWidget::branch{background-color: white;}"
                  "QTreeView::branch:has-siblings:adjoins-item {border-image: url(:/icons/branch-more.png);}"
                  "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: url(:/icons/branch-end.png);}"
                  "QTreeView::branch:has-children:!has-siblings:closed,"
                  "QTreeView::branch:closed:has-children:has-siblings {border-image: none; image: url(:/icons/branch-closed.png);}"
                  "QTreeView::branch:open:has-children:!has-siblings,"
                  "QTreeView::branch:open:has-children:has-siblings {border-image: none; image: url(:/icons/branch-open.png);}");
    setHeader(new TeamTreeHeaderView(this));
    header()->setSectionResizeMode(QHeaderView::Interactive);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSortingEnabled(false);
    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(this, &QAbstractItemView::entered, this, &TeamTreeWidget::itemEntered);
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
    for(int column = 2; column < columnCount(); column++)
    {
        item->setText(column, item->data(column, Qt::UserRole).toString());
    }

    QTreeWidget::collapseItem(item);

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
}

void TeamTreeWidget::collapseAll()
{
    for(int i = 0; i < topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = topLevelItem(i);
        for(int column = 2; column < columnCount(); column++)
        {
            item->setText(column, item->data(column, Qt::UserRole).toString());
        }
        QTreeWidget::collapseItem(item);
    }
    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
}

void TeamTreeWidget::expandItem(QTreeWidgetItem *item)
{
    bool itemIsStudent = (item->parent() != nullptr);
    if(itemIsStudent)    // only expand the top level items (teams, not students on the team)
    {
        return;
    }
    for(int column = 2; column < columnCount(); column++)
    {
        item->setText(column, "");
    }

    QTreeWidget::expandItem(item);

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
    }
}

void TeamTreeWidget::expandAll()
{
    for(int i = 0; i < topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = topLevelItem(i);
        for(int column = 2; column < columnCount(); column++)
        {
            item->setText(column, "");
        }
        QTreeWidget::expandItem(item);
    }

    for(int column = 0; column < columnCount(); column++)
    {
        resizeColumnToContents(column);
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
    if((draggedItem == dropItem) || (!dragItemIsStudent && dropItemIsStudent) || (draggedItem->parent() == dropItem))  // dragging item onto itself, team->student, or student->own team
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

///////////////////////////////////////////////////////////////////////

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
    return((1000*data(sortColumn, TEAMINFO_SORT_ROLE).toInt() + data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt()) <
           (1000*other.data(sortColumn, TEAMINFO_SORT_ROLE).toInt() + other.data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt()));
}

///////////////////////////////////////////////////////////////////////

TeamTreeHeaderView::TeamTreeHeaderView(TeamTreeWidget *parent)
    :QHeaderView(Qt::Horizontal, parent)
{
    connect(this, &TeamTreeHeaderView::sectionClicked, parent, &TeamTreeWidget::resorting);
}
