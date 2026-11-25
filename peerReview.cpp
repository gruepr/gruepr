#include "peerReview.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextDocumentFragment>

// names used for the enums when saving to file; order MUST match the order of the enum values
static const QStringList KINDNAMES = {"likert", "numberScale", "choice", "numberEntry", "contribution", "written"};
static const QStringList ABOUTNAMES = {"eachTeammate", "eachTeammateAndSelf", "self", "once"};
static const QStringList SHAREWITHNAMES = {"instructor", "personItsAbout", "team"};
static const QStringList CONTRIBUTIONSTYLENAMES = {"dropdown", "points", "grid"};

static const int MAXGRIDOPTIONS = 6;        // a choice question with more options than this is shown as a dropdown per team member, not a grid
static const int EQUALSHARE = 100;          // contribution: the score of an equal share of the work

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// One peer review question
/////////////////////////////////////////////////////////////////////////////////////////////////////////

QJsonObject PeerReviewQuestion::toJson() const
{
    QJsonObject content {
        {"kind", KINDNAMES.at(static_cast<int>(kind))},
        {"text", text},
        {"options", QJsonArray::fromStringList(options)},
        {"multi", multi},
        {"bestFirst", bestFirst},
        {"scaleLow", scaleLow},
        {"scaleHigh", scaleHigh},
        {"lowLabel", lowLabel},
        {"highLabel", highLabel},
        {"contributionStyle", CONTRIBUTIONSTYLENAMES.at(static_cast<int>(contributionStyle))},
        {"about", ABOUTNAMES.at(static_cast<int>(about))},
        {"shareWith", SHAREWITHNAMES.at(static_cast<int>(shareWith))},
        {"namesShown", namesShown},
        {"required", required}
    };

    return content;
}

PeerReviewQuestion PeerReviewQuestion::fromJson(const QJsonObject &json)
{
    PeerReviewQuestion question;
    question.kind = static_cast<Kind>(std::max(0, int(KINDNAMES.indexOf(json["kind"].toString()))));
    question.text = json["text"].toString();
    const QJsonArray optionsArray = json["options"].toArray();
    for(const auto &option : optionsArray) {
        question.options << option.toString();
    }
    question.multi = json["multi"].toBool(false);
    question.bestFirst = json["bestFirst"].toBool(true);
    question.scaleLow = json["scaleLow"].toInt(1);
    question.scaleHigh = std::max(question.scaleLow + 1, json["scaleHigh"].toInt(5));
    question.lowLabel = json["lowLabel"].toString();
    question.highLabel = json["highLabel"].toString();
    question.contributionStyle = static_cast<ContributionStyle>(std::max(0, int(CONTRIBUTIONSTYLENAMES.indexOf(json["contributionStyle"].toString()))));
    question.about = static_cast<About>(std::max(0, int(ABOUTNAMES.indexOf(json["about"].toString()))));
    question.shareWith = static_cast<ShareWith>(std::max(0, int(SHAREWITHNAMES.indexOf(json["shareWith"].toString()))));
    question.namesShown = json["namesShown"].toBool(false);
    question.required = json["required"].toBool(true);

    // a contribution question is always about each teammate and yourself, "about yourself" answers shared with the team always show
    // the writer's name, and a sharing choice must suit who the question is about
    if(question.kind == Kind::contribution) {
        question.about = About::eachTeammateAndSelf;
    }
    if((question.about == About::self) && (question.shareWith == ShareWith::team)) {
        question.namesShown = true;
    }
    if(question.shareWith == ShareWith::instructor) {
        question.namesShown = false;
    }
    if(!sharingChoices(question.about).contains(qMakePair(question.shareWith, question.namesShown))) {
        question.shareWith = ShareWith::instructor;
        question.namesShown = false;
    }

    return question;
}

bool PeerReviewQuestion::showsWriterNames() const
{
    return (shareWith != ShareWith::instructor) && (namesShown || (about == About::self));
}

bool PeerReviewQuestion::isScored() const
{
    return (kind == Kind::likert) || (kind == Kind::numberScale) || (kind == Kind::contribution);
}

bool PeerReviewQuestion::isAboutTeammates() const
{
    return (about == About::eachTeammate) || (about == About::eachTeammateAndSelf);
}

bool PeerReviewQuestion::requiredIsPossible() const
{
    return about != About::eachTeammate;
}

QStringList PeerReviewQuestion::formLabels() const
{
    QStringList labels;
    switch(kind) {
    case Kind::likert:
    case Kind::choice:
        for(const auto &option : options) {
            labels << withoutOrderNumbers(option).simplified();
        }
        break;
    case Kind::numberScale:
        for(int value = scaleLow; value <= scaleHigh; value++) {
            QString label = QString::number(value);
            if((value == scaleLow) && !lowLabel.simplified().isEmpty()) {
                label += " (" + lowLabel.simplified() + ")";
            }
            else if((value == scaleHigh) && !highLabel.simplified().isEmpty()) {
                label += " (" + highLabel.simplified() + ")";
            }
            labels << label;
        }
        break;
    case Kind::contribution:
        if(contributionStyle == ContributionStyle::dropdown) {
            for(const auto &level : contributionLevels()) {
                labels << level.first;
            }
        }
        else if(contributionStyle == ContributionStyle::grid) {
            for(const auto &level : contributionGridLevels()) {
                labels << level.first;
            }
        }
        break;
    case Kind::numberEntry:
    case Kind::written:
        break;
    }
    return labels;
}

bool PeerReviewQuestion::usesGrid() const
{
    if(!isAboutTeammates()) {
        return false;
    }
    return (kind == Kind::likert) || (kind == Kind::numberScale) || ((kind == Kind::choice) && (options.size() <= MAXGRIDOPTIONS)) ||
           ((kind == Kind::contribution) && (contributionStyle == ContributionStyle::grid));
}

double PeerReviewQuestion::scoreOf(const QString &answer, bool *ok) const
{
    *ok = false;
    const QString trimmedAnswer = answer.simplified();
    if(trimmedAnswer.isEmpty()) {
        return 0;
    }

    switch(kind) {
    case Kind::likert: {
        const QStringList labels = formLabels();
        int index = int(labels.indexOf(trimmedAnswer));
        if(index == -1) {
            index = int(options.indexOf(trimmedAnswer));   // the answer may still have its order number
        }
        if(index == -1) {
            return 0;
        }
        *ok = true;
        const int numOptions = int(labels.size());
        return bestFirst? (numOptions - index) : (index + 1);}
    case Kind::numberScale: {
        static const QRegularExpression leadingNumber(R"(^\s*(-?\d+))");
        const QRegularExpressionMatch match = leadingNumber.match(trimmedAnswer);
        if(!match.hasMatch()) {
            return 0;
        }
        const int value = match.captured(1).toInt();
        if((value < scaleLow) || (value > scaleHigh)) {
            return 0;
        }
        *ok = true;
        return value;}
    case Kind::contribution: {
        if(contributionStyle != ContributionStyle::points) {
            for(const auto &level : ((contributionStyle == ContributionStyle::grid)? contributionGridLevels() : contributionLevels())) {
                if(level.first == trimmedAnswer) {
                    *ok = (level.second >= 0);      // "Not sure" has no score
                    return std::max(0, level.second);
                }
            }
            // a grid answer that isn't one of the current columns, from a form made with an earlier version of the grid (e.g., "+40%")
            if(contributionStyle == ContributionStyle::grid) {
                static const QRegularExpression gridLevel(R"(^([+-])(\d+)%$)");
                const QRegularExpressionMatch match = gridLevel.match(trimmedAnswer);
                if(match.hasMatch()) {
                    const int percent = match.captured(2).toInt();
                    const int value = (match.captured(1) == "+")? (EQUALSHARE + percent) : (EQUALSHARE - percent);
                    if(value >= 0) {
                        *ok = true;
                        return value;
                    }
                }
            }
            return 0;
        }
        // points: a number, possibly with a percent sign
        QString number = trimmedAnswer;
        number.remove('%').remove(',');
        bool isNumber = false;
        const double value = number.trimmed().toDouble(&isNumber);
        if(!isNumber || (value < 0)) {
            return 0;
        }
        *ok = true;
        return value;}
    case Kind::choice:
    case Kind::numberEntry:
    case Kind::written:
        break;
    }
    return 0;
}

double PeerReviewQuestion::normalizedScore(double score) const
{
    if(kind == Kind::likert) {
        return score / double(options.size());
    }
    if(kind == Kind::numberScale) {
        return (score - scaleLow + 1) / double(scaleHigh - scaleLow + 1);
    }
    return score / EQUALSHARE;
}

QString PeerReviewQuestion::scoreDescription(double score) const
{
    if(std::isnan(score)) {
        return QObject::tr("no rating");
    }

    if(kind == Kind::contribution) {
        const int percentAboveEqual = int(std::round(score)) - EQUALSHARE;
        if(percentAboveEqual == 0) {
            return QObject::tr("an equal share");
        }
        return QObject::tr("equal share") + (percentAboveEqual > 0? " + " : " - ") + QString::number(std::abs(percentAboveEqual)) + "%";
    }

    if((kind == Kind::likert) && !options.isEmpty()) {
        // score of 1 is the worst option, score of numOptions is the best
        const QStringList labels = formLabels();
        const int numOptions = int(labels.size());
        auto labelOfScore = [&labels, numOptions, this](int scoreValue) {
            scoreValue = std::clamp(scoreValue, 1, numOptions);
            return labels.at(bestFirst? (numOptions - scoreValue) : (scoreValue - 1));
        };
        const QString scoreText = QString::number(score, 'f', 1);
        if(std::abs(score - std::round(score)) < 0.05) {
            return scoreText + " (" + labelOfScore(int(std::round(score))) + ")";
        }
        return scoreText + " (" + QObject::tr("between") + " " + labelOfScore(int(std::floor(score))) + " " + QObject::tr("and") + " " +
               labelOfScore(int(std::ceil(score))) + ")";
    }

    if(kind == Kind::numberScale) {
        return QString::number(score, 'f', 1) + " " + QObject::tr("on a scale of") + " " + QString::number(scaleLow) + " " +
               QObject::tr("to") + " " + QString::number(scaleHigh);
    }

    return QString::number(score, 'f', 1);
}

QString PeerReviewQuestion::visibilityNote() const
{
    switch(shareWith) {
    case ShareWith::instructor:
        return PEERREVIEWNOTEINSTRUCTOR;
    case ShareWith::personItsAbout:
        if(namesShown) {
            return PEERREVIEWNOTEPERSONNAMED;
        }
        return isScored()? PEERREVIEWNOTEPERSONAVERAGE : PEERREVIEWNOTEPERSON;
    case ShareWith::team:
        if(about == About::self) {
            return PEERREVIEWNOTETEAMNAMED;
        }
        return namesShown? PEERREVIEWNOTETEAMWITHNAMES : PEERREVIEWNOTETEAMANONYMOUS;
    }
    return PEERREVIEWNOTEINSTRUCTOR;
}

