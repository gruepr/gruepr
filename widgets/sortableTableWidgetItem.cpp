#include "sortableTableWidgetItem.h"
#include <QDateTime>


//////////////////
// Table Widget Item that allows timestamps to be sorted chronologically and other things alphanumerically
//////////////////
SortableTableWidgetItem::SortableTableWidgetItem(const SortType sortType, const QString &txt)
    :QTableWidgetItem(txt)
{
    this->sortType = sortType;
    sortAlphanumerically.setNumericMode(true);
    sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
}

void SortableTableWidgetItem::setSortKey(const QString &key)
{
    setData(Qt::UserRole, key);
}


bool SortableTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    if(sortType == SortType::datetime) {
        return QLocale::system().toDateTime(text(), QLocale::ShortFormat) < QLocale::system().toDateTime(other.text(), QLocale::ShortFormat);
    }

    const QString myKey = data(Qt::UserRole).toString();
    const QString otherKey = other.data(Qt::UserRole).toString();
    if(!myKey.isEmpty() || !otherKey.isEmpty()) {
        return (sortAlphanumerically.compare(myKey, otherKey) < 0);
    }

    return (sortAlphanumerically.compare(text(), other.text()) < 0);
}
