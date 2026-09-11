#ifndef SORTABLETABLEWIDGETITEM
#define SORTABLETABLEWIDGETITEM

// a subclassed QTableWidgetItem that allows sorting according to datetime or alphanumerical data

#include <QCollator>
#include <QDateTime>
#include <QTableWidgetItem>


class SortableTableWidgetItem : public QTableWidgetItem
{
public:
    enum class SortType{datetime, alphanumeric};
    explicit SortableTableWidgetItem(const SortType sortType, const QString &txt = "");
    ~SortableTableWidgetItem() override = default;
    SortableTableWidgetItem(const SortableTableWidgetItem&) = delete;
    SortableTableWidgetItem operator= (const SortableTableWidgetItem&) = delete;
    SortableTableWidgetItem(SortableTableWidgetItem&&) = delete;
    SortableTableWidgetItem& operator= (SortableTableWidgetItem&&) = delete;

    void setSortKey(const QString &key);
    // for datetime items: keep the original QDateTime so sorting never has to re-parse the displayed
    // (locale-formatted) text, which is far too slow to do inside a sort's comparison
    void setSortKey(const QDateTime &key);
    bool operator <(const QTableWidgetItem &other) const override;

private:
    SortType sortType;
    QDateTime datetimeSortKey;
    QCollator sortAlphanumerically;
};

#endif // SORTABLETABLEWIDGETITEM
