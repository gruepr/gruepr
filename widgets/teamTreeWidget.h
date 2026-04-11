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

// data with these roles are stored in columns of the team info display tree,
inline static const int TEAMINFO_DISPLAY_ROLE = Qt::UserRole;         // shown as the team's data value for each column
inline static const int TEAMINFO_SORT_ROLE = Qt::UserRole + 1;        // used when sorting the columns
inline static const int TEAM_NUMBER_ROLE = Qt::UserRole + 2;          // column 0 of the team info display tree, used when swapping teams or teammates
inline static const int SORT_TO_END = MAX_TEAMS + 100;                // flag to indicate moving this team to the end in the display table

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
    void refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &student,
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
    inline static const char TEAMTREEWIDGETSTYLE[] =
        "QTreeView{font-family: 'DM Sans'; font-size: 12pt;}"
        "QTreeView::branch:has-siblings:adjoins-item {border-image: url(:/icons_new/branch-more.png);}"
        "QTreeView::branch:!has-children:!has-siblings:adjoins-item {border-image: url(:/icons_new/branch-end.png);}"
        "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:closed:has-children:has-siblings {"
            "border-image: none; image: url(:/icons_new/smallRightButton.png);}"
        "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:open:has-children:has-siblings {"
            "border-image: none; image: url(:/icons_new/smallDownButton.png);}";
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
    inline static const int MAX_SECTION_WIDTH = 200;
    inline static const int MAX_HEADER_HEIGHT = 150;

    Qt::TextElideMode m_elideMode;
    QMap<int, Qt::TextElideMode> m_columnElideModes;
    mutable QMap<int, QString> m_fullTexts;
    QMap<int, QIcon> m_columnIcons;
    mutable QMap<int, int> m_lineCountPerColumn;
    int m_lineCount;
    QSize m_iconSize;
    static const int ICONSIZE = 16;
    inline static const char TEAMTREEWIDGETHEADERSTYLE[] =
        "QHeaderView {border-top: none; border-left: none; border-right: 1px solid lightGray;"
                     "border-bottom: none; background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; "
                     "font-size: 12pt; color: white; text-align:left;}"
        "QHeaderView::section {border-top: none; border-left: none; border-right: 1px solid gray; "
                              "border-bottom: none; background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; "
                              "font-size: 12pt; color: white; text-align:left;}"
        "QHeaderView::down-arrow{image: url(:/icons_new/downButton_white.png); width: 12px;"
                                 "subcontrol-origin: padding; subcontrol-position: bottom left;}"
        "QHeaderView::up-arrow{image: url(:/icons_new/upButton_white.png); width: 12px;"
                              "subcontrol-origin: padding; subcontrol-position: top left;}";
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
    Q_OBJECT

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
    Q_OBJECT

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
