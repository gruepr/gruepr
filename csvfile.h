#ifndef CSVFILE_H
#define CSVFILE_H

#include <QDialog>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>

class CsvFile : QObject
{
    Q_OBJECT

public:
    CsvFile();
    ~CsvFile();

    bool open(QWidget *parent = nullptr, const QString &caption = "", const QString &filepath = "", const QString &filetypeDescriptor = "");
    bool readHeader();
    QDialog* chooseFieldMeaningsDialog(const QStringList &possibleFieldMeanings, const QStringList &possibleMeaningMatchPatterns = {""}, QWidget *parent = nullptr);
    bool readDataRow(const int minFields = -1);
    QFileInfo fileInfo();
    void close();

    QStringList getLine(const int minFields = -1);
    static QStringList getLine(QTextStream &externalStream, const int minFields = -1);

    QStringList headerValues;
    QStringList fieldMeanings;
    QStringList fieldValues;

private:
    QFile *file = nullptr;
    QTextStream *stream = nullptr;
    QDialog *window = nullptr;
};

#endif // CSVFILE_H
