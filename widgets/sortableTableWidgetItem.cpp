#include "sortableTableWidgetItem.h"
#include <QDateTime>


//////////////////
// Table Widget Item for timestamps, allowing to sort chronologically
//////////////////
SortableTableWidgetItem::SortableTableWidgetItem(const SortType sortType, const QString &txt)
    :QTableWidgetItem(txt)
{
    this->sortType = sortType;
    sortAlphanumerically.setNumericMode(true);
    sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
}

bool SortableTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    if(sortType == datetime)
    {
        return QDateTime::fromString(text(), Qt::SystemLocaleShortDate) < QDateTime::fromString(other.text(), Qt::SystemLocaleShortDate);
    }

    return (sortAlphanumerically.compare(text(), other.text()) < 0);
}
