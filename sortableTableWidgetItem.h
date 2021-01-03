#ifndef SORTABLETABLEWIDGETITEM
#define SORTABLETABLEWIDGETITEM

// a subclassed QTableWidgetItem that allows sorting according to datetime or alphanumerical data

#include <QTableWidgetItem>
#include <QCollator>


class SortableTableWidgetItem : public QTableWidgetItem
{
public:
    enum SortType{datetime, alphanumeric};
    SortableTableWidgetItem(const SortType sortType, const QString &txt = "");
    bool operator <(const QTableWidgetItem &other) const;

private:
    SortType sortType;
    QCollator sortAlphanumerically;
};

#endif // SORTABLETABLEWIDGETITEM
