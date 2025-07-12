#include "getGrueprDataDialog.h"
#include "dialogs/dataTypesTableDialog.h"
#include "ui_getGrueprDataDialog.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include "gruepr_globals.h"
#include "dialogs/baseTimeZoneDialog.h"
#include <QCollator>
#include <QComboBox>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPainter>
#include <QProgressDialog>
#include <QSettings>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTimer>

GetGrueprDataDialog::GetGrueprDataDialog(StartDialog *parent) :
    QDialog(parent),
    ui(new Ui::GetGrueprDataDialog),
    parent(parent)
{
    ui->setupUi(this);
    setWindowTitle(tr("gruepr - Form teams"));
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);

    QSettings savedSettings;
    const int numPrevWorks = savedSettings.beginReadArray("prevWorks");
    ui->fromPrevWorkRadioButton->setVisible(numPrevWorks > 0);
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
            ui->prevWorkComboBox->addItem(std::get<1>(prevWork) + " [Last opened: " + std::get<2>(prevWork) + "]", std::get<3>(prevWork));
        }
    }
    savedSettings.endArray();

    ui->sourceFrame->setStyleSheet("QFrame {background-color: " OPENWATERHEX "; color: white; padding: 10px; border: none;}" +
                                   QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: white;"));
    ui->prevWorkComboBox->setStyleSheet(COMBOBOXSTYLE);
    ui->prevWorkComboBox->hide();
    connect(ui->fromPrevWorkRadioButton, &QRadioButton::toggled, ui->prevWorkComboBox, &QComboBox::setVisible);
    ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: " OPENWATERHEX "; color: white; font-family:'DM Sans'; font-size: 12pt; "
                                          "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; padding: 10px;}");
    ui->sourceButtonGroup->setId(ui->fromFileRadioButton, static_cast<int>(DataOptions::DataSource::fromUploadFile));
    ui->sourceButtonGroup->setId(ui->fromGoogleradioButton, static_cast<int>(DataOptions::DataSource::fromGoogle));
    ui->sourceButtonGroup->setId(ui->fromCanvasradioButton, static_cast<int>(DataOptions::DataSource::fromCanvas));
    ui->sourceButtonGroup->setId(ui->fromPrevWorkRadioButton, static_cast<int>(DataOptions::DataSource::fromPrevWork));
    ui->loadDataPushButton->adjustSize();
    QPixmap whiteUploadIcon(":/icons_new/upload_file.png");
    QPainter painter(&whiteUploadIcon);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteUploadIcon.rect(), QColor("white"));
    painter.end();
    int h = ui->loadDataPushButton->height();
    ui->loadDataPushButton->setIcon(whiteUploadIcon.scaledToHeight(h, Qt::SmoothTransformation));
    connect(ui->loadDataPushButton, &QPushButton::clicked, this, &GetGrueprDataDialog::loadData);

    ui->dataSourceFrame->setStyleSheet("QFrame {background-color: " TROPICALHEX "; color: " DEEPWATERHEX "; border: none;}"
                                       "QFrame::disabled {background-color: lightGray; color: darkGray; border: none;}");
    ui->dataSourceLabel->setStyleSheet("QLabel {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt;}"
                                       "QLabel::disabled {background-color: " TRANSPARENT "; color: darkGray; font-family:'DM Sans'; font-size: 12pt;}");
    ui->dataSourceLabel->adjustSize();
    const QPixmap fileIcon(":/icons_new/file.png");
    h = ui->dataSourceLabel->height();
    ui->dataSourceIcon->setPixmap(fileIcon.scaledToHeight(h, Qt::SmoothTransformation));

    ui->hLine->setStyleSheet("QFrame {color: " OPENWATERHEX ";}"
                             "QFrame::disabled {color: lightGray;}");
    ui->fieldsExplainer->setStyleSheet(LABEL10PTSTYLE);
    ui->headerRowCheckBox->setStyleSheet("QCheckBox {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt;}"
                                         "QCheckBox::disabled {color: darkGray; font-family: 'DM Sans'; font-size: 12pt;}");
    categoryHelpButton = new QPushButton("?", this);
    categoryHelpButton->setFixedSize(20, 20);
    categoryHelpButton->setStyleSheet(R"(
        QPushButton {
            font-weight: bold;
            color: white;
            background-color: darkGray;
            border-radius: 10px;
            border: none;
        }
        QPushButton:hover {
            color: darkGray;
            background-color: white;
        }
    )");

    connect(categoryHelpButton, &QPushButton::clicked, this, [this](){
        dataTypesTableDialog *dialog = new dataTypesTableDialog(this);
        dialog->exec();
    });

    // Create a widget to hold both the text and the button
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QHBoxLayout *categoryHeaderLayout = new QHBoxLayout();
    QLabel *questionTextLabel = new QLabel(HEADERTEXT, this);
    QLabel *categoryTextLabel = new QLabel(CATEGORYTEXT, this);
    questionTextLabel->setStyleSheet("color: white; font-family: 'DM Sans'; font-size: 12pt;");
    headerLayout->addWidget(questionTextLabel);
    categoryHeaderLayout->addWidget(categoryTextLabel); // Category text
    categoryTextLabel->setStyleSheet("color: white; font-family: 'DM Sans'; font-size: 12pt;");
    categoryHeaderLayout->addWidget(categoryHelpButton);
    categoryHeaderLayout->setContentsMargins(0, 0, 0, 0);
    categoryHeaderLayout->setSpacing(5);
    headerLayout->addLayout(categoryHeaderLayout);

    QHeaderView *header = ui->tableWidget->horizontalHeader();
    header->setLayout(headerLayout);

    // Add the widget to the horizontal header
    ui->tableWidget->setColumnCount(2);

    // Insert the custom widget into the header
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->setHorizontalHeaderLabels({"",""});
    connect(ui->headerRowCheckBox, &QCheckBox::clicked, this, [this, questionTextLabel]{if(ui->headerRowCheckBox->isChecked())
                                                                        {questionTextLabel->setText(HEADERTEXT);}
                                                                     else
                                                                        {questionTextLabel->setText(ROW1TEXT);}});
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: lightGray; color: white; padding: 5px; "
                                                                             "border-top: none; border-bottom: none; border-left: none; "
                                                                             "border-right: 1px solid darkGray; }");
    // ui->tableWidget->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // ui->tableWidget->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}");
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Confirm"));
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
}

GetGrueprDataDialog::~GetGrueprDataDialog()
{
    if(surveyFile != nullptr) {
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        surveyFile->deleteLater();
    }
    delete ui;
}

