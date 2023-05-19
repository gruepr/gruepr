#include "gruepr_globals.h"
#include "surveyMakerWizard.h"
#include <QMessageBox>
#include <QWebEngineView>
#include <QtNetwork>

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
