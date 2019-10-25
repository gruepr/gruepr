#include <QDateTime>
#include <QCollator>
#include <QDropEvent>
#include <QPainter>
#include "customWidgets.h"
#include "gruepr_structs_and_consts.h"

//////////////////
// Table Widget Item for timestamps, allowing to sort chronologically
//////////////////
TimestampTableWidgetItem::TimestampTableWidgetItem(const QString txt)
    :QTableWidgetItem(txt)
{
}

bool TimestampTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    return QDateTime::fromString(text(), Qt::SystemLocaleShortDate) < QDateTime::fromString(other.text(), Qt::SystemLocaleShortDate);
}


//////////////////
// Table Widget Item for section names, allowing to sort alphanumerically
//////////////////
SectionTableWidgetItem::SectionTableWidgetItem(const QString txt)
    :QTableWidgetItem(txt)
{
}

bool SectionTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    QCollator sortAlphanumerically;
    sortAlphanumerically.setNumericMode(true);
    sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
    return (sortAlphanumerically.compare(text(), other.text()) < 0);
}


//////////////////
// Tree display for teammates with swappable positions
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setStyleSheet("QHeaderView::section{border-top:0px solid #D8D8D8;border-left:0px solid #D8D8D8;border-right:1px solid black;"
                  "border-bottom: 1px solid black;background-color:Gainsboro;padding:4px;font-weight:bold;}"
                  "QHeaderView::down-arrow{image: url(:/icons/down_arrow.png);width:20px;subcontrol-origin:margin;subcontrol-position:left;}"
                  "QHeaderView::up-arrow{image: url(:/icons/up_arrow.png);width:20px;subcontrol-origin:margin;subcontrol-position:left;}"
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
    if(item->parent())    // only expand the top level items (teams, not students on the team)
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
        header()->resizeSection(column, std::max(header()->sectionSizeHint(column), this->sizeHintForColumn(column)));
    }
}

void TeamTreeWidget::expandItem(QTreeWidgetItem *item)
{
    if(item->parent())    // only expand the top level items (teams, not students on the team)
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
        header()->resizeSection(column, std::max(header()->sectionSizeHint(column), this->sizeHintForColumn(column)));
    }
}

void TeamTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    draggedItem = currentItem();
    QTreeWidget::dragEnterEvent(event);
}

void TeamTreeWidget::dropEvent(QDropEvent *event)
{
    droppedItem = itemAt(event->pos());
    QModelIndex droppedIndex = indexFromItem(droppedItem);
    if( !droppedIndex.isValid() )
    {
        return;
    }

    // in the tree view, students have a parent (the team number) but teams do not.
    // Iff dragged and dropped items are both students or both teams, then swap their places.
    if(draggedItem->parent() && droppedItem->parent())  // two students
    {
        // UserRole data stored in the item is the studentRecord.ID; TeamInfoSort data stored in the parent's column 0 is the team number
        emit swapChildren((draggedItem->parent()->data(0,TeamInfoSort)).toInt(), (draggedItem->data(0,Qt::UserRole)).toInt(),
                          (droppedItem->parent()->data(0,TeamInfoSort)).toInt(), (droppedItem->data(0,Qt::UserRole)).toInt());
    }
    else if(!(draggedItem->parent()) && !(droppedItem->parent()))   // two teams
    {
        // TeamInfoSort data stored in column 0 is the team number
        emit swapParents((draggedItem->data(0,TeamInfoSort)).toInt(), (droppedItem->data(0,TeamInfoSort)).toInt());
    }
    else
    {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
}

void TeamTreeWidget::resorting(int /*column*/)
{
    emit updateTeamOrder();
}

void TeamTreeWidget::itemEntered(const QModelIndex &index)
{
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

TeamTreeWidgetItem::TeamTreeWidgetItem(QTreeWidget *parent, int type)
    :QTreeWidgetItem (parent, type)
{
}

TeamTreeWidgetItem::TeamTreeWidgetItem(QTreeWidgetItem *parent, int type)
    :QTreeWidgetItem (parent, type)
{
}

bool TeamTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    if(parent())      // don't sort the students, only the teams
    {
        return false;
    }

    int sortColumn = treeWidget()->sortColumn();

    if(sortColumn == 0)
    {
        QCollator sortAlphanumerically;
        sortAlphanumerically.setNumericMode(true);
        sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
        return (sortAlphanumerically.compare(text(sortColumn), other.text(sortColumn)) < 0);
    }

    // sort using sortorder data in column, and use teamnumber to break ties
    return((1000*data(sortColumn, TeamInfoSort).toInt() + data(0, TeamInfoSort).toInt()) <
           (1000*other.data(sortColumn, TeamInfoSort).toInt() + other.data(0, TeamInfoSort).toInt()));
}

TeamTreeHeaderView::TeamTreeHeaderView(TeamTreeWidget *parent)
    :QHeaderView(Qt::Horizontal, parent)
{
    connect(this, &TeamTreeHeaderView::sectionClicked, parent, &TeamTreeWidget::resorting);
}


//////////////////
// QPushButton that passes mouse enter events to its parent
//////////////////
PushButtonThatSignalsMouseEnterEvents::PushButtonThatSignalsMouseEnterEvents(const QIcon &icon, const QString &text, QWidget *parent)
    :QPushButton (icon, text, parent)
{
    this->setFlat(true);
    this->setMouseTracking(true);
    this->setIconSize(QSize(20,20));
}

void PushButtonThatSignalsMouseEnterEvents::enterEvent(QEvent *event)
{
    emit mouseEntered();
    event->ignore();
}
