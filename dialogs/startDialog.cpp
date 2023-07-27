#include "startDialog.h"
#include "gruepr.h"
#include "gruepr_globals.h"
#include "dialogs/getGrueprDataDialog.h"
#include "dialogs/registerDialog.h"
#include "surveyMakerWizard.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QMenu>
#include <QScreen>
#include <QSettings>
#include <QStringList>
#include <QThread>
#include <QtNetwork>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to register the software
/////////////////////////////////////////////////////////////////////////////////////////////////////////

StartDialog::StartDialog(QWidget *parent)
    :QDialog (parent)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    setWindowTitle("Welcome to gruepr!");
    setAttribute(Qt::WA_DeleteOnClose);

    // Get the screen size
    QRect screenGeometry = QGuiApplication::screens().at(0)->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    const int BASEWINDOWWIDTH = 800;
    const int BASEWINDOWHEIGHT = 456;
    const int LEFTRIGHTSPACERWIDTH = 74;
    const int MIDDLESPACERWIDTH = 42;
    const int TOPSPACERHEIGHT = 30;
    const int MIDDLESPACERHEIGHT = 36;
    const int BOTTOMSPACERHEIGHT = 46;
    const QSize TOOLBUTTONSIZE(300, 256);
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    const QSize INFOBUTTONSIZE(25, 25);
#endif
    const int ICONHEIGHT = 117;
    const QSize ICONSIZE = QSize(QPixmap(":/icons_new/makeASurvey.png").width() * ICONHEIGHT / QPixmap(":/icons_new/makeASurvey.png").height(), ICONHEIGHT);
    const int BIGFONTSIZE = 24;
    const int LITTLEFONTSIZE = 12;
    const QString BUTTONSTYLE = "QToolButton {border-style: solid; border-width: 3px; border-radius: 8px; border-color: " DEEPWATERHEX "; "
                                             "color: " DEEPWATERHEX "; background-color: white;} "
                                "QToolButton:hover {border-color: " OPENWATERHEX "; background-color: " BUBBLYHEX "}";
    const QString INFOBUTTONSTYLE = "QToolButton {border-style: solid; border-width: 2px; border-radius: 3px; border-color: " DEEPWATERHEX "; "
                                                  "padding-top: 2px; padding-left: 2px; padding-right: 10px; padding-bottom: 2px; "
                                                  "color: " DEEPWATERHEX "; background-color: white;} "
                                    "QToolButton:hover {border-color: " OPENWATERHEX "; background-color: " BUBBLYHEX "}"
                                    "QToolButton::menu-indicator {subcontrol-origin: border; subcontrol-position: bottom right;}";

    mainBoxFont = new QFont("DM Sans", BIGFONTSIZE);
    labelFont = new QFont("DM Sans", LITTLEFONTSIZE);
    setFont(*mainBoxFont);

    QPixmap backgroundPic(":/icons_new/startup_new.png");
    setFixedSize(BASEWINDOWWIDTH, BASEWINDOWHEIGHT);
    backgroundPic = backgroundPic.scaled(size(), Qt::KeepAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Window, backgroundPic);
    setPalette(palette);

    theGrid = new QGridLayout(this);
    int row = 0, col = 0;

    theGrid->setRowMinimumHeight(row++, TOPSPACERHEIGHT);

    topLabel = new QLabel(tr("What would you like to do?"), this);
    topLabel->setFont(*mainBoxFont);
    topLabel->setStyleSheet("QLabel { color: " DEEPWATERHEX "; }");
    theGrid->addWidget(topLabel, row++, col, 1, -1, Qt::AlignCenter);

    theGrid->setRowMinimumHeight(row++, MIDDLESPACERHEIGHT);

    // Create buttons and add to window
    theGrid->setColumnMinimumWidth(col++, LEFTRIGHTSPACERWIDTH);

    survMakeButton = new QToolButton(this);
    survMakeButton->setFixedSize(TOOLBUTTONSIZE);
    survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    survMakeButton->setIcon(QIcon(":/icons_new/makeASurvey.png"));
    survMakeButton->setIconSize(ICONSIZE);
    survMakeButton->setFont(*labelFont);
    survMakeButton->setText(tr("Fill out our form building\nquestionnaire to create the\nperfect survey for your class."));
    survMakeButton->setStyleSheet(BUTTONSTYLE);
    connect(survMakeButton, &QToolButton::clicked, this, &StartDialog::openSurveyMaker);
    theGrid->addWidget(survMakeButton, row, col++, 1, 1, Qt::AlignLeft);

    theGrid->setColumnMinimumWidth(col++, MIDDLESPACERWIDTH);

    grueprButton = new QToolButton(this);
    grueprButton->setFixedSize(TOOLBUTTONSIZE);
    grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    grueprButton->setIcon(QIcon(":/icons_new/formTeams.png"));
    grueprButton->setIconSize(ICONSIZE);
    grueprButton->setFont(*labelFont);
    grueprButton->setText(tr("Upload your survey results\nand form your grueps."));
    grueprButton->setStyleSheet(BUTTONSTYLE);
    connect(grueprButton, &QToolButton::clicked, this, &StartDialog::openGruepr);
    theGrid->addWidget(grueprButton, row++, col++, 1, 1, Qt::AlignRight);

    theGrid->setColumnMinimumWidth(col, LEFTRIGHTSPACERWIDTH);

    theGrid->setRowMinimumHeight(row++, BOTTOMSPACERHEIGHT);

    // Create status labels to show when registered and/or latest version installed
    registerLabel = new QLabel(this);
    registerLabel->setStyleSheet("QLabel { color: " DEEPWATERHEX "; }");
    registerLabel->setFont(*labelFont);
    registerLabel->setTextFormat(Qt::RichText);
    registerLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    registerLabel->setOpenExternalLinks(false);
    connect(registerLabel, &QLabel::linkActivated, this, &StartDialog::openRegisterDialog);
    // check to see if this copy of gruepr has been registered
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    const bool registered = (!registeredUser.isEmpty() && (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex())));
    QString registerMessage = (registered? (tr("Thank you for being a registered user.")) : (tr("Just downloaded? <a href = \"register\">Register now</a>.")));
    registerLabel->setText(registerMessage);
    registerLabel->setAlignment(Qt::AlignLeft);
    upgradeLabel = new QLabel(this);
    upgradeLabel->setStyleSheet("QLabel { color: " DEEPWATERHEX "; }");
    upgradeLabel->setFont(*labelFont);
    upgradeLabel->setTextFormat(Qt::RichText);
    upgradeLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    upgradeLabel->setOpenExternalLinks(true);
    // find out if there is an upgrade available
    GrueprVersion version = getLatestVersionFromGithub();
    QString upgradeMessage = tr("Version") + ": " + GRUEPR_VERSION_NUMBER + " <a href=\"" + GRUEPRDOWNLOADPAGE + "\">";
    if(version == GrueprVersion::old)
    {
        upgradeMessage +=  tr("Upgrade available!");
    }
    else if(version == GrueprVersion::beta)
    {
        upgradeMessage += tr("(pre-release)");
    }
    upgradeMessage += "<\a>";
    upgradeLabel->setText(upgradeMessage);
    upgradeLabel->setAlignment(Qt::AlignRight);
    theGrid->addWidget(registerLabel, row, 1, 1, 1, Qt::AlignLeft);
    theGrid->addWidget(upgradeLabel, row, 3, 1, 1, Qt::AlignRight);

    // Add help/about items to the application menu on mac or a dropdown toolbutton in corner of dialog on windows
    helpActions << new QAction("gruepr homepage");
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [](){QDesktopServices::openUrl(QUrl(QString("https://") + GRUEPRHOMEPAGE));});
    helpActions << new QAction("Submit a bug report / feature request");
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [](){QDesktopServices::openUrl(QUrl(BUGREPORTPAGE));});
    helpActions << new QAction("About gruepr");
    helpActions.last()->setMenuRole(QAction::AboutRole);
    connect(helpActions.last(), &QAction::triggered, this, [this](){aboutWindow(this);});
    helpActions << new QAction("How gruepr works");
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [this](){helpWindow(this);});
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    helpButton = new QToolButton(this);
    helpButton->setStyleSheet(INFOBUTTONSTYLE);
    helpButton->setAutoRaise(false);
    helpButton->setPopupMode(QToolButton::InstantPopup);
    helpButton->setIcon(QIcon(":/icons_new/infoButton.png"));
    helpButton->setIconSize(INFOBUTTONSIZE);
    theGrid->addWidget(helpButton, row, 4, 1, 1, Qt::AlignRight);
    helpMenu = new QMenu;
    for(const auto &helpAction : helpActions) {
        helpAction->setFont(*labelFont);
        helpMenu->addAction(helpAction);
    }
    helpButton->setMenu(helpMenu);
