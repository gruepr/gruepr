#ifndef LISTTABLEDIALOG_H
#define LISTTABLEDIALOG_H

#define TABLECOLUMN0OVERWIDTH 1.2   // factor to exapnd the first column of the table, as it often seems to be too small

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QTableWidget>

class listTableDialog : public QDialog
{
    Q_OBJECT

public:
    listTableDialog(const QString &title = "", bool hideColHeaders = true, bool stretchLastColumn = true, QWidget *parent = nullptr);
    QGridLayout *theGrid;
    QTableWidget *theTable;
    void addSpacerRow(int row);

protected:
    QDialogButtonBox *buttonBox;
    const int tableRowInGrid = 2;
    const int buttonBoxRowInGrid = 6;
    const int heightOfSpacerRow = 20;
    bool eventFilter(QObject *object, QEvent *event);   // an event filter to remove scrollwheel events from widget(s) in the table, since table is in a scroll area
};


#endif // LISTTABLEDIALOG_H
