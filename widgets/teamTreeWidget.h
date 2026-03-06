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
#include <QPainter>
#include <QProxyStyle>
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
    void refreshTeam(RefreshType refreshType, TeamTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum,
                     const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions,
                     const QList<StudentRecord> &students, const QSet<long long> &IDsBeingTeamed);
    void refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu,
                        const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions);
    void setColumnHeaderIcon(int column, const QIcon &icon);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;        // remember which item is being dragged
    void dragLeaveEvent(QDragLeaveEvent *event) override;        // get rid of tooltip if drag leaves
    void dragMoveEvent(QDragMoveEvent *event) override;          // update tooltip during drag
    void dropEvent(QDropEvent *event) override;                  // handle when the dragged item is being dropped to allow swapping of teammates or teams

private slots:
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
    TeamTreeHeaderView *headerView = nullptr;
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
    QString wrapText(int logicalIndex, const QString &text, int availableWidth, const QFontMetrics &fm) const;
    static const int MAX_SECTION_WIDTH = 200;

    Qt::TextElideMode m_elideMode;
    QMap<int, Qt::TextElideMode> m_columnElideModes;
    mutable QMap<int, QString> m_fullTexts;
    QMap<int, QIcon> m_columnIcons;
    mutable QMap<int, int> m_lineCountPerColumn;
    int m_lineCount;
    QSize m_iconSize;
    static const int ICONSIZE = 16;
};

///////////////////////////////////////////////////////////////////////

class TeamTreeWidgetItem : public QTreeWidgetItem
{
public:
    enum class TreeItemType{section, team, student} treeItemType;
    explicit TeamTreeWidgetItem(TreeItemType type, int columns = 0);
    ~TeamTreeWidgetItem() override = default;
    TeamTreeWidgetItem(const TeamTreeWidgetItem&) = delete;
    TeamTreeWidgetItem operator= (const TeamTreeWidgetItem&) = delete;
    TeamTreeWidgetItem(TeamTreeWidgetItem&&) = delete;
    TeamTreeWidgetItem& operator= (TeamTreeWidgetItem&&) = delete;

private:
    bool operator<(const QTreeWidgetItem &other) const override;
};

///////////////////////////////////////////////////////////////////////
// Two classes to handle translucent highlighting of each row on hovering the mouse

class NoHoverDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        // Save hover state, then strip it so platform doesn't paint its own highlight
        option->state &= ~QStyle::State_Selected;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const bool hovered = option.state & QStyle::State_MouseOver;

        // Strip hover before default painting
        QStyleOptionViewItem opt(option);
        opt.state &= ~QStyle::State_MouseOver;
        QStyledItemDelegate::paint(painter, opt, index);

        // Paint translucent overlay if hovered
        if (hovered) {
            QColor overlay(OPENWATERHEX);
            overlay.setAlpha(33);
            painter->fillRect(option.rect, overlay);
        }
    }
};

class NoHoverStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override
    {
        if (element == PE_PanelItemViewRow || element == PE_PanelItemViewItem) {
            QStyleOption opt(*option);
            const bool hovered = opt.state & State_MouseOver;
            opt.state &= ~(State_MouseOver | State_Selected);
            QProxyStyle::drawPrimitive(element, &opt, painter, widget);
            if (hovered) {
                QColor overlay(OPENWATERHEX);
                overlay.setAlpha(33);
                painter->fillRect(opt.rect, overlay);
            }
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

///////////////////////////////////////////////////////////////////////

#endif // TEAMTREEWIDGET
