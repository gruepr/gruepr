#include "studentTableWidget.h"
#include "gruepr_globals.h"
#include <QHeaderView>


StudentTableWidget::StudentTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    connect(this, &QTableWidget::entered, this, &StudentTableWidget::itemEntered);
    connect(this, &QTableWidget::viewportEntered, this, [this] {leaveEvent(nullptr);});
    connect(this->horizontalHeader(), &QHeaderView::sectionClicked, this, &StudentTableWidget::sortByColumn);
}


void StudentTableWidget::resetTable()
{
    QTableWidget::sortByColumn(0, Qt::AscendingOrder);
    horizontalHeader()->setSortIndicatorShown(true);
    horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
    horizontalHeaderItem(0)->setIcon(QIcon(":/icons/blank_arrow.png"));
    prevSortColumn = 0;
    prevSortOrder = Qt::AscendingOrder;
}


void StudentTableWidget::clearSortIndicator()
{
    horizontalHeaderItem(horizontalHeader()->sortIndicatorSection())->setIcon(QIcon(":/icons/blank_arrow.png"));
}


void StudentTableWidget::sortByColumn(int column)
{
    // disallow sorting on the last two columns (edit button and remove button)
    if(column < columnCount()-2)
    {
        QTableWidget::sortByColumn(column, horizontalHeader()->sortIndicatorOrder());
        horizontalHeaderItem(column)->setIcon(QIcon(":/icons/blank_arrow.png"));
        if(column != prevSortColumn)
        {horizontalHeaderItem(prevSortColumn)->setIcon(QIcon(":/icons/updown_arrow.png"));}
        prevSortColumn = column;
        prevSortOrder = horizontalHeader()->sortIndicatorOrder();
    }
    else
    {
        QTableWidget::sortByColumn(prevSortColumn, prevSortOrder);
        horizontalHeader()->setSortIndicator(prevSortColumn, prevSortOrder);
    }
}


void StudentTableWidget::leaveEvent(QEvent *event)
{
    selectionModel()->clearSelection();
    for(int row = 0; row < rowCount(); row++)
    {
        cellLeft(row);
    }
    if(event != nullptr)
    {
        QWidget::leaveEvent(event);
    }
}


void StudentTableWidget::cellLeft(const int row)
{
    selectionModel()->clearSelection();
    const int numCols = columnCount();
    if(cellWidget(row, numCols-1)->property("duplicate").toBool())
    {
        cellWidget(row, numCols-1)->setStyleSheet(QString("QPushButton {background-color: #") + HIGHLIGHTYELLOWHEX + "; border: none;}");
        cellWidget(row, numCols-2)->setStyleSheet(QString("QPushButton {background-color: #") + HIGHLIGHTYELLOWHEX + "; border: none;}");
    }
    else
    {
        cellWidget(row, numCols-1)->setStyleSheet("");
        cellWidget(row, numCols-2)->setStyleSheet("");
    }
}


void StudentTableWidget::itemEntered(const QModelIndex &index)
{
    setSelection(this->visualRect(index), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    int row = index.row();
    cellEntered(row);
}


void StudentTableWidget::cellEntered(const int row)
{
    // select the current row, reset the background color of the edit and remover buttons in previously selected row and change their color in the current row
    selectRow(row);
    static int prevID = -1;
    const int numCols = columnCount();
    const int numRows = rowCount();
    if(prevID != -1)
    {
        int prevRow = 0;
        while((prevRow < numRows) && (prevID != cellWidget(prevRow, numCols-1)->property("StudentIndex").toInt()))
        {
            prevRow++;
        }
        if(prevRow < numRows)
        {
            if(cellWidget(prevRow, numCols-1)->property("duplicate").toBool())
            {
                cellWidget(prevRow, numCols-1)->setStyleSheet(QString("QPushButton {background-color: #") + HIGHLIGHTYELLOWHEX + "; border: none;}");
                cellWidget(prevRow, numCols-2)->setStyleSheet(QString("QPushButton {background-color: #") + HIGHLIGHTYELLOWHEX + "; border: none;}");
            }
            else
            {
                cellWidget(prevRow, numCols-1)->setStyleSheet("");
                cellWidget(prevRow, numCols-2)->setStyleSheet("");
            }
        }
    }
    prevID = cellWidget(row, numCols-1)->property("StudentIndex").toInt();
    cellWidget(row, numCols-1)->setStyleSheet("QPushButton {background-color: #ceeafb; border: none;}");
    cellWidget(row, numCols-2)->setStyleSheet("QPushButton {background-color: #ceeafb; border: none;}");
}