QString PeerReviewQuestion::formDescription() const
{
    QStringList parts;
    if((kind == Kind::contribution) && (contributionStyle == ContributionStyle::points)) {
        parts << PEERREVIEWPOINTSNOTE;
    }
    else if((kind == Kind::contribution) && (contributionStyle == ContributionStyle::grid)) {
        parts << PEERREVIEWGRIDNOTE;
    }
    if(about == About::eachTeammate) {
        parts << PEERREVIEWSKIPYOURSELF;
    }
    parts << "(" + visibilityNote() + ")";
    return parts.join("\n");
}

QList<QPair<PeerReviewQuestion::ShareWith, bool>> PeerReviewQuestion::sharingChoices(About about)
{
    if((about == About::eachTeammate) || (about == About::eachTeammateAndSelf)) {
        return {qMakePair(ShareWith::instructor, false), qMakePair(ShareWith::personItsAbout, false), qMakePair(ShareWith::personItsAbout, true)};
    }
    if(about == About::self) {
        return {qMakePair(ShareWith::instructor, false), qMakePair(ShareWith::team, true)};     // an answer about yourself is always yours
    }
    return {qMakePair(ShareWith::instructor, false), qMakePair(ShareWith::team, false), qMakePair(ShareWith::team, true)};
}

QString PeerReviewQuestion::aboutText(About about)
{
    switch(about) {
    case About::eachTeammate:
        return QObject::tr("Each teammate");
    case About::eachTeammateAndSelf:
        return QObject::tr("Each teammate and yourself");
    case About::self:
        return QObject::tr("Yourself");
    case About::once:
        return QObject::tr("Asked once (e.g., about the team)");
    }
    return {};
}

QString PeerReviewQuestion::shareWithText(ShareWith shareWith, About about, bool namesShown)
{
    switch(shareWith) {
    case ShareWith::instructor:
        return QObject::tr("Only the instructor");
    case ShareWith::personItsAbout:
        return namesShown? QObject::tr("The person it's about (with the writer's name)") : QObject::tr("The person it's about (without the writer's name)");
    case ShareWith::team:
        return (namesShown || (about == About::self))? QObject::tr("The whole team (with the writer's name)") :
                                                       QObject::tr("The whole team (without the writer's name)");
    }
    return {};
}

QString PeerReviewQuestion::kindText(Kind kind)
{
    switch(kind) {
    case Kind::likert:
        return QObject::tr("Rating scale (words)");
    case Kind::numberScale:
        return QObject::tr("Number scale");
    case Kind::choice:
        return QObject::tr("Choice (not scored)");
    case Kind::numberEntry:
        return QObject::tr("Number entry");
    case Kind::contribution:
        return QObject::tr("Contribution to the work");
    case Kind::written:
        return QObject::tr("Written");
    }
    return {};
}

const QList<QPair<QString, int>> &PeerReviewQuestion::contributionLevels()
{
    static const QList<QPair<QString, int>> levels = [](){
        QList<QPair<QString, int>> list;
        list << qMakePair(QObject::tr("No contribution"), 0);
        for(int percent = 90; percent >= 10; percent -= 10) {
            list << qMakePair(QObject::tr("Equal share") + " - " + QString::number(percent) + "%", EQUALSHARE - percent);
        }
        list << qMakePair(QObject::tr("Equal share"), EQUALSHARE);
        for(int percent = 10; percent <= 100; percent += 10) {
            list << qMakePair(QObject::tr("Equal share") + " + " + QString::number(percent) + "%", EQUALSHARE + percent);
        }
        list << qMakePair(QString(PEERREVIEWNOTSURE), -1);
        return list;}();
    return levels;
}

const QList<QPair<QString, int>> &PeerReviewQuestion::contributionGridLevels()
{
    // the steps are closer together near an equal share, so that a little more or less than an equal share can be given
    static const QList<QPair<QString, int>> levels = [](){
        const QList<int> percents = {100, 50, 25, 10};
        QList<QPair<QString, int>> list;
        for(const auto percent : percents) {
            list << qMakePair("-" + QString::number(percent) + "%", EQUALSHARE - percent);
        }
        list << qMakePair(QObject::tr("Equal share"), EQUALSHARE);
        for(auto percent = percents.crbegin(); percent != percents.crend(); ++percent) {
            list << qMakePair("+" + QString::number(*percent) + "%", EQUALSHARE + *percent);
        }
        list << qMakePair(QString(PEERREVIEWNOTSURE), -1);
        return list;}();
    return levels;
}

