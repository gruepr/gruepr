#include "getGrueprDataDialog.h"
#include "ui_getGrueprDataDialog.h"
#include "gruepr_globals.h"
#include "dialogs/baseTimeZoneDialog.h"
#include <QComboBox>
#include <QPainter>
#include <QSettings>
#include <QStandardItemModel>

GetGrueprDataDialog::GetGrueprDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GetGrueprDataDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("gruepr - Form teams"));

    ui->sourceFrame->setStyleSheet(QString() + "QFrame {background-color: " OPENWATERHEX "; color: white; padding: 10px; border: none;}" +
                                   QString(RADIOBUTTONSTYLE).replace("font-size: 10pt;", "font-size: 12pt; color: white;"));
    ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: " OPENWATERHEX "; color: white; font-family:'DM Sans'; font-size: 12pt; "
                                          "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; padding: 10px;}");
    ui->sourceButtonGroup->setId(ui->fromFileRadioButton, fromFile);
    ui->sourceButtonGroup->setId(ui->fromGoogleradioButton, fromGoogle);
    ui->sourceButtonGroup->setId(ui->fromCanvasradioButton, fromCanvas);
    ui->loadDataPushButton->adjustSize();
    QPixmap whiteUploadIcon(":/icons_new/upload_file.png");
    QPainter painter(&whiteUploadIcon);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteUploadIcon.rect(), QColor("white"));
    painter.end();
    int h = ui->loadDataPushButton->height();
    ui->loadDataPushButton->setIcon(whiteUploadIcon.scaledToHeight(h, Qt::SmoothTransformation));
    connect(ui->loadDataPushButton, &QPushButton::clicked, this, &GetGrueprDataDialog::loadData);

    ui->dataSourceFrame->setStyleSheet(QString() + "QFrame {background-color: " + (QColor::fromString(QString(STARFISHHEX)).lighter(133).name()) + "; color: " DEEPWATERHEX "; "
                                                           "border: none;}"
                                                   "QFrame::disabled {background-color: lightGray; color: darkGray; border: none;}");
    ui->dataSourceLabel->setStyleSheet("QLabel {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt;}"
                                       "QLabel::disabled {background-color: " TRANSPARENT "; color: darkGray; font-family:'DM Sans'; font-size: 12pt;}");
    ui->dataSourceLabel->adjustSize();
    QPixmap fileIcon(":/icons_new/file.png");
    h = ui->dataSourceLabel->height();
    ui->dataSourceIcon->setPixmap(fileIcon.scaledToHeight(h, Qt::SmoothTransformation));

    ui->hLine->setStyleSheet("QFrame {color: " OPENWATERHEX ";}"
                             "QFrame::disabled {color: lightGray;}");
    ui->fieldsExplainer->setStyleSheet(LABELSTYLE);
    ui->headerRowCheckBox->setStyleSheet("QCheckBox {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt;}"
                                         "QCheckBox::disabled {color: darkGray; font-family: 'DM Sans'; font-size: 12pt;}");
    connect(ui->headerRowCheckBox, &QCheckBox::clicked, this, [this]{if(ui->headerRowCheckBox->isChecked())
                                                                        {ui->tableWidget->setHorizontalHeaderLabels({HEADERTEXT, CATEGORYTEXT});}
                                                                     else
                                                                        {ui->tableWidget->setHorizontalHeaderLabels({ROW1TEXT, CATEGORYTEXT});}});
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: lightGray; color: darkGray; border: 1px solid black; padding: 5px; "
                                                                             "font-family: DM Sans; font-size: 12pt;}");
    ui->tableWidget->setHorizontalHeaderLabels({HEADERTEXT, CATEGORYTEXT});
    ui->tableWidget->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableWidget->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Confirm"));
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
}

GetGrueprDataDialog::~GetGrueprDataDialog()
{
    delete ui;
}


void GetGrueprDataDialog::loadData()
{
    delete dataOptions;
    dataOptions = new DataOptions;

    bool fileLoaded = false;
    switch(ui->sourceButtonGroup->checkedId()) {
    case fromFile:
        fileLoaded = getFromFile();
        break;
    case fromGoogle:
        fileLoaded = getFromGoogle();
        break;
    case fromCanvas:
        fileLoaded = getFromCanvas();
        break;
    }

    if(!fileLoaded) {
        return;
    }

    dataOptions->dataFile = surveyFile.fileInfo();

    if(!readQuestionsFromHeader()) {
        return;
    }


    ui->sourceFrame->setStyleSheet(QString() + "QFrame {background-color: white; color: " DEEPWATERHEX "; padding: 10px; border: none;}" +
                                   RADIOBUTTONSTYLE);
    ui->loadDataPushButton->setStyleSheet("QPushButton {background-color: white; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt; "
                                          "border-style: solid; border-width: 2px; border-radius: 5px; border-color: " DEEPWATERHEX "; padding: 10px;}");
    QPixmap uploadIcon(":/icons_new/upload_file.png");
    int h = ui->loadDataPushButton->height();
    ui->loadDataPushButton->setIcon(uploadIcon.scaledToHeight(h, Qt::SmoothTransformation));
    ui->dataSourceFrame->setEnabled(true);
    ui->dataSourceIcon->setEnabled(true);
    ui->dataSourceLabel->setEnabled(true);
    ui->dataSourceLabel->setText(tr("Current data source: ") + dataOptions->dataSource);
    ui->hLine->setEnabled(true);
    ui->fieldsExplainer->setEnabled(true);
    ui->headerRowCheckBox->setEnabled(true);
    ui->tableWidget->setEnabled(true);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: " OPENWATERHEX "; color: white;  border: 1px solid black; padding: 5px; "
                                                                             "font-family: DM Sans; font-size: 12pt;}");
    ui->tableWidget->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border-color: " OPENWATERHEX ";}"
                                   "QTableView::item{border-top: 1px solid " DEEPWATERHEX "; border-bottom: 1px solid " DEEPWATERHEX "; padding: 3px;}" +
                                   QString(SCROLLBARSTYLE).replace(DEEPWATERHEX, OPENWATERHEX));

    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    connect(this, &QDialog::accepted, this, &GetGrueprDataDialog::readData);
}

