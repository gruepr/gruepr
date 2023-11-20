#ifndef SORTABLETABLEWIDGETITEM
#define SORTABLETABLEWIDGETITEM

// a subclassed QTableWidgetItem that allows sorting according to datetime or alphanumerical data

#include <QCollator>
#include <QTableWidgetItem>


class SortableTableWidgetItem : public QTableWidgetItem
{
public:
    enum class SortType{datetime, alphanumeric};
    explicit SortableTableWidgetItem(const SortType sortType, const QString &txt = "");
    ~SortableTableWidgetItem() = default;
    SortableTableWidgetItem(const SortableTableWidgetItem&) = delete;
    SortableTableWidgetItem operator= (const SortableTableWidgetItem&) = delete;
    SortableTableWidgetItem(SortableTableWidgetItem&&) = delete;
    SortableTableWidgetItem& operator= (SortableTableWidgetItem&&) = delete;

    bool operator <(const QTableWidgetItem &other) const override;

private:
    SortType sortType;
    QCollator sortAlphanumerically;
};

#endif // SORTABLETABLEWIDGETITEM
