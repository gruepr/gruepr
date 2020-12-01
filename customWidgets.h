#ifndef CUSTOMWIDGETS
#define CUSTOMWIDGETS

// Code related to the subclassed widgets used in gruepr

#include <QTreeWidget>
#include <QHeaderView>
#include <QTableWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>


// a subclassed QTableWidgetItem that allows correct sorting according to data and time from timestamp text
class TimestampTableWidgetItem : public QTableWidgetItem
{
public:
    TimestampTableWidgetItem(const QString &txt = "") : QTableWidgetItem(txt) {}
    bool operator <(const QTableWidgetItem &other) const;
};


// a subclassed QTableWidgetItem that allows alphanumeric sorting
class SectionTableWidgetItem : public QTableWidgetItem
{
public:
    SectionTableWidgetItem(const QString &txt = "") : QTableWidgetItem(txt) {}
    bool operator <(const QTableWidgetItem &other) const;
};


// a subclassed QTreeWidget to show teams and students with summarized data on each
class TeamTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TeamTreeWidget(QWidget *parent = nullptr);
    void collapseItem(QTreeWidgetItem *item);           // when collapsing parent, summarize the children's data
    void collapseAll();
    void expandItem(QTreeWidgetItem *item);             // when expanding, simplify appearance by removing summary of children's data
    void expandAll();

protected:
    void dragEnterEvent(QDragEnterEvent *event);        // remember which item is being dragged
    void dragMoveEvent(QDragMoveEvent *event);          // update tooltip during drag
    void dropEvent(QDropEvent *event);                  // handle when the dragged item is being dropped to allow swapping of teammates or teams

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it

public slots:
    void resorting(int column);

signals:
    void swapChildren(int studentAteam, int studentAID, int studentBteam, int studentBID);  // if drag-and-dropping chid onto child, swap teammates
    void reorderParents(int teamA, int teamB);             // if drag-and-dropping parent onto parent, reorder teams
    void moveChild(int studentTeam, int studentID, int NewTeam);  // if drag-and-dropping child onto parent, move student
    void updateTeamOrder();

private:
    QTreeWidgetItem *draggedItem = nullptr;
    QTreeWidgetItem *droppedItem = nullptr;
    QLabel *dragDropEventLabel = nullptr;
};

class TeamTreeWidgetItem : public QTreeWidgetItem
{
public:
    bool operator<(const QTreeWidgetItem &other) const;
};

class TeamTreeHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    TeamTreeHeaderView(TeamTreeWidget *parent = nullptr);
};


// a subclassed QPushButton that passes mouse enter events to its parent
class PushButtonThatSignalsMouseEnterEvents : public QPushButton
{
    Q_OBJECT

public:
    PushButtonThatSignalsMouseEnterEvents(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

protected:
    void enterEvent(QEvent *event);

signals:
    void mouseEntered();
};


// a subclassed QSpinBox that replaces numerical values with categorical attribute responses in display
class CategoricalSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    CategoricalSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {}
    void setCategoricalValues(const QStringList &categoricalValues);
    QString textFromValue(int value) const;
    int valueFromText(const QString &text) const;
    QValidator::State validate (QString &input, int &pos) const;

private:
    QStringList categoricalValues;
};


// a subclassed combobox that paints with elided contents
class ComboBoxWithElidedContents : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxWithElidedContents(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // CUSTOMWIDGETS
