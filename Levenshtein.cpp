#include "Levenshtein.h"
#include <vector>

int levenshtein::distance(const QString &source, const QString &target, const Qt::CaseSensitivity cs)
{
    // (mostly from https://qgis.org/api/qgsstringutils_8cpp_source.html)

    int sourceLength = int(source.length());
    int targetLength = int(target.length());

    if (source.isEmpty()) {
        return targetLength;
    }

    if (target.isEmpty()) {
        return sourceLength;
    }

    //handle case sensitive flag (or not)
    QString s1(cs == Qt::CaseInsensitive ? source : source.toLower());
    QString s2(cs == Qt::CaseInsensitive ? target : target.toLower());

    const QChar *s1Char = s1.constData();
    const QChar *s2Char = s2.constData();

    //strip out any common prefix
    int commonPrefixLen = 0;
    while (sourceLength > 0 && targetLength > 0 && *s1Char == *s2Char)
    {
      commonPrefixLen++;
      sourceLength--;
      targetLength--;
      s1Char++;
      s2Char++;
    }

    //strip out any common suffix
    while ((sourceLength > 0) && (targetLength > 0) && (s1.at(commonPrefixLen + sourceLength - 1) == s2.at(commonPrefixLen + targetLength - 1)))
    {
      sourceLength--;
      targetLength--;
    }

    //fully checked either string? if so, the answer is easy...
    if (sourceLength == 0)
    {
      return targetLength;
    }
    if (targetLength == 0)
    {
      return sourceLength;
    }

    //ensure the inner loop is longer
    if (sourceLength > targetLength)
    {
      std::swap(s1, s2);
      std::swap(sourceLength, targetLength);
    }

    //levenshtein algorithm begins here
    std::vector<int> col(targetLength+1, 0);
    std::vector<int> prevCol;
    prevCol.reserve(targetLength + 1);
    for (int i = 0; i < targetLength + 1; ++i)
    {
      prevCol.push_back(i);
    }
    const QChar *s2start = s2Char;
    for (int i = 0; i < sourceLength; ++i)
    {
      col[0] = i + 1;
      s2Char = s2start;
      for (int j = 0; j < targetLength; ++j)
      {
        col[j + 1] = std::min(std::min(1 + col[j], 1 + prevCol[1 + j]), prevCol[j] + ((*s1Char == *s2Char) ? 0 : 1));
        s2Char++;
      }
      col.swap(prevCol);
      s1Char++;
    }
    return prevCol[targetLength];
}
