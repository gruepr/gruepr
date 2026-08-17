#ifndef EXCELFILE_H
#define EXCELFILE_H

#include "dataFile.h"
#include <memory>
#include <optional>

namespace QXlsx { class Document; }

/**
 * @brief The ExcelFile class corresponding to an Excel (.xlsx) file. Read-only: gruepr only ever
 * needs to import data from an xlsx file this way, never write one (team export writes xlsx
 * directly via QXlsx, see TeamsTabItem::writeTabularFile). Always reads the first/active sheet --
 * no multi-sheet selection UI.
 */
class ExcelFile : public DataFile
{
public:
    ExcelFile();
    ~ExcelFile() override;
    ExcelFile(const ExcelFile&) = delete;
    ExcelFile operator= (const ExcelFile&) = delete;
    ExcelFile(ExcelFile&&) = delete;
    ExcelFile& operator= (ExcelFile&&) = delete;

    bool openExistingFile(const QString &filepath) override;
    bool atEnd() override;

protected:
    QString fileDialogFilter(const QString &filetypeDescriptor) const override;
    void releaseResource() override;
    void resetToStart() override;
    std::optional<QStringList> readNextRawRow() override;

private:
    std::unique_ptr<QXlsx::Document> document;
    int currentRow = 0;   // 1-indexed cursor for readNextRawRow()
};

#endif // EXCELFILE_H
