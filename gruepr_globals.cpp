#include "gruepr_globals.h"
#include "surveyMakerWizard.h"
#include <QEvent>
#include <QMessageBox>
#include <QtNetwork>
#include <QWebEngineView>
#include <QWidget>

bool internetIsGood()
{
    //make sure we can connect to google
    auto *manager = new QNetworkAccessManager();
    QEventLoop loop;
    QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    bool weGotProblems = (networkReply->bytesAvailable() == 0);
    delete networkReply;
    delete manager;
    if(weGotProblems)
    {
        QMessageBox::critical(nullptr, QObject::tr("Error!"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                          "Check your network connection and try again."));
    }
    return !weGotProblems;
}

void testFunction() {
    auto *surveyMakerWizard = new SurveyMakerWizard;
    surveyMakerWizard->exec();
    delete surveyMakerWizard;
    return;

    auto *win = new QDialog;
    auto *view = new QWebEngineView(win);
    view->load(QUrl("http://gruepr.com/"));
    win->setMinimumSize(LG_DLG_SIZE*2, LG_DLG_SIZE);
    view->resize(win->size());
    win->exec();
    delete view;
    delete win;
}


MouseWheelBlocker::MouseWheelBlocker(QObject *parent) : QObject(parent)
{
}

bool MouseWheelBlocker::eventFilter(QObject *o, QEvent *e)
{
    const QWidget* widget = static_cast<QWidget*>(o);
    if (e->type() == QEvent::Wheel && widget && !widget->hasFocus())
    {
        e->ignore();
        return true;
    }

    return QObject::eventFilter(o, e);
}
