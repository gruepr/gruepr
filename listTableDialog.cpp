#include "listTableDialog.h"
#include <QEvent>
#include <QHeaderView>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A base dialog that has an explanation and a table to fill in with selectors of some kind
/////////////////////////////////////////////////////////////////////////////////////////////////////////

listTableDialog::listTableDialog(const QString &title, bool hideColHeaders, bool stretchLastColumn, QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setWindowTitle(title);

    theGrid = new QGridLayout(this);

    // Content allowed in rows 1&2

    // Table with active content put in row 3 of grid
    theTable = new QTableWidget(this);
    theTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    theTable->setSelectionMode(QAbstractItemView::NoSelection);
    theTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    theTable->setShowGrid(false);
    theTable->setAlternatingRowColors(true);
    theTable->setStyleSheet("QTableView::item{border-top: 1px solid black; border-bottom: 1px solid black; padding: 3px;}");
    theTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    theTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    theTable->verticalHeader()->hide();
    theTable->horizontalHeader()->setHidden(hideColHeaders);
    theTable->horizontalHeader()->setStretchLastSection(stretchLastColumn);
    theTable->setColumnCount(2);
    theGrid->addWidget(theTable, tableRowInGrid, 0, 1, -1);

    // Content allowed in rows 4&5

    // A spacer for row 6, then button box with OK & Cancel buttons in row 7, column 2 (column 2 to allow add'l. buttons in column 1)
    addSpacerRow(buttonBoxRowInGrid - 1);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, buttonBoxRowInGrid, 1, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void listTableDialog::addSpacerRow(int row)
{
    theGrid->setRowMinimumHeight(row, heightOfSpacerRow);
}

bool listTableDialog::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::Wheel)
    {
        event->ignore();
        return true;
    }

    event->accept();
    return QDialog::eventFilter(object, event);
}
