#include "loadDataDialog.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include "dialogs/baseTimeZoneDialog.h"
#include "dialogs/categorizingDialog.h"
#include "widgets/dropcsvframe.h"
#include <QCollator>
#include <QComboBox>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

loadDataDialog::loadDataDialog(StartDialog *parent) : QDialog(), parent(parent){
    setWindowTitle(tr("gruepr - Form teams"));
    setWindowIcon(QIcon(":/icons_new/icon.svg"));
    //setMinimumSize(BASEWINDOWWIDTH, BASEWINDOWHEIGHT);
    QSettings savedSettings;
    setStyleSheet("background-color: white");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(5);

    QLabel *headerLabel = new QLabel("Load your student data to form teams");

    headerLabel->setStyleSheet(LABEL14PTSTYLE);
    DropCSVFrame *dropCSVFileFrame = new DropCSVFrame(this);
    dropCSVFileFrame->setStyleSheet(DROPFRAME);
    dropCSVFileFrame->setMinimumWidth(0);
    dropCSVFileFrame->setMinimumHeight(200);
    dropCSVFileFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QPushButton* uploadButton = new QPushButton(this);
    uploadButton->setIcon(QIcon(":/icons_new/upload.png"));
    uploadButton->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    uploadButton->setStyleSheet(UPLOADBUTTONSTYLE);

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
    googleFormLabel->setIcon(QIcon(":/icons_new/google.png"));
    googleFormLabel->setIconSize(QSize(BASICICONSIZE,BASICICONSIZE));
    googleFormLabel->setText("Google Form");
    googleFormLabel->setStyleSheet(LABELONLYBUTTON);

    QPushButton* loadDataFromGoogleFormButton = new QPushButton(this);
    loadDataFromGoogleFormButton->setIcon(QIcon(":/icons_new/upload.png"));
    loadDataFromGoogleFormButton->setIconSize(QSize(SMALLERICONSIZE,SMALLERICONSIZE));
    loadDataFromGoogleFormButton->setText("Load data from google form results");
    loadDataFromGoogleFormButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
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

    QPushButton* loadDataFromCanvasSurveyButton = new QPushButton(this);
    loadDataFromCanvasSurveyButton->setIcon(QIcon(":/icons_new/upload.png"));
    loadDataFromCanvasSurveyButton->setIconSize(QSize(SMALLERICONSIZE,SMALLERICONSIZE));
    loadDataFromCanvasSurveyButton->setText("Load data from canvas survey results");
    loadDataFromCanvasSurveyButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    canvasSurveyFrameLayout->addWidget(canvasSurveyLabel, 0, Qt::AlignLeft);
    canvasSurveyFrameLayout->addWidget(loadDataFromCanvasSurveyButton, 0, Qt::AlignLeft);

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
    prevWorkComboBox = new QComboBox(this);
    QPushButton *loadPrevWorkButton = new QPushButton (this);
    loadPrevWorkButton->setIcon(QIcon(":/icons_new/upload.png"));
    loadPrevWorkButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    prevWorkComboBoxLayout->addWidget(prevWorkComboBox, 3);
    prevWorkComboBoxLayout->addWidget(loadPrevWorkButton, 1);
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

    dataSourceFrame = new QFrame(this);
    dataSourceLabel = new QPushButton(this);
    QHBoxLayout* dataSourceFrameLayout = new QHBoxLayout();
    dataSourceFrameLayout->addWidget(dataSourceLabel);
    dataSourceFrame->setLayout(dataSourceFrameLayout);
    dataSourceFrame->setStyleSheet("QFrame {background-color: " TROPICALHEX "; color: " DEEPWATERHEX "; border: none;}"
                                       "QFrame::disabled {background-color: lightGray; color: darkGray; border: none;}");
    dataSourceLabel->setStyleSheet(LABELONLYBUTTON);
    dataSourceFrame->setEnabled(false);
    dataSourceLabel->setEnabled(false);
    dataSourceLabel->setText("No data source loaded.");
    const QPixmap icon(":/icons_new/file.png");
    const int h = dataSourceLabel->height();
    dataSourceLabel->setIcon(icon.scaledToHeight(h, Qt::SmoothTransformation));

    confirmCancelButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Confirm"));
    confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);

    connect(dropCSVFileFrame, &DropCSVFrame::itemDropped, this, [this](const QString &filePathString) {
        source = DataOptions::DataSource::fromDragDropFile;
        loadData(filePathString);
    });

    connect(uploadButton, &QPushButton::clicked, this, [this](){
        source = DataOptions::DataSource::fromUploadFile;
        loadData("");
    });
    connect(loadDataFromGoogleFormButton, &QPushButton::clicked, this, [this](){
        source = DataOptions::DataSource::fromGoogle;
        loadData("");
    });
    connect(loadDataFromCanvasSurveyButton, &QPushButton::clicked, this, [this](){
        source = DataOptions::DataSource::fromCanvas;
        loadData("");
    });
    connect(loadPrevWorkButton, &QPushButton::clicked, this, [this](){
        source = DataOptions::DataSource::fromPrevWork;
        loadData("");
    });
    connect(confirmCancelButtonBox, &QDialogButtonBox::accepted, this, &loadDataDialog::accept);
    connect(confirmCancelButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(headerLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(dropCSVFileFrame);
    mainLayout->addStretch(1);
    mainLayout->addWidget(otherDataSourcesLabel, 0, Qt::AlignLeft);
    mainLayout->addLayout(otherDataSourcesLayout);
    mainLayout->addWidget(dataSourceFrame);
    mainLayout->addWidget(confirmCancelButtonBox);

    setLayout(mainLayout);
}

bool loadDataDialog::getFromDropFile(QString filePathString){
    const QPixmap icon(":/icons_new/file.png");
    QFileInfo fileInfo(filePathString);
    if (fileInfo.suffix().toLower() != "csv") {
        qDebug() << "Error: The dropped file is not a CSV file.";
        return false;
    }

    QSettings savedSettings;
    QFileInfo dataFileLocation;
    dataFileLocation.setFile(savedSettings.value("saveFileLocation", "").toString());

    try {
        if (!surveyFile->openExistingFile(filePathString)) {
            qDebug() << "Error: Failed to open CSV file.";
            return false;
        }
    } catch (const std::exception &e) {
        qDebug() << "Failed to open CSV File: " << e.what();
        return false;
    }

    savedSettings.setValue("saveFileLocation", surveyFile->fileInfo().canonicalFilePath());
    source = DataOptions::DataSource::fromDragDropFile;
    dataOptions->dataSource = source;
    dataOptions->dataSourceName = surveyFile->fileInfo().fileName();
    const int h = dataSourceLabel->height();
    dataSourceLabel->setIcon(icon.scaledToHeight(h, Qt::SmoothTransformation));
    return true;
}

void loadDataDialog::loadData(QString filePathString)
{
    if(surveyFile != nullptr) {
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        surveyFile->deleteLater();
    }
    delete dataOptions;

    bool fileLoaded = false;

    switch(source) {
    case DataOptions::DataSource::fromUploadFile:
        dataOptions = new DataOptions;
        surveyFile = new CsvFile;
        fileLoaded = getFromFile();
        break;
    case DataOptions::DataSource::fromDragDropFile:
        dataOptions = new DataOptions;
        surveyFile = new CsvFile;
        fileLoaded = getFromDropFile(filePathString);
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
        confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        confirmCancelButtonBox->button(QDialogButtonBox::Ok)->animateClick();
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

    dataSourceFrame->setEnabled(true);
    dataSourceLabel->setEnabled(true);
    dataSourceLabel->setText(tr("Data source: ") + dataOptions->dataSourceName);

    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

CsvFile* loadDataDialog::getSurveyFile(){
    return surveyFile;
}

bool loadDataDialog::readQuestionsFromHeader()
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
            if((source == DataOptions::DataSource::fromCanvas) && (headerVal.compare("id", Qt::CaseInsensitive) == 0)) {
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

    return true;
}

bool loadDataDialog::getFromFile()
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
    const int h = dataSourceLabel->height();
    dataSourceLabel->setIcon(icon.scaledToHeight(h, Qt::SmoothTransformation));
    return true;
}

bool loadDataDialog::getFromGoogle()
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
    const int h = dataSourceLabel->height();
    dataSourceLabel->setIcon(GoogleHandler::icon().scaledToHeight(h, Qt::SmoothTransformation));

    return true;
}

bool loadDataDialog::getFromCanvas()
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
    const int h = dataSourceLabel->height();
    dataSourceLabel->setIcon(CanvasHandler::icon().scaledToHeight(h, Qt::SmoothTransformation));

    return true;
}

