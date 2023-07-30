#ifndef SURVEY_H
#define SURVEY_H

#include "gruepr_globals.h"
#include <QString>
#include <QStringList>
#include <QList>


class Question
{
public:
    enum class QuestionType{shorttext, dropdown, radiobutton, checkbox, schedule, longtext};

    Question(const QString &questionText = "", const QuestionType questionType = QuestionType::longtext, const QStringList &responseOptions = {});

    QString text;
    QuestionType type;
    QStringList options;
};


class Survey
{
public:
    Survey();

    QString title = "";
    QList<Question> questions;
    int numAttributes = 0;
    QStringList schedDayNames;
    int schedStartTime = STANDARDSCHEDSTARTTIME;
    int schedEndTime = STANDARDSCHEDENDTIME;
};

#endif // SURVEY_H
