#include "excelFile.h"
#include "xlsxdocument.h"
#include <algorithm>

ExcelFile::ExcelFile() = default;

ExcelFile::~ExcelFile()
{
    close();
}


bool ExcelFile::openExistingFile(const QString &filepath)
{
    close();
    if(filepath.isEmpty()) {
        return false;
    }

    document = std::make_unique<QXlsx::Document>(filepath);
    if(!document->load() || !document->selectSheet(0)) {
        document.reset();
        return false;
    }

    const QXlsx::CellRange range = document->dimension();
    estimatedNumberRows = range.isValid() ? range.rowCount() : 0;
    openFilePath = filepath;
    currentRow = 0;
    return true;
}


QString ExcelFile::fileDialogFilter(const QString &filetypeDescriptor) const
{
    return filetypeDescriptor + " File (*.xlsx)";
}


void ExcelFile::releaseResource()
{
    document.reset();
    currentRow = 0;
}


bool ExcelFile::atEnd()
{
    if(document == nullptr) {
        return true;
    }
    const QXlsx::CellRange range = document->dimension();
    return !range.isValid() || currentRow >= range.lastRow();
}


void ExcelFile::resetToStart()
{
    if(document == nullptr) {
        return;
    }
    const QXlsx::CellRange range = document->dimension();
    currentRow = range.isValid() ? (range.firstRow() - 1) : 0;
}


//////////////////
// Read the next row from the sheet. A row where every cell (within the sheet's used column range)
// is empty is reported as an empty (but present) QStringList, meaning "blank row, keep reading" --
// same convention DelimitedTextFile uses for a blank line. std::nullopt means there are no more rows.
//////////////////
std::optional<QStringList> ExcelFile::readNextRawRow()
{
    if(document == nullptr) {
        return std::nullopt;
    }
    const QXlsx::CellRange range = document->dimension();
    if(!range.isValid()) {
        return std::nullopt;
    }

    currentRow++;
    if(currentRow > range.lastRow()) {
        return std::nullopt;
    }

    QStringList row;
    for(int col = range.firstColumn(); col <= range.lastColumn(); col++) {
        row << document->read(currentRow, col).toString();
    }

    if(std::all_of(row.cbegin(), row.cend(), [](const QString &s){ return s.isEmpty(); })) {
        return QStringList();
    }
    return row;
}
