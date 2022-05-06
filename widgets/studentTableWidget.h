#ifndef STUDENTTABLEWIDGET_H
#define STUDENTTABLEWIDGET_H

#include <QTableWidget>

class StudentTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    StudentTableWidget(QWidget *parent = nullptr);
    void resetTable();
    void clearSortIndicator();
    void cellEntered(const int row);
    void cellLeft(const int row);

public slots:
    void sortByColumn(int column);

protected:
    void leaveEvent(QEvent *event) override;

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it

private:
    int prevSortColumn = 0;
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;
};

#endif // STUDENTTABLEWIDGET_H
