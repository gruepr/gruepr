#include "getGrueprDataDialog.h"
#include "ui_getGrueprDataDialog.h"
#include "gruepr_globals.h"
#include <QComboBox>
#include <QPainter>
#include <QSettings>

GetGrueprDataDialog::GetGrueprDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GetGrueprDataDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Form teams"));

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
    connect(ui->loadDataPushButton, &QPushButton::clicked, this, &GetGrueprDataDialog::getData);

    ui->dataSourceFrame->setStyleSheet(QString() + "QFrame {background-color: " + (QColor::fromString(QString(STARFISHHEX)).lighter(133).name()) + "; color: " DEEPWATERHEX "; "
                                                                                                                                                   "border: none;}"
                                                                                                                                                   "QFrame::disabled {background-color: lightGray; color: darkGray; border: none;}");
    ui->dataSourceLabel->setStyleSheet("QLabel {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size: 12pt;}"
                                       "QLabel::disabled {background-color: " TRANSPARENT "; color: darkGray; font-family:'DM Sans'; font-size: 12pt;}");
    ui->dataSourceLabel->adjustSize();
    QPixmap fileIcon(":/icons_new/file.png");
    h = ui->dataSourceLabel->height() * 2 / 3;
    ui->dataSourceIcon->setPixmap(fileIcon.scaledToHeight(h, Qt::SmoothTransformation));

    ui->hLine->setStyleSheet("QFrame {color: " OPENWATERHEX ";}"
                             "QFrame::disabled {color: lightGray;}");
    ui->fieldsExplainer->setStyleSheet(LABELSTYLE);
    ui->headerRowCheckBox->setStyleSheet("QCheckBox {background-color: " TRANSPARENT "; color: " DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt;}"
                                         "QCheckBox::disabled {color: darkGray; font-family: 'DM Sans'; font-size: 12pt;}");

    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Confirm"));
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
}

GetGrueprDataDialog::~GetGrueprDataDialog()
{
    delete ui;
}


void GetGrueprDataDialog::getData()
{
    bool dataLoadFailed = true;
    CsvFile surveyFile;

    switch(ui->sourceButtonGroup->checkedId()) {
    case fromFile: {
        QSettings savedSettings;
        QFileInfo dataFile;
        dataFile.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

        if(!surveyFile.open(this, CsvFile::read, tr("Open Survey Data File"), dataFile.canonicalPath(), tr("Survey Data")))
        {
            return;
        }

        ui->dataSourceLabel->setText(tr("Survey file: ") + surveyFile.fileInfo().fileName());
        break;
    }
    case fromGoogle: {
        if(!internetIsGood())
        {
            return;
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
                    return;
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
                    return;
                }

                google->authenticate();

                loginDialog->setText(tr("Please use your browser to log in to Google and then return here."));
                loginDialog->setStandardButtons(QMessageBox::Cancel);
                connect(google, &GoogleHandler::granted, loginDialog, &QMessageBox::accept);
                if(loginDialog->exec() == QMessageBox::Cancel)
                {
                    delete loginDialog;
                    return;
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
            return;
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
            return;
        }
        busyBox->setText(tr("Success! The survey file is in your Downloads folder."));
        icon.load(":/icons/ok.png");
        busyBox->setIconPixmap(icon.scaled(iconSize));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        google->notBusy(busyBox);

        //open the downloaded file
        if(!surveyFile.openExistingFile(filepath))
        {
            return;
        }

        ui->dataSourceLabel->setText(tr("Google Form: ") + googleFormName);
        break;
    }
    case fromCanvas: {
        if(!internetIsGood())
        {
            return;
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
                return;
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
            return;
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
            return;
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
            return;
        }
        busyBox->setText(tr("Success! File will be saved in your Downloads folder."));
        icon.load(":/icons/ok.png");
        busyBox->setIconPixmap(icon.scaled(iconSize));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->notBusy(busyBox);

        //open the downloaded file
        if(!surveyFile.openExistingFile(filepath))
        {
            return;
        }

        // Only include the timestamp question ("submitted") and then the questions we've asked, which will all begin with (possibly a quotation mark then) an integer then a colon then a space.
        // (Also ignore the text "question" which serves as the text notifier that several schedule questions are coming up).
        surveyFile.fieldsToBeIgnored = QStringList{R"(^(?!(submitted)|("?\d+: .*)).*$)", ".*" + CanvasHandler::SCHEDULEQUESTIONINTRO2.trimmed() + ".*"};

        ui->dataSourceLabel->setText(tr("Canvas Survey: ") + canvasSurveyName);

        break;
    }
    default:
        return;
    }

    loadSurvey(surveyFile);

    if(dataLoadFailed) {
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
    ui->dataSourceLabel->setText(tr("Current data source:"));
    ui->hLine->setEnabled(true);
    ui->fieldsExplainer->setEnabled(true);
    ui->headerRowCheckBox->setEnabled(true);
    ui->confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}


void GetGrueprDataDialog::loadSurvey(CsvFile &surveyFile)
{
    students.clear();
    dataOptions = new DataOptions;
    dataOptions->dataFile = surveyFile.fileInfo();
}
