#include "gruepr_globals.h"
#include <QGridLayout>
#include <QEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTime>
#include <QtNetwork>
#include <QWidget>

float grueprGlobal::timeStringToHours(const QString &timeStr) {
    static QStringList formats = QString(TIMEFORMATS).split(';');
    int i = 0;
    for (const auto &format : formats) {
        QTime time = QTime::fromString(timeStr, format);
        if(time.isValid()) {
            if(i != 0) {
                // move this format to be first, since most likely all others will be same
                const QString currFormat = format;
                formats.removeOne(currFormat);
                formats.prepend(currFormat);
            }
            return time.hour() + (time.minute() / 60.0f) + (time.second()/3600.0f);
        }
        i++;
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
    auto *manager = new QNetworkAccessManager();
    QEventLoop loop;
    QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    bool weGotProblems = (networkReply->bytesAvailable() == 0);
    delete networkReply;
    delete manager;
    if(weGotProblems) {
        grueprGlobal::errorMessage(nullptr, QObject::tr("Error!"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                         "Check your network connection and try again."));
    }
    return !weGotProblems;
}

void grueprGlobal::errorMessage(QWidget *parent, const QString &windowTitle, const QString &message) {
    auto *win = new QMessageBox(parent);
    win->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    win->setStyleSheet(LABELSTYLE);
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
    win->setStyleSheet(LABELSTYLE);
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
    bool result = (win->clickedButton() == okButton);
    win->deleteLater();
    return result;
}

void grueprGlobal::aboutWindow(QWidget *parent) {
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString user = registeredUser.isEmpty()? QObject::tr("UNREGISTERED") : (QObject::tr("registered to ") + registeredUser);
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
    if (e->type() == QEvent::Wheel && widget && !widget->hasFocus()) {
        e->ignore();
        return true;
    }

    return QObject::eventFilter(o, e);
}
