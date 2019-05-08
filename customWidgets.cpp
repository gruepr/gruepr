#include <QDateTime>
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
    return QDateTime::fromString(text(), "d-MMM. h:mm AP") < QDateTime::fromString(other.text(), "d-MMM. h:mm AP");
}


//////////////////
// Tree display for teammates with swappable positions
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    setStyleSheet("QHeaderView::section{border-top:0px solid #D8D8D8;border-left:0px solid #D8D8D8;border-right:1px solid black;"
                  "border-bottom: 1px solid black;background-color:Gainsboro;padding:4px;font-weight:bold;}"
                  "QTreeWidget::item:selected{color: black;background-color: #85cbf8}"
                  "QTreeWidget::item:hover{color: black;background-color: #85cbf8}");
    setDragDropMode(QAbstractItemView::InternalMove);
    setSortingEnabled(false);
    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(this, &QAbstractItemView::entered, this, &TeamTreeWidget::itemEntered);
}

void TeamTreeWidget::collapseItem(QTreeWidgetItem *item)
{
    if(item->parent())    // only expand the top level items (teams, not students on the team)
    {
        return;
    }

    int numWomen=0, numMen=0, numNonbinary=0, numURM = 0, numNonURM = 0, attributeMin[maxAttributes], attributeMax[maxAttributes], column;
    for (int i = 0; i < item->childCount(); i++)
    {
        QTreeWidgetItem *child = item->child(i);
        column = 0;
        column++;
        if(headerItem()->text(column) == tr("gender"))
        {
            if(child->text(column) == tr("woman"))
            {
                numWomen++;

            }
            else if(child->text(column) == tr("man"))
            {
                numMen++;

            }
            else
            {
                numNonbinary++;
            }
            column++;
        }
        if(headerItem()->text(column) == tr("URM"))
        {
            if(child->text(column) == tr("yes"))
            {
                numURM++;

            }
            else
            {
                numNonURM++;
            }
            column++;
        }
        int attribute = 0;
        while(headerItem()->text(column).contains(tr("attribute"), Qt::CaseInsensitive))
        {
            int value = (child->text(column)).toInt();
            if(value != -1)
            {
                if(i==0)
                {
                    attributeMax[attribute] = value;
                    attributeMin[attribute] = value;
                }
                if(value > attributeMax[attribute])
                {
                    attributeMax[attribute] = value;
                }
                if(value < attributeMin[attribute])
                {
                    attributeMin[attribute] = value;
                }
            }
            column++;
            attribute++;
        }
    }
    column = 0;
    column++;
    if(headerItem()->text(column) == tr("gender"))
    {
        QString genderText;
        if(numWomen > 0)
        {
            genderText += QString::number(numWomen) + tr("W");
        }
        if(numWomen > 0 && (numMen > 0 || numNonbinary > 0))
        {
            genderText += ", ";
        }
        if(numMen > 0)
        {
            genderText += QString::number(numMen) + tr("M");
        }
        if(numMen > 0 && numNonbinary > 0)
        {
            genderText += ", ";
        }
        if(numNonbinary > 0)
        {
            genderText += QString::number(numNonbinary) + tr("X");
        }
        item->setText(column, genderText);
        resizeColumnToContents(column);
        column++;
    }
    if(headerItem()->text(column) == tr("URM"))
    {
        item->setText(column, QString::number(numURM));
        resizeColumnToContents(column);
        column++;
    }
    int attribute = 0;
    while(headerItem()->text(column).contains(tr("attribute"), Qt::CaseInsensitive))
    {
        QString attributeText = QString::number(attributeMin[attribute]);
        if(attributeMin[attribute] != attributeMax[attribute])
        {
            attributeText += " - " + QString::number(attributeMax[attribute]);
        }
        item->setText(column, attributeText);
        resizeColumnToContents(column);
        column++;
        attribute++;
    }
    item->setText(column, QString::number(item->toolTip(0).count("100%")));

    QTreeWidget::collapseItem(item);
}

void TeamTreeWidget::expandItem(QTreeWidgetItem *item)
{
    if(item->parent())    // only expand the top level items (teams, not students on the team)
    {
        return;
    }
    for(int column = 1; column < columnCount(); column++)
    {
        item->setText(column, "");
        resizeColumnToContents(column);
    }

    QTreeWidget::expandItem(item);
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

    // in the tree view, students have a parent (the team number) but teams do not. Iff dragged and dropped items are both students or both teams, then swap their places.
    if(draggedItem->parent() && droppedItem->parent())  // two students
    {
        // UserRole data stored in the item is the studentRecord.ID
        emit swapChildren((draggedItem->data(0,Qt::UserRole)).toInt(), (droppedItem->data(0,Qt::UserRole)).toInt());
        emit teamInfoChanged();
    }
    else if(!(draggedItem->parent()) && !(droppedItem->parent()))   // two teams
    {
        // UserRole data stored in the item is the team number
        emit swapParents((draggedItem->data(0,Qt::UserRole)).toInt(), (droppedItem->data(0,Qt::UserRole)).toInt());
        emit teamInfoChanged();
    }
    else
    {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
}

void TeamTreeWidget::itemEntered(const QModelIndex &index)
{
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
