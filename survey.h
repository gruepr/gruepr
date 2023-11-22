#ifndef SURVEY_H
#define SURVEY_H

#include <QList>
#include <QString>
#include <QStringList>


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
    QStringList schedTimeNames;
};

#endif // SURVEY_H
