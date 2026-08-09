#include "studentTableWidget.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QPersistentModelIndex>
#include <QVariant>


StudentTableWidget::StudentTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStyleSheet(STUDENTTABLEWIDGETHORIZONTALHEADERSTYLE);
    verticalHeader()->setStyleSheet(STUDENTTABLEWIDGETVERTICALALHEADERSTYLE);
    setStyleSheet(QString(STUDENTTABLEWIDGETSTYLE) + SCROLLBARSTYLE);

    connect(this, &QTableWidget::entered, this, &StudentTableWidget::itemEntered);
    connect(this, &QTableWidget::viewportEntered, this, [this] {leaveEvent(nullptr);});
    connect(this->horizontalHeader(), &QHeaderView::sectionClicked, this, &StudentTableWidget::sortByColumn);
}


void StudentTableWidget::resetTable(int sortColumn)
{
    QTableWidget::sortByColumn(sortColumn, Qt::AscendingOrder);
    horizontalHeader()->setSortIndicatorShown(true);
    horizontalHeader()->setSortIndicator(sortColumn, Qt::AscendingOrder);
    horizontalHeaderItem(sortColumn)->setIcon(QIcon(":/icons_new/blank_arrow.png"));
    prevSortColumn = sortColumn;
    prevSortOrder = Qt::AscendingOrder;
}


void StudentTableWidget::clearSortIndicator()
{
    horizontalHeaderItem(horizontalHeader()->sortIndicatorSection())->setIcon(QIcon(":/icons_new/blank_arrow.png"));
}


void StudentTableWidget::sortByColumn(int column)
{
    // disallow sorting on the last two columns (edit button and remove button)
    if(column < columnCount()-2) {
        QTableWidget::sortByColumn(column, horizontalHeader()->sortIndicatorOrder());
        horizontalHeaderItem(column)->setIcon(QIcon(":/icons_new/blank_arrow.png"));
        if(column != prevSortColumn) {
            horizontalHeaderItem(prevSortColumn)->setIcon(QIcon(":/icons_new/upDownButton_white.png"));
        }
        prevSortColumn = column;
        prevSortOrder = horizontalHeader()->sortIndicatorOrder();
    }
    else {
        QTableWidget::sortByColumn(prevSortColumn, prevSortOrder);
        horizontalHeader()->setSortIndicator(prevSortColumn, prevSortOrder);
    }
}


void StudentTableWidget::leaveEvent(QEvent *event)
{
    selectionModel()->clearSelection();
    if(event != nullptr) {
        QWidget::leaveEvent(event);
    }
}


void StudentTableWidget::trackHoverFor(QWidget *cellWidget, int row)
{
    // cell widgets (buttons, icon labels) sit on top of the viewport and swallow mouse-move events, so
    // QAbstractItemView::entered() never fires for them; watch for the widget's own Enter event instead.
    // Store a persistent index (not a plain row number) since sorting the table afterward moves rows
    // around, and a persistent index automatically tracks its row across that reordering.
    const QPersistentModelIndex anchor(model()->index(row, 0));
    cellWidget->setProperty("hoverAnchor", QVariant::fromValue(anchor));
    cellWidget->installEventFilter(this);
}


bool StudentTableWidget::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::Enter) {
        auto *widget = qobject_cast<QWidget*>(watched);
        const QVariant anchor = (widget != nullptr) ? widget->property("hoverAnchor") : QVariant();
        if(anchor.isValid()) {
            const auto index = anchor.value<QPersistentModelIndex>();
            if(index.isValid()) {
                hoverRow(index.row());
            }
        }
    }
    return QTableWidget::eventFilter(watched, event);
}


void StudentTableWidget::hoverRow(int row)
{
    const QModelIndex index = model()->index(row, 0);
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selectRow(row);
}


void StudentTableWidget::itemEntered(const QModelIndex &index)
{
    hoverRow(index.row());
}
