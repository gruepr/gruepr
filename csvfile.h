#ifndef CSVFILE_H
#define CSVFILE_H

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>
#include <QTextStream>
#include "dialogs/listTableDialog.h"

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

    CsvFile(Delimiter dlmtr = comma, QObject *parent = nullptr);
    ~CsvFile();

    enum Operation {read, write};
    bool open(QWidget *parent = nullptr, Operation operation = read, const QString &caption = tr("Open csv File"),
              const QString &filepath = "", const QString &filetypeDescriptor = "");
    QFileInfo fileInfo();
    void close();
    bool readHeader();
    //void setFieldMeanings();
    QDialog* chooseFieldMeaningsDialog(const QVector<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);
    bool readDataRow(bool resetToStart = false);
    bool writeHeader();
    void writeDataRow();

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1, const char delimiter = ',');

    QStringList headerValues;
    bool hasHeaderRow = true;
//    QVector<possFieldMeaning> defaultFieldMeanings;   // used if the open file dialog will display auto-read meanings
    QStringList fieldMeanings;
    QStringList fieldValues;

private:
    QFile *file = nullptr;
    QTextStream *stream = nullptr;
    int numFields = 0;
    char delimiter = ',';
    listTableDialog *window = nullptr;
    QStringList getLine(const int minFields = -1);
    void validateFieldSelectorBoxes(int callingRow = -1);
    const QString HEADERTEXT = tr("Column Headers");
    const QString CATEGORYTEXT = tr("Category");
    const QString ROW1TEXT = tr("First Row of Data");
    const QString UNUSEDTEXT = tr("Unused");
    const int DIALOGWIDTH = 500;
    const int DIALOGHEIGHT = 300;
};

#endif // CSVFILE_H
