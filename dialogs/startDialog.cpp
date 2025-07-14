#include "startDialog.h"
#include "gruepr.h"
#include "gruepr_globals.h"
#include "dialogs/getGrueprDataDialog.h"
#include "dialogs/registerDialog.h"
#include "surveyMakerWizard.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QGridLayout>
#include <QJsonDocument>
#include <QMenu>
#include <QMenuBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QToolButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose whether to create a survey or use survey results to form teams;
// also offers the ability to register an unregistered copy of the program and to download a new version
/////////////////////////////////////////////////////////////////////////////////////////////////////////

StartDialog::StartDialog(QWidget *parent)
    :QDialog (parent)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    setWindowTitle("gruepr - Welcome!");
    setAttribute(Qt::WA_DeleteOnClose);

    const QFont mainBoxFont("DM Sans", BIGFONTSIZE);
    const QFont labelFont("DM Sans", LITTLEFONTSIZE);
    setFont(mainBoxFont);

    QPixmap backgroundPic(":/icons_new/startup_new.png");
    setFixedSize(BASEWINDOWWIDTH, BASEWINDOWHEIGHT);
    backgroundPic = backgroundPic.scaled(size(), Qt::KeepAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Window, backgroundPic);
    setPalette(palette);

    auto *theGrid = new QGridLayout(this);
    int row = 0, col = 0;

    theGrid->setRowMinimumHeight(row++, TOPSPACERHEIGHT);

    auto *topLabel = new QLabel(tr("What would you like to do?"), this);
    topLabel->setStyleSheet(LABEL24PTSTYLE);
    theGrid->addWidget(topLabel, row++, col, 1, -1, Qt::AlignCenter);

    theGrid->setRowMinimumHeight(row++, MIDDLESPACERHEIGHT);

    // Create buttons and add to window
    theGrid->setColumnMinimumWidth(col++, LEFTRIGHTSPACERWIDTH);

    auto *survMakeButton = new QToolButton(this);
    survMakeButton->setFixedSize(TOOLBUTTONSIZE);
    survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    survMakeButton->setIcon(QIcon(":/icons_new/makeASurvey.png"));
    const QSize ICONSIZE = QSize(QPixmap(":/icons_new/makeASurvey.png").width() * ICONHEIGHT / QPixmap(":/icons_new/makeASurvey.png").height(), ICONHEIGHT);
    survMakeButton->setIconSize(ICONSIZE);
    survMakeButton->setFont(labelFont);
    survMakeButton->setText(tr("Use our form builder\nto create the\nperfect survey for your class."));
    survMakeButton->setStyleSheet(STARTDIALODBUTTONSTYLE);
    connect(survMakeButton, &QToolButton::clicked, this, &StartDialog::openSurveyMaker);
    theGrid->addWidget(survMakeButton, row, col++, 1, 1, Qt::AlignLeft);

    theGrid->setColumnMinimumWidth(col++, MIDDLESPACERWIDTH);

    auto *grueprButton = new QToolButton(this);
    grueprButton->setFixedSize(TOOLBUTTONSIZE);
    grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    grueprButton->setIcon(QIcon(":/icons_new/formTeams.png"));
    grueprButton->setIconSize(ICONSIZE);
    grueprButton->setFont(labelFont);
    grueprButton->setText(tr("Use the survey results\nto form your grueps."));
    grueprButton->setStyleSheet(STARTDIALODBUTTONSTYLE);
    connect(grueprButton, &QToolButton::clicked, this, &StartDialog::openGruepr);
    theGrid->addWidget(grueprButton, row++, col++, 1, 1, Qt::AlignRight);

    theGrid->setColumnMinimumWidth(col, LEFTRIGHTSPACERWIDTH);

    theGrid->setRowMinimumHeight(row++, BOTTOMSPACERHEIGHT);

    // Create status labels to show when registered and/or latest version installed
    registerLabel = new QLabel(this);
    registerLabel->setStyleSheet(LABEL12PTSTYLE);
    registerLabel->setTextFormat(Qt::RichText);
    registerLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    registerLabel->setOpenExternalLinks(false);
    connect(registerLabel, &QLabel::linkActivated, this, &StartDialog::openRegisterDialog);
    // check to see if this copy of gruepr has been registered
    const QSettings savedSettings;
    const QString registeredUser = savedSettings.value("registeredUser", "").toString();
    const QString UserID = savedSettings.value("registeredUserID", "").toString();
    const bool registered = !registeredUser.isEmpty() &&
                            (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex()));
    const QString registerMessage = (registered? (tr("Thank you for being a registered user.")) :
                                         (tr("Just downloaded? <a href = \"register\">Register now</a>.")));
    registerLabel->setText(registerMessage);
    registerLabel->setAlignment(Qt::AlignLeft);
    upgradeLabel = new QLabel(this);
    upgradeLabel->setStyleSheet(LABEL12PTSTYLE);
    upgradeLabel->setTextFormat(Qt::RichText);
    upgradeLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    upgradeLabel->setOpenExternalLinks(true);
    // find out if there is an upgrade available
    const GrueprVersion version = getLatestVersionFromGithub();
    QString upgradeMessage = tr("Version") + ": " + GRUEPR_VERSION_NUMBER + " <a href=\"" + GRUEPRDOWNLOADPAGE + "\">";
    if(version == GrueprVersion::old) {
        upgradeMessage +=  tr("Upgrade available!");
    }
    else if(version == GrueprVersion::beta) {
        upgradeMessage += tr("(pre-release)");
    }
    upgradeMessage += "<\a>";
    upgradeLabel->setText(upgradeMessage);
    upgradeLabel->setAlignment(Qt::AlignRight);
    theGrid->addWidget(registerLabel, row, 1, 1, 1, Qt::AlignLeft);
    theGrid->addWidget(upgradeLabel, row, 3, 1, 1, Qt::AlignRight);

    // Add help/about items to the application menu on mac or a dropdown toolbutton in corner of dialog on windows
    helpActions << new QAction("gruepr homepage", this);
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [](){QDesktopServices::openUrl(QUrl(QString("https://") + GRUEPRHOMEPAGE));});
    helpActions << new QAction("Submit a bug report / feature request", this);
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [](){QDesktopServices::openUrl(QUrl(BUGREPORTPAGE));});
    helpActions << new QAction("About gruepr", this);
    helpActions.last()->setMenuRole(QAction::AboutRole);
    connect(helpActions.last(), &QAction::triggered, this, [this](){grueprGlobal::aboutWindow(this);});
    helpActions << new QAction("How gruepr works", this);
    helpActions.last()->setMenuRole(QAction::ApplicationSpecificRole);
    connect(helpActions.last(), &QAction::triggered, this, [this](){grueprGlobal::helpWindow(this);});
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    auto *helpButton = new QToolButton(this);
    helpButton->setStyleSheet(INFOBUTTONSTYLE);
    helpButton->setAutoRaise(false);
    helpButton->setPopupMode(QToolButton::InstantPopup);
    helpButton->setIcon(QIcon(":/icons_new/infoButton.png"));
    helpButton->setIconSize(INFOBUTTONSIZE);
    theGrid->addWidget(helpButton, row, 4, 1, 1, Qt::AlignRight);
    auto *helpMenu = new QMenu(this);
    for(const auto &helpAction : qAsConst(helpActions)) {
        helpAction->setFont(labelFont);
        helpMenu->addAction(helpAction);
    }
    helpButton->setMenu(helpMenu);
