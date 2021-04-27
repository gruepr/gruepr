#ifndef LISTTABLEDIALOG_H
#define LISTTABLEDIALOG_H

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
};


#endif // LISTTABLEDIALOG_H
