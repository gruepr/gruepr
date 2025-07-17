#ifndef TEAMTREEWIDGET
#define TEAMTREEWIDGET

// a subclassed QTreeWidget to show teams and students with summarized data on each and special drag/drop behavior
// includes a subclassed QTreeWidgetItem and QHeaderView

#include "dataOptions.h"
#include "studentRecord.h"
#include "teamRecord.h"
#include "teamingOptions.h"
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QStyledItemDelegate>
#include <QTreeWidget>

// Need a couple of forward declarations; both are defined below
class TeamTreeHeaderView;
class TeamTreeWidgetItem;

class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseAll();
    void expandAll();
    void resetDisplay(const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions);
    void refreshSection(TeamTreeWidgetItem *sectionItem, const QString &sectionName);
    enum class RefreshType{newTeam, existingTeam};
    void refreshTeam(RefreshType refreshType, TeamTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum, const QString &firstStudentName,
                     const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions);
    void refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu,
                        const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions);
    void setColumnHeaderIcon(int column, const QIcon &icon);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;        // remember which item is being dragged
    void dragLeaveEvent(QDragLeaveEvent *event) override;        // get rid of tooltip if drag leaves
    void dragMoveEvent(QDragMoveEvent *event) override;          // update tooltip during drag
    void dropEvent(QDropEvent *event) override;                  // handle when the dragged item is being dropped to allow swapping of teammates or teams
    void leaveEvent(QEvent *event) override;

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it
    void itemCollapse(QTreeWidgetItem *item);
    void itemExpand(QTreeWidgetItem *item);

public slots:
    void resorting(int column);

signals:
    void swapStudents(const QList<int> &arguments);    // QList<int> arguments = int studentAteam, int studentAID, int studentBteam, int studentBID); // student onto student -> swap
    void reorderTeams(const QList<int> &arguments);    // QList<int> arguments = int teamA, int teamB); // team onto team -> reorder
    void moveStudent(const QList<int> &arguments);     // QList<int> arguments = int oldTeam, int studentID, int newTeam); // student onto team -> move student
    void updateTeamOrder();

private:
    TeamTreeHeaderView *m_header = nullptr;
    TeamTreeWidgetItem *draggedItem = nullptr;
    TeamTreeWidgetItem *droppedItem = nullptr;
    QLabel *dragDropEventLabel = nullptr;
};

///////////////////////////////////////////////////////////////////////

class TeamTreeHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    TeamTreeHeaderView(Qt::Orientation orientation, TeamTreeWidget *parent = nullptr);

    void setElideMode(Qt::TextElideMode mode);
    void setColumnElideMode(int column, Qt::TextElideMode mode);
    void setColumnIcon(int column, const QIcon &icon);
    void setIconSize(const QSize &size);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    QSize sectionSizeFromContents(int logicalIndex) const override;
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void updateHeaderHeight();
    QString wrapTextToTwoLines(int logicalIndex, const QString &text, int availableWidth, const QFontMetrics &fm) const;

    Qt::TextElideMode m_elideMode;
    QMap<int, Qt::TextElideMode> m_columnElideModes;
    mutable QMap<int, QString> m_fullTexts;
    QMap<int, QIcon> m_columnIcons;
    mutable QMap<int, bool> m_wordWrappeds;
    int m_lineCount;
    QSize m_iconSize;
};

///////////////////////////////////////////////////////////////////////

class TeamTreeWidgetItem : public QTreeWidgetItem
{
public:
    enum class TreeItemType{section, team, student} treeItemType;
    explicit TeamTreeWidgetItem(TreeItemType type, int columns = 0, float teamScore = 1);
    ~TeamTreeWidgetItem() override = default;
    TeamTreeWidgetItem(const TeamTreeWidgetItem&) = delete;
    TeamTreeWidgetItem operator= (const TeamTreeWidgetItem&) = delete;
    TeamTreeWidgetItem(TeamTreeWidgetItem&&) = delete;
    TeamTreeWidgetItem& operator= (TeamTreeWidgetItem&&) = delete;
    void setScoreColor(float teamScore);

private:
    bool operator<(const QTreeWidgetItem &other) const override;
};

///////////////////////////////////////////////////////////////////////

#endif // TEAMTREEWIDGET
