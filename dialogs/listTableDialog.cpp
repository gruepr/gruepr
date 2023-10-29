#include "listTableDialog.h"
#include "gruepr_globals.h"
#include <QEvent>
#include <QHeaderView>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A base dialog that has an explanation and a table to fill in with selectors of some kind
/////////////////////////////////////////////////////////////////////////////////////////////////////////

listTableDialog::listTableDialog(const QString &title, bool hideColHeaders, bool stretchLastColumn, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setSizeGripEnabled(true);
    setWindowTitle(title);
    setStyleSheet("QDialog{background-color: white;}");

    theGrid = new QGridLayout(this);
    theGrid->setSpacing(2);
    theGrid->setColumnMinimumWidth(0, DIALOG_SPACER_ROWHEIGHT);
    theGrid->setColumnMinimumWidth(2, DIALOG_SPACER_ROWHEIGHT);

    //Spacer in row 1
    addSpacerRow(0);

    // Content allowed in rows 2&3

    //Spacer in row 4
    addSpacerRow(TABLEROWINGRID - 1);

    // Table with active content put in row 5 of grid
    theTable = new QTableWidget(this);
    theTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    theTable->setSelectionMode(QAbstractItemView::NoSelection);
    theTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    theTable->setShowGrid(false);
    theTable->setStyleSheet(QString("QTableView{background-color: white; border: none;}"
                                    "QTableView::item{border: none; padding: 3px;}") + SCROLLBARSTYLE);
    theTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    theTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    theTable->verticalHeader()->hide();
    theTable->horizontalHeader()->setHidden(hideColHeaders);
    theTable->horizontalHeader()->setStretchLastSection(stretchLastColumn);
    theTable->setColumnCount(2);
    theGrid->addWidget(theTable, TABLEROWINGRID, 1, 1, 1);

    //Spacer in row 6
    addSpacerRow(TABLEROWINGRID + 1);

    // Content allowed in rows 7&8

    // A spacer for row 9, then button layout with OK & Cancel buttons in row 10
    addSpacerRow(BUTTONBOXROWINGRID - 1);
    buttonLayout = new QHBoxLayout;
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    buttonLayout->addWidget(buttonBox, 0, Qt::AlignRight);
    theGrid->addLayout(buttonLayout, BUTTONBOXROWINGRID, 0, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void listTableDialog::addSpacerRow(int row)
{
    theGrid->setRowMinimumHeight(row, DIALOG_SPACER_ROWHEIGHT);
}

void listTableDialog::addButton(QPushButton *newButton)
{
    buttonLayout->insertWidget(0, newButton, 0, Qt::AlignLeft);
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
