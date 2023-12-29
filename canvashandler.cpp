#include "canvashandler.h"
#include "gruepr_globals.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QOAuthHttpServerReplyHandler>
#include <QPushButton>
#include <QVBoxLayout>

CanvasHandler::CanvasHandler(const QString &authenticateURL, const QString &accessTokenURL, const QString &baseAPIURL) {

    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setTransferTimeout(TIMEOUT_TIME);
    canvas = new QOAuth2AuthorizationCodeFlow(authenticateURL, accessTokenURL, manager, this);

    baseURL = baseAPIURL;

    canvasCourses.clear();
    roster.clear();
}

CanvasHandler::~CanvasHandler() {
    delete manager;
    delete canvas;
}

// Retrieves course names, and loads the names, Canvas course IDs, and student counts
QStringList CanvasHandler::getCourses() {
    QStringList courseNames;
    QList<int> ids, studentCounts;
    QStringList x;

    QList<QStringList*> courseNamesInList = {&courseNames};
    QList<QList<int>*> idsAndStudentCounts = {&ids, &studentCounts};
    QList<QStringList*> stringInSubobjectParams = {&x};

    getPaginatedCanvasResults("/api/v1/courses?include[]=total_students", {"name"}, courseNamesInList, {"id", "total_students"}, idsAndStudentCounts, {}, stringInSubobjectParams);
    courseNames.removeAll("");

    canvasCourses.clear();
    for(int i = 0; i < courseNames.size(); i++) {
        canvasCourses.append({courseNames.at(i), ids.at(i), studentCounts.at(i)});
    }

    return courseNames;
}

// Retrieves the number of students in the given course
int CanvasHandler::getStudentCount(const QString &courseName) {
    for(const auto &course : qAsConst(canvasCourses)) {
        if(course.name == courseName) {
            return course.numStudents;
        }
    }
    return 0;
}

// Retrieves the StudentRoster in the given course
QList<StudentRecord> CanvasHandler::getStudentRoster(const QString &courseName) {
    const int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return {};
    }

    QStringList studentNames;
    QStringList studentEmails;
    QList<int> ids;
    QStringList x;
    QList<QStringList*> studentNamesandEmailsInList = {&studentNames, &studentEmails};
    QList<QList<int>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/users?enrollment_type[]=student", {"sortable_name", "email"}, studentNamesandEmailsInList, {"id"}, idsInList, {}, stringInSubobjectParams);

    QStringList firstNames, lastNames;
    for(const auto &studentName : studentNames) {
        auto names = studentName.split(',');
        firstNames << (names.at(1).isEmpty()? "" : names.at(1).trimmed());
        lastNames << (names.at(0).isEmpty()? "" : names.at(0).trimmed());
    }

    roster.clear();
    roster.reserve(studentNames.size());
    for(int i = 0; i < studentNames.size(); i++) {
        StudentRecord student;
        student.firstname = firstNames.at(i);
        student.lastname = lastNames.at(i);
        student.LMSID = ids.at(i);
        student.email = studentEmails.at(i);
        roster << student;
    }

    return roster;
}

