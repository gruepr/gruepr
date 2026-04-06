#include "studentTableWidget.h"
#include "gruepr_globals.h"
#include <QHeaderView>


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


void StudentTableWidget::resetTable()
{
    QTableWidget::sortByColumn(0, Qt::AscendingOrder);
    horizontalHeader()->setSortIndicatorShown(true);
    horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
    horizontalHeaderItem(0)->setIcon(QIcon(":/icons_new/blank_arrow.png"));
    prevSortColumn = 0;
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


void StudentTableWidget::itemEntered(const QModelIndex &index)
{
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selectRow(index.row());
}
