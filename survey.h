#ifndef SURVEY_H
#define SURVEY_H

#include <QList>
#include <QString>
#include <QStringList>


struct Question
{
    enum class QuestionType{shorttext, dropdown, radiobutton, checkbox, schedule, longtext};

    Question(const QString &questionText = "", const QuestionType questionType = QuestionType::longtext, const QStringList &responseOptions = {}) :
          text(questionText), type(questionType), options(responseOptions) {};

    QString text;
    QuestionType type;
    QStringList options;
};


struct Survey
{
    Survey(const QString &surveyTitle = "", const QList<Question> &surveyQuestions = {}, const int surveyAttributes = 0,
           const QStringList &surveyDayNames = {}, const QStringList &surveyTimeNames = {}) :
        title(surveyTitle), questions(surveyQuestions), numAttributes(surveyAttributes), schedDayNames(surveyDayNames), schedTimeNames(surveyTimeNames) {};

    QString title = "";
    QList<Question> questions;
    int numAttributes = 0;
    QStringList schedDayNames;
    QStringList schedTimeNames;
};

#endif // SURVEY_H