// Creates a teamset
bool CanvasHandler::createTeams(const QString &courseName, const QString &setName, const QStringList &teamNames, const QList<QList<StudentRecord>> &teams) {
    const int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return false;
    }

    //create the teamset
    QString url = "/api/v1/courses/" + QString::number(courseID) + "/group_categories";
    QUrlQuery query;
    query.addQueryItem("name", setName);
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QStringList groupCategoryName;
    QList<int> groupCategoryID;
    QStringList x;
    QList<QStringList*> stringParams = {&groupCategoryName};
    QList<QList<int>*> intParams = {&groupCategoryID};
    QList<QStringList*> stringInSubobjectParams = {&x};
    postToCanvasGetSingleResult(url, postData, {"name"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);

    //create the teams
    bool allGood = true;
    QStringList groupName;
    QList<int> groupID;
    QStringList workflowState;
    for(int i = 0; i < teamNames.size(); i++) {
        //create one team
        url = "/api/v1/group_categories/" + QString::number(groupCategoryID[0]) + "/groups";
        query.clear();
        query.addQueryItem("name", teamNames.at(i));
        postData = query.toString(QUrl::FullyEncoded).toUtf8();
        groupName.clear();
        groupID.clear();
        stringParams = {&groupName};
        intParams = {&groupID};
        postToCanvasGetSingleResult(url, postData, {"name"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);

        //add each student on the team
        for(const auto &student : teams.at(i)) {
            url = "/api/v1/groups/" + QString::number(groupID[0]) + "/memberships";
            query.clear();
            query.addQueryItem("user_id", QString::number(student.LMSID));
            postData = query.toString(QUrl::FullyEncoded).toUtf8();
            QList<int> membershipID, newUserID;
            workflowState.clear();
            stringParams = {&workflowState};
            intParams = {&membershipID, &newUserID};
            postToCanvasGetSingleResult(url, postData, {"workflow_state"}, stringParams, {"id", "user_id"}, intParams, {}, stringInSubobjectParams);
            allGood = allGood && (workflowState.constFirst() == "accepted") && (newUserID.constFirst() == student.LMSID);
        }
    }

    return allGood;
}

bool CanvasHandler::createSurvey(const QString &courseName, const Survey *const survey) {
    const int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return false;
    }

    //create the survey (a quiz type in Canvas)
    bool allGood = true;
    QString url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes";
    QUrlQuery query;
    query.addQueryItem("quiz[title]", (survey->title.isEmpty() ? QDateTime::currentDateTime().toString("hh:mm:ss dd MMMM yyyy") : survey->title));
    query.addQueryItem("quiz[quiz_type]", "survey");
    query.addQueryItem("quiz[allowed_attempts]", "-1");         //unlimited re-takes
    query.addQueryItem("quiz[scoring_policy]", "keep_latest");
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QStringList quizURL, mobileQuizURL;
    QList<int> quizID;
    QStringList x;
    QList<QStringList*> stringParams = {&quizURL, &mobileQuizURL};
    QList<QList<int>*> intParams = {&quizID};
    QList<QStringList*> stringInSubobjectParams = {&x};
    postToCanvasGetSingleResult(url, postData, {"html_url", "mobile_url"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);
    const int surveyID = quizID.constFirst();

    //add each question
    int questionNum = 0;
    QStringList newQuestionText;
    QList<int> questionID;
    for(const auto &question : survey->questions) {
        //create one question
        url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(surveyID) + "/questions";
        newQuestionText.clear();
        questionID.clear();
        stringParams = {&newQuestionText};
        intParams = {&questionID};
        query.clear();
        query.addQueryItem("question[question_name]", "Question " + QString::number(questionNum+1));
        query.addQueryItem("question[position]", QString::number(questionNum+1));
        switch(question.type) {
        case Question::QuestionType::shorttext:
            query.addQueryItem("question[question_type]", "short_answer_question");
            query.addQueryItem("question[question_text]", question.text);
            break;
        case Question::QuestionType::longtext:
            query.addQueryItem("question[question_type]", "essay_question");
            query.addQueryItem("question[question_text]", question.text);
            break;
        case Question::QuestionType::dropdown: {
            query.addQueryItem("question[question_type]", "multiple_dropdowns_question");
            query.addQueryItem("question[question_text]", question.text + "  [options]");
            int optionNum = 0;
            for(const auto &option : question.options) {
                query.addQueryItem("question[answers][" + QString::number(optionNum) + "][blank_id]", "options");
                query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]", option);
                query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                optionNum++;
            }
            break;}
        case Question::QuestionType::radiobutton:
        case Question::QuestionType::checkbox: {
            query.addQueryItem("question[question_type]", (question.type == Question::QuestionType::radiobutton ? "multiple_choice_question" : "multiple_answers_question"));
            query.addQueryItem("question[question_text]", question.text);
            int optionNum = 0;
            for(const auto &option : question.options) {
                query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]", option);
                query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                optionNum++;
            }
            break;}
        case Question::QuestionType::schedule: {
            if(survey->schedDayNames.size() == 1) {
                //just one question, set it up to post
                query.addQueryItem("question[question_type]", "multiple_answers_question");
                query.addQueryItem("question[question_text]", question.text + " <strong><u>[" + survey->schedDayNames.first() + "]</u></strong>");
                int optionNum = 0;
                for(const auto &schedTimeName : survey->schedTimeNames) {
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]", schedTimeName);
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                    optionNum++;
                }
            }
            else {
                // if there will be more than one day in the schedule, create a text "question" to clarify that the next n questions will be for the schedule on different days
                query.addQueryItem("question[question_type]", "text_only_question");
                QString scheduleIntroStatement = "<strong>" + SCHEDULEQUESTIONINTRO1 + QString::number(survey->schedDayNames.size()) + SCHEDULEQUESTIONINTRO2;
                for(const auto &dayName : survey->schedDayNames) {
                    scheduleIntroStatement += " <u>[" + dayName + "]</u> ";
                }
                scheduleIntroStatement += "</strong>:";
                query.addQueryItem("question[question_text]", scheduleIntroStatement);
                // post the question and then add a question for each day (final day will get posted outside of switch/case)
                for(const auto &dayName : survey->schedDayNames) {
                    postData = query.toString(QUrl::FullyEncoded).toUtf8();
                    postToCanvasGetSingleResult(url, postData, {"question_text"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);
                    allGood = allGood && (!newQuestionText.constFirst().isEmpty());
                    questionNum++;
                    newQuestionText.clear();
                    questionID.clear();
                    query.clear();
                    query.addQueryItem("question[question_name]", "Question " + QString::number(questionNum+1));
                    query.addQueryItem("question[position]", QString::number(questionNum+1));
                    query.addQueryItem("question[question_type]", "multiple_answers_question");
                    query.addQueryItem("question[question_text]", question.text + " <strong><u>[" + dayName + "]</u></strong>");
                    int optionNum = 0;
                    for(const auto &schedTimeName : survey->schedTimeNames) {
                        query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]", schedTimeName);
                        query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                        optionNum++;
                    }
                }
            }
            break;}
/*
 * Functional but aesthetically displeasing mechanism to ask for the schedule in a single Canvas quiz question as as grid of dropdowns (rows are days and columns are times)
            query.addQueryItem("question[question_type]", "multiple_dropdowns_question");
            QString cellText = "<td style =\"width: 80px;\"><span style=\"width: 80px;\">";
            QString rowText = "<tr style=\"height: 10px;\">";
            QString questionText = "<p>Please tell us about your weekly schedule. Use your best guess or estimate if necessary.</p>"
                                   "<p>In each box, please indicate whether you are BUSY (unavailable) or are FREE (available) for group work.</p>"
                                   "<table style=\"border-collapse: collapse; width: " + QString::number(80 * (2 + survey->schedTimeNames.size())) + "px; "
                                                                             "height: " + QString::number(10 * (1 + survey->schedDayNames.size())) + "px;\" border=\"1\">"
                                   "<tbody>" +
                                   rowText +
                                   cellText + "</td>";
            for(const auto &schedTimeName : survey->schedTimeNames) {
                questionText += cellText + schedTimeName + "</span></td>";
            }
            questionText += "</tr>";
            int responseBox = 101;
            int responseNum = 0;
            for(const auto &dayName : survey->schedDayNames) {
                questionText += rowText +
                                cellText + dayName + "</td>";
                for(const auto &schedTimeName : survey->schedTimeNames) {
                    questionText += cellText + "[" + QString::number(responseBox) + "]</td>";
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][blank_id]", QString::number(responseBox));
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][answer_text]", tr("BUSY"));
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][answer_weight]", "100");
                    responseNum++;
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][blank_id]", QString::number(responseBox));
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][answer_text]", tr("FREE"));
                    query.addQueryItem("question[answers][" + QString::number(responseNum) + "][answer_weight]", "100");
                    responseNum++;
                    responseBox++;
                }
                questionText += "</tr>";
            }
            questionText += "</tbody></table>";
            query.addQueryItem("question[question_text]", question.text);
            break;}
*/
        }
        postData = query.toString(QUrl::FullyEncoded).toUtf8();
        postToCanvasGetSingleResult(url, postData, {"question_text"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);
        allGood = allGood && (!newQuestionText.constFirst().isEmpty());
        questionNum++;
    }

    return allGood;

}

