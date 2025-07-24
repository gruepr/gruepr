#include "gruepr_globals.h"
#include <QEvent>
#include <QGridLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTextBrowser>
#include <QTime>
#include <QWidget>

float grueprGlobal::timeStringToHours(const QString &timeStr) {
    static const QStringList timeFormats = QString(TIMEFORMATS).split(';');
    static QString mostRecentTimeFormat = timeFormats[0];

    QTime time = QTime::fromString(timeStr, mostRecentTimeFormat);
    if(time.isValid()) {
        return time.hour() + (time.minute() / 60.0f) + (time.second()/3600.0f);
    }

    for (const auto &timeFormat : timeFormats) {
        time = QTime::fromString(timeStr, timeFormat);
        if(time.isValid()) {
            mostRecentTimeFormat = timeFormat;
            return time.hour() + (time.minute() / 60.0f) + (time.second()/3600.0f);
        }
    }

    if(timeStr.compare(QObject::tr("noon"), Qt::CaseInsensitive) == 0) {
        return 12;
    }
    if(timeStr.compare(QObject::tr("midnight"), Qt::CaseInsensitive) == 0) {
        return 0;
    }

    // Return -1.0 to indicate an invalid time string
    return -1.0;
}

bool grueprGlobal::internetIsGood() {
    //make sure we can connect to google
    auto *manager = new QNetworkAccessManager;
    QEventLoop loop;
    QNetworkReply *networkReply = manager->head(QNetworkRequest(QUrl("http://www.google.com")));
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const bool weGotProblems = (networkReply->rawHeaderList().isEmpty());
    networkReply->deleteLater();
    manager->deleteLater();
    if(weGotProblems) {
        grueprGlobal::errorMessage(nullptr, QObject::tr("Error!"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                         "Check your network connection and try again."));
    }
    return !weGotProblems;
}

QString grueprGlobal::genderToString(const Gender gender) {
    switch (gender) {
        case Gender::woman: return "Woman";
        case Gender::man: return "Man";
        case Gender::nonbinary: return "Nonbinary";
        case Gender::unknown: return "Unknown";
        default: return "Unknown";
    }
}

// Convert string to enum
Gender grueprGlobal::stringToGender(const QString& genderStr) {
    if (genderStr == "Woman") return Gender::woman;
    if (genderStr == "Man") return Gender::man;
    if (genderStr == "Nonbinary") return Gender::nonbinary;
    return Gender::unknown; // Default to unknown if input is invalid
}

void grueprGlobal::errorMessage(QWidget *parent, const QString &windowTitle, const QString &message) {
    auto *win = new QMessageBox(parent);
    win->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    win->setStyleSheet(LABEL10PTSTYLE);
    win->setIconPixmap(QPixmap(":/icons_new/error.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    win->setWindowTitle(windowTitle.isEmpty()? "gruepr" : windowTitle);
    win->setText(message.isEmpty()? QObject::tr("There was an unspecified error.") : message);
    win->setStandardButtons(QMessageBox::Ok);
    win->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    win->exec();
    win->deleteLater();
}

bool grueprGlobal::warningMessage(QWidget *parent, const QString &windowTitle, const QString &message, const QString &OKtext, const QString &cancelText) {
    auto *win = new QMessageBox(parent);
    win->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    win->setStyleSheet(LABEL10PTSTYLE);
    win->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    win->setWindowTitle(windowTitle);
    win->setText(message);
    auto *okButton = new QPushButton(OKtext, win);
    okButton->setStyleSheet(SMALLBUTTONSTYLE);
    win->addButton(okButton, QMessageBox::AcceptRole);
    if(!cancelText.isEmpty()) {
        auto *cancelButton = new QPushButton(cancelText, win);
        cancelButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        win->addButton(cancelButton, QMessageBox::RejectRole);
    }
    win->exec();
    const bool result = (win->clickedButton() == okButton);
    win->deleteLater();
    return result;
}

void grueprGlobal::aboutWindow(QWidget *parent) {
    const QSettings savedSettings;
    const QString registeredUser = savedSettings.value("registeredUser", "").toString();
    const QString user = registeredUser.isEmpty()? QObject::tr("UNREGISTERED") : (QObject::tr("registered to ") + registeredUser);
    QMessageBox::about(parent, QObject::tr("About gruepr"), ABOUTWINDOWCONTENT + QObject::tr("<p><b>This copy of gruepr is ") + user + "</b>.");
}

void grueprGlobal::helpWindow(QWidget *parent) {
    QDialog helpWindow(parent);
    helpWindow.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    helpWindow.setSizeGripEnabled(true);
    helpWindow.setWindowTitle("Help");
    QGridLayout theGrid(&helpWindow);
    QTextBrowser helpContents(&helpWindow);
    helpContents.setHtml(HELPWINDOWCONTENT);
    helpContents.setFont(QFont("DM Sans"));
    helpContents.setOpenExternalLinks(true);
    helpContents.setFrameShape(QFrame::NoFrame);
    helpContents.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    helpContents.verticalScrollBar()->setStyleSheet(SCROLLBARSTYLE);
    theGrid.addWidget(&helpContents, 0, 0, -1, -1);
    helpWindow.resize(LG_DLG_SIZE, LG_DLG_SIZE);
    helpWindow.exec();
}

MouseWheelBlocker::MouseWheelBlocker(QObject *parent) : QObject(parent) { }
bool MouseWheelBlocker::eventFilter(QObject *o, QEvent *e) {
    const QWidget* widget = static_cast<QWidget*>(o);
    if (e->type() == QEvent::Wheel && (widget != nullptr) && !widget->hasFocus()) {
        e->ignore();
        return true;
    }

    return QObject::eventFilter(o, e);
}
