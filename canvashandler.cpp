#include "canvashandler.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QOAuthHttpServerReplyHandler>
#include <QVBoxLayout>

CanvasHandler::CanvasHandler(const QString &authenticateURL, const QString &accessTokenURL, const QString &baseAPIURL) {

    manager = new QNetworkAccessManager;
    canvas = new QOAuth2AuthorizationCodeFlow(authenticateURL, accessTokenURL, manager, this);
    canvas->networkAccessManager()->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

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
QVector<StudentRecord> CanvasHandler::getStudentRoster(const QString &courseName) {
    int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return {};
    }

    QStringList studentNames;
    QList<int> ids;
    QStringList x;
    QList<QStringList*> studentNamesInList = {&studentNames};
    QList<QList<int>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/users?enrollment_type[]=student", {"sortable_name"}, studentNamesInList, {"id"}, idsInList, {}, stringInSubobjectParams);

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
        roster << student;
    }

    return roster;
}

// Creates a teamset
bool CanvasHandler::createTeams(const QString &courseName, const QString &setName, const QStringList &teamNames, const QVector<QVector<StudentRecord>> &teams) {
    int courseID = getCourseID(courseName);
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
    for(int i = 0; i < teamNames.size(); i++) {
        //create one team
        url = "/api/v1/group_categories/" + QString::number(groupCategoryID[0]) + "/groups";
        query.clear();
        query.addQueryItem("name", teamNames.at(i));
        postData = query.toString(QUrl::FullyEncoded).toUtf8();
        QStringList groupName;
        QList<int> groupID;
        stringParams = {&groupName};
        intParams = {&groupID};
        postToCanvasGetSingleResult(url, postData, {"name"}, stringParams, {"id"}, intParams, {}, stringInSubobjectParams);

        //add each student on the team
        for(const auto &student : teams.at(i)) {
            url = "/api/v1/groups/" + QString::number(groupID[0]) + "/memberships";
            query.clear();
            query.addQueryItem("user_id", QString::number(student.LMSID));
            postData = query.toString(QUrl::FullyEncoded).toUtf8();
            QStringList workflowState;
            QList<int> membershipID, newUserID;
            stringParams = {&workflowState};
            intParams = {&membershipID, &newUserID};
            postToCanvasGetSingleResult(url, postData, {"workflow_state"}, stringParams, {"id", "user_id"}, intParams, {}, stringInSubobjectParams);
            allGood = allGood && (workflowState.constFirst() == "accepted") && (newUserID.constFirst() == student.LMSID);
        }
    }

    return allGood;
}