void GetGrueprDataDialog::accept() {
    if(dataOptions->dataSource != DataOptions::DataSource::fromPrevWork) {
        if(!readData()) {
            ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            ui->sourceFrame->setStyleSheet("QFrame {background-color: " OPENWATERHEX "; color: white; padding: 10px; border: none;}" +
                                           QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: white;"));
            ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: " OPENWATERHEX "; color: white; font-family:'DM Sans'; font-size: 12pt; "
                                                  "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; padding: 10px;}");
            QPixmap whiteUploadIcon(":/icons_new/upload_file.png");
            QPainter painter(&whiteUploadIcon);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(whiteUploadIcon.rect(), QColor("white"));
            painter.end();
            ui->loadDataPushButton->setIcon(whiteUploadIcon.scaledToHeight(ui->loadDataPushButton->height(), Qt::SmoothTransformation));
            ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}");
            ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: lightGray; color: white; padding: 5px; "
                                                               "border-top: none; border-bottom: none; border-left: none; "
                                                               "border-right: 1px solid darkGray; "
                                                               "font-family: 'DM Sans'; font-size: 12pt;}");

            ui->tableWidget->clearContents();
            ui->tableWidget->setRowCount(0);
            ui->dataSourceFrame->setEnabled(false);
            ui->dataSourceIcon->setEnabled(false);
            ui->dataSourceLabel->setEnabled(false);
            ui->dataSourceLabel->setText(tr("No survey loaded"));
            ui->hLine->setEnabled(false);
            ui->fieldsExplainer->setEnabled(false);
            ui->headerRowCheckBox->setEnabled(false);
            ui->tableWidget->setEnabled(false);
            students.clear();
            return;
        }
    }

    QDialog::accept();
}

void GetGrueprDataDialog::loadData()
{
    if(surveyFile != nullptr) {
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        surveyFile->deleteLater();
    }
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    delete dataOptions;
    roster.clear();

    bool fileLoaded = false;
    switch(static_cast<DataOptions::DataSource>(ui->sourceButtonGroup->checkedId())) {
    case DataOptions::DataSource::fromUploadFile:
        dataOptions = new DataOptions;
        surveyFile = new CsvFile;
        fileLoaded = getFromFile();
        break;
    case DataOptions::DataSource::fromGoogle:
        dataOptions = new DataOptions;
        surveyFile = new CsvFile;
        fileLoaded = getFromGoogle();
        break;
    case DataOptions::DataSource::fromCanvas:
        dataOptions = new DataOptions;
        surveyFile = new CsvFile;
        fileLoaded = getFromCanvas();
        break;
    case DataOptions::DataSource::fromPrevWork:
        fileLoaded = getFromPrevWork();
        break;
    }

    if(!fileLoaded) {
        return;
    }

    if(source == DataOptions::DataSource::fromPrevWork) {
        ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->animateClick();
        return;
    }

    dataOptions->saveStateFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                     "/saveFiles/" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".gr";
    const QDir dir(QFileInfo(dataOptions->saveStateFileName).absoluteDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if(!readQuestionsFromHeader()) {
        return;
    }


    ui->sourceFrame->setStyleSheet("QFrame {background-color: white; color: " DEEPWATERHEX "; padding: 10px; border: none;}" +
                                   QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: " DEEPWATERHEX ";"));
    ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: white; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt; "
                                          "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; padding: 10px;}");
    const QPixmap uploadIcon(":/icons_new/upload_file.png");
    const int h = ui->loadDataPushButton->height();
    ui->loadDataPushButton->setIcon(uploadIcon.scaledToHeight(h, Qt::SmoothTransformation));
    ui->dataSourceFrame->setEnabled(true);
    ui->dataSourceIcon->setEnabled(true);
    ui->dataSourceLabel->setEnabled(true);
    ui->dataSourceLabel->setText(tr("Data source: ") + dataOptions->dataSourceName);
    ui->hLine->setEnabled(true);
    ui->fieldsExplainer->setEnabled(true);
    ui->headerRowCheckBox->setEnabled(true);
    ui->tableWidget->setEnabled(true);
    QString buttonStyle = QString(R"(
                QPushButton {
                    font-weight: bold;
                    color: white;
                    background-color: %1;
                    border-radius: 10px;
                    border: none;
                }
                QPushButton:hover {
                    color: %1;
                    background-color: white;
                }
            )").arg(DEEPWATERHEX);
    categoryHelpButton->setStyleSheet(buttonStyle);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: " OPENWATERHEX "; color: white; padding: 5px; "
                                                                             "border-top: none; border-bottom: none; border-left: none; "
                                                                             "border-right: 1px solid white; "
                                                                             "font-family: 'DM Sans'; font-size: 12pt;}");
    ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}"
                                   "QTableView::item{border-top: none; border-bottom: none; border-left: none; border-right: 1px solid darkGray; padding: 3px;}" +
                                   QString(SCROLLBARSTYLE).replace(DEEPWATERHEX, OPENWATERHEX));

    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

bool GetGrueprDataDialog::getFromFile()
{
    const QPixmap icon(":/icons_new/file.png");

    QSettings savedSettings;
    QFileInfo dataFileLocation;
    dataFileLocation.setFile(savedSettings.value("saveFileLocation", "").toString());

    if(!surveyFile->open(this, CsvFile::Operation::read, tr("Open Survey Data File"), dataFileLocation.canonicalPath(), tr("Survey Data"))) {
        return false;
    }

    savedSettings.setValue("saveFileLocation", surveyFile->fileInfo().canonicalFilePath());
    source = DataOptions::DataSource::fromUploadFile;
    dataOptions->dataSource = source;
    dataOptions->dataSourceName = surveyFile->fileInfo().fileName();
    const int h = ui->dataSourceLabel->height();
    ui->dataSourceIcon->setPixmap(icon.scaledToHeight(h, Qt::SmoothTransformation));
    return true;
}