#else
    auto *menuBar = new QMenuBar(nullptr);
    auto *helpMenu = new QMenu(this);
    for(const auto &helpAction : helpActions) {
        helpMenu->addAction(helpAction);
    }
    menuBar->addMenu(helpMenu);
#endif
}


void StartDialog::openSurveyMaker() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    this->hide();
    const QScopedPointer<SurveyMakerWizard> surveyMakerWizard(new SurveyMakerWizard);
    QApplication::restoreOverrideCursor();
    surveyMakerWizard->exec();
    this->show();
}


void StartDialog::openGruepr() {
    QApplication::setOverrideCursor(Qt::BusyCursor);

    bool spawnNewWindow = true;
    while(spawnNewWindow) {
        const QScopedPointer<GetGrueprDataDialog> getDataDialog(new GetGrueprDataDialog(this));
        QApplication::restoreOverrideCursor();
        auto result = getDataDialog->exec();
        if(result == QDialog::Accepted) {
            QApplication::setOverrideCursor(Qt::BusyCursor);
            const QScopedPointer<gruepr> grueprWindow(new gruepr(*getDataDialog->dataOptions, getDataDialog->students, this));
            this->hide();
            grueprWindow->show();
            emit closeDataDialogProgressBar();
            QApplication::restoreOverrideCursor();
            QEventLoop loop;
            connect(grueprWindow.data(), &gruepr::closed, &loop, &QEventLoop::quit);
            loop.exec();
            spawnNewWindow = grueprWindow->restartRequested;
            this->show();
        }
        else {
            spawnNewWindow = false;
            emit closeDataDialogProgressBar();
        }
    }
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
    if(reply->bytesAvailable() == 0) {
        latestVersionString = "0";
    }
    else {
        static const QRegularExpression versionNum(R"(\"tag_name\":\"v([\d*.]{1,})\")");
        const QRegularExpressionMatch match = versionNum.match(reply->readAll());
        latestVersionString = (match.hasMatch() ? match.captured(1) : ("0"));
    }
    reply->deleteLater();
    delete request;
    manager->deleteLater();

    QStringList latestVersion = latestVersionString.split('.');
    QStringList thisVersion = QString(GRUEPR_VERSION_NUMBER).split('.');
    // pad fields out to NUMBER_VERSION_FIELDS in size (e.g., 5.2 --> 5.2.0.0)
    for(int field = int(latestVersion.size()); field < NUMBER_VERSION_FIELDS; field++) {
        latestVersion << "0";
    }
    for(int field = int(thisVersion.size()); field < NUMBER_VERSION_FIELDS; field++) {
        thisVersion << "0";
    }
    // convert to single integer
    unsigned long long latestVersionAsInt = 0, thisVersionAsInt = 0;
    for(int field = 0; field < NUMBER_VERSION_FIELDS; field++) {
        latestVersionAsInt = (latestVersionAsInt*NUMBER_VERSION_PRECISION) + latestVersion.at(field).toInt();
        thisVersionAsInt = (thisVersionAsInt*NUMBER_VERSION_PRECISION) + thisVersion.at(field).toInt();
    }

    if(latestVersionAsInt == 0) {
        upgradeLabel->setToolTip(tr("Version ") + latestVersionString + tr(" is the latest available version"));
        return GrueprVersion::unknown;
    }
    if(latestVersionAsInt > thisVersionAsInt) {
        upgradeLabel->setToolTip(tr("Version ") + latestVersionString + tr(" is available to download"));
        return GrueprVersion::old;
    }
    if(thisVersionAsInt > latestVersionAsInt) {
        upgradeLabel->setToolTip(tr("Version ") + latestVersionString + tr(" is the latest available version"));
        return GrueprVersion::beta;
    }
    upgradeLabel->setToolTip(tr("Your version is up-to-date"));
    return GrueprVersion::current;
}

