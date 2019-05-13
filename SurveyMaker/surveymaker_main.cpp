#include "surveymaker.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SurveyMaker w;
    w.show();

    return a.exec();
}