bool GetGrueprDataDialog::getFromGoogle()
{
    if(!grueprGlobal::internetIsGood()) {
        return false;
    }

    //create googleHandler and authenticate
    auto *google = new GoogleHandler(this);
    if(!google->authenticate()) {
        google->deleteLater();
        return false;
    }

    //ask which survey to download
    QStringList formsList = google->getSurveyList();
    auto *googleFormsDialog = new QDialog(this);
    googleFormsDialog->setWindowTitle(tr("Choose Google survey"));
    googleFormsDialog->setWindowIcon(GoogleHandler::icon());
    googleFormsDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    auto *label = new QLabel(tr("Which survey should be opened?"), googleFormsDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *formsComboBox = new QComboBox(googleFormsDialog);
    formsComboBox->setStyleSheet(COMBOBOXSTYLE);
    for(const auto &form : std::as_const(formsList)) {
        formsComboBox->addItem(form);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    vLayout->addWidget(label);
    vLayout->addWidget(formsComboBox);
    vLayout->addWidget(buttonBox);
    googleFormsDialog->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, googleFormsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, googleFormsDialog, &QDialog::reject);
    if((googleFormsDialog->exec() == QDialog::Rejected)) {
        googleFormsDialog->deleteLater();
        google->deleteLater();
        return false;
    }
    const QString googleFormName = formsComboBox->currentText();
    googleFormsDialog->deleteLater();

    //download the survey
    auto *busyBox = google->actionDialog(this);
    connect(google, &GoogleHandler::retrying, busyBox, [&google, &busyBox](int attemptNum){
        if(attemptNum == 2) {
            google->actionDialogLabel->setText(google->actionDialogLabel->text() + "<br>" + tr("Error. Retrying") +
                                               " (" + QString::number(attemptNum) + " / " + QString::number(LMS::NUM_RETRIES_BEFORE_ABORT) + ")");
        }
        else {
            google->actionDialogLabel->setText(google->actionDialogLabel->text()
                                                   .replace(" (" + QString::number(attemptNum-1), " (" + QString::number(attemptNum)));
        }
        busyBox->adjustSize();
    });
    bool fileNotFound = false;
    connect(google, &GoogleHandler::requestFailed, busyBox, [&fileNotFound](QNetworkReply::NetworkError error){
        fileNotFound = (error == QNetworkReply::NetworkError::ContentNotFoundError);
    });
    const QString filepath = google->downloadSurveyResult(googleFormName);
    const bool fail = filepath.isEmpty() || !surveyFile->openExistingFile(filepath);

    const QPixmap resultIcon(fail? ":/icons_new/error.png" : ":/icons_new/ok.png");
    const QSize iconSize = google->actionDialogIcon->size();
    google->actionDialogIcon->setPixmap(resultIcon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    const QString resultText = (fail? (tr("Download failed.") + "<br>" +
                                      (fileNotFound? tr("The survey was not found in your Google Drive. ") :
                                       tr("Please retry later."))) :
                                tr("Survey downloaded"));
    google->actionDialogLabel->setText(resultText);
    QEventLoop loop;
    busyBox->adjustSize();
    QTimer::singleShot(UI_DISPLAY_DELAYTIME * (fail? 2 : 0.5), &loop, &QEventLoop::quit);
    loop.exec();
    google->actionComplete(busyBox);
    google->deleteLater();

    if(fail) {
        return false;
    }

    source = DataOptions::DataSource::fromGoogle;
    dataOptions->dataSource = source;
    dataOptions->dataSourceName = googleFormName;
    const int h = ui->dataSourceLabel->height();
    ui->dataSourceIcon->setPixmap(GoogleHandler::icon().scaledToHeight(h, Qt::SmoothTransformation));

    return true;
}

bool GetGrueprDataDialog::getFromCanvas()
{
    if(!grueprGlobal::internetIsGood()) {
        return false;
    }

    //create canvasHandler and/or authenticate as needed
    auto *canvas = new CanvasHandler(this);
    if(!canvas->authenticate()) {
        canvas->deleteLater();
        return false;
    }

    //ask the user from which course we're downloading the survey
    auto *busyBox = canvas->actionDialog(this);
    QList<CanvasHandler::CanvasCourse> canvasCourses = canvas->getCourses();
    canvas->actionComplete(busyBox);

    auto *canvasCoursesAndQuizzesDialog = new QDialog(this);
    canvasCoursesAndQuizzesDialog->setStyleSheet(QString(LABEL10PTSTYLE) + COMBOBOXSTYLE);
    canvasCoursesAndQuizzesDialog->setWindowTitle(tr("Choose Canvas course"));
    canvasCoursesAndQuizzesDialog->setWindowIcon(CanvasHandler::icon());
    canvasCoursesAndQuizzesDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    int i = 1;
    auto *label = new QLabel(tr("From which course should the survey be downloaded?"), canvasCoursesAndQuizzesDialog);
    auto *coursesAndQuizzesComboBox = new QComboBox(canvasCoursesAndQuizzesDialog);
    for(const auto &canvasCourse : std::as_const(canvasCourses)) {
        coursesAndQuizzesComboBox->addItem(canvasCourse.name);
        coursesAndQuizzesComboBox->setItemData(i++, QString::number(canvasCourse.numStudents) + " students", Qt::ToolTipRole);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, canvasCoursesAndQuizzesDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesAndQuizzesComboBox);
    vLayout->addWidget(buttonBox);
    canvasCoursesAndQuizzesDialog->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, canvasCoursesAndQuizzesDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, canvasCoursesAndQuizzesDialog, &QDialog::reject);
    if((canvasCoursesAndQuizzesDialog->exec() == QDialog::Rejected)) {
        canvasCoursesAndQuizzesDialog->deleteLater();
        canvas->deleteLater();
        return false;
    }
    const QString course = coursesAndQuizzesComboBox->currentText();
    canvasCoursesAndQuizzesDialog->hide();

    //ask which survey (canvas Quiz) to download
    busyBox = canvas->actionDialog(this);
    QStringList formsList = canvas->getQuizList(course);
    canvas->actionComplete(busyBox);

    canvasCoursesAndQuizzesDialog->setWindowTitle(tr("Choose Canvas quiz"));
    label->setText(tr("Which survey should be downloaded?"));
    coursesAndQuizzesComboBox->clear();
    for(const auto &form : std::as_const(formsList)) {
        coursesAndQuizzesComboBox->addItem(form);
    }
    if((canvasCoursesAndQuizzesDialog->exec() == QDialog::Rejected)) {
        canvasCoursesAndQuizzesDialog->deleteLater();
        canvas->deleteLater();
        return false;
    }
    const QString canvasSurveyName = coursesAndQuizzesComboBox->currentText();
    canvasCoursesAndQuizzesDialog->deleteLater();

    //download the survey and, if successful, the roster
    busyBox = canvas->actionDialog(this);
    const QString filepath = canvas->downloadQuizResult(course, canvasSurveyName);
    const bool fail = filepath.isEmpty() || !surveyFile->openExistingFile(filepath);
    if(!fail) {
        //get the roster for later comparison
        roster = canvas->getStudentRoster(course);
    }

    const QPixmap resultIcon(fail? ":/icons_new/error.png" : ":/icons_new/ok.png");
    const QSize iconSize = canvas->actionDialogIcon->size();
    canvas->actionDialogIcon->setPixmap(resultIcon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    const QString resultText = (fail? tr("Download failed. Please retry later.") : tr("Survey downloaded"));
    canvas->actionDialogLabel->setText(resultText);
    QEventLoop loop;
    busyBox->adjustSize();
    QTimer::singleShot(UI_DISPLAY_DELAYTIME * (fail? 1 : 0.5), &loop, &QEventLoop::quit);
    loop.exec();
    canvas->actionComplete(busyBox);
    canvas->deleteLater();

    if(fail) {
        return false;
    }

    // Only include the timestamp question ("submitted") and then the questions we've asked.
    // These will all begin with (possibly a quotation mark then) an integer, then a colon, then a space.
    // (Also ignore the text "question" which serves as the text notifier that several schedule questions are coming up).
    surveyFile->fieldsToBeIgnored = QStringList{R"(^(?!(submitted)|("?\d+: .*)).*$)", ".*" + CanvasHandler::SCHEDULEQUESTIONINTRO2.trimmed() + ".*"};

    source = DataOptions::DataSource::fromCanvas;
    dataOptions->dataSource = source;
    dataOptions->dataSourceName = canvasSurveyName;
    const int h = ui->dataSourceLabel->height();
    ui->dataSourceIcon->setPixmap(CanvasHandler::icon().scaledToHeight(h, Qt::SmoothTransformation));

    return true;
}

