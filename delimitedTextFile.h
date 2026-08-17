#ifndef DELIMITEDTEXTFILE_H
#define DELIMITEDTEXTFILE_H

#include "dataFile.h"
#include <memory>
#include <optional>
#include <QFile>
#include <QString>
#include <QTextStream>

/**
 * @brief The DelimitedTextFile class corresponding to a delimited text file (.csv, comma-delimited,
 * or .txt, tab-delimited), read by gruepr.
 */
class DelimitedTextFile : public DataFile
{
public:
    enum class Delimiter {comma, tab};

    DelimitedTextFile(Delimiter dlmtr = Delimiter::comma);
    ~DelimitedTextFile() override;
    DelimitedTextFile(const DelimitedTextFile&) = delete;
    DelimitedTextFile operator= (const DelimitedTextFile&) = delete;
    DelimitedTextFile(DelimitedTextFile&&) = delete;
    DelimitedTextFile& operator= (DelimitedTextFile&&) = delete;

    bool openExistingFile(const QString &filepath) override;
    bool atEnd() override;

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1, const char delimiter = ',');

protected:
    QString fileDialogFilter(const QString &filetypeDescriptor) const override;
    void releaseResource() override;
    void resetToStart() override;
    std::optional<QStringList> readNextRawRow() override;

private:
    std::unique_ptr<QFile> file;
    std::unique_ptr<QTextStream> stream;
    char delimiter = ',';
    QStringList getLine(const int minFields = -1);
};

#endif // DELIMITEDTEXTFILE_H