QStringList CanvasHandler::getQuizList(const QString &courseName) {
    const int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return {};
    }

    QStringList titles;
    QList<int> ids;
    QStringList x;
    QList<QStringList*> titlesInList = {&titles};
    QList<QList<int>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes", {"title"}, titlesInList, {"id"}, idsInList, {}, stringInSubobjectParams);

    quizList.clear();
    for(int i = 0; i < titles.size(); i++) {
        quizList.append({titles.at(i), ids.at(i)});
    }
    return titles;
}

QString CanvasHandler::downloadQuizResult(const QString &courseName, const QString &quizName) {
    const int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return {};
    }

    const int quizID = getQuizID(quizName);
    if(quizID == -1) {
        return {};
    }

    const QUrl URL = getQuizResultsURL(courseID, quizID);
    if(URL.isEmpty()) {
        return {};
    }

    // wait until the results file is ready
    QEventLoop loop;
    QStringList x;
    QList<int> ids;
    QStringList filename;
    QList<QStringList*> stringParams = {&x};
    QList<QList<int>*> intParams = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&filename};
    // check every two seconds--a file object (including a download URL) is added to the json results when it is
    do {
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
        filename.clear();
        ids.clear();
        getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports", {}, stringParams, {"id"}, intParams, {"file/filename"}, stringInSubobjectParams);
    } while(filename.first().isEmpty());
    const QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::TempLocation), quizName.simplified().replace(' ','_') + ".csv");
    // sometimes still a delay, so attempt to download every two seconds
    while(!downloadFile(URL, filepath.absoluteFilePath())) {
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
    }

    return filepath.absoluteFilePath();
}


