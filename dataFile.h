#ifndef DATAFILE_H
#define DATAFILE_H

#include "dialogs/listTableDialog.h"
#include <optional>
#include <QDialog>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>

/**
     * @brief possFieldMeaning represents what kind of data might be stored in a data file
     */
struct possFieldMeaning
{
    /**
     * @brief nameShownToUser Is the name of the field shown to the user.
     */
    QString nameShownToUser;

    /**
     * @brief regExSearchString The RegEx search string.
     */
    QString regExSearchString;

    /**
     * @brief maxNumOfFields The number of fields that are allowed to have this meaning.
     */
    int maxNumOfFields;
};

/**
 * @brief Abstract base for a read-only tabular data source (see DelimitedTextFile, ExcelFile).
 * Carries the read-facing interface and data shared by every format gruepr can import data from
 * -- survey responses, class rosters, or previously-exported team files. open()/close()/fileInfo()/
 * isOpen()/readHeader()/readDataRow() are implemented once here in terms of a small set of
 * protected primitives each subclass provides, so the two subclasses stay as parallel as possible.
 */
class DataFile
{
public:
    enum class ReadLocation {currentPosition, beginningOfFile};

    DataFile() = default;
    virtual ~DataFile();
    DataFile(const DataFile&) = delete;
    DataFile operator= (const DataFile&) = delete;
    DataFile(DataFile&&) = delete;
    DataFile& operator= (DataFile&&) = delete;

    bool open(QWidget *parent = nullptr, const QString &caption = QObject::tr("Open Data File"),
              const QString &filepath = "", const QString &filetypeDescriptor = "");
    virtual bool openExistingFile(const QString &filepath) = 0;
    QFileInfo fileInfo();
    bool isOpen();
    virtual bool atEnd() = 0;
    void close(bool deleteFile = false);
    bool readHeader();
    bool readDataRow(ReadLocation readLocation = ReadLocation::currentPosition);

    QDialog* chooseFieldMeaningsDialog(const QList<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);

    QStringList headerValues;
    bool hasHeaderRow = true;
    int numFields = 0;
    long long estimatedNumberRows = 0;    // exact for some formats, estimated (e.g. from newline count) for others
    QStringList fieldMeanings;
    QStringList fieldValues;
    QStringList fieldsToBeIgnored;

protected:
    // Format-specific hooks that open()/close()/fileInfo()/isOpen()/readHeader()/readDataRow() are built from.
    virtual QString fileDialogFilter(const QString &filetypeDescriptor) const = 0;
    virtual void releaseResource() = 0;
    virtual void resetToStart() = 0;
    // Returns the next row's raw values (possibly all-empty, meaning "blank row, keep reading"), or
    // std::nullopt once there are no more rows.
    virtual std::optional<QStringList> readNextRawRow() = 0;

    listTableDialog *window = nullptr;
    QString openFilePath;

private:
    void validateFieldSelectorBoxes(int callingRow = -1);
    inline static const QString HEADERTEXT = QObject::tr("Column Headers");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");
    inline static const int DIALOGWIDTH = 500;
    inline static const int DIALOGHEIGHT = 300;
};

#endif // DATAFILE_H
