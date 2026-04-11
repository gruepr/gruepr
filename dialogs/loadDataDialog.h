#ifndef LOADDATADIALOG_H
#define LOADDATADIALOG_H

#include "csvfile.h"
#include "dataOptions.h"
#include "dialogs/startDialog.h"
#include "studentRecord.h"
#include <memory>
#include <QButtonGroup>
#include <QDialog>
#include <QProgressDialog>

namespace Ui {
class loadDataDialog;
}

class loadDataDialog : public QDialog
{
    Q_OBJECT
public:
    explicit loadDataDialog(StartDialog *parent = nullptr);
    ~loadDataDialog() override;

    QProgressDialog *loadingProgressDialog = nullptr;

    std::unique_ptr<DataOptions> dataOptions;
    QList<StudentRecord> students;
    CsvFile* getSurveyFile();

public slots:
    void accept() override;
    void acceptWithManualCategories();

private:
    Ui::loadDataDialog *ui;
    StartDialog *parent;
    void loadData(QString filePathString);
    void finalizeAccept(bool showCategorizingDialog);
    std::unique_ptr<CsvFile> surveyFile;
    bool getFromFile();
    bool getFromGoogle();
    bool getFromCanvas();
    bool getFromPrevWork();
    bool getFromDropFile(QString filePathString);
    bool readData();
    bool readQuestionsFromHeader();
    DataOptions::DataSource source = DataOptions::DataSource::fromUploadFile;
    QButtonGroup *sourceButtonGroup = nullptr;
    QList<StudentRecord> roster;

    // Page indices for the stacked widget
    enum SourcePage { CsvPage = 0, GooglePage = 1, CanvasPage = 2, PrevWorkPage = 3 };

    inline static const QString HEADERTEXT = QObject::tr("Question text");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");
    inline static const int BASEWINDOWWIDTH = 800;
    inline static const int BASEWINDOWHEIGHT = 456;
    inline static const int BASICICONSIZE = 30;
    inline static const int SMALLERICONSIZE = 20;

    inline static const char PILLCARDSELECTED[] = "QPushButton {background-color: " FOAMHEX "; color: " DEEPWATERHEX "; "
                                                               "border: 1.5px solid " DEEPWATERHEX "; "
                                                               "border-radius: 8px; padding: 10px 8px;"
                                                               "font-family: 'DM Sans'; font-size: 12pt;}";

    inline static const char PILLCARDUNSELECTED[] = "QPushButton {background-color: white; color: gray; "
                                                                 "border: 1.5px solid " DEEPWATERHEX "; "
                                                                 "border-radius: 8px; padding: 10px 8px; "
                                                                 "font-family: 'DM Sans'; font-size: 12pt;}"
                                                    "QPushButton:disabled {background-color: lightGray; "
                                                                          "border-color: darkGray;}"
                                                    "QPushButton:hover {background-color: " FOAMHEX "; "
                                                                       "border-color: " OPENWATERHEX ";}";
};

#endif // LOADDATADIALOG_H
