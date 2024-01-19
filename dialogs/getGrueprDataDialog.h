#ifndef GETGRUEPRDATADIALOG_H
#define GETGRUEPRDATADIALOG_H

#include <QDialog>
#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/startDialog.h"
#include "studentRecord.h"

namespace Ui {
class GetGrueprDataDialog;
}

class GetGrueprDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GetGrueprDataDialog(StartDialog *parent = nullptr);
    ~GetGrueprDataDialog() override;
    GetGrueprDataDialog(const GetGrueprDataDialog&) = delete;
    GetGrueprDataDialog operator= (const GetGrueprDataDialog&) = delete;
    GetGrueprDataDialog(GetGrueprDataDialog&&) = delete;
    GetGrueprDataDialog& operator= (GetGrueprDataDialog&&) = delete;

    DataOptions *dataOptions = nullptr;
    QList<StudentRecord> students;

public slots:
    void accept() override;

private:
    Ui::GetGrueprDataDialog *ui;
    StartDialog *parent;

    DataOptions::DataSource source = DataOptions::DataSource::fromFile;

    QList<StudentRecord> roster;    // holds roster of students from alternative source (in order to add names of non-submitters)

    void loadData();
    CsvFile *surveyFile = nullptr;
    bool getFromFile();
    bool getFromGoogle();
    bool getFromCanvas();
    bool getFromPrevWork();
    bool readQuestionsFromHeader();
    void validateFieldSelectorBoxes(int callingRow = -1);
    bool readData();
    inline static const QString HEADERTEXT = QObject::tr("Question text");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");
};

#endif // GETGRUEPRDATADIALOG_H
