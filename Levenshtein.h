#ifndef Levenshtein_H
#define Levenshtein_H

#include <QString>

namespace levenshtein {
    int distance(const QString &source, const QString &target, const Qt::CaseSensitivity cs = Qt::CaseSensitive);
}

#endif