#else
    menuBar = new QMenuBar(nullptr);
    helpMenu = new QMenu;
    for(const auto &helpAction : helpActions) {
        helpMenu->addAction(helpAction);
    }
    menuBar->addMenu(helpMenu);
#endif
}


StartDialog::~StartDialog() {
    delete labelFont;
    delete mainBoxFont;
    delete helpMenu;
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    //
#else
    delete menuBar;
#endif
}


void StartDialog::openSurveyMaker() {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    this->hide();
    auto *surveyMakerWizard = new SurveyMakerWizard;
    QApplication::restoreOverrideCursor();
    surveyMakerWizard->exec();
    this->show();
    delete surveyMakerWizard;
}


void StartDialog::openGruepr() {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    this->hide();

    bool restart = false;
    do {
        auto *getDataDialog = new GetGrueprDataDialog;
        QApplication::restoreOverrideCursor();
        getDataDialog->exec();
        if(getDataDialog->result() == QDialog::Accepted) {
            QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
            auto *grueprWindow = new gruepr(*getDataDialog->dataOptions, getDataDialog->students);
            delete getDataDialog;
            QApplication::restoreOverrideCursor();
            grueprWindow->show();
            QEventLoop loop;
            connect(grueprWindow, &gruepr::closed, &loop, &QEventLoop::quit);
            loop.exec();
            restart = grueprWindow->restartRequested;
            delete grueprWindow;
        }
        else {
            restart = false;
            delete getDataDialog;
        }
    } while(restart);

    this->show();
}


