#ifndef TEAMTREEWIDGET
#define TEAMTREEWIDGET

// a subclassed QTreeWidget to show teams and students with summarized data on each and special drag/drop behavior
// includes a subclassed QTreeWidgetItem and QHeaderView

#include "dataOptions.h"
#include "gruepr_globals.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>


///////////////////////////////////////////////////////////////////////

class TeamTreeWidgetItem : public QTreeWidgetItem
{
public:
    enum TreeItemType{team, student};
    TeamTreeWidgetItem(TreeItemType type, int columns = 0, float teamScore = 1);
    void setBackgroundColor(float teamScore);
    bool operator<(const QTreeWidgetItem &other) const override;

private:
    int numColumns;
};

///////////////////////////////////////////////////////////////////////

class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseItem(QTreeWidgetItem *item);           // when collapsing parent, summarize the children's data
    void collapseAll();
    void expandItem(QTreeWidgetItem *item);             // when expanding, simplify appearance by removing summary of children's data
    void expandAll();
    void resetDisplay(const DataOptions *const dataOptions);
    void refreshTeam(QTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum, const QString &firstStudentName, const DataOptions *const dataOptions);
    void refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu, const DataOptions *const dataOptions);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;        // remember which item is being dragged
    void dragMoveEvent(QDragMoveEvent *event) override;          // update tooltip during drag
    void dropEvent(QDropEvent *event) override;                  // handle when the dragged item is being dropped to allow swapping of teammates or teams
    void leaveEvent(QEvent *event) override;

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it

public slots:
    void resorting(int column);

signals:
    void swapChildren(QVector<int> arguments); // QVector<int> arguments = int studentAteam, int studentAID, int studentBteam, int studentBID); // chid onto child, swap teammates
    void reorderParents(QVector<int> arguments); // QVector<int> arguments = int teamA, int teamB); // parent onto parent, reorder teams
    void moveChild(QVector<int> arguments); // QVector<int> arguments = int studentTeam, int studentID, int NewTeam); // child onto parent, move student
    void updateTeamOrder();

private:
    QTreeWidgetItem *draggedItem = nullptr;
    QTreeWidgetItem *droppedItem = nullptr;
    QLabel *dragDropEventLabel = nullptr;
};

///////////////////////////////////////////////////////////////////////

class TeamTreeHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    TeamTreeHeaderView(TeamTreeWidget *parent = nullptr);
};

///////////////////////////////////////////////////////////////////////

#endif // TEAMTREEWIDGET
