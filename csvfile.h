#ifndef CSVFILE_H
#define CSVFILE_H

#include "dialogs/listTableDialog.h"
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>
#include <QTextStream>

struct possFieldMeaning      // 1) name of field shown to user, 2) RegEx search string, 3) number of fields that might have this meaning
{
    QString nameShownToUser;
    QString regExSearchString;
    int maxNumOfFields;
};

class CsvFile : public QObject
{
    Q_OBJECT

public:
    enum Delimiter {comma, tab};
    enum Source {fileOnly, fileOrOnline};

    CsvFile(Delimiter dlmtr = comma, QObject *parent = nullptr);
    ~CsvFile();
    CsvFile(const CsvFile&) = delete;
    CsvFile operator= (const CsvFile&) = delete;
    CsvFile(CsvFile&&) = delete;
    CsvFile& operator= (CsvFile&&) = delete;

    enum Operation {read, write};
    bool open(QWidget *parent = nullptr, Operation operation = read, const QString &caption = tr("Open csv File"),
              const QString &filepath = "", const QString &filetypeDescriptor = "");
    bool openExistingFile(const QString &filepath);
    QFileInfo fileInfo();
    void close(bool deleteFile = false);
    bool readHeader();
    //void setFieldMeanings();
    QDialog* chooseFieldMeaningsDialog(const QVector<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);
    bool readDataRow(bool resetToStart = false);
    bool writeHeader();
    void writeDataRow();

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1, const char delimiter = ',');

    QStringList headerValues;
    bool hasHeaderRow = true;
    int numFields = 0;
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
    const QString HEADERTEXT = tr("Column Headers");
    const QString CATEGORYTEXT = tr("Category");
    const QString ROW1TEXT = tr("First Row of Data");
    const QString UNUSEDTEXT = tr("Unused");
    inline static const int DIALOGWIDTH = 500;
    inline static const int DIALOGHEIGHT = 300;
};

#endif // CSVFILE_H
