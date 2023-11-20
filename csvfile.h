#ifndef CSVFILE_H
#define CSVFILE_H

#include "dialogs/listTableDialog.h"
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>
#include <QTextStream>

struct possFieldMeaning      // 1) name of field shown to user, 2) RegEx search string, 3) number of fields that are allowed to have this meaning
{
    QString nameShownToUser;
    QString regExSearchString;
    int maxNumOfFields;
};

class CsvFile : public QObject
{
    Q_OBJECT

public:
    enum class Delimiter {comma, tab};
    enum class Source {fileOnly, fileOrOnline};

    CsvFile(Delimiter dlmtr = Delimiter::comma, QObject *parent = nullptr);
    ~CsvFile() override;
    CsvFile(const CsvFile&) = delete;
    CsvFile operator= (const CsvFile&) = delete;
    CsvFile(CsvFile&&) = delete;
    CsvFile& operator= (CsvFile&&) = delete;

    enum class Operation {read, write};
    bool open(QWidget *parent = nullptr, Operation operation = Operation::read, const QString &caption = tr("Open csv File"),
              const QString &filepath = "", const QString &filetypeDescriptor = "");
    bool openExistingFile(const QString &filepath);
    QFileInfo fileInfo();
    bool isOpen();
    void close(bool deleteFile = false);
    bool readHeader();
    //void setFieldMeanings();
    QDialog* chooseFieldMeaningsDialog(const QList<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);
    bool readDataRow(bool resetToStart = false);
    bool writeHeader();
    void writeDataRow();

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1, const char delimiter = ',');

    QStringList headerValues;
    bool hasHeaderRow = true;
    int numFields = 0;
    int estimatedNumberRows = 0;    // estimated because only based on number of newlines (doesn't account for blank lines or other unused rows)
    QStringList fieldMeanings;
    QStringList fieldValues;
    QStringList fieldsToBeIgnored;

private:
    QFile *file = nullptr;
    QTextStream *stream = nullptr;
    char delimiter = ',';
    listTableDialog *window = nullptr;
    QStringList getLine(const int minFields = -1);
    void validateFieldSelectorBoxes(int callingRow = -1);
    inline static const QString HEADERTEXT = QObject::tr("Column Headers");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");
    inline static const int DIALOGWIDTH = 500;
    inline static const int DIALOGHEIGHT = 300;
};

#endif // CSVFILE_H
