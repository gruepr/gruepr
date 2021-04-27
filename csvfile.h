#ifndef CSVFILE_H
#define CSVFILE_H

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTableWidget>
#include <QTextStream>
#include "listTableDialog.h"

typedef  std::tuple<QString, QString, int> possFieldMeaning;

class CsvFile : public QObject
{
    Q_OBJECT

public:
    CsvFile(QObject *parent = nullptr);
    ~CsvFile();

    bool open(QWidget *parent = nullptr, const QString &caption = "", const QString &filepath = "", const QString &filetypeDescriptor = "");
    bool readHeader();
    QDialog* chooseFieldMeaningsDialog(const QVector<possFieldMeaning> &possibleFieldMeanings = {}, QWidget *parent = nullptr);
    bool readDataRow();
    QFileInfo fileInfo();
    void close();

    static QStringList getLine(QTextStream &externalStream, const int minFields = -1);

    QStringList headerValues;
    QStringList fieldMeanings;
    QStringList fieldValues;

private:
    QFile *file = nullptr;
    QTextStream *stream = nullptr;
    listTableDialog *window = nullptr;
    int numFields = 0;
    QStringList getLine(const int minFields = -1);
    void validateFieldSelectorBoxes(int callingRow = -1);
};

#endif // CSVFILE_H
