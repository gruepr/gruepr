#include "survey.h"

Question::Question(const QString &questionText, const QuestionType questionType, const QStringList &responseOptions)
{
    text = questionText;
    type = questionType;
    options = responseOptions;
}


Survey::Survey()
{
    questions.clear();
    schedDayNames.clear();
    schedTimeNames.clear();
}