bool GetGrueprDataDialog::getFromFile()
{
    QSettings savedSettings;
    QFileInfo dataFile;
    dataFile.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

    if(!surveyFile.open(this, CsvFile::read, tr("Open Survey Data File"), dataFile.canonicalFilePath(), tr("Survey Data")))
    {
        return false;
    }

    isTempFile = false;
    dataOptions->dataSource = surveyFile.fileInfo().fileName();
    return true;
}

bool GetGrueprDataDialog::getFromGoogle()
{
    if(!internetIsGood())
    {
        return false;
    }

    //create googleHandler and/or authenticate as needed
    if(google == nullptr)
    {
        google = new GoogleHandler();
    }
    if(!google->authenticated)
    {
        auto *loginDialog = new QMessageBox(this);
        QPixmap icon(":/icons/google.png");
        loginDialog->setIconPixmap(icon.scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE));
        loginDialog->setText("");

        // if refreshToken is found, try to use it to get accessTokens without re-granting permission
        if(google->refreshTokenExists)
        {
            loginDialog->setText(tr("Contacting Google..."));
            loginDialog->setStandardButtons(QMessageBox::Cancel);
            connect(google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
            connect(google, &GoogleHandler::denied, loginDialog, [&loginDialog]() {loginDialog->setText(tr("Google is requesting that you re-authorize gruepr.\n\n"));
                                                                                   loginDialog->accept();});

            google->authenticate();

            if(loginDialog->exec() == QMessageBox::Cancel)
            {
                delete loginDialog;
                return false;
            }

            //refreshToken failed, so need to start over
            if(!google->authenticated)
            {
                delete google;
                google = new GoogleHandler();
            }
        }

        // still not authenticated, so either didn't have a refreshToken to use or the refreshToken didn't work; need to re-log in on the browser
        if(!google->authenticated)
        {
            loginDialog->setText(loginDialog->text() + tr("The next step will open a browser window so you can sign in with Google.\n\n"
                                                          "  » Your computer may ask whether gruepr can access the network. "
                                                          "This access is needed so that gruepr and Google can communicate.\n\n"
                                                          "  » In the browser, Google will ask whether you authorize gruepr to access the files gruepr created on your Google Drive. "
                                                          "This access is needed so that the survey responses can now be downloaded.\n\n"
                                                          "  » All data associated with this survey, including the questions asked and responses received, exist in your Google Drive only. "
                                                          "No data from or about this survey will ever be stored or sent anywhere else."));
            loginDialog->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
            loginDialog->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
            auto *okButton = loginDialog->button(QMessageBox::Ok);
            auto *cancelButton = loginDialog->button(QMessageBox::Cancel);
            int height = okButton->height();
            QPixmap loginpic(":/icons_new/google_signin_button.png");
            loginpic = loginpic.scaledToHeight(int(1.5f * float(height)), Qt::SmoothTransformation);
            okButton->setText("");
            okButton->setIconSize(loginpic.rect().size());
            okButton->setIcon(loginpic);
            okButton->adjustSize();
            QPixmap cancelpic(":/icons/cancel_signin_button.png");
            cancelpic = cancelpic.scaledToHeight(int(1.5f * float(height)), Qt::SmoothTransformation);
            cancelButton->setText("");
            cancelButton->setIconSize(cancelpic.rect().size());
            cancelButton->setIcon(cancelpic);
            cancelButton->adjustSize();
            if(loginDialog->exec() == QMessageBox::Cancel)
            {
                delete loginDialog;
                return false;
            }

            google->authenticate();

            loginDialog->setText(tr("Please use your browser to log in to Google and then return here."));
            loginDialog->setStandardButtons(QMessageBox::Cancel);
            connect(google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
            if(loginDialog->exec() == QMessageBox::Cancel)
            {
                delete loginDialog;
                return false;
            }
        }
        delete loginDialog;
    }

    //ask which survey to download
    QStringList formsList = google->getSurveyList();
    auto *googleFormsDialog = new QDialog(this);
    googleFormsDialog->setWindowTitle(tr("Choose Google survey"));
    googleFormsDialog->setWindowIcon(QIcon(":/icons/google.png"));
    googleFormsDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    auto *label = new QLabel(tr("Which survey should be opened?"));
    auto *formsComboBox = new QComboBox;
    for(const auto &form : qAsConst(formsList))
    {
        formsComboBox->addItem(form);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vLayout->addWidget(label);
    vLayout->addWidget(formsComboBox);
    vLayout->addWidget(buttonBox);
    googleFormsDialog->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, googleFormsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, googleFormsDialog, &QDialog::reject);
    if((googleFormsDialog->exec() == QDialog::Rejected))
    {
        delete googleFormsDialog;
        return false;
    }
    const QString googleFormName = formsComboBox->currentText();
    delete googleFormsDialog;

    //download the survey
    auto *busyBox = google->busy();
    QString filepath = google->downloadSurveyResult(googleFormName);
    QPixmap icon;
    QSize iconSize = busyBox->iconPixmap().size();
    QEventLoop loop;
    if(filepath.isEmpty())
    {
        busyBox->setText(tr("Error. Survey not downloaded."));
        icon.load(":/icons/delete.png");
        busyBox->setIconPixmap(icon.scaled(iconSize));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        google->notBusy(busyBox);
        return false;
    }
    busyBox->setText(tr("Success!"));
    icon.load(":/icons/ok.png");
    busyBox->setIconPixmap(icon.scaled(iconSize));
    QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
    loop.exec();
    google->notBusy(busyBox);

    //open the downloaded file
    if(!surveyFile.openExistingFile(filepath))
    {
        return false;
    }

    isTempFile = true;
    dataOptions->dataSource = googleFormName;
    return true;
}

bool GetGrueprDataDialog::getFromCanvas()
{
    if(!internetIsGood())
    {
        return false;
    }

    //create canvasHandler and/or authenticate as needed
    if(canvas == nullptr)
    {
        canvas = new CanvasHandler();
    }
    if(!canvas->authenticated)
    {
        //IN BETA--GETS USER'S API TOKEM MANUALLY
        QSettings savedSettings;
        QString savedCanvasURL = savedSettings.value("canvasURL").toString();
        QString savedCanvasToken = savedSettings.value("canvasToken").toString();

        QStringList newURLAndToken = canvas->askUserForManualToken(savedCanvasURL, savedCanvasToken);
        if(newURLAndToken.isEmpty())
        {
            return false;
        }

        savedCanvasURL = (newURLAndToken.at(0).isEmpty() ? savedCanvasURL : newURLAndToken.at(0));
        savedCanvasToken =  (newURLAndToken.at(1).isEmpty() ? savedCanvasToken : newURLAndToken.at(1));
        savedSettings.setValue("canvasURL", savedCanvasURL);
        savedSettings.setValue("canvasToken", savedCanvasToken);

        canvas->setBaseURL(savedCanvasURL);
        canvas->authenticate(savedCanvasToken);
    }

    //ask the user from which course we're downloading the survey
    auto *busyBox = canvas->busy();
    QStringList courseNames = canvas->getCourses();
    canvas->notBusy(busyBox);

    auto *canvasCoursesAndQuizzesDialog = new QDialog(this);
    canvasCoursesAndQuizzesDialog->setWindowTitle(tr("Choose Canvas course"));
    canvasCoursesAndQuizzesDialog->setWindowIcon(QIcon(":/icons/canvas.png"));
    canvasCoursesAndQuizzesDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    int i = 1;
    auto *label = new QLabel(tr("From which course should the survey be downloaded?"));
    auto *coursesAndQuizzesComboBox = new QComboBox;
    for(const auto &courseName : qAsConst(courseNames))
    {
        coursesAndQuizzesComboBox->addItem(courseName);
        coursesAndQuizzesComboBox->setItemData(i++, QString::number(canvas->getStudentCount(courseName)) + " students", Qt::ToolTipRole);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesAndQuizzesComboBox);
    vLayout->addWidget(buttonBox);
    canvasCoursesAndQuizzesDialog->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, canvasCoursesAndQuizzesDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, canvasCoursesAndQuizzesDialog, &QDialog::reject);
    if((canvasCoursesAndQuizzesDialog->exec() == QDialog::Rejected))
    {
        delete canvasCoursesAndQuizzesDialog;
        return false;
    }
    const QString course = coursesAndQuizzesComboBox->currentText();
    canvasCoursesAndQuizzesDialog->hide();

    //ask which survey (canvas Quiz) to download
    busyBox = canvas->busy();
    QStringList formsList = canvas->getQuizList(course);
    canvas->notBusy(busyBox);

    canvasCoursesAndQuizzesDialog->setWindowTitle(tr("Choose Canvas quiz"));
    label->setText(tr("Which survey should be downloaded?"));
    coursesAndQuizzesComboBox->clear();
    for(const auto &form : qAsConst(formsList))
    {
        coursesAndQuizzesComboBox->addItem(form);
    }
    if((canvasCoursesAndQuizzesDialog->exec() == QDialog::Rejected))
    {
        delete canvasCoursesAndQuizzesDialog;
        return false;
    }
    const QString canvasSurveyName = coursesAndQuizzesComboBox->currentText();
    delete canvasCoursesAndQuizzesDialog;

    //download the survey
    busyBox = canvas->busy();
    QString filepath = canvas->downloadQuizResult(course, canvasSurveyName);
    QPixmap icon;
    QSize iconSize = busyBox->iconPixmap().size();
    QEventLoop loop;
    if(filepath.isEmpty())
    {
        busyBox->setText(tr("Error. Survey not received."));
        icon.load(":/icons/delete.png");
        busyBox->setIconPixmap(icon.scaled(iconSize));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->notBusy(busyBox);
        return false;
    }
    busyBox->setText(tr("Success!"));
    icon.load(":/icons/ok.png");
    busyBox->setIconPixmap(icon.scaled(iconSize));
    QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
    loop.exec();
    canvas->notBusy(busyBox);

    //open the downloaded file
    if(!surveyFile.openExistingFile(filepath))
    {
        return false;
    }

    // Only include the timestamp question ("submitted") and then the questions we've asked, which will all begin with (possibly a quotation mark then) an integer then a colon then a space.
    // (Also ignore the text "question" which serves as the text notifier that several schedule questions are coming up).
    surveyFile.fieldsToBeIgnored = QStringList{R"(^(?!(submitted)|("?\d+: .*)).*$)", ".*" + CanvasHandler::SCHEDULEQUESTIONINTRO2.trimmed() + ".*"};

    isTempFile = true;
    dataOptions->dataSource = canvasSurveyName;
    return true;
}