bool GetGrueprDataDialog::getFromPrevWork()
{
    QFile savedFile(ui->prevWorkComboBox->currentData().toString(), this);
    if(!savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        return false;
    }

    auto *loadingProgressDialog = new QProgressDialog(tr("Loading data..."), QString(), 0, 100, this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    loadingProgressDialog->setWindowModality(Qt::ApplicationModal);
    loadingProgressDialog->setAutoReset(false);
    connect(parent, &StartDialog::closeDataDialogProgressBar, loadingProgressDialog, &QProgressDialog::reset);
    loadingProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    loadingProgressDialog->setMinimumDuration(0);
    loadingProgressDialog->setStyleSheet(QString(LABEL10PTSTYLE) + PROGRESSBARSTYLE);

    const QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
    savedFile.close();

    loadingProgressDialog->setValue(1);

    QJsonObject content = doc.object();
    const QJsonArray studentjsons = content["students"].toArray();
    students.reserve(studentjsons.size());
    int i = 2;
    loadingProgressDialog->setMaximum(studentjsons.size() + 3);
    for(const auto &studentjson : studentjsons) {
        students.emplaceBack(studentjson.toObject());
        loadingProgressDialog->setValue(i++);
    }
    dataOptions = new DataOptions(content["dataoptions"].toObject());
    source = DataOptions::DataSource::fromPrevWork;
    dataOptions->dataSource = source;

    loadingProgressDialog->setValue(loadingProgressDialog->maximum());
    loadingProgressDialog->setLabelText("Opening gruepr window...");

    return true;
}

bool GetGrueprDataDialog::readQuestionsFromHeader()
{
    if(!surveyFile->readHeader()) {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    if(surveyFile->headerValues.size() < 2) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    // See if there are header fields after any of (preferred teammates / non-teammates, section, or schedule) since those are probably notes fields
    static const QRegularExpression lastKnownMeaningfulField("(.*(like to not have on your team).*)|(.*(want to avoid working with).*)|"
                                                             "(.*(like to have on your team).*)|(.*(want to work with).*)|"
                                                             ".*(which section are you enrolled).*|(.*(check).+(times).*)",
                                                             QRegularExpression::CaseInsensitiveOption);
    const int notesFieldsProbBeginAt = 1 + int(surveyFile->headerValues.lastIndexOf(lastKnownMeaningfulField));
    if((notesFieldsProbBeginAt != 0) && (notesFieldsProbBeginAt != surveyFile->headerValues.size())) {
        //if notesFieldsProbBeginAt == 0 then none of these questions exist, so assume no notes because list ends with attributes
        //and if notesFieldsProbBeginAt == headervalues size, also assume no notes because list ends with one of these questions
        for(int field = notesFieldsProbBeginAt; field < surveyFile->fieldMeanings.size(); field++) {
            surveyFile->fieldMeanings[field] = "Notes";
        }
    }

    // Ask user what the columns mean
    QList<possFieldMeaning> surveyFieldOptions = {{"Timestamp", "(timestamp)|(^submitted$)", 1},
                                                  {"First Name", "((first)|(given)|(preferred))(?!.*last).*(name)", 1},
                                                  {"Last Name", "^(?!.*first).*((last)|(sur)|(family)).*(name)", 1},
                                                  {"Email Address", "(e).*(mail)", 1},
                                                  {"Gender", "((gender)|(pronouns))", 1},
                                                  {"Racial/ethnic identity", "((minority)|(ethnic))", 1},
                                                  {"Schedule", "((check)|(select)).+(times)", MAX_DAYS},
                                                  {"Section", "which section are you enrolled", 1},
                                                  {"Timezone","(time zone)", 1},
                                                  {"Preferred Teammates", "(like to have on your team)|(want to work with)", MAX_PREFTEAMMATES},
                                                  {"Preferred Non-teammates", "(like to not have on your team)|(want to avoid working with)", MAX_PREFTEAMMATES},
                                                  {"Multiple Choice", ".*", MAX_ATTRIBUTES},
                                                  {"Notes", "", 99}};
    // see if each field is a value to be ignored; if not and the fieldMeaning is empty, preload with possibleFieldMeaning based on matches to the patterns
    for(int i = 0; i < surveyFile->numFields; i++) {
        const QString &headerVal = surveyFile->headerValues.at(i);

        bool ignore = false;
        for(const auto &matchpattern : std::as_const(surveyFile->fieldsToBeIgnored)) {
            if(headerVal.contains(QRegularExpression(matchpattern, QRegularExpression::CaseInsensitiveOption))) {
                surveyFile->fieldMeanings[i] = "**IGNORE**";
                ignore = true;
            }
            // if this is coming from Canvas, see if it's the LMSID field and, if so, set the field
            if((ui->sourceButtonGroup->checkedId() == static_cast<int>(DataOptions::DataSource::fromCanvas)) && (headerVal.compare("id", Qt::CaseInsensitive) == 0)) {
                surveyFile->fieldMeanings[i] = "**LMSID**";
                ignore = true;
            }
        }

        if(!ignore && surveyFile->fieldMeanings.at(i).isEmpty()) {
            int matchPattern = 0;
            QString match;
            do {
                match = surveyFieldOptions.at(matchPattern).regExSearchString;
                matchPattern++;
            } while((matchPattern < surveyFieldOptions.size()) && !headerVal.contains(QRegularExpression(match, QRegularExpression::CaseInsensitiveOption)));

            if(matchPattern != surveyFieldOptions.size()) {
                surveyFile->fieldMeanings[i] = surveyFieldOptions.at(matchPattern - 1).nameShownToUser;
            }
            else {
                surveyFile->fieldMeanings[i] = UNUSEDTEXT;
            }
        }
    }

    ui->tableWidget->setRowCount(surveyFile->numFields);
    // a label and combobox for each column
    for(int row = 0; row < surveyFile->numFields; row++) {
        auto *label = new QLabel("\n" + surveyFile->headerValues.at(row) + "\n", this);
        label->setStyleSheet(LABEL10PTSTYLE);
        label->setWordWrap(true);
        ui->tableWidget->setCellWidget(row, 0, label);

        auto *selector = new QComboBox(this);
        selector->setStyleSheet(COMBOBOXSTYLE);
        selector->setFocusPolicy(Qt::StrongFocus);  // remove scrollwheel from affecting the value,
        selector->installEventFilter(new MouseWheelBlocker(selector)); // as it's too easy to mistake scrolling through the rows with changing the value
        for(const auto &meaning : std::as_const(surveyFieldOptions)) {
            selector->addItem(meaning.nameShownToUser, meaning.maxNumOfFields);
        }
        selector->insertItem(0, UNUSEDTEXT);
        auto *model = qobject_cast<QStandardItemModel *>(selector->model());
        model->item(0)->setForeground(Qt::darkRed);
        selector->insertSeparator(1);
        if((surveyFile->fieldMeanings.at(row) == "**IGNORE**") || (surveyFile->fieldMeanings.at(row) == "**LMSID**")) {
            selector->addItem(surveyFile->fieldMeanings.at(row));
            selector->setCurrentText(surveyFile->fieldMeanings.at(row));
            ui->tableWidget->hideRow(row);
        }
        else {
            selector->setCurrentText(surveyFile->fieldMeanings.at(row));
        }
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        const int width = selector->minimumSizeHint().width();
        selector->setMinimumWidth(width);
        selector->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        ui->tableWidget->setCellWidget(row, 1, selector);
        connect(selector, &QComboBox::currentTextChanged, this, [this, row]{validateFieldSelectorBoxes(row);});
    }
    validateFieldSelectorBoxes();
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->adjustSize();

    return true;
}

//////////////////
// Validate the selector boxes in the choose field meaning table:
// one per field unless there's an asterisk in the name, in which case there are as many as the number after
//////////////////
void GetGrueprDataDialog::validateFieldSelectorBoxes(int callingRow)
{
    // get list of rows in top-to-bottom order, but if this function is getting called by a selector box, then put its row at the front of the line
    QList<int> rows(surveyFile->numFields);
    std::iota(rows.begin(), rows.end(), 0);
    if(callingRow != -1) {
        rows.remove(callingRow);
        rows.prepend(callingRow);
    }

    // start by counting all the values to count how many times each are used, and which are fully used
    std::map<QString, int> takenValues;     // mapping fieldMeaning -> number of fields selected with this meaning
    std::map<QString, int> fullyUsedValues; // mapping the same, but saving how many extra fields with this meaning
    for(auto row : rows) {
        // get the selected fieldMeaning
        const auto *box = qobject_cast<QComboBox *>(ui->tableWidget->cellWidget(row, 1));
        if((box->currentText() == "**IGNORE**") || (box->currentText() == "**LMSID**")) {
            continue;
        }
        const QString selection = box->currentText();

        // set it in the CsvFile's data
        surveyFile->fieldMeanings[row] = selection;

        // add this occurence in the takenValues mapping
        if(takenValues.count(selection) == 0) {
            // first ocurrence of this field; create the key/value
            takenValues[selection] = 1;
        }
        else {
            // key already exists
            takenValues[selection]++;
        }

        // if we are at or above the allowed number of ocurrences, note it
        if(takenValues[selection] >= box->currentData().toInt()) { //combo box notes allowed number of ocurrences
            // add this occurence in the takenValues mapping
            if(fullyUsedValues.count(selection) == 0) {
                // field has just reached capacity; create the key/value
                fullyUsedValues[selection] = 1;
            }
            else {
                // key already exists; we have MORE than are allowed
                fullyUsedValues[selection]++;
            }
        }
        else {
            fullyUsedValues[selection] = 0;
        }
    }

    // Now go back through in reverse order and:
    //  1) replacing overused values with "Unused",
    //  2) setting fully used values in other boxes to red with a tooltip,
    //  3) clearing formatting of all non-overused values (except "Unused") and the fully used values that are currently chosen.
    // Then:
    //  4) clearing formatting of all items unchosen in any box (except "Unused").
    for(auto row = rows.rbegin(); row != rows.rend(); ++row) {
        auto *box = qobject_cast<QComboBox *>(ui->tableWidget->cellWidget(*row, 1));
        if((box->currentText() == "**IGNORE**") || (box->currentText() == "**LMSID**")) {
            continue;
        }
        box->blockSignals(true);
        auto *model = qobject_cast<QStandardItemModel *>(box->model());
        for(auto &takenValue : takenValues) {
            const QString fieldval = takenValue.first;
            const int numAllowed = box->itemData(box->findText(fieldval)).toInt();
            QStandardItem *item = model->item(box->findText(fieldval));
            if((fullyUsedValues[fieldval] > 1) && (box->currentText() == fieldval)) {
                // number exceeds max. allowed somehow, so set to unused
                box->setCurrentText(UNUSEDTEXT);
                surveyFile->fieldMeanings[*row] = UNUSEDTEXT;
                fullyUsedValues[fieldval]--;
                if(numAllowed == 1) {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if((fullyUsedValues[fieldval] == 1) && (box->currentText() != fieldval)) {
                // at capacity, and not selected in this box
                item->setForeground(Qt::darkRed);
                if(numAllowed == 1) {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if(fieldval != UNUSEDTEXT) {
                // below capacity or at capacity including this one
                item->setForeground(Qt::black);
                item->setToolTip("");
            }
        }

        // clearing formatting of all unchosen items except "Unused"
        for(int itemNum = 0; itemNum < box->count(); itemNum++) {
            if((takenValues.count(box->itemText(itemNum)) == 0) && (box->itemText(itemNum) != UNUSEDTEXT)) {
                model->item(itemNum)->setForeground(Qt::black);
                model->item(itemNum)->setToolTip("");
            }
        }
        box->blockSignals(false);
    }
}

bool GetGrueprDataDialog::readData()
{
    auto *loadingProgressDialog = new QProgressDialog(tr("Loading data..."), tr("Cancel"), 0, surveyFile->estimatedNumberRows + MAX_ATTRIBUTES + 6,
                                                      this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    loadingProgressDialog->setAutoReset(false);
    connect(parent, &StartDialog::closeDataDialogProgressBar, loadingProgressDialog, &QProgressDialog::reset);
    loadingProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    loadingProgressDialog->setMinimumDuration(0);
    loadingProgressDialog->setWindowModality(Qt::ApplicationModal);
    loadingProgressDialog->setStyleSheet(QString(LABEL10PTSTYLE) + PROGRESSBARSTYLE + SMALLBUTTONSTYLEINVERTED);

    // set field values now according to user's selection of field meanings (defaulting to FIELDNOTPRESENT if not chosen)
    dataOptions->timestampField = int(surveyFile->fieldMeanings.indexOf("Timestamp"));
    dataOptions->LMSIDField = int(surveyFile->fieldMeanings.indexOf("**LMSID**"));
    dataOptions->firstNameField = int(surveyFile->fieldMeanings.indexOf("First Name"));
    dataOptions->lastNameField = int(surveyFile->fieldMeanings.indexOf("Last Name"));
    dataOptions->emailField = int(surveyFile->fieldMeanings.indexOf("Email Address"));
    dataOptions->genderField = int(surveyFile->fieldMeanings.indexOf("Gender"));
    dataOptions->genderIncluded = (dataOptions->genderField != DataOptions::FIELDNOTPRESENT);
    dataOptions->URMField = int(surveyFile->fieldMeanings.indexOf("Racial/ethnic identity"));
    dataOptions->URMIncluded = (dataOptions->URMField != DataOptions::FIELDNOTPRESENT);
    dataOptions->sectionField = int(surveyFile->fieldMeanings.indexOf("Section"));
    dataOptions->sectionIncluded = (dataOptions->sectionField != DataOptions::FIELDNOTPRESENT);
    dataOptions->timezoneField = int(surveyFile->fieldMeanings.indexOf("Timezone"));
    dataOptions->timezoneIncluded = (dataOptions->timezoneField != DataOptions::FIELDNOTPRESENT);

    //associating column numbers with fields

    // pref teammates fields
    int lastFoundIndex = 0;
    const int numTeammateQs = int(surveyFile->fieldMeanings.count("Preferred Teammates"));
    for(int prefQ = 0; prefQ < numTeammateQs; prefQ++) {
        dataOptions->prefTeammatesField << int(surveyFile->fieldMeanings.indexOf("Preferred Teammates", lastFoundIndex));
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile->fieldMeanings.indexOf("Preferred Teammates", lastFoundIndex)));
    }
    // pref non-teammates fields
    lastFoundIndex = 0;
    const int numNonTeammateQs = int(surveyFile->fieldMeanings.count("Preferred Non-teammates"));
    for(int prefQ = 0; prefQ < numNonTeammateQs; prefQ++) {
        dataOptions->prefNonTeammatesField << int(surveyFile->fieldMeanings.indexOf("Preferred Non-teammates", lastFoundIndex));
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile->fieldMeanings.indexOf("Preferred Non-teammates", lastFoundIndex)));
    }
    // notes fields
    lastFoundIndex = 0;
    const int numNotes = int(surveyFile->fieldMeanings.count("Notes"));
    for(int note = 0; note < numNotes; note++) {
        dataOptions->notesFields << int(surveyFile->fieldMeanings.indexOf("Notes", lastFoundIndex));
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile->fieldMeanings.indexOf("Notes", lastFoundIndex)));
    }
    // attribute fields, adding timezone field as an attribute if it exists
    lastFoundIndex = 0;
    dataOptions->numAttributes = int(surveyFile->fieldMeanings.count("Multiple Choice"));
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        dataOptions->attributeField[attribute] = int(surveyFile->fieldMeanings.indexOf("Multiple Choice", lastFoundIndex));
        dataOptions->attributeQuestionText << surveyFile->headerValues.at(dataOptions->attributeField[attribute]);
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile->fieldMeanings.indexOf("Multiple Choice", lastFoundIndex)));
    }
    if(dataOptions->timezoneIncluded) {
        dataOptions->attributeField[dataOptions->numAttributes] = dataOptions->timezoneField;
        dataOptions->attributeQuestionText << surveyFile->headerValues.at(dataOptions->timezoneField);
        dataOptions->numAttributes++;
    }
    // schedule fields
    lastFoundIndex = 0;
    for(int scheduleQuestion = 0, numScheduleFields = int(surveyFile->fieldMeanings.count("Schedule")); scheduleQuestion < numScheduleFields; scheduleQuestion++) {
        const int field = int(surveyFile->fieldMeanings.indexOf("Schedule", lastFoundIndex));
        dataOptions->scheduleField << field;
        const QString scheduleQuestionText = surveyFile->headerValues.at(field);
        static const QRegularExpression freeOrAvailable(".+\\b(free|available)\\b.+", QRegularExpression::CaseInsensitiveOption);
        if(scheduleQuestionText.contains(freeOrAvailable)) {
            // if >=1 field has this language, all interpreted as free time
            dataOptions->scheduleDataIsFreetime = true;
        }
        static const QRegularExpression homeTimezone(".+\\b(your home)\\b.+", QRegularExpression::CaseInsensitiveOption);
        if(scheduleQuestionText.contains(homeTimezone)) {
            // if >=1 field has this language, all interpreted as referring to each student's home timezone
            dataOptions->homeTimezoneUsed = true;
        }
        static const QRegularExpression dayNameFinder("\\[([^[]*)\\]");   // Day name is in brackets at end of field (where Google Forms puts column titles in matrix questions)
        const QRegularExpressionMatch dayName = dayNameFinder.match(scheduleQuestionText);
        if(dayName.hasMatch()) {
            dataOptions->dayNames << dayName.captured(1);
        }
        else {
            dataOptions->dayNames << " " + QString::number(scheduleQuestion+1) + " ";
        }
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile->fieldMeanings.indexOf("Schedule", lastFoundIndex)));
    }
    loadingProgressDialog->setValue(1);
    // read one line of data; if no data after header row then file is invalid
    if(!surveyFile->readDataRow()) {
        grueprGlobal::errorMessage(this, tr("Insufficient number of students."),
                                   tr("There are no survey responses in this file."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    // If there is schedule info, read through the schedule fields in all of the responses to compile a list of time names, save as dataOptions->TimeNames
    if(!dataOptions->dayNames.isEmpty()) {
        QStringList allTimeNames;
        do {
            for(const int fieldNum : std::as_const(dataOptions->scheduleField)) {
                QString scheduleFieldText = (surveyFile->fieldValues.at(fieldNum)).toLower().split(';').join(',');
                QTextStream scheduleFieldStream(&scheduleFieldText);
                allTimeNames << CsvFile::getLine(scheduleFieldStream);
            }
        } while(surveyFile->readDataRow());
        allTimeNames.removeDuplicates();
        allTimeNames.removeOne("");

        //sort allTimeNames smartly, using string -> hour of day float; any timeName not found is put at the beginning of the list
        std::sort(allTimeNames.begin(), allTimeNames.end(), [] (const QString &a, const QString &b)
                                            {return grueprGlobal::timeStringToHours(a) < grueprGlobal::timeStringToHours(b);});
        dataOptions->timeNames = allTimeNames;

        // Set the schedule resolution (in units of hours) by looking at all the time values.
        // If any end with 0.25, set schedule resolution to 0.25 immediately and stop looking.
        // If none do, still keep looking for any that end 0.5, in which case resolution is 0.5.
        // If none do, keep at default of 1.
        dataOptions->scheduleResolution = 1;
        for(const auto &timeName : std::as_const(dataOptions->timeNames)) {
            const int numOfQuarterHours = std::lround(4 * grueprGlobal::timeStringToHours(timeName)) % 4;
            if((numOfQuarterHours == 1) || (numOfQuarterHours == 3)) {
                dataOptions->scheduleResolution = 0.25;
                break;
            }
            else if(numOfQuarterHours == 2) {
                dataOptions->scheduleResolution = 0.5;
            }
        }

        //pad the timeNames to include all 24 hours if we will be time-shifting student responses based on their home timezones later
        if(dataOptions->homeTimezoneUsed) {
            dataOptions->earlyTimeAsked = std::max(0.0f, grueprGlobal::timeStringToHours(dataOptions->timeNames.constFirst()));
            dataOptions->lateTimeAsked = std::max(0.0f, grueprGlobal::timeStringToHours(dataOptions->timeNames.constLast()));
            const QStringList formats = QString(TIMEFORMATS).split(';');

            //figure out which format to use for the timenames we're adding
            auto timeName = dataOptions->timeNames.constBegin();
            QString timeFormat;
            QTime time;
            do {
                for(const auto &format : formats) {
                    time = QTime::fromString(*timeName, format);
                    if(time.isValid()) {
                        timeFormat = format;
                        break;
                    }
                }
                timeName++;
            } while(!time.isValid() && timeName != dataOptions->timeNames.constEnd());

            float hoursSinceMidnight = 0;
            for(int timeBlock = 0; timeBlock < int(24 / dataOptions->scheduleResolution); timeBlock++) {
                if(dataOptions->timeNames.size() > timeBlock) {
                    if(grueprGlobal::timeStringToHours(dataOptions->timeNames.at(timeBlock)) == hoursSinceMidnight) {
                        //this timename already exists in the list
                        hoursSinceMidnight += dataOptions->scheduleResolution;
                        continue;
                    }
                }
                time.setHMS(int(hoursSinceMidnight), int(60 * (hoursSinceMidnight - int(hoursSinceMidnight))), 0);
                dataOptions->timeNames.insert(timeBlock, time.toString(timeFormat));
                hoursSinceMidnight += dataOptions->scheduleResolution;
            }

            // Ask what should be used as the base timezone to which schedules will all be adjusted
            auto *window = new baseTimezoneDialog(this);
            window->exec();
            dataOptions->baseTimezone = window->baseTimezoneVal;
            window->deleteLater();
        }
    }
    loadingProgressDialog->setValue(2);

    // Having read the header row and determined time names, if any, read each remaining row as a student record
    surveyFile->readDataRow(CsvFile::ReadLocation::beginningOfFile);    // put cursor back to beginning and read first row
    if(surveyFile->hasHeaderRow) {
        // that first row was headers, so get next row
        surveyFile->readDataRow();
    }

    students.reserve(surveyFile->estimatedNumberRows);
    int numStudents = 0;
    StudentRecord currStudent;
    do {
        if(loadingProgressDialog->wasCanceled()) {
            surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
            return false;
        }

        currStudent.clear();
        currStudent.parseRecordFromStringList(surveyFile->fieldValues, *dataOptions);
        currStudent.ID = students.size();

        // see if this record is a duplicate; assume it isn't and then check
        currStudent.duplicateRecord = false;
        for(auto &student : students) {
            if((((currStudent.firstname + currStudent.lastname).compare(student.firstname + student.lastname, Qt::CaseInsensitive) == 0) &&
                 !(currStudent.firstname + currStudent.lastname).isEmpty()) ||
                ((currStudent.email.compare(student.email, Qt::CaseInsensitive) == 0) &&
                 !currStudent.email.isEmpty())) {
                currStudent.duplicateRecord = true;
                student.duplicateRecord = true;
            }
        }

        // Figure out what type of gender data was given (if any) -- initialized value is GenderType::adult, and we're checking each student
        // because some values are ambiguous to GenderType (e.g. "nonbinary")
        if(dataOptions->genderIncluded) {
            const QString genderText = surveyFile->fieldValues.at(dataOptions->genderField);
            if(genderText.contains(tr("male"), Qt::CaseInsensitive)) {  // contains "male" also picks up "female"
                dataOptions->genderType = GenderType::biol;
            }
            else if(genderText.contains(tr("man"), Qt::CaseInsensitive)) {  // contains "man" also picks up "woman"
                dataOptions->genderType = GenderType::adult;
            }
            else if((genderText.contains(tr("girl"), Qt::CaseInsensitive)) || (genderText.contains("boy", Qt::CaseInsensitive))) {
                dataOptions->genderType = GenderType::child;
            }
            else if(genderText.contains(tr("he"), Qt::CaseInsensitive)) {  // contains "he" also picks up "she" and "they"
                dataOptions->genderType = GenderType::pronoun;
            }
        }

        numStudents++;
        students << currStudent;
        loadingProgressDialog->setValue(2 + numStudents);
    } while(surveyFile->readDataRow() && numStudents < MAX_STUDENTS);

    if(numStudents < MIN_STUDENTS) {
        grueprGlobal::errorMessage(this, tr("Insufficient number of students."),
                                   tr("There are only ") + QString::number(numStudents) + tr(" survey responses.\n") + QString::number(MIN_STUDENTS) + tr(" is the minimum."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    // if there's a (separately-sourced) roster of students, compare against the list of submissions
    // add the name, section and email data if available in roster and blank in survey data,
    // (ask whether to replace section data if present in both)
    // and add the info of any non-submitters now
    if(!roster.isEmpty()) {

        // if there's section data in both the survey and the roster, ask which to use
        bool loadSectionFromRoster = false;
        if(std::any_of(students.constBegin(), students.constEnd(), [](const auto &student){return !student.section.isEmpty();}) &&
           std::any_of(roster.constBegin(), roster.constEnd(), [](const auto &student){return !student.section.isEmpty();})) {
            loadSectionFromRoster = grueprGlobal::warningMessage(this, "gruepr",
                                                                 tr("The survey contains section data, but so does the roster") +
                                                                    (dataOptions->dataSource == DataOptions::DataSource::fromCanvas? tr(" from Canvas") : "") + ".\n" +
                                                                     tr("Would you like to replace the survey data with that from the roster?"),
                                                                 tr("Yes"), tr("No"));
        }

        students.reserve(roster.size());
        int numNonSubmitters = 0;
        for(const auto &studentOnRoster : std::as_const(roster)) {
            int index = 0;
            const long long LMSid = studentOnRoster.LMSID;
            while((index < numStudents) && (LMSid != students.at(index).LMSID)) {
                index++;
            }

            if(index == numStudents && numStudents < MAX_STUDENTS) {
                // Match not found -- student did not submit a survey -- so add a record with their name
                numNonSubmitters++;
                currStudent.clear();
                currStudent.surveyTimestamp = QDateTime();
                currStudent.LMSID = LMSid;
                currStudent.ID = students.size();
                currStudent.firstname = studentOnRoster.firstname;
                currStudent.lastname = studentOnRoster.lastname;
                currStudent.email = studentOnRoster.email;
                currStudent.section = studentOnRoster.section;
                for(auto &day : currStudent.unavailable) {
                    for(auto &time : day) {
                        time = false;
                    }
                }
                currStudent.ambiguousSchedule = true;
                students << currStudent;
                numStudents++;
            }
            else {
                // Match found, but student doesn't have a name, email address, or section yet, so copy it from roster info
                if(students.at(index).firstname.isEmpty() && !studentOnRoster.firstname.isEmpty()) {
                    students[index].firstname = studentOnRoster.firstname;
                    dataOptions->firstNameField = DataOptions::DATAFROMOTHERSOURCE;
                }
                if(students.at(index).lastname.isEmpty() && !studentOnRoster.lastname.isEmpty()) {
                    students[index].lastname = studentOnRoster.lastname;
                    dataOptions->lastNameField = DataOptions::DATAFROMOTHERSOURCE;
                }
                if(students.at(index).email.isEmpty() && !studentOnRoster.email.isEmpty()) {
                    students[index].email = studentOnRoster.email;
                    dataOptions->emailField = DataOptions::DATAFROMOTHERSOURCE;
                }
                if((students.at(index).section.isEmpty() || loadSectionFromRoster) && !studentOnRoster.section.isEmpty()) {
                    students[index].section = studentOnRoster.section;
                    dataOptions->sectionField = DataOptions::DATAFROMOTHERSOURCE;
                    dataOptions->sectionIncluded = true;
                }
            }
        }

        if(numNonSubmitters > 0) {
            grueprGlobal::errorMessage(this, tr("Not all surveys submitted"),
                                       QString::number(numNonSubmitters) + " " + (numNonSubmitters == 1? tr("student has") : tr("students have")) +
                                       tr(" not submitted a survey. Their ") + (numNonSubmitters == 1? tr("name has") : tr("names have")) +
                                       tr(" been added to the roster."));
        }
    }

    if(numStudents == MAX_STUDENTS) {
        grueprGlobal::errorMessage(this, tr("Reached maximum number of students."),
                                   tr("The maximum number of students have been read."
                                      " This version of gruepr does not allow more than ") + QString::number(MAX_STUDENTS) + ".");
    }

    // Set the attribute question options and numerical values for each student
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++) {
        if(loadingProgressDialog->wasCanceled()) {
            surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
            return false;
        }

        if(dataOptions->attributeField[attribute] != DataOptions::FIELDNOTPRESENT) {
            auto &responses = dataOptions->attributeQuestionResponses[attribute];
            auto &attributeType = dataOptions->attributeType[attribute];
            // gather all unique attribute question responses, then remove a blank response if it exists in a list with other responses
            for(const auto &student : std::as_const(students)) {
                if(!responses.contains(student.attributeResponse[attribute])) {
                    responses << student.attributeResponse[attribute];
                }
            }
            if(responses.size() > 1) {
                responses.removeAll(QString(""));
            }

            // Figure out what type of attribute this is: timezone, ordered/numerical, categorical (one response), or categorical (mult. responses)
            // If this is the timezone field, it's timezone type;
            // otherwise, if any response contains a comma, then it's multicategorical
            // otheriwse, if every response starts with an integer, it is ordered (numerical);
            // otherwise, if any response is missing an integer at the start, then it is categorical
            // The regex to recognize ordered/numerical is:
            // digit(s) then, optionally, "." or "," then end; OR digit(s) then "." or "," then any character but digits; OR digit(s) then any character but "." or ","
            static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
            if(dataOptions->attributeField[attribute] == dataOptions->timezoneField) {
                attributeType = DataOptions::AttributeType::timezone;
            }
            else if(std::any_of(responses.constBegin(), responses.constEnd(), [](const QString &response)
                                                                              {return response.contains(',');})) {
                attributeType = DataOptions::AttributeType::multicategorical;   // might be multiordered, this gets sorted out below
            }
            else if(std::all_of(responses.constBegin(), responses.constEnd(), [](const QString &response)
                                                                              {return startsWithInteger.match(response).hasMatch();})) {
                attributeType = DataOptions::AttributeType::ordered;
            }
            else {
                attributeType = DataOptions::AttributeType::categorical;
            }

            // for multicategorical, have to reprocess the responses to delimit at the commas and then determine if actually multiordered
            if(attributeType == DataOptions::AttributeType::multicategorical) {
                for(int originalResponseNum = 0, numOriginalResponses = int(responses.size()); originalResponseNum < numOriginalResponses; originalResponseNum++) {
                    QStringList newResponses = responses.takeFirst().split(',');
                    for(auto &newResponse : newResponses) {
                        newResponse = newResponse.trimmed();
                        if(!responses.contains(newResponse)) {
                            responses << newResponse;
                        }
                    }
                }
                responses.removeAll(QString(""));
                // now that we've split them up, let's see if actually multiordered instead of multicategorical
                if(std::all_of(responses.constBegin(), responses.constEnd(), [](const QString &response) {return startsWithInteger.match(response).hasMatch();})) {
                    attributeType = DataOptions::AttributeType::multiordered;
                }
            }

            // sort alphanumerically unless it's timezone, in which case sort according to offset from GMT
            if(attributeType != DataOptions::AttributeType::timezone) {
                QCollator sortAlphanumerically;
                sortAlphanumerically.setNumericMode(true);
                sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
                std::sort(responses.begin(), responses.end(), sortAlphanumerically);
            }
            else {
                std::sort(responses.begin(), responses.end(), [] (const QString &A, const QString &B) {
                    float timezoneA = 0, timezoneB = 0;
                    QString unusedtimezoneName;
                    DataOptions::parseTimezoneInfoFromText(A, unusedtimezoneName, timezoneA);
                    DataOptions::parseTimezoneInfoFromText(B, unusedtimezoneName, timezoneB);
                    return timezoneA < timezoneB;});
            }

            // set values associated with each response and create a spot to hold the responseCounts
            if((attributeType == DataOptions::AttributeType::ordered) ||
                (attributeType == DataOptions::AttributeType::multiordered)) {
                // ordered/numerical values. value is based on number at start of response
                for(const auto &response : std::as_const(responses)) {
                    dataOptions->attributeVals[attribute].insert(startsWithInteger.match(response).captured(1).toInt());
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({response, 0});
                }
            }
            else if((attributeType == DataOptions::AttributeType::categorical) ||
                     (attributeType == DataOptions::AttributeType::multicategorical)) {
                // categorical values. value is based on index of response within sorted list
                for(int i = 1; i <= responses.size(); i++) {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }
            else {
                for(int i = 1; i <= responses.size(); i++) {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }

            // set numerical value of each student's response and record in dataOptions a tally for each response
            for(auto &student : students) {
                const QString &currentStudentResponse = student.attributeResponse[attribute];
                QList<int> &currentStudentAttributeVals = student.attributeVals[attribute];
                if(!student.attributeResponse[attribute].isEmpty()) {
                    if(attributeType == DataOptions::AttributeType::ordered) {
                        // for numerical/ordered, set numerical value of students' attribute responses according to the number at the start of the response
                        currentStudentAttributeVals << startsWithInteger.match(currentStudentResponse).captured(1).toInt();
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if((attributeType == DataOptions::AttributeType::categorical) ||
                             (attributeType == DataOptions::AttributeType::timezone)) {
                        // set numerical value instead according to their place in the sorted list of responses
                        currentStudentAttributeVals << int(responses.indexOf(currentStudentResponse)) + 1;
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if(attributeType == DataOptions::AttributeType::multicategorical) {
                        //multicategorical - set numerical values according to each value
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent) {
                            currentStudentAttributeVals << int(responses.indexOf(responseFromStudent.trimmed())) + 1;
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                    else if(attributeType == DataOptions::AttributeType::multiordered) {
                        //multiordered - set numerical values according to the numbers at the start of the responses
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent) {
                            currentStudentAttributeVals << startsWithInteger.match(responseFromStudent.trimmed()).captured(1).toInt();
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                }
                else {
                    currentStudentAttributeVals << -1;
                }
            }
        }
        loadingProgressDialog->setValue(2 + numStudents + attribute);
    }
    loadingProgressDialog->setValue(2 + numStudents + MAX_ATTRIBUTES);
    // gather all unique URM and section question responses and sort
    for(auto &student : students) {
        if(!dataOptions->URMResponses.contains(student.URMResponse, Qt::CaseInsensitive)) {
            dataOptions->URMResponses << student.URMResponse;
        }
        if(!(dataOptions->sectionNames.contains(student.section, Qt::CaseInsensitive))) {
            dataOptions->sectionNames << student.section;
        }

        for(auto gender: student.gender){
            if(!dataOptions->Genders.contains(gender)) {
                dataOptions->Genders << gender;
            }
        }
        //Only take the first gender, otherwise numOfIdentities > numOfStudents
        QString studentFirstGender = grueprGlobal::genderToString(student.gender.values()[0]);
        if (dataOptions->numberOfIdentitiesInPopulation.contains(studentFirstGender)){
            dataOptions->numberOfIdentitiesInPopulation[studentFirstGender] += 1;
        } else{
            dataOptions->numberOfIdentitiesInPopulation[studentFirstGender] = 0;
        }

        if (dataOptions->numberOfIdentitiesInPopulation.contains(student.URMResponse)){
            dataOptions->numberOfIdentitiesInPopulation[student.URMResponse] += 1;
        } else{
            dataOptions->numberOfIdentitiesInPopulation[student.URMResponse] = 0;
        }
    }

    QCollator sortAlphanumerically;
    sortAlphanumerically.setNumericMode(true);
    sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(dataOptions->URMResponses.begin(), dataOptions->URMResponses.end(), sortAlphanumerically);
    if(dataOptions->URMResponses.contains("--")) {
        // put the blank response option at the end of the list
        dataOptions->URMResponses.removeAll("--");
        dataOptions->URMResponses << "--";
    }
    std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);

    loadingProgressDialog->setValue(surveyFile->estimatedNumberRows + 3 + MAX_ATTRIBUTES);

    // set all of the students' tooltips
    for(auto &student : students) {
        student.createTooltip(*dataOptions);
    }
    loadingProgressDialog->setValue(surveyFile->estimatedNumberRows + 4 + MAX_ATTRIBUTES);
    surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
    loadingProgressDialog->setValue(loadingProgressDialog->maximum());
    loadingProgressDialog->setLabelText("Opening gruepr window...");
    return true;
}
