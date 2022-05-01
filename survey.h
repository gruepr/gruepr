#ifndef SURVEY_H
#define SURVEY_H

#include "gruepr_consts.h"
#include <QString>
#include <QStringList>
#include <QVector>


class Question
{
public:
    enum QuestionType{shorttext, dropdown, radiobutton, checkbox, timezone, schedule, longtext} type;

    Question(const QString &questionText = "", QuestionType questionType = longtext, const QStringList &responseOptions = {});

    QString text;
    QStringList options;
};


class Survey
{
public:
    Survey();

    bool isValid() const;

    QString title = "";
    QVector<Question> questions;
    int numAttributes = 0;
    QString schedDayNames[MAX_DAYS];
    int schedStartTime = 10;
    int schedEndTime = 17;
};

#endif // SURVEY_H
