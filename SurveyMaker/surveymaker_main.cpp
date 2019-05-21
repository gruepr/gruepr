#include "surveymaker.h"
#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SurveyMaker w;
    w.show();

    QFontDatabase::addApplicationFont(":/OxygenMono-Regular.otf");

    return a.exec();
}
