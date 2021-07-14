#ifndef CSVFILE_H
#define CSVFILE_H

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>
#include <QTextStream>
#include "listTableDialog.h"

typedef  std::tuple<QString, QString, int> possFieldMeaning;    // 1) name of field shown to user, 2) RegEx search string, 3) number of fields that might have this meaning

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
    QDialog* chooseFieldMeaningsDialog(const QVector<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);
    bool readDataRow();
    bool writeHeader();
    void writeDataRow();

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1, const char delimiter = ',');

    QStringList headerValues;
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
    const QString UNUSEDTEXT = tr("Unused");
    const int DIALOGWIDTH = 500;
    const int DIALOGHEIGHT = 300;
};

#endif // CSVFILE_H