bool loadDataDialog::getFromPrevWork()
{
    QFile savedFile(prevWorkComboBox->currentData().toString(), this);
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



void loadDataDialog::accept() {

    if((dataOptions->dataSource == DataOptions::DataSource::fromUploadFile) ||
       (dataOptions->dataSource == DataOptions::DataSource::fromDragDropFile) ||
       (false /* Checkbox to manually choose to select categories */)) {
        const QScopedPointer<CategorizingDialog> categorizingDataDialog(new CategorizingDialog(this, surveyFile, source));
        auto categorizing_result = categorizingDataDialog->exec();
        if (categorizing_result != QDialog::Accepted){
            return;
        } else if(!readData()){  //NEED THIS READDATA TO HAPPEN W/O CATEGORIZINGDATADIALOG WHEN SOURCE IS CANVAS OR GOOGLE
            confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            dataSourceFrame->setEnabled(false);
            dataSourceLabel->setEnabled(false);
            dataSourceLabel->setText(tr("No survey loaded"));
            students.clear();
            return;
        } else {
            QDialog::accept();
        }
    }
    QDialog::accept();
}

//reads data from csv file and assigns dataOption variables accordingly, dataOptions are fields that characterize the information from the data file and categorizing stage.
bool loadDataDialog::readData()
{
    auto *loadingProgressDialog = new QProgressDialog(tr("Loading data..."), tr("Cancel"), 0, surveyFile->estimatedNumberRows + MAX_ATTRIBUTES + 6,
                                                      this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    loadingProgressDialog->setAutoReset(false);
    connect(parent, &StartDialog::closeDataDialogProgressBar, loadingProgressDialog, &QProgressDialog::reset);
    loadingProgressDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    loadingProgressDialog->setMinimumDuration(0);
    loadingProgressDialog->setWindowModality(Qt::ApplicationModal);
    loadingProgressDialog->setStyleSheet(QString(LABEL10PTSTYLE) + PROGRESSBARSTYLE + SMALLBUTTONSTYLEINVERTED);

    // set field values now according to user's selection of field meanings (defaulting to FIELDNOTPRESENT (i.e., -1) if not chosen)
    dataOptions->timestampField = int(surveyFile->fieldMeanings.indexOf("Timestamp"));
    dataOptions->LMSIDField = int(surveyFile->fieldMeanings.indexOf("**LMSID**"));
    dataOptions->firstNameField = int(surveyFile->fieldMeanings.indexOf("First Name"));
    dataOptions->lastNameField = int(surveyFile->fieldMeanings.indexOf("Last Name"));
    dataOptions->emailField = int(surveyFile->fieldMeanings.indexOf("Email Address"));
    dataOptions->genderField = int(surveyFile->fieldMeanings.indexOf("Gender"));
    dataOptions->gradeField = int(surveyFile->fieldMeanings.indexOf("Grade"));
    dataOptions->gradeIncluded = (dataOptions->gradeField != DataOptions::FIELDNOTPRESENT);
    dataOptions->genderIncluded = (dataOptions->genderField != DataOptions::FIELDNOTPRESENT);
    dataOptions->URMField = int(surveyFile->fieldMeanings.indexOf("Racial/ethnic identity"));
    dataOptions->URMIncluded = (dataOptions->URMField != DataOptions::FIELDNOTPRESENT);
    dataOptions->sectionField = int(surveyFile->fieldMeanings.indexOf("Section"));
    dataOptions->sectionIncluded = (dataOptions->sectionField != DataOptions::FIELDNOTPRESENT);
    dataOptions->timezoneField = int(surveyFile->fieldMeanings.indexOf("Timezone"));
    dataOptions->timezoneIncluded = (dataOptions->timezoneField != DataOptions::FIELDNOTPRESENT);

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
        currStudent.parseRecordFromStringList(surveyFile->fieldValues, *dataOptions); //copy survey file fieldValue onto studentRecord
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
                currStudent.grade = studentOnRoster.grade;
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

        for(auto gender : student.gender){
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
