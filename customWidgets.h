#ifndef CUSTOMWIDGETS
#define CUSTOMWIDGETS

// Code related to the subclassed widgets used in gruepr

#include <QTreeWidget>
#include <QTableWidget>


// a subclassed QTableWidgetItem that allows correct sorting according to data and time from timestamp text
class TimestampTableWidgetItem : public QTableWidgetItem
{
public:
    TimestampTableWidgetItem(const QString txt = "");
    bool operator <(const QTableWidgetItem &other) const;
};



// a subclassed QTreeWidget to show teams and students with summarized data on each
class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseItem(QTreeWidgetItem *item);           // when collapsing parent, summarize the children's data
    void expandItem(QTreeWidgetItem *item);             // when expanding, simplify appearance by removing data summary

protected:
    void dragEnterEvent(QDragEnterEvent *event);        // remember which item is being dragged
    void dropEvent(QDropEvent *event);                  // handle when the dragged item is being dropped to allow swapping of teammates or teams

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it

signals:
    void swapChildren(int studentAID, int studentBID);  // if drag-and-dropping children in the view, swap teammates
    void swapParents(int teamA, int teamB);             // "   "    "     "     parents   "  "    "    "    teams
    void teamInfoChanged();                             // need to refresh the display

private:
    QTreeWidgetItem* draggedItem;
    QTreeWidgetItem* droppedItem;
};


#endif // CUSTOMWIDGETS
