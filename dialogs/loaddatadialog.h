#ifndef LOADDATADIALOG_H
#define LOADDATADIALOG_H

#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/startDialog.h"
#include "qdialog.h"
#include "studentRecord.h"

class loadDataDialog : public QDialog
{
    Q_OBJECT
public:
    explicit loadDataDialog(StartDialog *parent = nullptr);

    DataOptions *dataOptions = nullptr;
    QList<StudentRecord> students;
public slots:
    void accept() override;

private:
    void loadData();
    CsvFile *surveyFile = nullptr;
    bool getFromFile();
    bool getFromGoogle();
    bool getFromCanvas();
    bool getFromPrevWork();
    bool readData();
    DataOptions::DataSource source = DataOptions::DataSource::fromFile;

    inline static const QString HEADERTEXT = QObject::tr("Question text");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");
    inline static const int BASEWINDOWWIDTH = 800;
    inline static const int BASEWINDOWHEIGHT = 456;
    inline static const int BASICICONSIZE = 30;
    inline static const int SMALLERICONSIZE = 20;

};

#endif // LOADDATADIALOG_H
