#include "survey.h"
#include <algorithm>


Question::Question(const QString &questionText, QuestionType questionType, const QStringList &responseOptions)
{
    text = questionText;
    type = questionType;
    options = responseOptions;
}


Survey::Survey()
{
    questions.clear();
}

bool Survey::isValid() const
{
    // a survey must have at least one attribute question (including timezone) or schedule question
    return (numAttributes > 0) || (std::any_of(questions.constBegin(), questions.constEnd(),
                                               [](const Question &question){return ((question.type == Question::timezone) || (question.type == Question::schedule));}));
}