QString PeerReviewQuestion::withoutOrderNumbers(const QString &option)
{
    static const QRegularExpression orderNumber(R"(^\s*\d+\.\s+)");
    QString strippedOption = option;
    return strippedOption.remove(orderNumber);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


QJsonObject PeerReviewStudent::toJson() const
{
    QJsonObject content {
        {"ID", ID},
        {"LMSID", LMSID},
        {"firstname", firstname},
        {"lastname", lastname},
        {"email", email},
        {"section", section}
    };

    return content;
}

PeerReviewStudent PeerReviewStudent::fromJson(const QJsonObject &json)
{
    PeerReviewStudent student;
    student.ID = json["ID"].toInteger(-1);
    student.LMSID = json["LMSID"].toInteger(-1);
    student.firstname = json["firstname"].toString();
    student.lastname = json["lastname"].toString();
    student.email = json["email"].toString();
    student.section = json["section"].toString();
    return student;
}

QJsonObject PeerReviewTeam::toJson() const
{
    QJsonArray membersArray;
    for(const auto &member : members) {
        membersArray.append(member.toJson());
    }

    QJsonObject content {
        {"name", name},
        {"members", membersArray}
    };

    return content;
}

PeerReviewTeam PeerReviewTeam::fromJson(const QJsonObject &json)
{
    PeerReviewTeam team;
    team.name = json["name"].toString();
    const QJsonArray membersArray = json["members"].toArray();
    for(const auto &member : membersArray) {
        team.members << PeerReviewStudent::fromJson(member.toObject());
    }
    return team;
}

QJsonObject PeerReviewThresholds::toJson() const
{
    QJsonObject content {
        {"lowContributor", lowContributor},
        {"highContributor", highContributor},
        {"selfGapPercent", selfGapPercent},
        {"disagreementPercent", disagreementPercent},
        {"capFactor", capFactor},
        {"factorCap", factorCap},
        {"factorFromAllQuestions", factorFromAllQuestions}
    };

    return content;
}

PeerReviewThresholds PeerReviewThresholds::fromJson(const QJsonObject &json)
{
    PeerReviewThresholds thresholds;
    thresholds.lowContributor = json["lowContributor"].toDouble(thresholds.lowContributor);
    thresholds.highContributor = json["highContributor"].toDouble(thresholds.highContributor);
    thresholds.selfGapPercent = json["selfGapPercent"].toInt(thresholds.selfGapPercent);
    thresholds.disagreementPercent = json["disagreementPercent"].toInt(thresholds.disagreementPercent);
    thresholds.capFactor = json["capFactor"].toBool(thresholds.capFactor);
    thresholds.factorCap = json["factorCap"].toDouble(thresholds.factorCap);
    thresholds.factorFromAllQuestions = json["factorFromAllQuestions"].toBool(thresholds.factorFromAllQuestions);
    return thresholds;
}

QJsonObject PeerReviewRelease::toJson() const
{
    QJsonArray heldBackArray, minimumOverriddenArray, personalNotesArray, hiddenCommentsArray, logArray;
    for(const auto ID : heldBack) {
        heldBackArray.append(ID);
    }
    for(const auto ID : minimumOverridden) {
        minimumOverriddenArray.append(ID);
    }
    for(auto note = personalNotes.cbegin(); note != personalNotes.cend(); ++note) {
        personalNotesArray.append(QJsonObject{{"ID", note.key()}, {"note", note.value()}});
    }
    QJsonObject editedCommentsObject;
    for(auto comment = editedComments.cbegin(); comment != editedComments.cend(); ++comment) {
        editedCommentsObject[comment.key()] = comment.value();
    }
    for(const auto &key : hiddenComments) {
        hiddenCommentsArray.append(key);
    }
    for(const auto &entry : log) {
        logArray.append(QJsonObject{{"time", entry.time.toString(Qt::ISODate)}, {"studentID", entry.studentID},
                                    {"method", entry.method}, {"detail", entry.detail}});
    }

    QJsonObject content {
        {"introText", introText},
        {"minimumResponders", minimumResponders},
        {"includeTeamAverage", includeTeamAverage},
        {"includeAdjustmentFactor", includeAdjustmentFactor},
        {"heldBack", heldBackArray},
        {"minimumOverridden", minimumOverriddenArray},
        {"personalNotes", personalNotesArray},
        {"editedComments", editedCommentsObject},
        {"hiddenComments", hiddenCommentsArray},
        {"log", logArray}
    };

    return content;
}

PeerReviewRelease PeerReviewRelease::fromJson(const QJsonObject &json)
{
    PeerReviewRelease release;
    release.introText = json["introText"].toString(PEERREVIEWREPORTINTRO);
    release.minimumResponders = json["minimumResponders"].toInt(release.minimumResponders);
    release.includeTeamAverage = json["includeTeamAverage"].toBool(false);
    release.includeAdjustmentFactor = json["includeAdjustmentFactor"].toBool(false);
    const QJsonArray heldBackArray = json["heldBack"].toArray();
    for(const auto &ID : heldBackArray) {
        release.heldBack.insert(ID.toInteger());
    }
    const QJsonArray minimumOverriddenArray = json["minimumOverridden"].toArray();
    for(const auto &ID : minimumOverriddenArray) {
        release.minimumOverridden.insert(ID.toInteger());
    }
    const QJsonArray personalNotesArray = json["personalNotes"].toArray();
    for(const auto &note : personalNotesArray) {
        release.personalNotes.insert(note.toObject()["ID"].toInteger(), note.toObject()["note"].toString());
    }
    const QJsonObject editedCommentsObject = json["editedComments"].toObject();
    for(auto comment = editedCommentsObject.constBegin(); comment != editedCommentsObject.constEnd(); ++comment) {
        release.editedComments.insert(comment.key(), comment.value().toString());
    }
    const QJsonArray hiddenCommentsArray = json["hiddenComments"].toArray();
    for(const auto &key : hiddenCommentsArray) {
        release.hiddenComments.insert(key.toString());
    }
    const QJsonArray logArray = json["log"].toArray();
    for(const auto &entry : logArray) {
        const QJsonObject entryObject = entry.toObject();
        release.log.append({QDateTime::fromString(entryObject["time"].toString(), Qt::ISODate), entryObject["studentID"].toInteger(-1),
                            entryObject["method"].toString(), entryObject["detail"].toString()});
    }
    return release;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// One peer review: saving and loading
/////////////////////////////////////////////////////////////////////////////////////////////////////////

QJsonObject PeerReview::toJson() const
{
    QJsonArray teamsArray, questionsArray, formItemsArray;
    for(const auto &team : teams) {
        teamsArray.append(team.toJson());
    }
    for(const auto &question : questions) {
        questionsArray.append(question.toJson());
    }
    for(const auto &formItem : formItems) {
        formItemsArray.append(QJsonObject{{"id", formItem.itemID}, {"question", formItem.questionNum},
                                          {"team", formItem.teamNum}, {"about", formItem.aboutID}});
    }
    QJsonArray canvasQuizzesArray;
    for(const auto &canvasQuiz : canvasQuizzes) {
        canvasQuizzesArray.append(QJsonObject{{"quiz", canvasQuiz.quizID}, {"assignment", canvasQuiz.assignmentID},
                                              {"override", canvasQuiz.overrideID}, {"url", canvasQuiz.url}});
    }
    QJsonObject canvasAnswerLabelsObject;
    for(auto label = canvasAnswerLabels.constBegin(); label != canvasAnswerLabels.constEnd(); ++label) {
        canvasAnswerLabelsObject[label.key()] = label.value();
    }
    QJsonArray remindersArray;
    for(const auto &reminder : reminders) {
        remindersArray.append(QJsonObject{{"time", reminder.time.toString(Qt::ISODate)}, {"studentID", reminder.studentID}});
    }

    QJsonObject content {
        {"fileType", "gruepr peer review"},
        {"title", title},
        {"createdTime", createdTime.toString(Qt::ISODate)},
        {"teamsSource", teamsSource},
        {"teams", teamsArray},
        {"questions", questionsArray},
        {"instructions", instructions},
        {"requireSignIn", requireSignIn},
        {"platform", (platform == Platform::canvasQuizzes)? "canvas" : "google"},
        {"canvas", QJsonObject{{"courseID", canvasCourseID}, {"courseName", canvasCourseName}, {"courseURL", canvasCourseURL},
                               {"assignmentGroupID", canvasAssignmentGroupID}, {"countTowardFinalGrade", countTowardFinalGrade},
                               {"quizzes", canvasQuizzesArray}, {"answerLabels", canvasAnswerLabelsObject}}},
        {"formID", formID},
        {"responderURL", responderURL},
        {"accountName", accountName},
        {"formPublished", formPublished},
        {"acceptingResponses", acceptingResponses},
        {"nameQuestionID", nameQuestionID},
        {"formItems", formItemsArray},
        {"lastResponses", lastResponses},
        {"lastResponsesTime", lastResponsesTime.toString(Qt::ISODate)},
        {"reminders", remindersArray},
        {"thresholds", thresholds.toJson()},
        {"release", release.toJson()}
    };

    return content;
}

PeerReview PeerReview::fromJson(const QJsonObject &json)
{
    PeerReview peerReview;
    peerReview.title = json["title"].toString();
    peerReview.createdTime = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    peerReview.teamsSource = json["teamsSource"].toString();
    const QJsonArray teamsArray = json["teams"].toArray();
    for(const auto &team : teamsArray) {
        peerReview.teams << PeerReviewTeam::fromJson(team.toObject());
    }
    const QJsonArray questionsArray = json["questions"].toArray();
    for(const auto &question : questionsArray) {
        peerReview.questions << PeerReviewQuestion::fromJson(question.toObject());
    }
    peerReview.instructions = json["instructions"].toString(PEERREVIEWINSTRUCTIONS);
    peerReview.requireSignIn = json["requireSignIn"].toBool(false);
    peerReview.platform = (json["platform"].toString() == "canvas")? Platform::canvasQuizzes : Platform::googleForm;
    const QJsonObject canvasObject = json["canvas"].toObject();
    peerReview.canvasCourseID = canvasObject["courseID"].toInteger(-1);
    peerReview.canvasCourseName = canvasObject["courseName"].toString();
    peerReview.canvasCourseURL = canvasObject["courseURL"].toString();
    peerReview.canvasAssignmentGroupID = canvasObject["assignmentGroupID"].toInteger(-1);
    peerReview.countTowardFinalGrade = canvasObject["countTowardFinalGrade"].toBool(false);
    const QJsonArray canvasQuizzesArray = canvasObject["quizzes"].toArray();
    for(const auto &canvasQuiz : canvasQuizzesArray) {
        const QJsonObject canvasQuizObject = canvasQuiz.toObject();
        peerReview.canvasQuizzes.append({canvasQuizObject["quiz"].toInteger(-1), canvasQuizObject["assignment"].toInteger(-1),
                                         canvasQuizObject["override"].toInteger(-1), canvasQuizObject["url"].toString()});
    }
    const QJsonObject canvasAnswerLabelsObject = canvasObject["answerLabels"].toObject();
    for(auto label = canvasAnswerLabelsObject.constBegin(); label != canvasAnswerLabelsObject.constEnd(); ++label) {
        peerReview.canvasAnswerLabels.insert(label.key(), label.value().toString());
    }
    peerReview.formID = json["formID"].toString();
    peerReview.responderURL = json["responderURL"].toString();
    peerReview.accountName = json["accountName"].toString();
    peerReview.formPublished = json["formPublished"].toBool(false);
    peerReview.acceptingResponses = json["acceptingResponses"].toBool(true);
    peerReview.nameQuestionID = json["nameQuestionID"].toString();
    const QJsonArray formItemsArray = json["formItems"].toArray();
    for(const auto &formItem : formItemsArray) {
        const QJsonObject formItemObject = formItem.toObject();
        peerReview.formItems.append({formItemObject["id"].toString(), formItemObject["question"].toInt(-1),
                                     formItemObject["team"].toInt(-1), formItemObject["about"].toInteger(PeerReviewFormItem::ABOUTNOONE)});
    }
    peerReview.lastResponses = json["lastResponses"].toArray();
    peerReview.lastResponsesTime = QDateTime::fromString(json["lastResponsesTime"].toString(), Qt::ISODate);
    const QJsonArray remindersArray = json["reminders"].toArray();     // (empty in a peer review saved before reminders were recorded)
    for(const auto &reminder : remindersArray) {
        const QJsonObject reminderObject = reminder.toObject();
        peerReview.reminders.append({QDateTime::fromString(reminderObject["time"].toString(), Qt::ISODate), reminderObject["studentID"].toInteger(-1)});
    }
    peerReview.thresholds = PeerReviewThresholds::fromJson(json["thresholds"].toObject());
    peerReview.release = PeerReviewRelease::fromJson(json["release"].toObject());
    return peerReview;
}

bool PeerReview::save()
{
    if(fileName.isEmpty()) {
        // named for when it's first saved; if another peer review was first saved in the same second, a number is added, so neither is overwritten
        const QString baseName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                 "/peerReviews/" + QString::number(QDateTime::currentSecsSinceEpoch());
        fileName = baseName + ".gpr";
        for(int copyNum = 2; QFileInfo::exists(fileName); copyNum++) {
            fileName = baseName + "-" + QString::number(copyNum) + ".gpr";
        }
        const QDir dir(QFileInfo(fileName).absoluteDir());
        if(!dir.exists()) {
            dir.mkpath(".");
        }
    }

    QSaveFile saveFile(fileName);
    if(!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    saveFile.write(QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
    if(!saveFile.commit()) {
        return false;
    }

    // add or update this peer review in the saved list, keeping the others as they are
    QSettings savedSettings;
    QList<std::tuple<QString, QString, QString, int, bool, bool>> storedReviews;     // name, fileName, createdTime, numStudents, hasForm, onCanvas
    const int numStored = savedSettings.beginReadArray("peerReviews");
    for(int stored = 0; stored < numStored; stored++) {
        savedSettings.setArrayIndex(stored);
        const QString storedFileName = savedSettings.value("peerReviewFile", "").toString();
        if(storedFileName.compare(fileName, Qt::CaseInsensitive) != 0) {
            storedReviews.emplaceBack(savedSettings.value("peerReviewName", "").toString(), storedFileName,
                                      savedSettings.value("peerReviewCreated", "").toString(), savedSettings.value("peerReviewNumStudents", 0).toInt(),
                                      savedSettings.value("peerReviewHasForm", false).toBool(), savedSettings.value("peerReviewOnCanvas", false).toBool());
        }
    }
    savedSettings.endArray();
    storedReviews.emplaceBack(title, fileName, createdTime.toString(Qt::ISODate), numStudents(), hasForm(), platform == Platform::canvasQuizzes);

    savedSettings.remove("peerReviews");
    savedSettings.beginWriteArray("peerReviews", int(storedReviews.size()));
    for(int stored = 0; stored < storedReviews.size(); stored++) {
        savedSettings.setArrayIndex(stored);
        savedSettings.setValue("peerReviewName", std::get<0>(storedReviews.at(stored)));
        savedSettings.setValue("peerReviewFile", std::get<1>(storedReviews.at(stored)));
        savedSettings.setValue("peerReviewCreated", std::get<2>(storedReviews.at(stored)));
        savedSettings.setValue("peerReviewNumStudents", std::get<3>(storedReviews.at(stored)));
        savedSettings.setValue("peerReviewHasForm", std::get<4>(storedReviews.at(stored)));
        savedSettings.setValue("peerReviewOnCanvas", std::get<5>(storedReviews.at(stored)));
    }
    savedSettings.endArray();

    return true;
}

bool PeerReview::hasForm() const
{
    if(platform == Platform::canvasQuizzes) {
        return std::any_of(canvasQuizzes.cbegin(), canvasQuizzes.cend(), [](const CanvasTeamQuiz &canvasQuiz){return canvasQuiz.quizID != -1;});
    }
    return !formID.isEmpty();
}

bool PeerReview::load(const QString &fileName, PeerReview &peerReview)
{
    QFile loadFile(fileName);
    if(!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    const QJsonDocument loadDoc = QJsonDocument::fromJson(loadFile.readAll());
    loadFile.close();
    if(!loadDoc.isObject() || (loadDoc.object()["fileType"].toString() != "gruepr peer review")) {
        return false;
    }

    peerReview = fromJson(loadDoc.object());
    peerReview.fileName = fileName;
    return true;
}

QList<PeerReview::StoredPeerReview> PeerReview::storedPeerReviews()
{
    QList<StoredPeerReview> storedReviews;
    QStringList missingFiles;   // listed peer reviews whose file no longer exists, to be removed from the list
    QSettings savedSettings;
    const int numStored = savedSettings.beginReadArray("peerReviews");
    for(int stored = 0; stored < numStored; stored++) {
        savedSettings.setArrayIndex(stored);
        const QString storedFileName = savedSettings.value("peerReviewFile", "").toString();
        if(storedFileName.isEmpty()) {
            continue;
        }
        if(!QFileInfo::exists(storedFileName)) {
            missingFiles << storedFileName;
            continue;
        }
        storedReviews.append({savedSettings.value("peerReviewName", "").toString(), storedFileName,
                              QDateTime::fromString(savedSettings.value("peerReviewCreated", "").toString(), Qt::ISODate),
                              savedSettings.value("peerReviewNumStudents", 0).toInt(), savedSettings.value("peerReviewHasForm", false).toBool(),
                              savedSettings.value("peerReviewOnCanvas", false).toBool()});
    }
    savedSettings.endArray();

    for(const auto &missingFile : std::as_const(missingFiles)) {
        deleteStored(missingFile);
    }

    std::sort(storedReviews.begin(), storedReviews.end(), [](const StoredPeerReview &a, const StoredPeerReview &b)
                                                          {return a.createdTime > b.createdTime;});    // newest first
    return storedReviews;
}

bool PeerReview::deleteStored(const QString &fileName)
{
    if(QFileInfo::exists(fileName) && !QFile::remove(fileName)) {
        return false;
    }

    // rewrite the saved list, leaving out this file
    QSettings savedSettings;
    QList<std::tuple<QString, QString, QString, int, bool, bool>> keptReviews;     // name, fileName, createdTime, numStudents, hasForm, onCanvas
    const int numStored = savedSettings.beginReadArray("peerReviews");
    for(int stored = 0; stored < numStored; stored++) {
        savedSettings.setArrayIndex(stored);
        const QString storedFileName = savedSettings.value("peerReviewFile", "").toString();
        if(storedFileName.compare(fileName, Qt::CaseInsensitive) != 0) {
            keptReviews.emplaceBack(savedSettings.value("peerReviewName", "").toString(), storedFileName,
                                    savedSettings.value("peerReviewCreated", "").toString(), savedSettings.value("peerReviewNumStudents", 0).toInt(),
                                    savedSettings.value("peerReviewHasForm", false).toBool(), savedSettings.value("peerReviewOnCanvas", false).toBool());
        }
    }
    savedSettings.endArray();

    savedSettings.remove("peerReviews");
    savedSettings.beginWriteArray("peerReviews", int(keptReviews.size()));
    for(int stored = 0; stored < keptReviews.size(); stored++) {
        savedSettings.setArrayIndex(stored);
        savedSettings.setValue("peerReviewName", std::get<0>(keptReviews.at(stored)));
        savedSettings.setValue("peerReviewFile", std::get<1>(keptReviews.at(stored)));
        savedSettings.setValue("peerReviewCreated", std::get<2>(keptReviews.at(stored)));
        savedSettings.setValue("peerReviewNumStudents", std::get<3>(keptReviews.at(stored)));
        savedSettings.setValue("peerReviewHasForm", std::get<4>(keptReviews.at(stored)));
        savedSettings.setValue("peerReviewOnCanvas", std::get<5>(keptReviews.at(stored)));
    }
    savedSettings.endArray();

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Getting the teams: from a saved gruepr session or from a spreadsheet
/////////////////////////////////////////////////////////////////////////////////////////////////////////

QList<QPair<QString, QString>> PeerReview::savedSessions()
{
    QList<std::tuple<QDateTime, QString, QString>> sessions;     // lastModifiedDate, displayName, fileName
    QSettings savedSettings;
    const int numPrevWorks = savedSettings.beginReadArray("prevWorks");
    for(int prevWork = 0; prevWork < numPrevWorks; prevWork++) {
        savedSettings.setArrayIndex(prevWork);
        const QString displayName = savedSettings.value("prevWorkName", "").toString();
        const QDateTime lastModifiedDate = QDateTime::fromString(savedSettings.value("prevWorkDate", "").toString(),
                                                                 QLocale::system().dateTimeFormat(QLocale::LongFormat));
        const QString fileName = savedSettings.value("prevWorkFile", "").toString();
        if(!displayName.isEmpty() && !fileName.isEmpty() && QFileInfo::exists(fileName)) {
            sessions.emplaceBack(lastModifiedDate, displayName + " [" + QObject::tr("Last opened") + ": " +
                                                       lastModifiedDate.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)) + "]",
                                 fileName);
        }
    }
    savedSettings.endArray();

    std::sort(sessions.begin(), sessions.end(), [](const std::tuple<QDateTime, QString, QString> &a, const std::tuple<QDateTime, QString, QString> &b)
                                                {return std::get<0>(a) > std::get<0>(b);});   // sort by last modified date descending
    QList<QPair<QString, QString>> result;
    result.reserve(sessions.size());
    for(const auto &session : sessions) {
        result << qMakePair(std::get<1>(session), std::get<2>(session));
    }
    return result;
}

QList<PeerReview::SavedSessionTeamSet> PeerReview::teamSetsInSavedSession(const QString &fileName)
{
    QFile savedFile(fileName);
    if(!savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
    savedFile.close();

    // each teams tab in the session saved its own copy of the students, and its teams list each member by student ID;
    // only the few fields needed here are read from each student, rather than their whole record, since sessions can be large
    QList<SavedSessionTeamSet> teamSets;
    const QJsonArray teamsetjsons = doc.object()["teamsets"].toArray();
    for(const auto &teamsetjson : teamsetjsons) {
        const QJsonObject teamset = teamsetjson.toObject();
        QHash<long long, PeerReviewStudent> studentsInSet;
        QList<long long> studentIDsInOrder;      // for older saved sessions, whose teams list their students by place in this list
        QSet<long long> removedStudents;
        const QJsonArray studentsArray = teamset["students"].toArray();
        for(const auto &studentjson : studentsArray) {
            const QJsonObject studentObject = studentjson.toObject();
            const PeerReviewStudent student = {studentObject["ID"].toInteger(), studentObject["LMSID"].toInteger(-1),
                                               studentObject["firstname"].toString().trimmed(), studentObject["lastname"].toString().trimmed(),
                                               studentObject["email"].toString().trimmed(), studentObject["section"].toString()};
            studentsInSet.insert(student.ID, student);
            studentIDsInOrder << student.ID;
            if(studentObject["deleted"].toBool()) {
                removedStudents.insert(student.ID);
            }
        }

        SavedSessionTeamSet teamSet;
        teamSet.name = teamset["tabName"].toString();
        const QJsonArray teamsArray = teamset["teams"].toArray();
        for(const auto &teamjson : teamsArray) {
            const QJsonObject teamObject = teamjson.toObject();
            PeerReviewTeam team;
            team.name = teamObject["name"].toString();
            QList<long long> memberIDs;
            if(teamObject.contains("studentIDs")) {
                const QJsonArray studentIDsArray = teamObject["studentIDs"].toArray();
                for(const auto &studentID : studentIDsArray) {
                    memberIDs << studentID.toInteger();
                }
            }
            else {
                const QJsonArray studentIndexesArray = teamObject["studentIndexes"].toArray();
                for(const auto &studentIndex : studentIndexesArray) {
                    memberIDs << studentIDsInOrder.value(studentIndex.toInt(), -1);
                }
            }
            for(const auto memberID : std::as_const(memberIDs)) {
                const auto student = studentsInSet.constFind(memberID);
                if((student == studentsInSet.constEnd()) || removedStudents.contains(memberID)) {
                    continue;   // removed students aren't asked for or about
                }
                team.members << student.value();
            }
            if(!team.members.isEmpty()) {
                teamSet.teams << team;
            }
        }
        if(!teamSet.teams.isEmpty()) {
            teamSets << teamSet;
        }
    }
    return teamSets;
}

QList<PeerReviewTeam> PeerReview::teamsFromSpreadsheetRows(const QList<QStringList> &rows)
{
    // rows are {team name, first name, last name, email}; teams are kept in the order each first appears
    QList<PeerReviewTeam> teamsFromRows;
    long long nextID = 0;
    for(const auto &row : rows) {
        if(row.size() < 4) {
            continue;
        }
        int teamNum = 0;
        while((teamNum < teamsFromRows.size()) && (teamsFromRows.at(teamNum).name != row.at(0))) {
            teamNum++;
        }
        if(teamNum == teamsFromRows.size()) {
            teamsFromRows.append({row.at(0), {}});
        }
        teamsFromRows[teamNum].members.append({nextID++, -1, row.at(1), row.at(2), row.at(3), ""});
    }
    return teamsFromRows;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Question templates, sample questions, and question set files
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// helpers to build the built-in questions briefly
static PeerReviewQuestion likertQuestion(const QString &text, const QString &options, PeerReviewQuestion::About about,
                                         PeerReviewQuestion::ShareWith shareWith)
{
    PeerReviewQuestion question;
    question.kind = PeerReviewQuestion::Kind::likert;
    question.text = text;
    question.options = options.split(" / ");
    question.about = about;
    question.shareWith = shareWith;
    return question;
}

static PeerReviewQuestion writtenQuestion(const QString &text, PeerReviewQuestion::About about, PeerReviewQuestion::ShareWith shareWith)
{
    PeerReviewQuestion question;
    question.kind = PeerReviewQuestion::Kind::written;
    question.text = text;
    question.about = about;
    question.shareWith = shareWith;
    question.namesShown = (about == PeerReviewQuestion::About::self) && (shareWith == PeerReviewQuestion::ShareWith::team);
    question.required = false;
    return question;
}

static const char ALWAYSTONEVER[] = "1. Always / 2. Usually / 3. Sometimes / 4. Rarely / 5. Never";

QList<PeerReviewQuestion> PeerReview::defaultQuestions()
{
    using About = PeerReviewQuestion::About;
    using ShareWith = PeerReviewQuestion::ShareWith;
    return {likertQuestion(QObject::tr("Contributed a fair share of the work"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout),
            likertQuestion(QObject::tr("Communicated clearly and respectfully"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout),
            likertQuestion(QObject::tr("Kept the team on track"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout),
            likertQuestion(QObject::tr("Produced high-quality work"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout),
            likertQuestion(QObject::tr("Brought useful knowledge and skills"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout)};
}

QList<PeerReviewQuestion> PeerReview::contributionBasedQuestions()
{
    using About = PeerReviewQuestion::About;
    using ShareWith = PeerReviewQuestion::ShareWith;
    PeerReviewQuestion contribution;
    contribution.kind = PeerReviewQuestion::Kind::contribution;
    contribution.text = QObject::tr("How much of the team's work did each person, including you, do?");
    contribution.about = About::eachTeammateAndSelf;
    contribution.shareWith = ShareWith::personItsAbout;

    return {contribution,
            writtenQuestion(QObject::tr("What have you contributed to the team so far?"), About::self, ShareWith::team),
            writtenQuestion(QObject::tr("What would you like your instructor to know about each teammate's work?"), About::eachTeammate, ShareWith::instructor),
            writtenQuestion(QObject::tr("How is your team working together so far?"), About::once, ShareWith::instructor),
            writtenQuestion(QObject::tr("What is one thing each teammate does well, and one thing that would help the team even more?"),
                            About::eachTeammate, ShareWith::personItsAbout)};
}

QList<PeerReviewQuestion> PeerReview::sampleQuestions()
{
    using Kind = PeerReviewQuestion::Kind;
    using About = PeerReviewQuestion::About;
    using ShareWith = PeerReviewQuestion::ShareWith;
    static const char EXPECTATIONS[] = "1. Far above expectations / 2. Above expectations / 3. Met expectations / 4. Below expectations / 5. Far below expectations";
    static const char EXCELLENTTOPOOR[] = "1. Excellent / 2. Very good / 3. Good / 4. Fair / 5. Poor";

    QList<PeerReviewQuestion> samples;

    // rated
    samples << likertQuestion(QObject::tr("Came to meetings prepared"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Finished their part of the work on time"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Listened to and built on others' ideas"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Helped the team work through disagreements"), ALWAYSTONEVER, About::eachTeammateAndSelf, ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Asked for help when stuck, and helped others who were stuck"), ALWAYSTONEVER, About::eachTeammateAndSelf,
                              ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Overall, as a teammate, this person's work was:"), EXPECTATIONS, About::eachTeammateAndSelf, ShareWith::personItsAbout);
    samples << likertQuestion(QObject::tr("Overall, how well is your team working together?"), EXCELLENTTOPOOR, About::once, ShareWith::instructor);

    PeerReviewQuestion numberScale;
    numberScale.kind = Kind::numberScale;
    numberScale.text = QObject::tr("Overall quality of this person's work");
    numberScale.scaleLow = 1;
    numberScale.scaleHigh = 10;
    numberScale.lowLabel = QObject::tr("Poor");
    numberScale.highLabel = QObject::tr("Excellent");
    numberScale.about = About::eachTeammateAndSelf;
    numberScale.shareWith = ShareWith::personItsAbout;
    samples << numberScale;

    // contribution
    PeerReviewQuestion contribution;
    contribution.kind = Kind::contribution;
    contribution.text = QObject::tr("How much of the team's work did each person, including you, do?");
    contribution.shareWith = ShareWith::personItsAbout;
    samples << contribution;
    contribution.contributionStyle = PeerReviewQuestion::ContributionStyle::points;
    contribution.text = QObject::tr("How many points does each person, including you, get for their share of the work?");
    contribution.shareWith = ShareWith::instructor;
    samples << contribution;

    // choice, not scored
    PeerReviewQuestion choice;
    choice.kind = Kind::choice;
    choice.text = QObject::tr("Which role(s) did this person play on the team?");
    choice.options = QString("1. Leader / 2. Organizer / 3. Researcher / 4. Builder / 5. Writer / 6. Presenter / 7. Supporter").split(" / ");
    choice.multi = true;
    choice.about = About::eachTeammateAndSelf;
    choice.shareWith = ShareWith::personItsAbout;
    samples << choice;
    choice.text = QObject::tr("Would you want to work with this person again?");
    choice.options = QString("1. Yes / 2. Maybe / 3. No").split(" / ");
    choice.multi = false;
    choice.about = About::eachTeammate;
    choice.shareWith = ShareWith::instructor;
    samples << choice;
    choice.text = QObject::tr("The amount of work for this project so far has been:");
    choice.options = QString("1. Too much / 2. About right / 3. Too little").split(" / ");
    choice.about = About::once;
    samples << choice;

    // number entry
    PeerReviewQuestion numberEntry;
    numberEntry.kind = Kind::numberEntry;
    numberEntry.text = QObject::tr("About how many hours per week did this person spend on the project?");
    numberEntry.about = About::eachTeammateAndSelf;
    numberEntry.shareWith = ShareWith::instructor;
    samples << numberEntry;

    // written
    samples << writtenQuestion(QObject::tr("What is one thing each teammate does well?"), About::eachTeammate, ShareWith::personItsAbout);
    samples << writtenQuestion(QObject::tr("What is one thing each teammate could do to help the team more?"), About::eachTeammate, ShareWith::personItsAbout);
    samples << writtenQuestion(QObject::tr("Anything else your instructor should know about each teammate?"), About::eachTeammate, ShareWith::instructor);
    samples << writtenQuestion(QObject::tr("What have you contributed to the team so far?"), About::self, ShareWith::team);
    samples << writtenQuestion(QObject::tr("What will you do differently in the next part of the project?"), About::self, ShareWith::instructor);
    samples << writtenQuestion(QObject::tr("How is your team working together so far?"), About::once, ShareWith::instructor);
    samples << writtenQuestion(QObject::tr("What is your team doing well, and what should it change?"), About::once, ShareWith::team);

    return samples;
}

bool PeerReview::saveQuestionSet(const QString &fileName, const QList<PeerReviewQuestion> &questionSet)
{
    QJsonArray questionsArray;
    for(const auto &question : questionSet) {
        questionsArray.append(question.toJson());
    }
    const QJsonObject content {
        {"fileType", "gruepr peer review questions"},
        {"questions", questionsArray}
    };

    QSaveFile saveFile(fileName);
    if(!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    saveFile.write(QJsonDocument(content).toJson());
    return saveFile.commit();
}

bool PeerReview::loadQuestionSet(const QString &fileName, QList<PeerReviewQuestion> &questionSet)
{
    QFile loadFile(fileName);
    if(!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    const QJsonDocument loadDoc = QJsonDocument::fromJson(loadFile.readAll());
    loadFile.close();
    if(!loadDoc.isObject() || (loadDoc.object()["fileType"].toString() != "gruepr peer review questions")) {
        return false;
    }

    QList<PeerReviewQuestion> newQuestionSet;
    const QJsonArray questionsArray = loadDoc.object()["questions"].toArray();
    for(const auto &question : questionsArray) {
        newQuestionSet << PeerReviewQuestion::fromJson(question.toObject());
    }
    questionSet = newQuestionSet;
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Finding students, and reading the responses
/////////////////////////////////////////////////////////////////////////////////////////////////////////

int PeerReview::numStudents() const
{
    int count = 0;
    for(const auto &team : teams) {
        count += int(team.members.size());
    }
    return count;
}

const PeerReviewStudent *PeerReview::student(const long long studentID) const
{
    for(const auto &team : teams) {
        for(const auto &member : team.members) {
            if(member.ID == studentID) {
                return &member;
            }
        }
    }
    return nullptr;
}

int PeerReview::teamOf(const long long studentID) const
{
    for(int teamNum = 0; teamNum < teams.size(); teamNum++) {
        for(const auto &member : teams.at(teamNum).members) {
            if(member.ID == studentID) {
                return teamNum;
            }
        }
    }
    return -1;
}

QList<QPair<QString, long long>> PeerReview::nameOptions() const
{
    QHash<QString, int> timesNameUsed;
    for(const auto &team : teams) {
        for(const auto &member : team.members) {
            timesNameUsed[member.fullName().toLower()]++;
        }
    }

    QList<QPair<QString, long long>> options;
    for(int teamNum = 0; teamNum < teams.size(); teamNum++) {
        const auto &members = teams.at(teamNum).members;
        for(int memberNum = 0; memberNum < members.size(); memberNum++) {
            const QString name = members.at(memberNum).fullName();
            QStringList additions;      // to tell apart students with the same name: their team and, if on the same team, a number
            if(timesNameUsed.value(name.toLower()) > 1) {
                additions << teams.at(teamNum).name;
            }
            const int sameNameNumber = sameNameNumberOnTeam(teamNum, memberNum);
            if(sameNameNumber > 0) {
                additions << QString::number(sameNameNumber);
            }
            options << qMakePair(additions.isEmpty()? name : (name + " (" + additions.join(", ") + ")"), members.at(memberNum).ID);
        }
    }
    std::sort(options.begin(), options.end(), [](const QPair<QString, long long> &a, const QPair<QString, long long> &b)
                                              {return QString::localeAwareCompare(a.first, b.first) < 0;});
    return options;
}

QList<PeerReviewResponse> PeerReview::parseResponses(const QJsonArray &responsesJson) const
{
    if(platform == Platform::canvasQuizzes) {
        return parseCanvasSubmissions(responsesJson);
    }

    QHash<QString, long long> studentOfName;
    for(const auto &nameOption : nameOptions()) {
        studentOfName.insert(nameOption.first.simplified(), nameOption.second);
    }

    QList<PeerReviewResponse> responses;
    responses.reserve(responsesJson.size());
    for(const auto &responseJson : responsesJson) {
        const QJsonObject responseObject = responseJson.toObject();
        PeerReviewResponse response;
        response.responseID = responseObject["responseId"].toString();
        // Google gives the time to the nanosecond, so keep just the milliseconds
        static const QRegularExpression extraDigits(R"((\.\d{3})\d+)");
        response.submittedTime = QDateTime::fromString(responseObject["lastSubmittedTime"].toString().replace(extraDigits, "\\1"), Qt::ISODateWithMs);
        response.respondentEmail = responseObject["respondentEmail"].toString();

        //pull out the answer(s) to each question
        const QJsonObject answers = responseObject["answers"].toObject();
        for(auto answer = answers.constBegin(); answer != answers.constEnd(); ++answer) {
            const QJsonArray nestedAnswers = answer.value().toObject()["textAnswers"].toObject()["answers"].toArray();
            QStringList values;
            values.reserve(nestedAnswers.size());
            for(const auto &nestedAnswer : nestedAnswers) {
                values << nestedAnswer.toObject()["value"].toString();
            }
            response.answers.insert(answer.key(), values);
        }

        response.nameChosen = response.answers.value(nameQuestionID).value(0).simplified();
        response.giverID = studentOfName.value(response.nameChosen, -1);
        response.enteredByInstructor = !response.respondentEmail.isEmpty() && !accountName.isEmpty() &&
                                       (response.respondentEmail.compare(accountName, Qt::CaseInsensitive) == 0);
        responses << response;
    }

    // oldest first, and only the latest response under each name is used
    std::sort(responses.begin(), responses.end(), [](const PeerReviewResponse &a, const PeerReviewResponse &b)
                                                  {return a.submittedTime < b.submittedTime;});
    QSet<long long> giversSeen;
    for(auto response = responses.rbegin(); response != responses.rend(); ++response) {
        if(response->giverID == -1) {
            continue;
        }
        response->superseded = giversSeen.contains(response->giverID);
        giversSeen.insert(response->giverID);
    }

    return responses;
}

QList<PeerReviewResponse> PeerReview::parseCanvasSubmissions(const QJsonArray &submissionsJson) const
{
    QHash<long long, long long> studentOfCanvasID;
    for(const auto &team : teams) {
        for(const auto &member : team.members) {
            if(member.LMSID > 0) {
                studentOfCanvasID.insert(member.LMSID, member.ID);
            }
        }
    }
    QHash<QString, int> questionOfCanvasQuestion;       // the ID Canvas gave a question -> the peer review question it asks
    for(const auto &formItem : formItems) {
        questionOfCanvasQuestion.insert(formItem.itemID.section('/', 0, 0), formItem.questionNum);
    }

    // the text of a written answer: Canvas gives it as html, which may also have a school's own stylesheet and script added to it
    auto plainText = [](QString html) {
        static const QRegularExpression addedCode(R"(<script\b[^>]*>.*?</script>|<style\b[^>]*>.*?</style>|<link\b[^>]*>)",
                                                  QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        html.remove(addedCode);
        return QTextDocumentFragment::fromHtml(html).toPlainText().trimmed();
    };
    // the option a Canvas option ID stands for in a question (empty if none); Canvas's option IDs are unique only within a question
    // (a test peer review made before this was keyed by option ID alone, which is still read)
    auto labelOf = [this](const QString &questionID, const QJsonValue &optionID) {
        const QString option = QString::number(optionID.toVariant().toLongLong());
        return canvasAnswerLabels.value(questionID + ":" + option, canvasAnswerLabels.value(option));
    };
    static const QRegularExpression dropdownAnswer(R"(^answer_id_for_(\w+)$)");
    static const QRegularExpression blankAnswer(R"(^answer_for_(\w+)$)");
    static const QRegularExpression checkedAnswer(R"(^answer_(\d+)$)");

    // each attempt at a quiz is a response, so a student who took their quiz more than once has earlier responses that are superseded
    QList<PeerReviewResponse> responses;
    for(const auto &submissionJson : submissionsJson) {
        const QJsonObject submission = submissionJson.toObject();
        const long long canvasID = submission["user_id"].toInteger(-1);
        const QJsonArray history = submission["submission_history"].toArray();
        for(const auto &attemptJson : history) {
            const QJsonObject attempt = attemptJson.toObject();
            const QJsonArray submissionData = attempt["submission_data"].toArray();
            if(submissionData.isEmpty()) {
                continue;
            }
            PeerReviewResponse response;
            response.responseID = QString::number(submission["id"].toInteger()) + "-" + QString::number(attempt["attempt"].toInt());
            response.submittedTime = QDateTime::fromString(attempt["submitted_at"].toString(), Qt::ISODate);
            response.nameChosen = submission["user"].toObject()["name"].toString();
            response.giverID = studentOfCanvasID.value(canvasID, -1);

            for(const auto &dataJson : submissionData) {
                const QJsonObject data = dataJson.toObject();
                const QString questionID = QString::number(data["question_id"].toInteger());
                const int questionNum = questionOfCanvasQuestion.value(questionID, -1);
                if((questionNum < 0) || (questionNum >= questions.size())) {
                    continue;
                }
                const QStringList labels = questions.at(questionNum).formLabels();
                if(!labels.isEmpty()) {
                    // options chosen: a dropdown for each team member, one option, or checkboxes
                    QStringList checked;
                    for(auto value = data.constBegin(); value != data.constEnd(); ++value) {
                        const QRegularExpressionMatch dropdown = dropdownAnswer.match(value.key());
                        const QRegularExpressionMatch checkbox = checkedAnswer.match(value.key());
                        if(dropdown.hasMatch() && !labelOf(questionID, value.value()).isEmpty()) {
                            response.answers.insert(questionID + "/" + dropdown.captured(1), {labelOf(questionID, value.value())});
                        }
                        else if(checkbox.hasMatch() && (value.value().toVariant().toString() == "1")) {
                            checked << labelOf(questionID, QJsonValue(checkbox.captured(1).toLongLong()));
                        }
                    }
                    if(data.contains("answer_id") && !labelOf(questionID, data["answer_id"]).isEmpty()) {
                        response.answers.insert(questionID, {labelOf(questionID, data["answer_id"])});
                    }
                    checked.removeAll("");
                    if(!checked.isEmpty()) {
                        // in the order the options are listed
                        std::sort(checked.begin(), checked.end(), [&labels](const QString &a, const QString &b){return labels.indexOf(a) < labels.indexOf(b);});
                        response.answers.insert(questionID, checked);
                    }
                }
                else {
                    // typed answers: a box for each team member, a number, or an essay
                    for(auto value = data.constBegin(); value != data.constEnd(); ++value) {
                        const QRegularExpressionMatch blank = blankAnswer.match(value.key());
                        if(blank.hasMatch() && !plainText(value.value().toVariant().toString()).isEmpty()) {
                            response.answers.insert(questionID + "/" + blank.captured(1), {plainText(value.value().toVariant().toString())});
                        }
                    }
                    const QString text = plainText(data["text"].toString());
                    if(!text.isEmpty()) {
                        response.answers.insert(questionID, {text});
                    }
                }
            }
            responses << response;
        }
    }

    // oldest first, and only each student's latest attempt is used
    std::sort(responses.begin(), responses.end(), [](const PeerReviewResponse &a, const PeerReviewResponse &b)
                                                  {return a.submittedTime < b.submittedTime;});
    QSet<long long> giversSeen;
    for(auto response = responses.rbegin(); response != responses.rend(); ++response) {
        if(response->giverID == -1) {
            continue;
        }
        response->superseded = giversSeen.contains(response->giverID);
        giversSeen.insert(response->giverID);
    }

    return responses;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculating the results
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// TEAMMATES-style normalization: scale the values that are present so that they average an equal share; if they sum to zero, leave them as is
static QList<double> normalizedToEqualShare(const QList<double> &values)
{
    double sum = 0;
    int count = 0;
    for(const auto value : values) {
        if(!std::isnan(value)) {
            sum += value;
            count++;
        }
    }
    if(sum == 0) {
        return values;
    }
    const double factor = (count * EQUALSHARE) / sum;
    QList<double> normalized;
    normalized.reserve(values.size());
    for(const auto value : values) {
        normalized << (std::isnan(value)? value : value * factor);
    }
    return normalized;
}

PeerReviewResults::PeerReviewResults(const PeerReview &peerReview, const QList<PeerReviewResponse> &allResponses)
    : responses(allResponses)
{
    const int numQuestions = int(peerReview.questions.size());
    for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
        if(peerReview.questions.at(questionNum).kind == PeerReviewQuestion::Kind::contribution) {
            contributionQuestionNum = questionNum;
            break;
        }
    }

    // every student, in team order
    for(int teamNum = 0; teamNum < peerReview.teams.size(); teamNum++) {
        for(const auto &member : peerReview.teams.at(teamNum).members) {
            StudentResult studentResult;
            studentResult.ID = member.ID;
            studentResult.teamNum = teamNum;
            studentResult.scores.resize(numQuestions);
            studentResult.choiceCounts.resize(numQuestions);
            indexOfStudent.insert(member.ID, int(students.size()));
            students << studentResult;
        }
    }

    // collect every answer from the responses being used
    for(const auto &response : std::as_const(responses)) {
        if((response.giverID == -1) || !indexOfStudent.contains(response.giverID)) {
            if(!unmatchedNames.contains(response.nameChosen)) {
                unmatchedNames << response.nameChosen;
            }
            continue;
        }
        auto &giver = students[indexOfStudent.value(response.giverID)];
        giver.numResponses++;
        if(response.superseded) {
            continue;
        }
        if(response.enteredByInstructor) {
            giver.enteredByInstructor = true;
        }
        else if(peerReview.requireSignIn && !response.respondentEmail.isEmpty()) {
            const auto *giverStudent = peerReview.student(response.giverID);
            if((giverStudent != nullptr) && !giverStudent->email.isEmpty() &&
                (giverStudent->email.compare(response.respondentEmail, Qt::CaseInsensitive) != 0)) {
                giver.mismatchedEmail = response.respondentEmail;
            }
        }

        for(const auto &formItem : peerReview.formItems) {
            if(formItem.teamNum != giver.teamNum) {
                continue;   // only the giver's own team's page is answered
            }
            QStringList values = response.answers.value(formItem.itemID);
            values.removeAll("");
            if(values.isEmpty() || (formItem.questionNum < 0) || (formItem.questionNum >= numQuestions)) {
                continue;
            }
            const auto &question = peerReview.questions.at(formItem.questionNum);
            PeerReviewAnswer answer;
            answer.questionNum = formItem.questionNum;
            answer.teamNum = formItem.teamNum;
            answer.giverID = response.giverID;
            answer.aboutID = (formItem.aboutID == PeerReviewFormItem::ABOUTGIVER)? response.giverID : formItem.aboutID;
            if((question.about == PeerReviewQuestion::About::eachTeammate) && (answer.aboutID == answer.giverID)) {
                continue;   // the line with their own name, which they were asked to skip
            }
            answer.values = values;
            if(question.isScored()) {
                answer.score = question.scoreOf(values.constFirst(), &answer.hasScore);
            }
            else if(question.kind == PeerReviewQuestion::Kind::numberEntry) {
                QString number = values.constFirst().simplified();
                number.remove('%').remove(',');
                answer.score = number.toDouble(&answer.hasScore);
            }
            answers << answer;
        }
    }

    // what teammates said about each student, question by question
    for(auto &studentResult : students) {
        QSet<long long> raters;
        for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
            const auto &question = peerReview.questions.at(questionNum);
            if(!question.isAboutTeammates()) {
                continue;
            }
            auto &summary = studentResult.scores[questionNum];
            double sum = 0;
            QList<QPair<QString, int>> &counts = studentResult.choiceCounts[questionNum];
            if(question.kind == PeerReviewQuestion::Kind::choice) {
                for(const auto &label : question.formLabels()) {
                    counts << qMakePair(label, 0);
                }
            }
            for(const auto &answer : std::as_const(answers)) {
                if((answer.questionNum != questionNum) || (answer.aboutID != studentResult.ID)) {
                    continue;
                }
                if(answer.giverID == studentResult.ID) {
                    if(answer.hasScore) {
                        summary.self = answer.score;
                    }
                    continue;
                }
                raters.insert(answer.giverID);
                if(answer.hasScore) {
                    sum += answer.score;
                    summary.low = (summary.count == 0)? answer.score : std::min(summary.low, answer.score);
                    summary.high = (summary.count == 0)? answer.score : std::max(summary.high, answer.score);
                    summary.count++;
                }
                for(auto &count : counts) {
                    if(answer.values.contains(count.first)) {
                        count.second++;
                    }
                }
            }
            if(summary.count > 0) {
                summary.average = sum / summary.count;
            }
        }
        studentResult.numRaters = int(raters.size());
    }

    // overall ratings (likert and number scale questions only, each scaled to (0, 1] so questions with different scales can be combined)
    for(auto &studentResult : students) {
        double peerSum = 0, selfSum = 0, givenSum = 0;
        int peerCount = 0, selfCount = 0, givenCount = 0;
        for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
            const auto &question = peerReview.questions.at(questionNum);
            if(!question.isAboutTeammates() || !((question.kind == PeerReviewQuestion::Kind::likert) ||
                                                  (question.kind == PeerReviewQuestion::Kind::numberScale))) {
                continue;
            }
            const auto &summary = studentResult.scores.at(questionNum);
            if(summary.count > 0) {
                peerSum += question.normalizedScore(summary.average);
                peerCount++;
                const double scaleRange = (question.kind == PeerReviewQuestion::Kind::likert)? (question.options.size() - 1) :
                                                                                               (question.scaleHigh - question.scaleLow);
                if(scaleRange > 0) {
                    const double spreadOfQuestion = (summary.high - summary.low) / scaleRange;
                    studentResult.spread = std::isnan(studentResult.spread)? spreadOfQuestion : std::max(studentResult.spread, spreadOfQuestion);
                }
            }
            if(!std::isnan(summary.self)) {
                selfSum += question.normalizedScore(summary.self);
                selfCount++;
            }

            // what this student gave their teammates on this question
            double givenQuestionSum = 0;
            int givenQuestionCount = 0;
            for(const auto &answer : std::as_const(answers)) {
                if((answer.questionNum == questionNum) && (answer.giverID == studentResult.ID) && (answer.aboutID != studentResult.ID) && answer.hasScore) {
                    givenQuestionSum += question.normalizedScore(answer.score);
                    givenQuestionCount++;
                }
            }
            if(givenQuestionCount > 0) {
                givenSum += givenQuestionSum / givenQuestionCount;
                givenCount++;
            }
        }
        if(peerCount > 0) {
            studentResult.peerOverall = peerSum / peerCount;
        }
        if(selfCount > 0) {
            studentResult.selfOverall = selfSum / selfCount;
        }
        if(givenCount > 0) {
            studentResult.givenToTeammates = givenSum / givenCount;
        }
    }

    // contribution, team by team
    contributions.resize(peerReview.teams.size());
    if(contributionQuestionNum != -1) {
        for(int teamNum = 0; teamNum < peerReview.teams.size(); teamNum++) {
            calculateContribution(peerReview, teamNum);
        }
    }

    // adjustment factor: teammates' view of contribution if asked, otherwise overall rating compared to the team's average overall rating
    // (or, if chosen, both: the contribution question counts as one question alongside the rating questions)
    const auto &thresholds = peerReview.thresholds;
    for(int teamNum = 0; teamNum < peerReview.teams.size(); teamNum++) {
        double teamSum = 0;
        int teamCount = 0;
        for(const auto &studentResult : std::as_const(students)) {
            if((studentResult.teamNum == teamNum) && !std::isnan(studentResult.peerOverall)) {
                teamSum += studentResult.peerOverall;
                teamCount++;
            }
        }
        const double teamMean = (teamCount > 0)? (teamSum / teamCount) : NOVALUE;
        for(auto &studentResult : students) {
            if(studentResult.teamNum != teamNum) {
                continue;
            }
            const double contributionFactor = studentResult.perceived / EQUALSHARE;
            const double ratingsFactor = (!std::isnan(studentResult.peerOverall) && !std::isnan(teamMean) && (teamMean > 0))?
                                             (studentResult.peerOverall / teamMean) : NOVALUE;
            if(!std::isnan(contributionFactor) && !std::isnan(ratingsFactor) && thresholds.factorFromAllQuestions) {
                int numRatingQuestions = 0;
                for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
                    const auto &question = peerReview.questions.at(questionNum);
                    if(question.isAboutTeammates() && ((question.kind == PeerReviewQuestion::Kind::likert) ||
                                                       (question.kind == PeerReviewQuestion::Kind::numberScale)) &&
                        (studentResult.scores.at(questionNum).count > 0)) {
                        numRatingQuestions++;
                    }
                }
                studentResult.factor = ((numRatingQuestions * ratingsFactor) + contributionFactor) / (numRatingQuestions + 1);
            }
            else if(!std::isnan(contributionFactor)) {
                studentResult.factor = contributionFactor;
            }
            else if(!std::isnan(ratingsFactor)) {
                studentResult.factor = ratingsFactor;
            }
            if(thresholds.capFactor && !std::isnan(studentResult.factor)) {
                studentResult.factor = std::min(studentResult.factor, thresholds.factorCap);
            }
        }
    }

    // flags
    const double selfGap = thresholds.selfGapPercent / 100.0;
    const double disagreement = thresholds.disagreementPercent / 100.0;
    for(auto &studentResult : students) {
        auto &flags = studentResult.flags;
        if(studentResult.numResponses == 0) {
            flags << QObject::tr("No response");
        }
        else if(studentResult.numResponses > 1) {
            flags << QObject::tr("Submitted %1 times (the latest is used)").arg(studentResult.numResponses);
        }
        if(!studentResult.mismatchedEmail.isEmpty()) {
            flags << QObject::tr("Signed in as %1").arg(studentResult.mismatchedEmail);
        }
        if(!std::isnan(studentResult.factor)) {
            if(studentResult.factor <= thresholds.lowContributor) {
                flags << QObject::tr("Low contributor");
            }
            else if(studentResult.factor >= thresholds.highContributor) {
                flags << QObject::tr("High contributor");
            }
        }
        if(!std::isnan(studentResult.claimed) && !std::isnan(studentResult.perceived)) {
            const double gap = (studentResult.claimed - studentResult.perceived) / EQUALSHARE;
            if(gap >= selfGap) {
                flags << QObject::tr("Rates own contribution higher than teammates do");
            }
            else if(gap <= -selfGap) {
                flags << QObject::tr("Rates own contribution lower than teammates do");
            }
        }
        else if(!std::isnan(studentResult.selfOverall) && !std::isnan(studentResult.peerOverall)) {
            const double gap = studentResult.selfOverall - studentResult.peerOverall;
            if(gap >= selfGap) {
                flags << QObject::tr("Rates self higher than teammates do");
            }
            else if(gap <= -selfGap) {
                flags << QObject::tr("Rates self lower than teammates do");
            }
        }
        if(!std::isnan(studentResult.selfOverall) && !std::isnan(studentResult.givenToTeammates) &&
            ((studentResult.selfOverall - studentResult.givenToTeammates) >= selfGap)) {
            flags << QObject::tr("Rated self high and teammates low");
        }
        if(!std::isnan(studentResult.spread) && (studentResult.spread >= disagreement)) {
            flags << QObject::tr("Teammates' ratings disagree");
        }
        if(studentResult.enteredByInstructor) {
            flags << QObject::tr("Entered by instructor");
        }
    }
}

void PeerReviewResults::calculateContribution(const PeerReview &peerReview, const int teamNum)
{
    // following the method used by TEAMMATES (github.com/TEAMMATES/teammates, TeamEvalResult.java):
    // 1) scale each student's ratings so they average an equal share; that student's own rating is then their "claimed" contribution
    // 2) leave out each student's rating of themself and scale their ratings of their teammates again
    // 3) average what each student received from their teammates, then scale those averages: this is their "perceived" contribution
    const auto &members = peerReview.teams.at(teamNum).members;
    const int teamSize = int(members.size());
    QList<QList<double>> claimedRatings(teamSize, QList<double>(teamSize, NOVALUE));    // [giver][recipient]
    for(const auto &answer : std::as_const(answers)) {
        if((answer.questionNum != contributionQuestionNum) || (answer.teamNum != teamNum) || !answer.hasScore) {
            continue;
        }
        int giver = 0, recipient = 0;
        while((giver < teamSize) && (members.at(giver).ID != answer.giverID)) {
            giver++;
        }
        while((recipient < teamSize) && (members.at(recipient).ID != answer.aboutID)) {
            recipient++;
        }
        if((giver < teamSize) && (recipient < teamSize)) {
            claimedRatings[giver][recipient] = answer.score;
        }
    }

    auto &contribution = contributions[teamNum];
    contribution.ratings.clear();
    for(int giver = 0; giver < teamSize; giver++) {
        const QList<double> normalizedClaimed = normalizedToEqualShare(claimedRatings.at(giver));
        auto &giverResult = students[indexOfStudent.value(members.at(giver).ID)];
        giverResult.claimed = normalizedClaimed.at(giver);

        QList<double> peerRatings = normalizedClaimed;
        peerRatings[giver] = NOVALUE;
        contribution.ratings << normalizedToEqualShare(peerRatings);
    }

    QList<double> averagePerceived(teamSize, NOVALUE);
    for(int recipient = 0; recipient < teamSize; recipient++) {
        double sum = 0, low = 0, high = 0;
        int count = 0;
        for(int giver = 0; giver < teamSize; giver++) {
            const double rating = contribution.ratings.at(giver).at(recipient);
            if(std::isnan(rating)) {
                continue;
            }
            low = (count == 0)? rating : std::min(low, rating);
            high = (count == 0)? rating : std::max(high, rating);
            sum += rating;
            count++;
        }
        if(count > 0) {
            averagePerceived[recipient] = sum / count;
            auto &recipientResult = students[indexOfStudent.value(members.at(recipient).ID)];
            const double spreadOfRatings = (high - low) / EQUALSHARE;
            recipientResult.spread = std::isnan(recipientResult.spread)? spreadOfRatings : std::max(recipientResult.spread, spreadOfRatings);
            if(spreadOfRatings >= (peerReview.thresholds.disagreementPercent / 100.0)) {
                contribution.disagreement = true;
            }
        }
    }

    const QList<double> normalizedPerceived = normalizedToEqualShare(averagePerceived);
    for(int recipient = 0; recipient < teamSize; recipient++) {
        students[indexOfStudent.value(members.at(recipient).ID)].perceived = normalizedPerceived.at(recipient);
    }
}

const PeerReviewResults::StudentResult *PeerReviewResults::result(long long studentID) const
{
    const auto index = indexOfStudent.constFind(studentID);
    return (index == indexOfStudent.constEnd())? nullptr : &students.at(index.value());
}

int PeerReviewResults::numResponded() const
{
    return int(std::count_if(students.cbegin(), students.cend(), [](const StudentResult &studentResult){return studentResult.numResponses > 0;}));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// The feedback reports sent to students
/////////////////////////////////////////////////////////////////////////////////////////////////////////

QString PeerReview::commentKey(const PeerReviewAnswer &answer)
{
    return "q" + QString::number(answer.questionNum) + "_g" + QString::number(answer.giverID) + "_a" + QString::number(answer.aboutID);
}

QString PeerReview::commentText(const PeerReviewAnswer &answer) const
{
    const QString key = commentKey(answer);
    return release.editedComments.contains(key)? release.editedComments.value(key) : answer.values.join(", ");
}

bool PeerReview::commentIsHidden(const PeerReviewAnswer &answer) const
{
    return release.hiddenComments.contains(commentKey(answer));
}

bool PeerReview::feedbackIsWithheld(const PeerReviewResults &results, const long long studentID) const
{
    const auto *studentResult = results.result(studentID);
    if(studentResult == nullptr) {
        return true;
    }
    return (studentResult->numRaters < release.minimumResponders) && !release.minimumOverridden.contains(studentID);
}

QString PeerReview::describeScores(const PeerReviewResults &results, const PeerReviewResults::StudentResult &studentResult, const int questionNum) const
{
    const auto &question = questions.at(questionNum);

    if(questionNum == results.contributionQuestionNum) {
        if(std::isnan(studentResult.perceived)) {
            return {};
        }
        QString text = QObject::tr("Your teammates' view of your contribution:") + " <b>" + question.scoreDescription(studentResult.perceived) + "</b>.";
        if(!std::isnan(studentResult.claimed)) {
            text += " " + QObject::tr("Your own estimate:") + " " + question.scoreDescription(studentResult.claimed) + ".";
        }
        return text;
    }

    const auto &summary = studentResult.scores.at(questionNum);
    QString text;
    if(summary.count > 0) {
        text = QObject::tr("Your teammates' average:") + " <b>" + question.scoreDescription(summary.average) + "</b>.";
    }
    if(!std::isnan(summary.self)) {
        text += (text.isEmpty()? "" : " ") + QObject::tr("Your own rating:") + " " + question.scoreDescription(summary.self) + ".";
    }
    if(release.includeTeamAverage) {
        double sum = 0;
        int count = 0;
        for(const auto &otherResult : results.students) {
            if((otherResult.teamNum == studentResult.teamNum) && (otherResult.scores.at(questionNum).count > 0)) {
                sum += otherResult.scores.at(questionNum).average;
                count++;
            }
        }
        if(count > 0) {
            text += " " + QObject::tr("Your team's average:") + " " + question.scoreDescription(sum / count) + ".";
        }
    }
    return text;
}

QStringList PeerReview::sharedCommentsFor(const PeerReviewResults &results, const long long studentID, const int questionNum) const
{
    // the answers shared with this student for this question: if the question shows names, each with its writer's first name (or full
    // name, if a teammate has the same first name), in order of the names; otherwise without names, in a random (but always the same) order
    const auto &question = questions.at(questionNum);
    const bool named = question.showsWriterNames();
    const int teamNum = teamOf(studentID);
    QList<QPair<QString, QString>> comments;    // writer's name, comment
    for(const auto &answer : results.answers) {
        if((answer.questionNum != questionNum) || (answer.giverID == studentID) || commentIsHidden(answer)) {
            continue;
        }
        if(question.isAboutTeammates() && (answer.aboutID != studentID)) {
            continue;
        }
        if(!question.isAboutTeammates() && (answer.teamNum != teamNum)) {
            continue;
        }
        const QString text = commentText(answer).trimmed();
        if(text.isEmpty()) {
            continue;
        }
        QString name;
        const auto *giver = student(answer.giverID);
        if(named && (giver != nullptr) && (teamNum >= 0)) {
            const auto &members = teams.at(teamNum).members;
            const bool sharedFirstName = std::any_of(members.cbegin(), members.cend(), [giver](const PeerReviewStudent &member)
                                                     {return (member.ID != giver->ID) && (member.firstname.compare(giver->firstname, Qt::CaseInsensitive) == 0);});
            name = sharedFirstName? giver->fullName() : giver->firstname;
        }
        comments.append({name, text});
    }

    QStringList formattedComments;
    formattedComments.reserve(comments.size());
    if(named) {
        std::sort(comments.begin(), comments.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b)
                                                    {return QString::localeAwareCompare(a.first, b.first) < 0;});
        for(const auto &comment : std::as_const(comments)) {
            formattedComments << "<b>" + comment.first.toHtmlEscaped() + ":</b> " + comment.second.toHtmlEscaped().replace("\n", "<br>");
        }
    }
    else {
        for(const auto &comment : std::as_const(comments)) {
            formattedComments << comment.second.toHtmlEscaped().replace("\n", "<br>");
        }
        std::mt19937 generator(uint(qHash(fileName + title) ^ uint(studentID) ^ uint(questionNum << 16)));
        std::shuffle(formattedComments.begin(), formattedComments.end(), generator);
    }
    return formattedComments;
}

QString PeerReview::reportHtml(const PeerReviewResults &results, const long long studentID) const
{
    const auto *studentResult = results.result(studentID);
    const auto *thisStudent = student(studentID);
    if((studentResult == nullptr) || (thisStudent == nullptr)) {
        return {};
    }
    const bool withheld = feedbackIsWithheld(results, studentID);

    QString html = "<html><body>";
    html += "<h2>" + title.toHtmlEscaped() + "</h2>";
    html += "<p><b>" + thisStudent->fullName().toHtmlEscaped() + "</b><br>" + teams.at(studentResult->teamNum).name.toHtmlEscaped() + "</p>";
    const QStringList introParagraphs = release.introText.split("\n\n", Qt::SkipEmptyParts);
    for(const auto &paragraph : introParagraphs) {
        html += "<p>" + paragraph.toHtmlEscaped().replace("\n", "<br>") + "</p>";
    }
    const QString personalNote = release.personalNotes.value(studentID).trimmed();
    if(!personalNote.isEmpty()) {
        html += "<p><b>" + QObject::tr("A note from your instructor:") + "</b> " + personalNote.toHtmlEscaped().replace("\n", "<br>") + "</p>";
    }
    if(withheld) {
        html += "<p><i>" + QObject::tr("Too few of your teammates responded, so their ratings and comments aren't shared with you.") + "</i></p>";
    }

    for(int questionNum = 0; questionNum < questions.size(); questionNum++) {
        const auto &question = questions.at(questionNum);
        if(question.shareWith == PeerReviewQuestion::ShareWith::instructor) {
            continue;
        }
        // answers shared without names could be traced to their writer when too few teammates responded, so those are withheld;
        // answers shared with names are shown either way
        const bool named = question.showsWriterNames();
        if(withheld && !named) {
            continue;
        }

        // when names are shown, each teammate's answer is listed with their name (after the average, for a rating)
        QString namedAnswers;
        if(named) {
            const QStringList answers = sharedCommentsFor(results, studentID, questionNum);
            if(!answers.isEmpty()) {
                namedAnswers = "<ul><li>" + answers.join("</li><li>") + "</li></ul>";
            }
        }

        QString content;
        if(question.isAboutTeammates() && question.isScored()) {
            const QString scores = describeScores(results, *studentResult, questionNum);
            if(!scores.isEmpty()) {
                content = "<p>" + scores + "</p>";
            }
            content += namedAnswers;
        }
        else if(question.isAboutTeammates() && (question.kind == PeerReviewQuestion::Kind::choice)) {
            if(named) {
                content = namedAnswers;
            }
            else {
                QStringList counts;
                for(const auto &count : studentResult->choiceCounts.at(questionNum)) {
                    if(count.second > 0) {
                        counts << QObject::tr("%1 of %2 teammates:").arg(count.second).arg(studentResult->numRaters) + " " + count.first.toHtmlEscaped();
                    }
                }
                if(!counts.isEmpty()) {
                    content = "<p>" + counts.join("<br>") + "</p>";
                }
            }
        }
        else if(question.isAboutTeammates() && (question.kind == PeerReviewQuestion::Kind::numberEntry)) {
            const auto &summary = studentResult->scores.at(questionNum);
            if(summary.count > 0) {
                content = "<p>" + QObject::tr("Your teammates' average:") + " <b>" + QString::number(summary.average, 'f', 1) + "</b>.</p>";
            }
            content += namedAnswers;
        }
        else if(!question.isAboutTeammates() && question.isScored()) {
            // a rating asked once or about yourself, shared with the team: the team's average
            double sum = 0;
            int count = 0;
            for(const auto &answer : results.answers) {
                if((answer.questionNum == questionNum) && (answer.teamNum == studentResult->teamNum) && answer.hasScore) {
                    sum += answer.score;
                    count++;
                }
            }
            if(count > 0) {
                content = "<p>" + QObject::tr("Your team's average:") + " <b>" + question.scoreDescription(sum / count) + "</b>.</p>";
            }
            content += namedAnswers;
        }
        else {
            const QStringList comments = sharedCommentsFor(results, studentID, questionNum);
            if(!comments.isEmpty()) {
                content = "<ul><li>" + comments.join("</li><li>") + "</li></ul>";
            }
        }

        if(!content.isEmpty()) {
            html += "<h3>" + question.text.toHtmlEscaped() + (named? (" <i>(" + QObject::tr("with names") + ")</i>") : "") + "</h3>" + content;
        }
    }

    if(release.includeAdjustmentFactor && !withheld && !std::isnan(studentResult->factor)) {
        html += "<h3>" + QObject::tr("Adjustment factor") + "</h3><p>" +
                QObject::tr("Based on your teammates' ratings, your adjustment factor is") + " <b>" + QString::number(studentResult->factor, 'f', 2) + "</b>.</p>";
    }

    html += "</body></html>";
    return html;
}

QString PeerReview::reportPlainText(const PeerReviewResults &results, const long long studentID) const
{
    QTextDocument document;
    document.setHtml(reportHtml(results, studentID));
    return document.toPlainText();
}

QString PeerReview::reportFileName(const long long studentID) const
{
    static const QRegularExpression unallowedChars(R"([#&{}\\/<>*?$!'":@+`|=\s])");
    auto baseName = [](const PeerReviewStudent &member) {
        QString name = member.lastname + "_" + member.firstname;
        return name.replace(unallowedChars, "_");
    };

    // "Lastname_Firstname_feedback", with the team name added if another student would get the same name,
    // and a number added too if that other student is on the same team
    for(int teamNum = 0; teamNum < teams.size(); teamNum++) {
        const auto &members = teams.at(teamNum).members;
        for(int memberNum = 0; memberNum < members.size(); memberNum++) {
            if(members.at(memberNum).ID != studentID) {
                continue;
            }
            const QString name = baseName(members.at(memberNum));
            bool nameShared = false;
            for(const auto &team : teams) {
                for(const auto &member : team.members) {
                    if((member.ID != studentID) && (baseName(member).compare(name, Qt::CaseInsensitive) == 0)) {
                        nameShared = true;
                    }
                }
            }
            QString fileName = name;
            if(nameShared) {
                QString teamName = teams.at(teamNum).name;
                fileName += "_" + teamName.replace(unallowedChars, "_");
                const int sameNameNumber = sameNameNumberOnTeam(teamNum, memberNum);
                if(sameNameNumber > 0) {
                    fileName += "_" + QString::number(sameNameNumber);
                }
            }
            return fileName + "_" + QObject::tr("feedback");
        }
    }
    return {};
}

int PeerReview::sameNameNumberOnTeam(const int teamNum, const int memberNum) const
{
    const auto &members = teams.at(teamNum).members;
    const QString name = members.at(memberNum).fullName();
    int numBefore = 0, numOthers = 0;
    for(int otherNum = 0; otherNum < members.size(); otherNum++) {
        if((otherNum != memberNum) && (members.at(otherNum).fullName().compare(name, Qt::CaseInsensitive) == 0)) {
            numOthers++;
            if(otherNum < memberNum) {
                numBefore++;
            }
        }
    }
    return (numOthers == 0)? 0 : (numBefore + 1);
}

QString PeerReview::memberLabel(const int teamNum, const int memberNum) const
{
    const QString name = teams.at(teamNum).members.at(memberNum).fullName();
    const int sameNameNumber = sameNameNumberOnTeam(teamNum, memberNum);
    return (sameNameNumber == 0)? name : (name + " (" + QString::number(sameNameNumber) + ")");
}

QStringList PeerReview::teamsWithSameNameMembers() const
{
    QStringList descriptions;
    for(int teamNum = 0; teamNum < teams.size(); teamNum++) {
        const auto &members = teams.at(teamNum).members;
        QStringList sameNames;
        for(int memberNum = 0; memberNum < members.size(); memberNum++) {
            if(sameNameNumberOnTeam(teamNum, memberNum) > 0) {
                sameNames << memberLabel(teamNum, memberNum);
            }
        }
        if(!sameNames.isEmpty()) {
            descriptions << teams.at(teamNum).name + ": " + sameNames.join(", ");
        }
    }
    return descriptions;
}
