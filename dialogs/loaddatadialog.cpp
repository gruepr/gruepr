#include "loaddatadialog.h"
#include "qcombobox.h"
#include "qdir.h"
#include "qsettings.h"
#include "qstandardpaths.h"

loadDataDialog::loadDataDialog(StartDialog *parent) : QDialog(parent){
    setWindowTitle(tr("gruepr - Form teams"));
    setWindowIcon(QIcon(":/icons_new/icon.svg"));
    //setMinimumSize(BASEWINDOWWIDTH, BASEWINDOWHEIGHT);
    QSettings savedSettings;
    setStyleSheet("background-color: white");
    //set background image same as the beginning page?

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(5);
    QLabel *headerLabel = new QLabel("Load your student data to form teams");
    QLabel *headerLabel2 = new QLabel("GruePR does not collect any student data - it is stored in your computer.");

    headerLabel->setStyleSheet(LABEL14PTSTYLE);
    QFrame *dropCSVFileFrame = new QFrame(this);
    dropCSVFileFrame->setAcceptDrops(true);
    dropCSVFileFrame->setStyleSheet(DROPFRAME);
    dropCSVFileFrame->setMinimumWidth(0);
    dropCSVFileFrame->setMinimumHeight(200);
    dropCSVFileFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QPushButton* uploadButton = new QPushButton(this);
    uploadButton->setIcon(QIcon(":/icons_new/upload.png"));
    uploadButton->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    uploadButton->setStyleSheet(UPLOADBUTTONSTYLE);
    connect(uploadButton, &QPushButton::clicked, this, &loadDataDialog::loadData);

    QLabel *uploadSourceLabel = new QLabel("Drag & drop or Choose file to upload.", this);
    QLabel *acceptedFormatsLabel = new QLabel("Accepted format: .csv");
    uploadSourceLabel->setStyleSheet(LABEL10PTSTYLE);
    acceptedFormatsLabel->setStyleSheet(LABEL10PTSTYLE);

    QVBoxLayout* dropCSVFileFrameLayout = new QVBoxLayout();

    dropCSVFileFrameLayout->addWidget(uploadButton, 0, Qt::AlignCenter);
    dropCSVFileFrameLayout->addWidget(uploadSourceLabel, 0, Qt::AlignCenter);
    dropCSVFileFrameLayout->addWidget(acceptedFormatsLabel, 0, Qt::AlignCenter);
    dropCSVFileFrameLayout->setContentsMargins(0, 0, 0, 0);
    dropCSVFileFrameLayout->setSpacing(5);

    dropCSVFileFrame->setLayout(dropCSVFileFrameLayout);

    QHBoxLayout *otherDataSourcesLayout = new QHBoxLayout();
    QLabel *otherDataSourcesLabel = new QLabel("Other data sources", this);
    otherDataSourcesLabel->setStyleSheet(LABEL12PTSTYLE);

    QFrame *googleFormFrame = new QFrame(this);
    googleFormFrame->setStyleSheet(BASICFRAME);
    QVBoxLayout* googleFormFrameLayout = new QVBoxLayout();
    googleFormFrame->setLayout(googleFormFrameLayout);
    //add Label, button
    QPushButton* googleFormLabel = new QPushButton(this);
    googleFormLabel->setIcon(QIcon(":/icons_new/googleform.png"));
    googleFormLabel->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    googleFormLabel->setText("Google Form");
    googleFormLabel->setStyleSheet(LABELONLYBUTTON);

    QPushButton* loadDataFromGoogleFormButton = new QPushButton(this);
    loadDataFromGoogleFormButton->setIcon(QIcon(":/icons_new/upload.png"));
    loadDataFromGoogleFormButton->setIconSize(QSize(SMALLERICONSIZE,SMALLERICONSIZE));
    loadDataFromGoogleFormButton->setText("Load data from google form results");
    loadDataFromGoogleFormButton->setStyleSheet(STANDARDBUTTON);
    googleFormFrameLayout->addWidget(googleFormLabel, 0, Qt::AlignLeft);
    googleFormFrameLayout->addWidget(loadDataFromGoogleFormButton, 0, Qt::AlignLeft);

    //Canvas Survey Frame
    QFrame *canvasSurveyFrame = new QFrame(this);
    canvasSurveyFrame->setStyleSheet(BASICFRAME);
    QVBoxLayout* canvasSurveyFrameLayout = new QVBoxLayout();
    canvasSurveyFrame->setLayout(canvasSurveyFrameLayout);
    QPushButton* canvasSurveyLabel = new QPushButton(this);
    canvasSurveyLabel->setIcon(QIcon(":/icons_new/canvas.png"));
    canvasSurveyLabel->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    canvasSurveyLabel->setText("Canvas Survey");
    canvasSurveyLabel->setStyleSheet(LABELONLYBUTTON);

    QPushButton* loadDataFromCanvasSurveyLabel = new QPushButton(this);
    loadDataFromCanvasSurveyLabel->setIcon(QIcon(":/icons_new/upload.png"));
    loadDataFromCanvasSurveyLabel->setIconSize(QSize(SMALLERICONSIZE,SMALLERICONSIZE));
    loadDataFromCanvasSurveyLabel->setText("Load data from canvas survey results");
    loadDataFromCanvasSurveyLabel->setStyleSheet(STANDARDBUTTON);
    canvasSurveyFrameLayout->addWidget(canvasSurveyLabel, 0, Qt::AlignLeft);
    canvasSurveyFrameLayout->addWidget(loadDataFromCanvasSurveyLabel, 0, Qt::AlignLeft);

    //Reopen Prev Work Frame
    QFrame *reopenPrevWorkFrame = new QFrame(this);
    reopenPrevWorkFrame->setStyleSheet(BASICFRAME);
    QVBoxLayout* reopenPrevWorkFrameLayout = new QVBoxLayout();
    reopenPrevWorkFrame->setLayout(reopenPrevWorkFrameLayout);

    QPushButton* reopenPrevWorkLabel = new QPushButton(this);
    reopenPrevWorkLabel->setIcon(QIcon(":/icons_new/save.png"));
    reopenPrevWorkLabel->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    reopenPrevWorkLabel->setText("Reopen previous work");
    reopenPrevWorkLabel->setStyleSheet(LABELONLYBUTTON);

    QHBoxLayout *prevWorkComboBoxLayout = new QHBoxLayout();
    QComboBox *prevWorkComboBox = new QComboBox(this);
    QPushButton *confirmButton = new QPushButton ("Confirm", this);
    confirmButton->setStyleSheet(STANDARDBUTTON);
    prevWorkComboBoxLayout->addWidget(prevWorkComboBox, 3);
    prevWorkComboBoxLayout->addWidget(confirmButton, 1);
    prevWorkComboBox->setStyleSheet(COMBOBOXSTYLE);
    prevWorkComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    reopenPrevWorkFrameLayout->addWidget(reopenPrevWorkLabel, 0, Qt::AlignLeft);
    reopenPrevWorkFrameLayout->addLayout(prevWorkComboBoxLayout);

    const int numPrevWorks = savedSettings.beginReadArray("prevWorks");
    prevWorkComboBox->setVisible(numPrevWorks > 0);
    if(numPrevWorks > 0) {
        QList<std::tuple<QDateTime, QString, QString, QString>> prevWorkInfos;   //lastModifiedDate, displayName, lastModifiedDateString, fileName
        prevWorkInfos.reserve(numPrevWorks);
        for (int prevWork = 0; prevWork < numPrevWorks; prevWork++) {
            savedSettings.setArrayIndex(prevWork);
            const QString displayName = savedSettings.value("prevWorkName", "").toString();
            const QDateTime lastModifiedDate = QDateTime::fromString(savedSettings.value("prevWorkDate", "").toString(), QLocale::system().dateTimeFormat(QLocale::LongFormat));
            const QString lastModifiedDateString = lastModifiedDate.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
            const QString fileName = savedSettings.value("prevWorkFile", "").toString();
            if(!displayName.isEmpty() && !lastModifiedDateString.isEmpty() && !fileName.isEmpty()) {
                prevWorkInfos.emplaceBack(lastModifiedDate, displayName, lastModifiedDateString, fileName);
            }
        }
        std::sort(prevWorkInfos.begin(), prevWorkInfos.end(), [](std::tuple<QDateTime, QString, QString, QString> a, std::tuple<QDateTime, QString, QString, QString> b)
                  {return std::get<0>(a) > std::get<0>(b);});  // sort by last modified date descending
        for(const auto &prevWork : prevWorkInfos) {
            prevWorkComboBox->addItem(std::get<1>(prevWork) + " [Last opened: " + std::get<2>(prevWork) + "]", std::get<3>(prevWork));
        }
    }
    savedSettings.endArray();

    otherDataSourcesLayout->addWidget(googleFormFrame, 0);
    otherDataSourcesLayout->addWidget(canvasSurveyFrame, 0);
    otherDataSourcesLayout->addWidget(reopenPrevWorkFrame, 0);

    mainLayout->addWidget(headerLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(headerLabel2, 0, Qt::AlignCenter);
    mainLayout->addWidget(dropCSVFileFrame);
    mainLayout->addStretch(1);
    mainLayout->addWidget(otherDataSourcesLabel, 0, Qt::AlignLeft);
    mainLayout->addLayout(otherDataSourcesLayout);
    setLayout(mainLayout);
}

// void loadDataDialog::loadData()
// {
//     if(surveyFile != nullptr) {
//         surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
//         surveyFile->deleteLater();
//     }
//     //ui->tableWidget->clearContents();
//     //ui->tableWidget->setRowCount(0);
//     delete dataOptions;
//     //roster.clear();

//     bool fileLoaded = false;
//     const QPixmap icon(":/icons_new/file.png");

//     QSettings savedSettings;
//     QFileInfo dataFileLocation;
//     dataFileLocation.setFile(savedSettings.value("saveFileLocation", "").toString());

//     if(surveyFile->open(this, CsvFile::Operation::read, tr("Open Survey Data File"), dataFileLocation.canonicalPath(), tr("Survey Data"))) {
//         fileLoaded = true;
//     }

//     savedSettings.setValue("saveFileLocation", surveyFile->fileInfo().canonicalFilePath());
//     source = DataOptions::DataSource::fromFile;
//     dataOptions->dataSource = source;
//     dataOptions->dataSourceName = surveyFile->fileInfo().fileName();

//     if(!fileLoaded) {
//         return;
//     }

//     if(source == DataOptions::DataSource::fromPrevWork) {
//         //ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
//         //ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->animateClick();
//         return;
//     }

//     dataOptions->saveStateFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
//                                      "/saveFiles/" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".gr";
//     const QDir dir(QFileInfo(dataOptions->saveStateFileName).absoluteDir());
//     if (!dir.exists()) {
//         dir.mkpath(".");
//     }

//     if(!readQuestionsFromHeader()) {
//         return;
//     }


//     ui->sourceFrame->setStyleSheet("QFrame {background-color: white; color: " DEEPWATERHEX "; padding: 10px; border: none;}" +
//                                    QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: " DEEPWATERHEX ";"));
//     ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: white; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt; "
//                                           "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; padding: 10px;}");
//     const QPixmap uploadIcon(":/icons_new/upload_file.png");
//     const int h = ui->loadDataPushButton->height();
//     ui->loadDataPushButton->setIcon(uploadIcon.scaledToHeight(h, Qt::SmoothTransformation));
//     ui->dataSourceFrame->setEnabled(true);
//     ui->dataSourceIcon->setEnabled(true);
//     ui->dataSourceLabel->setEnabled(true);
//     ui->dataSourceLabel->setText(tr("Data source: ") + dataOptions->dataSourceName);
//     ui->hLine->setEnabled(true);
//     ui->fieldsExplainer->setEnabled(true);
//     ui->headerRowCheckBox->setEnabled(true);
//     ui->tableWidget->setEnabled(true);
//     QString buttonStyle = QString(R"(
//                 QPushButton {
//                     font-weight: bold;
//                     color: white;
//                     background-color: %1;
//                     border-radius: 10px;
//                     border: none;
//                 }
//                 QPushButton:hover {
//                     color: %1;
//                     background-color: white;
//                 }
//             )").arg(DEEPWATERHEX);
//     categoryHelpButton->setStyleSheet(buttonStyle);
//     ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: " OPENWATERHEX "; color: white; padding: 5px; "
//                                                        "border-top: none; border-bottom: none; border-left: none; "
//                                                        "border-right: 1px solid white; "
//                                                        "font-family: 'DM Sans'; font-size: 12pt;}");
//     ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}"
//                                    "QTableView::item{border-top: none; border-bottom: none; border-left: none; border-right: 1px solid darkGray; padding: 3px;}" +
//                                    QString(SCROLLBARSTYLE).replace(DEEPWATERHEX, OPENWATERHEX));

//     ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
// }

void loadDataDialog::accept() {
    // if(dataOptions->dataSource != DataOptions::DataSource::fromPrevWork) {
    //     if(!readData()) {
    //         ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    //         ui->sourceFrame->setStyleSheet("QFrame {background-color: " OPENWATERHEX "; color: white; padding: 10px; border: none;}" +
    //                                        QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: white;"));
    //         ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: " OPENWATERHEX "; color: white; font-family:'DM Sans'; font-size: 12pt; "
    //                                               "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; padding: 10px;}");
    //         QPixmap whiteUploadIcon(":/icons_new/upload_file.png");
    //         QPainter painter(&whiteUploadIcon);
    //         painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    //         painter.fillRect(whiteUploadIcon.rect(), QColor("white"));
    //         painter.end();
    //         ui->loadDataPushButton->setIcon(whiteUploadIcon.scaledToHeight(ui->loadDataPushButton->height(), Qt::SmoothTransformation));
    //         ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}");
    //         ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: lightGray; color: white; padding: 5px; "
    //                                                            "border-top: none; border-bottom: none; border-left: none; "
    //                                                            "border-right: 1px solid darkGray; "
    //                                                            "font-family: 'DM Sans'; font-size: 12pt;}");

    //         ui->tableWidget->clearContents();
    //         ui->tableWidget->setRowCount(0);
    //         ui->dataSourceFrame->setEnabled(false);
    //         ui->dataSourceIcon->setEnabled(false);
    //         ui->dataSourceLabel->setEnabled(false);
    //         ui->dataSourceLabel->setText(tr("No survey loaded"));
    //         ui->hLine->setEnabled(false);
    //         ui->fieldsExplainer->setEnabled(false);
    //         ui->headerRowCheckBox->setEnabled(false);
    //         ui->tableWidget->setEnabled(false);
    //         students.clear();
    //         return;
    //     }
    // }

    QDialog::accept();
}
