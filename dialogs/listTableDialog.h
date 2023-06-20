#ifndef LISTTABLEDIALOG_H
#define LISTTABLEDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>

class listTableDialog : public QDialog
{
    Q_OBJECT

public:
    listTableDialog(const QString &title = "", bool hideColHeaders = true, bool stretchLastColumn = true, QWidget *parent = nullptr);
    QGridLayout *theGrid = nullptr;
    QTableWidget *theTable = nullptr;
    void addSpacerRow(int row);
    void addButton(QPushButton *newButton);

protected:
    QHBoxLayout *buttonLayout = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    inline static const int TABLEROWINGRID = 5;
    inline static const int BUTTONBOXROWINGRID = 9;
    inline static const int HEIGHTOFSPACERROW = 20;
    inline static const float TABLECOLUMN0OVERWIDTH = 1.2f;   // factor to exapnd the first column of the table, as it often seems to be too small
    bool eventFilter(QObject *object, QEvent *event) override;   // an event filter to remove scrollwheel events from widget(s) in the table,
                                                                 // since table is in a scroll area
};


#endif // LISTTABLEDIALOG_H