bool CanvasHandler::createSurvey(const QString &courseName, const Survey *const survey) {
    int courseID = getCourseID(courseName);
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
    int surveyID = quizID.constFirst();

    //add each question
    int questionNum = 0;
    for(const auto &question : survey->questions) {
        //create one question
        url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(surveyID) + "/questions";
        QStringList newQuestionText;
        QList<int> questionID;
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
        case Question::QuestionType::schedule:{
            if(survey->schedDayNames.size() == 1)
            {
                //just one question, set it up to post
                query.addQueryItem("question[question_type]", "multiple_answers_question");
                query.addQueryItem("question[question_text]", question.text + " <strong><u>[" + survey->schedDayNames.first() + "]</u></strong>");
                int optionNum = 0;
                for(int i = survey->schedStartTime; i <= survey->schedEndTime; i++) {
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]",
                                                 QString::number((i <= 12) ? i : (i - 12)) + ":00" + (i < 12 ? "am" : "pm"));
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                    optionNum++;
                }
            }
            else
            {
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
                    for(int i = survey->schedStartTime; i <= survey->schedEndTime; i++) {
                        query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]",
                                           QString::number((i <= 12) ? i : (i - 12)) + ":00" + (i < 12 ? "am" : "pm"));
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
                                   "<table style=\"border-collapse: collapse; width: " + QString::number(80 * (2 + survey->schedEndTime - survey->schedStartTime)) + "px; "
                                                                             "height: " + QString::number(10 * (1 + survey->schedDayNames.size())) + "px;\" border=\"1\">"
                                   "<tbody>" +
                                   rowText +
                                   cellText + "</td>";
            for(int i = survey->schedStartTime; i <= survey->schedEndTime; i++) {
                questionText += cellText + QString::number(i) + ":00" + (i < 12 ? " am" : " pm") + "</span></td>";
            }
            questionText += "</tr>";
            int responseBox = 101;
            int responseNum = 0;
            for(const auto &dayName : survey->schedDayNames) {
                questionText += rowText +
                                cellText + dayName + "</td>";
                for(int i = survey->schedStartTime; i <= survey->schedEndTime; i++) {
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
    int courseID = getCourseID(courseName);
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
    int courseID = getCourseID(courseName);
    if(courseID == -1) {
        return {};
    }

    int quizID = getQuizID(quizName);
    if(quizID == -1) {
        return {};
    }

    QUrl URL = getQuizResultsURL(courseID, quizID);
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
    QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), quizName.simplified().replace(' ','_') + ".csv");
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
    QString url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports";
    QUrlQuery query;
    query.addQueryItem("quiz_report[report_type]", "student_analysis");
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
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

    do
    {
        reply = canvas->get(url);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if(reply->bytesAvailable() == 0)
        {
            //qDebug() << "no reply";
            delete reply;
            return;
        }

        replyHeader = reply->rawHeader("Link");
        replyBody = reply->readAll();
        //qDebug() << replyBody;
        json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
        if(json_doc.isArray())
        {
            json_array = json_doc.array();
        }
        else if(json_doc.isObject())
        {
            json_array << json_doc.object();
        }
        else
        {
            //empty or null
            break;
        }

        for(const auto &value : qAsConst(json_array))
        {
            QJsonObject json_obj = value.toObject();
            for(int i = 0; i < stringParams.size(); i++)
            {
                *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
            }
            for(int i = 0; i < intParams.size(); i++)
            {
                *(intVals[i]) << json_obj[intParams.at(i)].toInt();
            }
            for(int i = 0; i < stringInSubobjectParams.size(); i++)
            {
                QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
                QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
                *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
            }
        }
        QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
        QRegularExpressionMatch nextURLMatch = nextURL.match(replyHeader);
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
    QString url = baseURL+URL;
    QString replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;

    auto *reply = canvas->post(url, postData);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0)
    {
        //qDebug() << "no reply";
        delete reply;
        return;
    }

    replyBody = reply->readAll();
    //qDebug() << replyBody;
    json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
    if(json_doc.isArray())
    {
        json_array = json_doc.array();
    }
    else if(json_doc.isObject())
    {
        json_array << json_doc.object();
    }
    else
    {
        //empty or null
        reply->deleteLater();
        return;
    }

    for(const auto &value : qAsConst(json_array))
    {
        QJsonObject json_obj = value.toObject();
        for(int i = 0; i < stringParams.size(); i++)
        {
            *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
        }
        for(int i = 0; i < intParams.size(); i++)
        {
            *(intVals[i]) << json_obj[intParams.at(i)].toInt();
        }
        for(int i = 0; i < stringInSubobjectParams.size(); i++)
        {
            QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
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
    if(reply->bytesAvailable() == 0)
    {
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
    canvas->setScope("placeholder");
    QAbstractOAuth2::connect(canvas, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    const QUrl redirectUri = QString(REDIRECT_URI);
    const auto port = static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT));

    canvas->setClientIdentifier(CLIENT_ID);
    canvas->setClientIdentifierSharedKey(CLIENT_SECRET);

    canvas->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QVariantMap* parameters) {
       // Percent-decode the "code" parameter so Google can match it
       if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
          QByteArray code = parameters->value("code").toByteArray();
          (*parameters)["code"] = QUrl::fromPercentEncoding(code);
       }
    });

    auto *replyHandler = new QOAuthHttpServerReplyHandler(port);
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
    getCanvasInfoDialog->setWindowIcon(QIcon(":/icons/canvas.png"));
    getCanvasInfoDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    auto *label = new QLabel(tr("The next step will download the roster from your Canvas course. This feature is currently in beta.\n"
                            "You will only have to perform the following steps once.\n\n"
                            "1) enter your institution's canvas URL (e.g.: https://example.instructure.com) in the first field below.\n"
                            "2) create a token so that gruepr can access your Canvas account. You can generally do this by:\n"
                            "   »  Log into Canvas,\n"
                            "   »  click \"Account\" in the left menu\n"
                            "   »  click \"Settings\", \n"
                            "   »  scroll to Approved Integration,\n"
                            "   »  click \"+ New Access Token\",\n"
                            "   »  fill in \"gruepr\" for the Purpose field and keep the expiration date blank,\n"
                            "   »  click \"Generate Token\", and\n"
                            "   »  copy your freshly generated token and paste it into the second field below.\n\n"));
    auto *canvasURL = new QLineEdit(currentURL);
    canvasURL->setPlaceholderText(tr("Canvas URL (e.g., https://example.instructure.com)"));
    auto *canvasToken = new QLineEdit(currentToken);
    canvasToken->setPlaceholderText(tr("User-generated Canvas token"));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, getCanvasInfoDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, getCanvasInfoDialog, &QDialog::reject);
    vLayout->addWidget(label);
    vLayout->addWidget(canvasURL);
    vLayout->addWidget(canvasToken);
    vLayout->addWidget(buttonBox);
    getCanvasInfoDialog->setLayout(vLayout);

    if(getCanvasInfoDialog->exec() == QDialog::Rejected)
    {
        return {};
    }
    return {canvasURL->text(), canvasToken->text()};
}

void CanvasHandler::setBaseURL(const QString &baseAPIURL) {
    baseURL = baseAPIURL;
}

QMessageBox* CanvasHandler::busy(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *busyDialog = new QMessageBox(parent);
    QPixmap icon(":/icons/canvas.png");
    busyDialog->setIconPixmap(icon.scaled(CANVASICONSIZE));
    busyDialog->setText(tr("Communicating with Canvas..."));
    busyDialog->setStandardButtons(QMessageBox::NoButton);
    busyDialog->setModal(false);
    busyDialog->show();
    return busyDialog;
}

void CanvasHandler::notBusy(QMessageBox *busyDialog) {
    QApplication::restoreOverrideCursor();
    busyDialog->close();
    delete busyDialog;
}