bool GetGrueprDataDialog::readQuestionsFromHeader()
{
    if(!surveyFile.readHeader())
    {
        // header row could not be read as valid data
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        surveyFile.close(isTempFile);
        return false;
    }

    if(surveyFile.headerValues.size() < 2)
    {
        QMessageBox::critical(this, tr("File error."), tr("This file is empty or there is an error in its format."), QMessageBox::Ok);
        surveyFile.close(isTempFile);
        return false;
    }

    // See if there are header fields after any of (preferred teammates / non-teammates, section, or schedule) since those are probably notes fields
    auto lastKnownMeaningfulField = QRegularExpression("(.*(like to not have on your team).*)|(.*(want to avoid working with).*)|"
                                                       "(.*(like to have on your team).*)|(.*(want to work with).*)|"
                                                       ".*(which section are you enrolled).*|(.*(check).+(times).*)", QRegularExpression::CaseInsensitiveOption);
    int notesFieldsProbBeginAt = 1 + int(surveyFile.headerValues.lastIndexOf(lastKnownMeaningfulField));
    if((notesFieldsProbBeginAt != 0) && (notesFieldsProbBeginAt != surveyFile.headerValues.size()))
    {
        //if notesFieldsProbBeginAt == 0 then none of these questions exist, so assume no notes because list ends with attributes
        //and if notesFieldsProbBeginAt == headervalues size, also assume no notes because list ends with one of these questions
        for(int field = notesFieldsProbBeginAt; field < surveyFile.fieldMeanings.size(); field++)
        {
            surveyFile.fieldMeanings[field] = "Notes";
        }
    }

    // Ask user what the columns mean
    QList<possFieldMeaning> surveyFieldOptions = {{"Timestamp", "(timestamp)|(^submitted$)", 1}, {"First Name", "((first)|(given)|(preferred))(?!.*last).*(name)", 1},
                                                  {"Last Name", "^(?!.*first).*((last)|(sur)|(family)).*(name)", 1}, {"Email Address", "(e).*(mail)", 1},
                                                  {"Gender", "((gender)|(pronouns))", 1}, {"Racial/ethnic identity", "((minority)|(ethnic))", 1},
                                                  {"Schedule", "(check).+(times)", MAX_DAYS}, {"Section", "which section are you enrolled", 1},
                                                  {"Timezone","(time zone)", 1}, {"Preferred Teammates", "(like to have on your team)|(want to work with)", MAX_PREFTEAMMATES},
                                                  {"Preferred Non-teammates", "(like to not have on your team)|(want to avoid working with)", MAX_PREFTEAMMATES},
                                                  {"Multiple Choice", ".*", MAX_ATTRIBUTES}, {"Notes", "", MAX_NOTES_FIELDS}};
    // see if each field is a value to be ignored; if not and the fieldMeaning is empty, preload with possibleFieldMeaning based on matches to the patterns
    for(int i = 0; i < surveyFile.numFields; i++)
    {
        const QString &headerVal = surveyFile.headerValues.at(i);

        bool ignore = false;
        for(const auto &matchpattern : qAsConst(surveyFile.fieldsToBeIgnored))
        {
            if(headerVal.contains(QRegularExpression(matchpattern, QRegularExpression::CaseInsensitiveOption)))
            {
                surveyFile.fieldMeanings[i] = "**IGNORE**";
                ignore = true;
            }
        }

        if(!ignore && surveyFile.fieldMeanings.at(i).isEmpty())
        {
            int matchPattern = 0;
            QString match;
            do
            {
                match = surveyFieldOptions.at(matchPattern).regExSearchString;
                matchPattern++;
            }
            while((matchPattern < surveyFieldOptions.size()) &&
                   !headerVal.contains(QRegularExpression(match, QRegularExpression::CaseInsensitiveOption)));
            if(matchPattern != surveyFieldOptions.size())
            {
                surveyFile.fieldMeanings[i] = surveyFieldOptions.at(matchPattern - 1).nameShownToUser;
            }
            else
            {
                surveyFile.fieldMeanings[i] = UNUSEDTEXT;
            }
        }
    }

    ui->tableWidget->setRowCount(surveyFile.numFields);
    // a label and combobox for each column
    for(int row = 0; row < surveyFile.numFields; row++)
    {
        auto *label = new QLabel("\n" + surveyFile.headerValues.at(row) + "\n");
        label->setStyleSheet(LABELSTYLE);
        label->setWordWrap(true);
        ui->tableWidget->setCellWidget(row, 0, label);

        auto *selector = new QComboBox;
        selector->setStyleSheet(COMBOBOXSTYLE);
        selector->setFocusPolicy(Qt::StrongFocus);  // remove scrollwheel from affecting the value,
        selector->installEventFilter(new MouseWheelBlocker(selector)); // as it's too easy to mistake scrolling through the rows with changing the value
        for(const auto &meaning : qAsConst(surveyFieldOptions))
        {
            selector->addItem(meaning.nameShownToUser, meaning.maxNumOfFields);
        }
        selector->insertItem(0, UNUSEDTEXT);
        auto *model = qobject_cast<QStandardItemModel *>(selector->model());
        model->item(0)->setForeground(Qt::darkRed);
        selector->insertSeparator(1);
        if(surveyFile.fieldMeanings.at(row) == "**IGNORE**")
        {
            selector->setCurrentText(UNUSEDTEXT);
            surveyFile.fieldMeanings[row] = UNUSEDTEXT;
            ui->tableWidget->hideRow(row);
        }
        else
        {
            selector->setCurrentText(surveyFile.fieldMeanings.at(row));
        }
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        int width = selector->minimumSizeHint().width();
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
    QList<int> rows(surveyFile.numFields);
    std::iota(rows.begin(), rows.end(), 0);
    if(callingRow != -1)
    {
        rows.remove(callingRow);
        rows.prepend(callingRow);
    }

    // start by counting all the values to count how many times each are used, and which are fully used
    std::map<QString, int> takenValues;     // mapping fieldMeaning -> number of fields selected with this meaning
    std::map<QString, int> fullyUsedValues; // mapping the same, but saving how many extra fields with this meaning
    for(auto row : rows)
    {
        // get the selected fieldMeaning
        const auto *box = qobject_cast<QComboBox *>(ui->tableWidget->cellWidget(row, 1));
        QString selection = box->currentText();

        // set it in the CsvFile's data
        surveyFile.fieldMeanings[row] = selection;

        // add this occurence in the takenValues mapping
        if(takenValues.count(selection) == 0)
        {
            // first ocurrence of this field; create the key/value
            takenValues[selection] = 1;
        }
        else
        {
            // key already exists
            takenValues[selection]++;
        }

        // if we are at or above the allowed number of ocurrences, note it
        if(takenValues[selection] >= box->currentData().toInt())
        {
            // add this occurence in the takenValues mapping
            if(fullyUsedValues.count(selection) == 0)
            {
                // field has just reached capacity; create the key/value
                fullyUsedValues[selection] = 1;
            }
            else
            {
                // key already exists; we have MORE than are allowed
                fullyUsedValues[selection]++;
            }
        }
        else
        {
            fullyUsedValues[selection] = 0;
        }
    }

    // Now go back through in reverse order and:
    //  1) replacing overused values with "Unused",
    //  2) setting fully used values in other boxes to red with a tooltip,
    //  3) clearing formatting of all non-overused values (except "Unused") and the fully used values that are currently chosen.
    // Then:
    //  4) clearing formatting of all items unchosen in any box (except "Unused").
    for(auto row = rows.rbegin(); row != rows.rend(); ++row)
    {
        auto *box = qobject_cast<QComboBox *>(ui->tableWidget->cellWidget(*row, 1));
        box->blockSignals(true);
        auto *model = qobject_cast<QStandardItemModel *>(box->model());
        for(auto &takenValue : takenValues)
        {
            QString fieldval = takenValue.first;
            int numAllowed = box->itemData(box->findText(fieldval)).toInt();
            QStandardItem *item = model->item(box->findText(fieldval));
            if((fullyUsedValues[fieldval] > 1) && (box->currentText() == fieldval))
            {
                // number exceeds max. allowed somehow, so set to unused
                box->setCurrentText(UNUSEDTEXT);
                surveyFile.fieldMeanings[*row] = UNUSEDTEXT;
                fullyUsedValues[fieldval]--;
                if(numAllowed == 1)
                {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else
                {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if((fullyUsedValues[fieldval] == 1) && (box->currentText() != fieldval))
            {
                // at capacity, and not selected in this box
                item->setForeground(Qt::darkRed);
                if(numAllowed == 1)
                {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else
                {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if(fieldval != UNUSEDTEXT)
            {
                // below capacity or at capacity including this one
                item->setForeground(Qt::black);
                item->setToolTip("");
            }
        }

        // clearing formatting of all unchosen items except "Unused"
        for(int itemNum = 0; itemNum < box->count(); itemNum++)
        {
            if((takenValues.count(box->itemText(itemNum)) == 0) && (box->itemText(itemNum) != UNUSEDTEXT))
            {
                model->item(itemNum)->setForeground(Qt::black);
                model->item(itemNum)->setToolTip("");
            }
        }
        box->blockSignals(false);
    }
}

bool GetGrueprDataDialog::readData()
{
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    // set field values now according to user's selection of field meanings (defaulting to -1 if not chosen)
    dataOptions->timestampField = int(surveyFile.fieldMeanings.indexOf("Timestamp"));
    dataOptions->firstNameField = int(surveyFile.fieldMeanings.indexOf("First Name"));
    dataOptions->lastNameField = int(surveyFile.fieldMeanings.indexOf("Last Name"));
    dataOptions->emailField = int(surveyFile.fieldMeanings.indexOf("Email Address"));
    dataOptions->genderField = int(surveyFile.fieldMeanings.indexOf("Gender"));
    dataOptions->genderIncluded = (dataOptions->genderField != -1);
    dataOptions->URMField = int(surveyFile.fieldMeanings.indexOf("Racial/ethnic identity"));
    dataOptions->URMIncluded = (dataOptions->URMField != -1);
    dataOptions->sectionField = int(surveyFile.fieldMeanings.indexOf("Section"));
    dataOptions->sectionIncluded = (dataOptions->sectionField != -1);
    dataOptions->timezoneField = int(surveyFile.fieldMeanings.indexOf("Timezone"));
    dataOptions->timezoneIncluded = (dataOptions->timezoneField != -1);
    dataOptions->prefTeammatesField = int(surveyFile.fieldMeanings.indexOf("Preferred Teammates"));
    dataOptions->prefTeammatesIncluded = (dataOptions->prefTeammatesField != -1);
    dataOptions->prefNonTeammatesField = int(surveyFile.fieldMeanings.indexOf("Preferred Non-teammates"));
    dataOptions->prefNonTeammatesIncluded = (dataOptions->prefNonTeammatesField != -1);
    // notes fields
    int lastFoundIndex = 0;
    dataOptions->numNotes = int(surveyFile.fieldMeanings.count("Notes"));
    for(int note = 0; note < dataOptions->numNotes; note++)
    {
        dataOptions->notesField[note] = int(surveyFile.fieldMeanings.indexOf("Notes", lastFoundIndex));
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile.fieldMeanings.indexOf("Notes", lastFoundIndex)));
    }
    // attribute fields, adding timezone field as an attribute if it exists
    lastFoundIndex = 0;
    dataOptions->numAttributes = int(surveyFile.fieldMeanings.count("Multiple Choice"));
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        dataOptions->attributeField[attribute] = int(surveyFile.fieldMeanings.indexOf("Multiple Choice", lastFoundIndex));
        dataOptions->attributeQuestionText << surveyFile.headerValues.at(dataOptions->attributeField[attribute]);
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile.fieldMeanings.indexOf("Multiple Choice", lastFoundIndex)));
    }
    if(dataOptions->timezoneIncluded)
    {
        dataOptions->attributeField[dataOptions->numAttributes] = dataOptions->timezoneField;
        dataOptions->attributeQuestionText << surveyFile.headerValues.at(dataOptions->timezoneField);
        dataOptions->numAttributes++;
    }
    // schedule fields
    lastFoundIndex = 0;
    for(int scheduleQuestion = 0, numScheduleFields = int(surveyFile.fieldMeanings.count("Schedule")); scheduleQuestion < numScheduleFields; scheduleQuestion++)
    {
        dataOptions->scheduleField[scheduleQuestion] = int(surveyFile.fieldMeanings.indexOf("Schedule", lastFoundIndex));
        QString scheduleQuestionText = surveyFile.headerValues.at(dataOptions->scheduleField[scheduleQuestion]);
        if(scheduleQuestionText.contains(QRegularExpression(".+\\b(free|available)\\b.+", QRegularExpression::CaseInsensitiveOption)))
        {
            // if >=1 field has this language, all interpreted as free time
            dataOptions->scheduleDataIsFreetime = true;
        }
        if(scheduleQuestionText.contains(QRegularExpression(".+\\b(your home)\\b.+", QRegularExpression::CaseInsensitiveOption)))
        {
            // if >=1 field has this language, all interpreted as referring to each student's home timezone
            dataOptions->homeTimezoneUsed = true;
        }
        QRegularExpression dayNameFinder("\\[([^[]*)\\]");   // Day name is in brackets at end of field (where Google Forms puts column titles in matrix questions)
        QRegularExpressionMatch dayName = dayNameFinder.match(scheduleQuestionText);
        if(dayName.hasMatch())
        {
            dataOptions->dayNames << dayName.captured(1);
        }
        else
        {
            dataOptions->dayNames << " " + QString::number(scheduleQuestion+1) + " ";
        }
        lastFoundIndex = std::max(lastFoundIndex, 1 + int(surveyFile.fieldMeanings.indexOf("Schedule", lastFoundIndex)));
    }

    // read one line of data; if no data after header row then file is invalid
    if(!surveyFile.readDataRow())
    {
        QMessageBox::critical(this, tr("Insufficient number of students."),
                              tr("There are no survey responses in this file."), QMessageBox::Ok);
        surveyFile.close(isTempFile);
        return false;
    }

    // If there is schedule info, read through the schedule fields in all of the responses to compile a list of time names, save as dataOptions->TimeNames
    if(!dataOptions->dayNames.isEmpty())
    {
        QStringList allTimeNames;
        do
        {
            for(int scheduleQuestion = 0, numScheduleQuestions = int(surveyFile.fieldMeanings.count("Schedule")); scheduleQuestion < numScheduleQuestions; scheduleQuestion++)
            {
                QString scheduleFieldText = QString(surveyFile.fieldValues.at(dataOptions->scheduleField[scheduleQuestion]).toUtf8()).toLower().split(';').join(',');
                QTextStream scheduleFieldStream(&scheduleFieldText);
                allTimeNames << CsvFile::getLine(scheduleFieldStream);
            }
        }
        while(surveyFile.readDataRow());
        allTimeNames.removeDuplicates();
        allTimeNames.removeOne("");
        //sort allTimeNames smartly, using mapped string -> hour of day integer; any timeName not found is put at the beginning of the list
        QStringList timeNamesStrings = QString(TIME_NAMES).split(",");
        std::sort(allTimeNames.begin(), allTimeNames.end(), [&timeNamesStrings] (const QString &a, const QString &b)
                  {return TIME_MEANINGS[std::max(0,int(timeNamesStrings.indexOf(a)))] < TIME_MEANINGS[std::max(0,int(timeNamesStrings.indexOf(b)))];});
        dataOptions->timeNames = allTimeNames;
        //pad the timeNames to include all 24 hours if we will be time-shifting student responses based on their home timezones later
        if(dataOptions->homeTimezoneUsed)
        {
            dataOptions->earlyHourAsked = TIME_MEANINGS[std::max(0,int(timeNamesStrings.indexOf(dataOptions->timeNames.constFirst())))];
            dataOptions->lateHourAsked = TIME_MEANINGS[std::max(0,int(timeNamesStrings.indexOf(dataOptions->timeNames.constLast())))];
            const int offsetToTimeNamesUsed = std::min(NUMOFTIMENAMESPERHOUR, std::max(0,
                                                                                 int(timeNamesStrings.indexOf(dataOptions->timeNames.constFirst())) -
                                                                                 static_cast<int>(std::distance(TIME_MEANINGS,
                                                                                 std::find(std::cbegin(TIME_MEANINGS), std::cend(TIME_MEANINGS), dataOptions->earlyHourAsked)))));
            for(int hour = 0; hour < MAX_BLOCKS_PER_DAY; hour++)
            {
                if((dataOptions->timeNames.size() > hour) && (TIME_MEANINGS[std::max(0,int(timeNamesStrings.indexOf(dataOptions->timeNames[hour])))] == hour))
                {
                    continue;
                }
                const int *val = std::find(std::begin(TIME_MEANINGS), std::end(TIME_MEANINGS), hour);
                const int index = int(std::distance(TIME_MEANINGS, val) + offsetToTimeNamesUsed);
                dataOptions->timeNames.insert(hour, timeNamesStrings.at(index));
            }

            // Ask what should be used as the base timezone to which schedules will all be adjusted
            auto *window = new baseTimezoneDialog(this);
            window->exec();
            dataOptions->baseTimezone = window->baseTimezoneVal;
            window->deleteLater();
        }
    }

    // Having read the header row and determined time names, if any, read each remaining row as a student record
    surveyFile.readDataRow(true);    // put cursor back to beginning and read first row
    if(surveyFile.hasHeaderRow)
    {
        // that first row was headers, so get next row
        surveyFile.readDataRow();
    }

    int numStudents = 0;            // counter for the number of records in the file; used to set the number of students to be teamed for the rest of the program
    do
    {
        students << StudentRecord();
        students.last().parseRecordFromStringList(surveyFile.fieldValues, dataOptions);
        students.last().ID = dataOptions->latestStudentID;
        dataOptions->latestStudentID++;

        // see if this record is a duplicate; assume it isn't and then check
        students.last().duplicateRecord = false;
        for(int index = 0; index < numStudents; index++)
        {
            if((students.last().firstname + students.last().lastname == students[index].firstname + students[index].lastname) ||
                ((students.last().email == students[index].email) && !students.last().email.isEmpty()))
            {
                students.last().duplicateRecord = true;
                students[index].duplicateRecord = true;
            }
        }

        // Figure out what type of gender data was given (if any) -- initialized value is GenderType::adult, and we're checking each student
        // because some values are ambiguous to GenderType (e.g. "nonbinary")
        if(dataOptions->genderIncluded)
        {
            QString genderText = surveyFile.fieldValues.at(dataOptions->genderField).toUtf8();
            if(genderText.contains(tr("male"), Qt::CaseInsensitive))    // contains "male" also picks up "female"
            {
                dataOptions->genderType = GenderType::biol;
            }
            else if(genderText.contains(tr("man"), Qt::CaseInsensitive))    // contains "man" also picks up "woman"
            {
                dataOptions->genderType = GenderType::adult;
            }
            else if((genderText.contains(tr("girl"), Qt::CaseInsensitive)) || (genderText.contains("boy", Qt::CaseInsensitive)))
            {
                dataOptions->genderType = GenderType::child;
            }
            else if(genderText.contains(tr("he"), Qt::CaseInsensitive))    // contains "he" also picks up "she" and "they"
            {
                dataOptions->genderType = GenderType::pronoun;
            }
        }

        numStudents++;
    }
    while(surveyFile.readDataRow() && numStudents < MAX_STUDENTS);
    dataOptions->numStudentsInSystem = numStudents;

    if(numStudents == MAX_STUDENTS)
    {
        QMessageBox::warning(this, tr("Reached maximum number of students."),
                             tr("The maximum number of students have been read from the file."
                                " This version of gruepr does not allow more than ") + QString::number(MAX_STUDENTS) + ".", QMessageBox::Ok);
    }

    // Set the attribute question options and numerical values for each student
    for(int attribute = 0; attribute < MAX_ATTRIBUTES; attribute++)
    {
        if(dataOptions->attributeField[attribute] != -1)
        {
            auto &responses = dataOptions->attributeQuestionResponses[attribute];
            auto &attributeType = dataOptions->attributeType[attribute];
            // gather all unique attribute question responses, then remove a blank response if it exists in a list with other responses
            for(int index = 0; index < dataOptions->numStudentsInSystem; index++)
            {
                if(!responses.contains(students[index].attributeResponse[attribute]))
                {
                    responses << students[index].attributeResponse[attribute];
                }
            }
            if(responses.size() > 1)
            {
                responses.removeAll(QString(""));
            }

            // Figure out what type of attribute this is: timezone, ordered/numerical, categorical (one response), or categorical (mult. responses)
            // If this is the timezone field, it's timezone type;
            // otherwise, if any response contains a comma, then it's multicategorical
            // otheriwse, if every response starts with an integer, it is ordered (numerical);
            // otherwise, if any response is missing an integer at the start, then it is categorical
            // The regex to recognize ordered/numerical is:
            // digit(s) then, optionally, "." or "," then end; OR digit(s) then "." or "," then any character but digits; OR digit(s) then any character but "." or ","
            QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
            if(dataOptions->attributeField[attribute] == dataOptions->timezoneField)
            {
                attributeType = DataOptions::AttributeType::timezone;
            }
            else if(std::any_of(responses.constBegin(), responses.constEnd(), [](const QString &response) {return response.contains(',');}))
            {
                attributeType = DataOptions::AttributeType::multicategorical;   // might be multiordered, this gets sorted out below
            }
            else if(std::all_of(responses.constBegin(), responses.constEnd(), [&startsWithInteger](const QString &response) {return startsWithInteger.match(response).hasMatch();}))
            {
                attributeType = DataOptions::AttributeType::ordered;
            }
            else
            {
                attributeType = DataOptions::AttributeType::categorical;
            }

            // for multicategorical, have to reprocess the responses to delimit at the commas and then determine if actually multiordered
            if(attributeType == DataOptions::AttributeType::multicategorical)
            {
                for(int originalResponseNum = 0, numOriginalResponses = int(responses.size()); originalResponseNum < numOriginalResponses; originalResponseNum++)
                {
                    QStringList newResponses = responses.takeFirst().split(',');
                    for(auto &newResponse : newResponses)
                    {
                        newResponse = newResponse.trimmed();
                        if(!responses.contains(newResponse))
                        {
                            responses << newResponse;
                        }
                    }
                }
                responses.removeAll(QString(""));
                // now that we've split them up, let's see if actually multiordered instead of multicategorical
                if(std::all_of(responses.constBegin(), responses.constEnd(), [&startsWithInteger](const QString &response) {return startsWithInteger.match(response).hasMatch();}))
                {
                    attributeType = DataOptions::AttributeType::multiordered;
                }
            }

            // sort alphanumerically unless it's timezone, in which case sort according to offset from GMT
            if(attributeType != DataOptions::AttributeType::timezone)
            {
                QCollator sortAlphanumerically;
                sortAlphanumerically.setNumericMode(true);
                sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
                std::sort(responses.begin(), responses.end(), sortAlphanumerically);
            }
            else
            {
                std::sort(responses.begin(), responses.end(), [] (const QString &A, const QString &B) {
                    float timezoneA = 0, timezoneB = 0;
                    QString unusedtimezoneName;
                    DataOptions::parseTimezoneInfoFromText(A, unusedtimezoneName, timezoneA);
                    DataOptions::parseTimezoneInfoFromText(B, unusedtimezoneName, timezoneB);
                    return timezoneA < timezoneB;});
            }

            // set values associated with each response and create a spot to hold the responseCounts
            if((attributeType == DataOptions::AttributeType::ordered) ||
                (attributeType == DataOptions::AttributeType::multiordered))
            {
                // ordered/numerical values. value is based on number at start of response
                for(const auto &response : qAsConst(responses))
                {
                    dataOptions->attributeVals[attribute].insert(startsWithInteger.match(response).captured(1).toInt());
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({response, 0});
                }
            }
            else if((attributeType == DataOptions::AttributeType::categorical) ||
                     (attributeType == DataOptions::AttributeType::multicategorical))
            {
                // categorical values. value is based on index of response within sorted list
                for(int i = 1; i <= responses.size(); i++)
                {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }
            else
            {
                for(int i = 1; i <= responses.size(); i++)
                {
                    dataOptions->attributeVals[attribute].insert(i);
                    dataOptions->attributeQuestionResponseCounts[attribute].insert({responses.at(i-1), 0});
                }
            }

            // set numerical value of each student's response and record in dataOptions a tally for each response
            for(auto &student : students) {
                const QString &currentStudentResponse = student.attributeResponse[attribute];
                QList<int> &currentStudentAttributeVals = student.attributeVals[attribute];
                if(!student.attributeResponse[attribute].isEmpty())
                {
                    if(attributeType == DataOptions::AttributeType::ordered)
                    {
                        // for numerical/ordered, set numerical value of students' attribute responses according to the number at the start of the response
                        currentStudentAttributeVals << startsWithInteger.match(currentStudentResponse).captured(1).toInt();
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if((attributeType == DataOptions::AttributeType::categorical) ||
                             (attributeType == DataOptions::AttributeType::timezone))
                    {
                        // set numerical value instead according to their place in the sorted list of responses
                        currentStudentAttributeVals << int(responses.indexOf(currentStudentResponse)) + 1;
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                    else if(attributeType == DataOptions::AttributeType::multicategorical)
                    {
                        //multicategorical - set numerical values according to each value
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent)
                        {
                            currentStudentAttributeVals << int(responses.indexOf(responseFromStudent.trimmed())) + 1;
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                    else if(attributeType == DataOptions::AttributeType::multiordered)
                    {
                        //multiordered - set numerical values according to the numbers at the start of the responses
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : setOfResponsesFromStudent)
                        {
                            currentStudentAttributeVals << startsWithInteger.match(responseFromStudent.trimmed()).captured(1).toInt();
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                }
                else
                {
                    currentStudentAttributeVals << -1;
                }
            }
        }
    }

    // gather all unique URM and section question responses and sort
    for(auto &student : students) {
        if(!dataOptions->URMResponses.contains(student.URMResponse, Qt::CaseInsensitive))
        {
            dataOptions->URMResponses << student.URMResponse;
        }
        if(!(dataOptions->sectionNames.contains(student.section, Qt::CaseInsensitive)))
        {
            dataOptions->sectionNames << student.section;
        }
    }
    QCollator sortAlphanumerically;
    sortAlphanumerically.setNumericMode(true);
    sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(dataOptions->URMResponses.begin(), dataOptions->URMResponses.end(), sortAlphanumerically);
    if(dataOptions->URMResponses.contains("--"))
    {
        // put the blank response option at the end of the list
        dataOptions->URMResponses.removeAll("--");
        dataOptions->URMResponses << "--";
    }
    std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);

    // set all of the students' tooltips
    for(auto &student : students) {
        student.createTooltip(dataOptions);
    }

    surveyFile.close(isTempFile);
    QApplication::restoreOverrideCursor();
    return true;
}
