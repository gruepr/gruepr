#include "studentTableWidget.h"
#include "gruepr_globals.h"
#include <QHeaderView>


StudentTableWidget::StudentTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: 1px solid lightGray; border-bottom: none;"
                                                   "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}"
                                       "QHeaderView::section{border-top: none; border-left: none; border-right: 1px solid lightGray; border-bottom: none;"
                                                             "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}"
                                       "QHeaderView::down-arrow{image: url(:/icons_new/downButton_white.png); width: 15px; subcontrol-origin: padding; subcontrol-position: bottom left;}"
                                       "QHeaderView::up-arrow{image: url(:/icons_new/upButton_white.png); width: 15px; subcontrol-origin: padding; subcontrol-position: top left;}");
    verticalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: none; border-bottom: none; padding: 1px;"
                                                 "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}"
                                     "QHeaderView::section{border-top: none; border-left: none; border-right: none; border-bottom: none; padding: 1px;"
                                                           "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
    setStyleSheet(QString() + "QTableView{gridline-color: lightGray; font-family: 'DM Sans'; font-size: 12pt;}"
                               "QTableCornerButton::section{border-top: none; border-left: none; border-right: 1px solid gray; border-bottom: none;"
                                                            "background-color: " DEEPWATERHEX ";}"
                               "QTableWidget::item{border-right: 1px solid lightGray; color: black;}"
                               "QTableWidget::item:selected{background-color: " BUBBLYHEX ";}"
                               "QTableWidget::item:hover{background-color: " BUBBLYHEX ";}" +
                  SCROLLBARSTYLE);

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
    if(column < columnCount()-2)
    {
        QTableWidget::sortByColumn(column, horizontalHeader()->sortIndicatorOrder());
        horizontalHeaderItem(column)->setIcon(QIcon(":/icons_new/blank_arrow.png"));
        if(column != prevSortColumn)
        {horizontalHeaderItem(prevSortColumn)->setIcon(QIcon(":/icons_new/upDownButton_white.png"));}
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
        cellWidget(row, numCols-1)->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
        cellWidget(row, numCols-2)->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
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
                cellWidget(prevRow, numCols-1)->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
                cellWidget(prevRow, numCols-2)->setStyleSheet("QPushButton {background-color: " STARFISHHEX "; border: none;}");
            }
            else
            {
                cellWidget(prevRow, numCols-1)->setStyleSheet("");
                cellWidget(prevRow, numCols-2)->setStyleSheet("");
            }
        }
    }
    prevID = cellWidget(row, numCols-1)->property("StudentIndex").toInt();
    cellWidget(row, numCols-1)->setStyleSheet("QPushButton {background-color: " BUBBLYHEX "; border: none;}");
    cellWidget(row, numCols-2)->setStyleSheet("QPushButton {background-color: " BUBBLYHEX "; border: none;}");
}
