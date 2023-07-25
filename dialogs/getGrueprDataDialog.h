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

    enum Source{fromFile, fromGoogle, fromCanvas};

private:
    Ui::GetGrueprDataDialog *ui;

    void getData();
    void loadSurvey(CsvFile &surveyFile);
    CanvasHandler *canvas = nullptr;
    GoogleHandler *google = nullptr;

    QList<StudentRecord> students;
    DataOptions *dataOptions = nullptr;
};

#endif // GETGRUEPRDATADIALOG_H
