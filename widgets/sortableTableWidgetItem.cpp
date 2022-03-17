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
        return QLocale::system().toDateTime(text(), QLocale::ShortFormat) < QLocale::system().toDateTime(other.text(), QLocale::ShortFormat);
    }

    return (sortAlphanumerically.compare(text(), other.text()) < 0);
}
