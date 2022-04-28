#include "survey.h"
#include "gruepr_consts.h"
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

bool Survey::isValid()
{
    return (numAttributes > 0) || (std::any_of(questions.constBegin(), questions.constEnd(),
                                               [](Question question){return ((question.type == Question::timezone) || (question.type == Question::schedule));}));
}