////////////////////////////////////////////

int CanvasHandler::getCourseID(const QString &courseName) {
    int courseID = -1;
    for(const auto &course : qAsConst(canvasCourses)) {
        if(course.name == courseName) {
            courseID = course.ID;
        }
    }
    return courseID;
}

int CanvasHandler::getQuizID(const QString &quizName) {
    int quizID = -1;
    for(const auto &quiz : qAsConst(quizList)) {
        if(quiz.name == quizName) {
            quizID = quiz.ID;
        }
    }
    return quizID;
}

QUrl CanvasHandler::getQuizResultsURL(const int courseID, const int quizID) {
    const QString url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports";
    QUrlQuery query;
    query.addQueryItem("quiz_report[report_type]", "student_analysis");
    const QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QStringList x;
    QList<int> quizReportID;
    QStringList quizReportFileURL;
    QList<QStringList*> stringParams = {&x};
    QList<QList<int>*> intParams = {&quizReportID};
    QList<QStringList*> stringInSubobjectParams = {&quizReportFileURL};    
    QEventLoop loop;
    // check every two seconds--a file object (including a download URL) is added to the json results when it is
    do {
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
        quizReportID.clear();
        quizReportFileURL.clear();
        postToCanvasGetSingleResult(url, postData, {}, {stringParams}, {"id"}, intParams, {"file/url"}, stringInSubobjectParams);
    } while(quizReportFileURL.first().isEmpty());

    return {quizReportFileURL.first()};
}

void CanvasHandler::getPaginatedCanvasResults(const QString &initialURL, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                         const QStringList &intParams, QList<QList<int>*> &intVals,
                                                                         const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals)
{
    QEventLoop loop;
    QNetworkReply *reply = nullptr;
    QString url = baseURL+initialURL, replyHeader, replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;
    int numPages = 0;

    do {
        reply = canvas->get(url);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if(reply->bytesAvailable() == 0) {
            //qDebug() << "no reply";
            delete reply;
            return;
        }

        replyHeader = reply->rawHeader("Link");
        replyBody = reply->readAll();
        //qDebug() << replyBody;
        json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
        if(json_doc.isArray()) {
            json_array = json_doc.array();
        }
        else if(json_doc.isObject()) {
            json_array << json_doc.object();
        }
        else {
            //empty or null
            break;
        }

        for(const auto &value : qAsConst(json_array)) {
            QJsonObject json_obj = value.toObject();
            for(int i = 0; i < stringParams.size(); i++) {
                *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
            }
            for(int i = 0; i < intParams.size(); i++) {
                *(intVals[i]) << json_obj[intParams.at(i)].toInt();
            }
            for(int i = 0; i < stringInSubobjectParams.size(); i++) {
                const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
                QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
                *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
            }
        }
        static const QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
        const QRegularExpressionMatch nextURLMatch = nextURL.match(replyHeader);
        url = nextURLMatch.captured(1);
    }
    while(!url.isNull() && ++numPages < NUM_PAGES_TO_LOAD);

    reply->deleteLater();
}