void StartDialog::openRegisterDialog() {
    // open dialog window to allow the user to submit registration info to the Google Form
    QString registeredUser;
    if(grueprGlobal::internetIsGood()) {
        //we can connect, so gather name, institution, and email address for submission
        auto *registerWin = new registerDialog(this);
        if(registerWin->exec() == QDialog::Accepted) {
            //If user clicks OK, add to saved settings
            registerWin->show();
            registerWin->raise();
            auto *box = new QHBoxLayout;
            auto *icon = new QLabel(registerWin);
            auto *message = new QLabel(registerWin);
            message->setStyleSheet(LABEL10PTSTYLE);
            box->addWidget(icon);
            box->addWidget(message, 0, Qt::AlignLeft);
            (qobject_cast<QBoxLayout *>(registerWin->layout()))->addLayout(box);
            message->setText(tr("Communicating..."));

            auto *manager = new QNetworkAccessManager(registerWin);
            auto *request = new QNetworkRequest(QUrl(USER_REGISTRATION_URL));
            request->setSslConfiguration(QSslConfiguration::defaultConfiguration());
            request->setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            request->setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));
            QJsonObject data;
            data["name"] = registerWin->name;
            data["institution"] = registerWin->institution;
            data["email"] = registerWin->email;
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
            data["os"] = QString("Windows");
#else
            data["os"] = QString("macOS");
#endif
            const QJsonDocument doc(data);
            const QByteArray postData = doc.toJson();
            auto *reply = manager->post(*request, postData);
            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            const QString replyBody = (reply->bytesAvailable() == 0 ? "" : QString(reply->readAll()));
            reply->deleteLater();
            if(replyBody.contains("Registration successful")) {
                registeredUser = registerWin->name;
                QSettings savedSettings;
                savedSettings.setValue("registeredUser", registeredUser);
                savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex()));
                icon->setPixmap(QPixmap(":/icons_new/ok.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                message->setText(tr("Success!"));
                registerLabel->setText(tr("Thank you for being a registered user."));
            }
            else {
                registeredUser.clear();
                icon->setPixmap(QPixmap(":/icons_new/error.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                message->setText(tr("Error. Please try again later or contact <" GRUEPRHELPEMAIL ">."));
            }
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            registerWin->hide();
            delete box;
            delete request;
        }
        registerWin->deleteLater();
    }
}
