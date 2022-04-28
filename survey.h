#ifndef SURVEY_H
#define SURVEY_H

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

    bool isValid();

    QString title = "";
    QVector<Question> questions;
    int numAttributes = 0;
};

#endif // SURVEY_H