void CanvasHandler::postToCanvasGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                                const QStringList &intParams, QList<QList<int>*> &intVals,
                                                                                                const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals)
{
    QEventLoop loop;
    const QString url = baseURL+URL;
    QString replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;

    auto *reply = canvas->post(url, postData);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0) {
        //qDebug() << "no reply";
        delete reply;
        return;
    }

    replyBody = reply->readAll();
    //qDebug() << replyBody;
    json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
    if(json_doc.isArray()) {
        json_array = json_doc.array();
    }
    else if(json_doc.isObject()) {
        json_array << json_doc.object();
    }
    else {
        //empty or null
        reply->deleteLater();
        return;
    }

    for(const auto &value : qAsConst(json_array)) {
        QJsonObject json_obj = value.toObject();
        for(int i = 0; i < stringParams.size(); i++) {
            *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
        }
        for(int i = 0; i < intParams.size(); i++) {
            *(intVals[i]) << json_obj[intParams.at(i)].toInt();
        }
        for(int i = 0; i < stringInSubobjectParams.size(); i++) {
            const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
            QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
            *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
        }
    }

    reply->deleteLater();
}

bool CanvasHandler::downloadFile(const QUrl &URL, const QString &filepath) {
    QEventLoop loop;
    QNetworkReply *reply = nullptr;
    QByteArray replyBody;

    reply = canvas->get(URL);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0) {
        //qDebug() << "no reply";
        delete reply;
        return false;
    }

    replyBody = reply->readAll();
    //qDebug() << replyBody;
    QFile file(filepath);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out << replyBody;
    reply->deleteLater();
    return true;
}


//////////////////////////////////////////////////////////
// IN PROG
// Authenticates data access using OAuth2 protocol
void CanvasHandler::authenticate() {
    canvas->setScope("url:GET|/api/v1/courses "                                             // get list of user's courses
                     "url:GET|/api/v1/courses/:course_id/users "                            // get roster of students in a course
                     "url:POST|/api/v1/courses/:course_id/quizzes "                         // create a quiz (i.e., survey)
                     "url:POST|/api/v1/courses/:course_id/quizzes/:quiz_id/questions "      // create a quiz question
                     "url:GET|/api/v1/courses/:course_id/quizzes "                          // get list of quizzes in a course
                     "url:POST|/api/v1/courses/:course_id/quizzes/:quiz_id/reports "        // create a quiz report (i.e., csv file of responses)
                     "url:GET|/api/v1/courses/:course_id/quizzes/:quiz_id/reports "         // obtain the URL of the quiz report
                     "url:POST|/api/v1/courses/:course_id/group_categories "                // create a group set
                     "url:POST|/api/v1/group_categories/:group_category_id/groups "         // create a group in a group set
                     "url:POST|/api/v1/groups/:group_id/memberships");                      // put students on a group
    QAbstractOAuth2::connect(canvas, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    const QUrl redirectUri = QString(REDIRECT_URI);
    const auto port = static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT));

    canvas->setClientIdentifier(CLIENT_ID);
    canvas->setClientIdentifierSharedKey(CLIENT_SECRET);

    canvas->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
       // Percent-decode the "code" parameter so Google can match it
       if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
          const QByteArray code = parameters->value("code").toByteArray();
          parameters->insert("code", QUrl::fromPercentEncoding(code));
       }
    });

    auto *replyHandler = new QOAuthHttpServerReplyHandler(port, this);
    canvas->setReplyHandler(replyHandler);

    canvas->grant();
    connect(canvas, &QOAuth2AuthorizationCodeFlow::granted, this, [this](){emit granted(); authenticated = true;});
}

// For testing: sets token manually
void CanvasHandler::authenticate(const QString &token) {
    canvas->setToken(token);
    authenticated = true;
}

