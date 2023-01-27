#include "startDialog.h"
#include "dialogs/registerDialog.h"
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QSettings>
#include <QStringList>
#include <QThread>
#include <QToolButton>
#include <QtNetwork>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to register the software
/////////////////////////////////////////////////////////////////////////////////////////////////////////

startDialog::startDialog(QWidget *parent)
    :QDialog (parent)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    setWindowTitle("Welcome to gruepr!");

    mainBoxFont = new QFont("DM Sans", 24);
    labelFont = new QFont("DM Sans", 12);
    setFont(*mainBoxFont);
    QPixmap backgroundPic(":/icons_new/startup_new");
    setFixedSize(BASEWINDOWWIDTH, BASEWINDOWHEIGHT);
    backgroundPic = backgroundPic.scaled(size(), Qt::KeepAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Window, backgroundPic);
    setPalette(palette);
    theGrid = new QGridLayout(this);
    int row = 0, col = 0;

    theGrid->setRowMinimumHeight(row++, 30);

    auto *topLabel = new QLabel(tr("What would you like to do?"), this);
    topLabel->setFont(*mainBoxFont);
    topLabel->setStyleSheet(QString("QLabel { color: #") + GRUEPRDARKBLUEHEX + "; }");
    theGrid->addWidget(topLabel, row++, col, 1, -1, Qt::AlignCenter);

    theGrid->setRowMinimumHeight(row++, 48);

    // Create buttons and add to window
    theGrid->setColumnMinimumWidth(col++, 74);

    auto *survMakeButton = new QToolButton(this);
    survMakeButton->setFixedSize(QSize(300, 256));
    survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    survMakeButton->setIcon(QIcon(":/icons_new/makeASurvey.png"));
    survMakeButton->setIconSize(QSize(177, 117));
    survMakeButton->setFont(*labelFont);
    survMakeButton->setText(tr("Fill out our form building\nquestionnaire to create the\nperfect survey for your class."));
    survMakeButton->setStyleSheet(BUTTONSTYLE);
    connect(survMakeButton, &QToolButton::clicked, this, [&](){done(Result::makeSurvey);});
    theGrid->addWidget(survMakeButton, row, col++, 1, 1, Qt::AlignLeft);

    theGrid->setColumnMinimumWidth(col++, 42);

    auto *grueprButton = new QToolButton(this);
    grueprButton->setFixedSize(QSize(300, 256));
    grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    grueprButton->setIcon(QIcon(":/icons_new/formTeams.png"));
    grueprButton->setIconSize(QSize(148, 117));
    grueprButton->setFont(*labelFont);
    grueprButton->setText(tr("Upload your survey results\nand have the grueps appear\nright before your eyes."));
    grueprButton->setStyleSheet(BUTTONSTYLE);
    connect(grueprButton, &QToolButton::clicked, this, [&](){done(Result::makeGroups);});
    theGrid->addWidget(grueprButton, row++, col++, 1, 1, Qt::AlignRight);

    theGrid->setColumnMinimumWidth(col, 74);

    theGrid->setRowMinimumHeight(row++, 34);

    // find out if there is an upgrade available
    GrueprVersion version = getLatestVersionFromGithub();

    // check to see if this copy of gruepr has been registered
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    const bool registered = (!registeredUser.isEmpty() && (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex())));

    // Create status labels to show when registered and/or latest version installed
    auto *registerLabel = new QLabel(this);
    auto *upgradeLabel = new QLabel(this);
    registerLabel->setFont(*labelFont);
    registerLabel->setTextFormat(Qt::RichText);
    registerLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    registerLabel->setOpenExternalLinks(false);
    connect(registerLabel, &QLabel::linkActivated, this, &startDialog::openRegisterDialog);
    registerLabel->setStyleSheet(QString("QLabel { color: #") + GRUEPRDARKBLUEHEX + "; }");
    upgradeLabel->setFont(*labelFont);
    upgradeLabel->setTextFormat(Qt::RichText);
    upgradeLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    upgradeLabel->setOpenExternalLinks(true);
    upgradeLabel->setStyleSheet(QString("QLabel { color: #") + GRUEPRDARKBLUEHEX + "; }");
    QString registerMessage = (registered? (tr("Thank you for being a registered user.")) : (tr("Just downloaded? <a href = \"openRegisterDialog\">Register now</a>.")));
    QString upgradeMessage = tr("Version") + ": " + GRUEPR_VERSION_NUMBER;
    if(version == GrueprVersion::old)
    {
        upgradeMessage += " <a href = \"https://www.gruepr.com/#/Download\">" + tr("Upgrade available!") + "<\a>";
    }
    else if(version == GrueprVersion::beta)
    {
        upgradeMessage += " <small><a href = \"https://www.gruepr.com/#/Download\">" + tr("(pre-release)") + "<\a></small>";
    }
    registerLabel->setText(registerMessage);
    upgradeLabel->setText(upgradeMessage);
    upgradeLabel->setAlignment(Qt::AlignRight);
    theGrid->addWidget(registerLabel, row, 1, 1, 2, Qt::AlignLeft);
    theGrid->addWidget(upgradeLabel, row, 3, 1, 1, Qt::AlignRight);

}


startDialog::~startDialog() {
    delete labelFont;
    delete mainBoxFont;
}


startDialog::GrueprVersion startDialog::getLatestVersionFromGithub() {
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
    }
    delete reply;
    delete request;
    delete manager;

    QStringList latestVersion = latestVersionString.split('.');
    QStringList thisVersion = QString(GRUEPR_VERSION_NUMBER).split('.');
    // pad fields out to NUMBER_VERSION_FIELDS in size (e.g., 5.2 --> 5.2.0.0)
    for(int field = latestVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
    {
        latestVersion << "0";
    }
    for(int field = thisVersion.size(); field < NUMBER_VERSION_FIELDS; field++)
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

void startDialog::openRegisterDialog() {
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
            auto *box = new QHBoxLayout(registerWin);
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
            QString replyBody = (reply->bytesAvailable() == 0 ? "" : reply->readAll());
            if(replyBody.contains("Registration successful"))
            {
                registeredUser = registerWin->name->text();
                QSettings savedSettings;
                savedSettings.setValue("registeredUser", registeredUser);
                savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex()));
                icon->setPixmap(QPixmap(":/icons/ok.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                message->setText(tr("Success!"));
            }
            else
            {
                registeredUser.clear();
                icon->setPixmap(QPixmap(":/icons/delete.png").scaled(REDUCED_ICON_SIZE, REDUCED_ICON_SIZE));
                message->setText(tr("Error. Please try again later or contact <info@gruepr.com>."));
            }
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            registerWin->hide();
            delete reply;
            delete request;
            delete manager;
        }
        delete registerWin;
    }
}
