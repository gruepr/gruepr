#ifndef CUSTOMWIDGETS
#define CUSTOMWIDGETS

// Code related to the subclassed widgets used in gruepr

#include <QTreeWidget>
#include <QTableWidget>


class TimestampTableWidgetItem : public QTableWidgetItem
{
public:
    TimestampTableWidgetItem(const QString txt = "");
    bool operator <(const QTableWidgetItem &other) const;
};


class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseItem(QTreeWidgetItem *item);
    void expandItem(QTreeWidgetItem *item);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

signals:
    void swapChildren(int studentAID, int studentBID);
    void swapParents(int teamA, int teamB);
    void teamInfoChanged();

private:
    QTreeWidgetItem* draggedItem;
    QTreeWidgetItem* droppedItem;
};


#endif // CUSTOMWIDGETS