StartDialog::GrueprVersion StartDialog::getLatestVersionFromGithub() {
    // check github for the latest version available for download
    auto *manager = new QNetworkAccessManager(this);
    auto *request = new QNetworkRequest(QUrl(VERSION_CHECK_URL));
    request->setSslConfiguration(QSslConfiguration::defaultConfiguration());
    request->setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = manager->get(*request);
    QString latestVersionString;
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0)
    {
        latestVersionString = "0";
    }
    else
    {
        QRegularExpression versionNum(R"(\"tag_name\":\"v([\d*.]{1,})\")");
        QRegularExpressionMatch match = versionNum.match(reply->readAll());
        latestVersionString = (match.hasMatch() ? match.captured(1) : ("0"));
        upgradeLabel->setToolTip(tr("Version ") + latestVersionString + tr(" is available for download"));
    }
    delete reply;
    delete request;
    delete manager;

    QStringList latestVersion = latestVersionString.split('.');
    QStringList thisVersion = QString(GRUEPR_VERSION_NUMBER).split('.');
    // pad fields out to NUMBER_VERSION_FIELDS in size (e.g., 5.2 --> 5.2.0.0)
    for(int field = int(latestVersion.size()); field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersion << "0";
    }
    for(int field = int(thisVersion.size()); field < NUMBER_VERSION_FIELDS; field++)
    {
        thisVersion << "0";
    }
    // convert to single integer
    unsigned long long int latestVersionAsInt = 0, thisVersionAsInt = 0;
    for(int field = 0; field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersionAsInt = (latestVersionAsInt*NUMBER_VERSION_PRECISION) + latestVersion.at(field).toInt();
        thisVersionAsInt = (thisVersionAsInt*NUMBER_VERSION_PRECISION) + thisVersion.at(field).toInt();
    }

    if(latestVersionAsInt == 0) {
        return GrueprVersion::unknown;
    }
    if(latestVersionAsInt > thisVersionAsInt) {
        return GrueprVersion::old;
    }
    if(thisVersionAsInt > latestVersionAsInt) {
        return GrueprVersion::beta;
    }
    return GrueprVersion::current;
}

void StartDialog::openRegisterDialog() {
    // open dialog window to allow the user to submit registration info to the Google Form
    QString registeredUser;
    if(internetIsGood())
    {
        //we can connect, so gather name, institution, and email address for submission
        auto *registerWin = new registerDialog;
        if(registerWin->exec() == QDialog::Accepted)
        {
            //If user clicks OK, add to saved settings
            registerWin->show();
            registerWin->raise();
            auto *box = new QHBoxLayout;
            auto *icon = new QLabel(registerWin);
            auto *message = new QLabel(registerWin);
            box->addWidget(icon);
            box->addWidget(message, 0, Qt::AlignLeft);
            (qobject_cast<QBoxLayout *>(registerWin->layout()))->addLayout(box);
            icon->setPixmap(QPixmap(":/icons/wait.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
            message->setText(tr("Communicating..."));

            auto *manager = new QNetworkAccessManager(registerWin);
            auto *request = new QNetworkRequest(QUrl(USER_REGISTRATION_URL));
            request->setSslConfiguration(QSslConfiguration::defaultConfiguration());
            request->setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            request->setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
            QJsonObject data;
            data["name"] = registerWin->name->text();
            data["institution"] = registerWin->institution->text();
            data["email"] = registerWin->email->text();
            QJsonDocument doc(data);
            QByteArray postData = doc.toJson();
            auto *reply = manager->post(*request, postData);
            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            QString replyBody = (reply->bytesAvailable() == 0 ? "" : QString(reply->readAll()));
            if(replyBody.contains("Registration successful"))
            {
                registeredUser = registerWin->name->text();
                QSettings savedSettings;
                savedSettings.setValue("registeredUser", registeredUser);
                savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex()));
                icon->setPixmap(QPixmap(":/icons/ok.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                message->setText(tr("Success!"));
                registerLabel->setText(tr("Thank you for being a registered user."));
            }
            else
            {
                registeredUser.clear();
                icon->setPixmap(QPixmap(":/icons/delete.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                message->setText(tr("Error. Please try again later or contact <" GRUEPRHELPEMAIL ">."));
            }
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            registerWin->hide();
            delete box;
            delete reply;
            delete request;
            delete manager;
        }
        delete registerWin;
    }
}
