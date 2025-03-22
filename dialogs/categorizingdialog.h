#ifndef CATEGORIZINGDIALOG_H
#define CATEGORIZINGDIALOG_H

#include "csvfile.h"
#include "dataOptions.h"
#include "qcombobox.h"
#include "qdialog.h"
#include "qtablewidget.h"
class CategorizingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CategorizingDialog(QWidget* parent = nullptr, CsvFile* surveyFile = nullptr, DataOptions::DataSource dataSource = DataOptions::DataSource::fromUploadFile);
    QTableWidget* datasetTableWidget;
    QHBoxLayout* datasetTableHeaderLayout;
    CsvFile* surveyFile;
    DataOptions::DataSource source;
    QList<QComboBox*> dataTypeComboBoxes;
    QList<QWidget*> columnWidgets;
    QDialogButtonBox* confirmCancelButtonBox;
    bool initializeComboBoxes();
    void validateFieldSelectorBoxes(int callingRow);


    void populateTable();
public slots:
    void accept() override;
private:
    inline static const QString HEADERTEXT = QObject::tr("Question text");
    inline static const QString CATEGORYTEXT = QObject::tr("Category");
    inline static const QString ROW1TEXT = QObject::tr("First Row of Data");
    inline static const QString UNUSEDTEXT = QObject::tr("Unused");

};

#endif // CATEGORIZINGDIALOG_H
