#include "customWidgets.h"
#include "gruepr_structs_and_consts.h"
#include <QCollator>
#include <QDateTime>
#include <QDropEvent>
#include <QPainter>
#include <QStylePainter>
#include <QGuiApplication>

//////////////////
// Table Widget Item for timestamps, allowing to sort chronologically
//////////////////
bool TimestampTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    return QDateTime::fromString(text(), Qt::SystemLocaleShortDate) < QDateTime::fromString(other.text(), Qt::SystemLocaleShortDate);
}


//////////////////
// Table Widget Item for section names, allowing to sort alphanumerically
//////////////////
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

void TeamTreeWidget::collapseAll()
{
    QTreeWidget::collapseAll();

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

    dragDropEventLabel = new QLabel;
    dragDropEventLabel->setWindowFlag(Qt::ToolTip);
    dragDropEventLabel->setTextFormat(Qt::RichText);
    dragDropEventLabel->setStyleSheet("QLabel {background-color: #bdfff2; color: black; border: 2px solid black;padding: 2px 2px 2px 2px;}");
}

void TeamTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    // get the TeamTreeWidgetItem currently under the cursor
    QTreeWidgetItem* itemUnderCursor = itemAt(event->pos());
    if(itemUnderCursor == nullptr)
    {
        dragDropEventLabel->hide();
        return;
    }
    TeamTreeWidgetItem* dropItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);

    // adjust the location and text of the tooltip
    dragDropEventLabel->move(QCursor::pos() + QPoint(32, 32));
    if((draggedItem == dropItem) || (!draggedItem->parent() && dropItem->parent()) || (draggedItem->parent() == dropItem))
    {
        // dragging item onto itself, team->student, or student->own team
        dragDropEventLabel->hide();
    }
    else if(draggedItem->parent() && dropItem->parent())
    {
        // dragging student->student
        dragDropEventLabel->setText(tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + dropItem->text(0) + "</b>");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
    else if(draggedItem->parent() && !dropItem->parent())
    {
        // dragging student->team
        dragDropEventLabel->setText(tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + dropItem->text(0) + "</b>");
        dragDropEventLabel->show();
        dragDropEventLabel->adjustSize();
    }
    else if(!draggedItem->parent() && !dropItem->parent())
    {
        // dragging team->team
        dragDropEventLabel->setText(tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("to this position"));
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
    // Iff dragged and dropped items are both students or both teams, then swap their places.
    if(draggedItem->parent() && droppedItem->parent())  // two students
    {
        // UserRole data stored in the item is the studentRecord.ID; TeamNumber data stored in the parent's column 0 is the team number
        emit swapChildren((draggedItem->parent()->data(0,TeamNumber)).toInt(), (draggedItem->data(0,Qt::UserRole)).toInt(),
                          (droppedItem->parent()->data(0,TeamNumber)).toInt(), (droppedItem->data(0,Qt::UserRole)).toInt());
    }
    else if(!(draggedItem->parent()) && !(droppedItem->parent()))   // two teams
    {
        emit reorderParents((draggedItem->data(0,TeamNumber)).toInt(), (droppedItem->data(0,TeamNumber)).toInt());
    }
    else if(draggedItem->parent() && !(droppedItem->parent()))  // dragging student onto team
    {
        emit moveChild((draggedItem->parent()->data(0,TeamNumber)).toInt(), (draggedItem->data(0,Qt::UserRole)).toInt(), (droppedItem->data(0,TeamNumber)).toInt());
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

bool TeamTreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    if(parent())      // don't sort the students, only the teams
    {
        return false;
    }

    int sortColumn = treeWidget()->sortColumn();

    if(sortColumn == 0)
    {
        return (data(sortColumn, TeamInfoSort).toString() < other.data(sortColumn, TeamInfoSort).toString());
    }

    // sort using sortorder data in column, and use existing order to break ties
    return((1000*data(sortColumn, TeamInfoSort).toInt() + data(columnCount()-1, TeamInfoSort).toInt()) <
           (1000*other.data(sortColumn, TeamInfoSort).toInt() + other.data(columnCount()-1, TeamInfoSort).toInt()));
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


//////////////////
// QSpinBox that replaces numerical values with categorical attribute responses in display
//////////////////
void CategoricalSpinBox::setCategoricalValues(const QStringList &categoricalValues)
{
    this->categoricalValues = categoricalValues;
}

QString CategoricalSpinBox::textFromValue(int value) const
{
    return ((value > 0) ? (value <= 26 ? QString(char(value + 'A' - 1)) :
                                         QString(char((value - 1)%26 + 'A')).repeated(1 + ((value-1)/26))) + " - " + categoricalValues.at(value - 1) : "0");
}

int CategoricalSpinBox::valueFromText(const QString &text) const
{
    return (categoricalValues.indexOf(text.split(" - ").last()) + 1);
}

QValidator::State CategoricalSpinBox::validate (QString &input, int &pos) const
{
    (void)input;
    (void)pos;
    return QValidator::Acceptable;
}


//////////////////
// QComboBox that paints with elided contents
//////////////////
ComboBoxWithElidedContents::ComboBoxWithElidedContents(QWidget *parent)
    :QComboBox(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

QSize ComboBoxWithElidedContents::sizeHint() const
{
    return minimumSizeHint();
}

QSize ComboBoxWithElidedContents::minimumSizeHint() const
{
    return {(QFontMetrics(this->font())).boundingRect("Very high / Above average / Average / Below average / Very low").width()+15, QComboBox::minimumSizeHint().height()};
}

void ComboBoxWithElidedContents::paintEvent(QPaintEvent *event)
{
    (void)event;
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_ComboBox, opt);

    QRect textRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    opt.currentText = p.fontMetrics().elidedText(opt.currentText, Qt::ElideMiddle, textRect.width());
    p.drawControl(QStyle::CE_ComboBoxLabel, opt);
}
