#ifndef Levenshtein_H
#define Levenshtein_H

#include <QString>

namespace levenshtein {
    int distance(const QString &source, const QString &target, Qt::CaseSensitivity cs = Qt::CaseSensitive);
}

#endif
