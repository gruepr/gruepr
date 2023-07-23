#include "gruepr_globals.h"
#include <QGridLayout>
#include <QEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBrowser>
#include <QtNetwork>
//#include <QWebEngineView>
#include <QWidget>

bool internetIsGood() {
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
        QMessageBox::critical(nullptr, QObject::tr("Error!"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                          "Check your network connection and try again."));
    }
    return !weGotProblems;
}

/*
void testFunction() {
    auto *win = new QDialog;
    auto *view = new QWebEngineView(win);
    view->load(QUrl(GRUEPRHOMEPAGE));
    win->setMinimumSize(LG_DLG_SIZE*2, LG_DLG_SIZE);
    view->resize(win->size());
    win->exec();
    delete view;
    delete win;
}
*/

void aboutWindow(QWidget *parent)
{
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString user = registeredUser.isEmpty()? QObject::tr("UNREGISTERED") : (QObject::tr("registered to ") + registeredUser);
    QMessageBox::about(parent, QObject::tr("About gruepr"), ABOUTWINDOWCONTENT + QObject::tr("<p><b>This copy of gruepr is ") + user + "</b>.");
}

void helpWindow(QWidget *parent)
{
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
