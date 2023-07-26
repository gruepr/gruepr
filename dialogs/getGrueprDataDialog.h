#ifndef GETGRUEPRDATADIALOG_H
#define GETGRUEPRDATADIALOG_H

#include <QDialog>
#include "canvashandler.h"
#include "csvfile.h"
#include "dataOptions.h"
#include "googlehandler.h"
#include "studentRecord.h"

namespace Ui {
class GetGrueprDataDialog;
}

class GetGrueprDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GetGrueprDataDialog(QWidget *parent = nullptr);
    ~GetGrueprDataDialog();

    DataOptions *dataOptions = nullptr;
    QList<StudentRecord> students;

private:
    Ui::GetGrueprDataDialog *ui;

    enum Source{fromFile, fromGoogle, fromCanvas};

    void loadData();
    CsvFile surveyFile;
    bool isTempFile;
    bool getFromFile();
    bool getFromGoogle();
    bool getFromCanvas();
    bool readQuestionsFromHeader();
    void validateFieldSelectorBoxes(int callingRow = -1);
    bool readData();
    CanvasHandler *canvas = nullptr;
    GoogleHandler *google = nullptr;

    const QString HEADERTEXT = tr("Column Headers");
    const QString CATEGORYTEXT = tr("Category");
    const QString ROW1TEXT = tr("First Row of Data");
    const QString UNUSEDTEXT = tr("Unused");
};

#endif // GETGRUEPRDATADIALOG_H