QStringList CanvasHandler::askUserForManualToken(const QString &currentURL, const QString &currentToken, QWidget *parent) {
    auto *getCanvasInfoDialog = new QDialog(parent);
    getCanvasInfoDialog->setWindowTitle(tr("Connect to Canvas"));
    getCanvasInfoDialog->setWindowIcon(QIcon(":/icons_new/canvas.png"));
    getCanvasInfoDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    auto *label = new QLabel(tr("The next step will allow gruepr to interact with your Canvas course. This feature is currently in beta.\n"
                            "The information typed below will be saved to your computer so that you only have to perform the following steps once.\n"
                            "This information will not be sent anywhere and should not be shared with anyone.\n\n"
                            "1) enter your institution's canvas URL (e.g.: https://example.instructure.com) in the first field below.\n"
                            "2) create a token so that gruepr can access your Canvas account. You can generally do this by:\n"
                            "   »  Log into Canvas,\n"
                            "   »  click \"Account\" in the left menu\n"
                            "   »  click \"Settings\", \n"
                            "   »  scroll to Approved Integration,\n"
                            "   »  click \"+ New Access Token\",\n"
                            "   »  fill in \"gruepr\" for the Purpose field and keep the expiration date blank,\n"
                            "   »  click \"Generate Token\", and\n"
                            "   »  copy your freshly generated token and paste it into the second field below.\n"
                            "Depending on your institution's settings, you may have to request that "
                             "your token be generated by the campus Canvas administrators.\n\n"), getCanvasInfoDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *canvasURL = new QLineEdit(currentURL, getCanvasInfoDialog);
    canvasURL->setPlaceholderText(tr("Canvas URL (e.g., https://example.instructure.com)"));
    canvasURL->setStyleSheet(LINEEDITSTYLE);
    auto *canvasToken = new QLineEdit(currentToken, getCanvasInfoDialog);
    canvasToken->setPlaceholderText(tr("User-generated Canvas token"));
    canvasToken->setStyleSheet(LINEEDITSTYLE);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, getCanvasInfoDialog);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(buttonBox, &QDialogButtonBox::accepted, getCanvasInfoDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, getCanvasInfoDialog, &QDialog::reject);
    vLayout->addWidget(label);
    vLayout->addWidget(canvasURL);
    vLayout->addWidget(canvasToken);
    vLayout->addWidget(buttonBox);
    getCanvasInfoDialog->setLayout(vLayout);

    auto result = getCanvasInfoDialog->exec();
    getCanvasInfoDialog->deleteLater();

    if(result == QDialog::Rejected) {
        return {};
    }
    QString url = canvasURL->text();
    if(!url.startsWith("https://") && !url.startsWith("http://")) {
        url.prepend("https://");
    }
    const QString token = canvasToken->text().trimmed();
    return {url, token};
}

void CanvasHandler::setBaseURL(const QString &baseAPIURL) {
    baseURL = baseAPIURL;
}

QDialog* CanvasHandler::actionDialog(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *busyDialog = new QDialog(parent);
    busyDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
    actionDialogIcon = new QLabel(busyDialog);
    actionDialogIcon->setPixmap(QPixmap(":/icons_new/canvas.png").scaled(CANVASICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionDialogLabel = new QLabel(busyDialog);
    actionDialogLabel->setStyleSheet(LABEL10PTSTYLE);
    actionDialogLabel->setText(tr("Communicating with Canvas..."));
    actionDialogButtons = new QDialogButtonBox(busyDialog);
    actionDialogButtons->setStyleSheet(SMALLBUTTONSTYLE);
    actionDialogButtons->setStandardButtons(QDialogButtonBox::NoButton);
    connect(actionDialogButtons, &QDialogButtonBox::accepted, busyDialog, &QDialog::accept);
    auto *theGrid = new QGridLayout;
    busyDialog->setLayout(theGrid);
    theGrid->addWidget(actionDialogIcon, 0, 0);
    theGrid->addWidget(actionDialogLabel, 0, 1);
    theGrid->addWidget(actionDialogButtons, 1, 0, 1, -1);
    busyDialog->setModal(true);
    busyDialog->show();
    busyDialog->adjustSize();
    return busyDialog;
}

void CanvasHandler::actionComplete(QDialog *busyDialog) {
    QApplication::restoreOverrideCursor();
    busyDialog->accept();
    busyDialog->deleteLater();
}
