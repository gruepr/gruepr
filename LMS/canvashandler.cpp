#include "canvashandler.h"
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

CanvasHandler::CanvasHandler(QWidget *parent) : LMS(parent), parent(parent) {
    initOAuth2();

    const QSettings settings;
    const QByteArray key = "gruepr";

    QByteArray encToken = settings.value("CanvasRefreshToken", "").toByteArray();
    if(!encToken.isEmpty()) {
        for (int i = 0; i < encToken.size(); ++i) {
            encToken[i] ^= key[i%key.size()];
        }
        OAuthFlow->setRefreshToken(encToken);
    }

    setBaseURL(baseAPIURL);
}

CanvasHandler::~CanvasHandler() {
    if(!OAuthFlow->refreshToken().isEmpty()) {
        QSettings settings;
        const QByteArray key = "gruepr";
        QByteArray encToken = OAuthFlow->refreshToken().toUtf8();
        for (int i = 0; i < encToken.size(); ++i) {
            encToken[i] ^= key[i%key.size()];
        }
        settings.setValue("CanvasRefreshToken", encToken);
    }
}

bool CanvasHandler::authenticate() {

    //***************************************************
    //IN BETA--GETS USER'S API TOKEN MANUALLY
    QSettings savedSettings;
    const QByteArray key = "gruepr";    // used to prevent saving the URL and token as plaintext
    QString savedCanvasNickname = savedSettings.value("canvasNickname", "").toString();
    QString savedCanvasURL = savedSettings.value("canvasURL", "").toString();
    QString savedCanvasToken = savedSettings.value("canvasToken", "").toString();
    //older versions of gruepr saves as plaintext qstring; if saved as QByteArray, need to 'decrypt'
    if(!savedCanvasURL.isEmpty() && !savedCanvasURL.contains("http", Qt::CaseInsensitive)) {
        QByteArray encURL = savedSettings.value("canvasURL", "").toByteArray();
        for (int i = 0; i < encURL.size(); ++i) {
            encURL[i] ^= key[i%key.size()];
        }
        savedCanvasURL = encURL;
        QByteArray encToken = savedSettings.value("canvasToken", "").toByteArray();
        for (int i = 0; i < encToken.size(); ++i) {
            encToken[i] ^= key[i%key.size()];
        }
        savedCanvasToken = encToken;
    }

    const QStringList newAccountnameURLAndToken = askUserForManualURLandToken(savedCanvasNickname, savedCanvasURL, savedCanvasToken);
    if(newAccountnameURLAndToken.size() != 3) {
        return false;
    }

    savedCanvasNickname = (newAccountnameURLAndToken.at(0).isEmpty() ? savedCanvasNickname : newAccountnameURLAndToken.at(0));
    savedCanvasURL = (newAccountnameURLAndToken.at(1).isEmpty() ? savedCanvasURL : newAccountnameURLAndToken.at(1));
    savedCanvasToken =  (newAccountnameURLAndToken.at(2).isEmpty() ? savedCanvasToken : newAccountnameURLAndToken.at(2));
    if(savedCanvasNickname.isEmpty() || savedCanvasURL.isEmpty() || savedCanvasToken.isEmpty()) {
        return false;
    }

    QByteArray encURL = savedCanvasURL.toUtf8();
    for (int i = 0; i < encURL.size(); ++i) {
        encURL[i] ^= key[i%key.size()];
    }
    QByteArray encToken = savedCanvasToken.toUtf8();
    for (int i = 0; i < encToken.size(); ++i) {
        encToken[i] ^= key[i%key.size()];
    }
    savedSettings.setValue("canvasNickname", savedCanvasNickname);
    savedSettings.setValue("canvasURL", encURL);
    savedSettings.setValue("canvasToken", encToken);

    setBaseURL(savedCanvasURL);
    authenticateWithManualToken(savedCanvasToken);

    return true;
    //IN BETA--GETS USER'S API TOKEN MANUALLY
    //***************************************************

    QDialog *loginDialog = actionDialog(parent);

    connect(OAuthFlow, &QOAuth2AuthorizationCodeFlow::granted, loginDialog, [this, &loginDialog]() {
        //Authorization granted
        actionDialogLabel->setText(actionDialogLabel->text() + "<br>" + tr("Login successful."));
        loginDialog->adjustSize();
        QEventLoop loop;
        QTimer::singleShot(UI_DISPLAY_DELAYTIME / 2, &loop, &QEventLoop::quit);
        loop.exec();
        actionComplete(loginDialog);
    });
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, loginDialog, [this, &loginDialog](const QString &error) {
        if((OAuthFlow->status() == QAbstractOAuth::Status::RefreshingToken) &&
            (error.contains("ProtocolInvalidOperationError", Qt::CaseInsensitive) || error.contains("AuthenticationRequiredError", Qt::CaseInsensitive))) {
            //Need user to re-grant permission
            actionDialogLabel->setText(actionDialogLabel->text() + "<br>" + tr("Canvas is requesting that you re-authorize gruepr."));
            loginDialog->adjustSize();
            OAuthFlow->setRefreshToken("");
            QEventLoop loop;
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            loginDialog->accept();
        }
        else {
            //Unknown connection error, will need to start over
            actionDialogLabel->setText(actionDialogLabel->text() + "<br>" + tr("There is an error connecting to Canvas.<br>Plese retry later."));
            loginDialog->adjustSize();
            QEventLoop loop;
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            loginDialog->reject();
        }
    });

    if(!OAuthFlow->refreshToken().isEmpty()) {
        //RefreshToken is found, try to use it to get accessTokens without having to re-grant permission
        actionDialogLabel->setText(tr("Connecting to Canvas..."));
        actionDialogButtons->setStandardButtons(QDialogButtonBox::Cancel);
        connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);
        actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        loginDialog->adjustSize();

        LMS::authenticate();

        if(loginDialog->exec() == QDialog::Rejected) {
            actionComplete(loginDialog);
            return false;
        }
    }

    if(!authenticated()) {
        // didn't have a refreshToken, the refreshToken didn't work, or user wants to change accounts: time to (re)authorize
        actionDialogLabel->setText(tr("The next step will open a browser window so you can sign in with Canvas "
                                      "and then authorize gruepr to access the surveys that it creates in your Canvas class. "
                                      "You will need to select all checkboxes to provide this authorization.<br><br>"
                                      "ALL data associated with your survey will exist in your Canvas class only. "
                                      "No data will ever be stored or sent anywhere else."));
        actionDialogLabel->setWordWrap(true);
        actionDialogButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);
        actionDialogButtons->setStyleSheet("");
        actionDialogButtons->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
        actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        loginDialog->show();
        loginDialog->adjustSize();

        if(loginDialog->exec() == QDialog::Rejected) {
            actionComplete(loginDialog);
            return false;
        }

        actionDialogLabel->setText(tr("Please use your browser to log in<br>to Canvas and then return here."));
        actionDialogLabel->setWordWrap(false);
        actionDialogButtons->setStandardButtons(QDialogButtonBox::Cancel);
        actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        loginDialog->show();
        loginDialog->adjustSize();
        connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);

        LMS::authenticate();

        if(loginDialog->exec() == QDialog::Rejected) {
            actionComplete(loginDialog);
            return false;
        }
    }

    return true;
}

// Retrieves course names, and loads the names, Canvas course IDs, and student counts
QList<CanvasHandler::CanvasCourse> CanvasHandler::getCourses() {
    QStringList courseNames;
    QStringList courseCreatedDates;
    QList<int> ids, studentCounts;
    QStringList x;
    QList<int> y;
    QList<QStringList*> courseNamesAndCreatedDatesInList = {&courseNames, &courseCreatedDates};
    QList<QList<int>*> idsAndStudentCounts = {&ids, &studentCounts};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<int>*> intInSubArrayParams = {&y};

    getPaginatedCanvasResults("/api/v1/courses?include[]=total_students",
                              {"name", "created_at"}, courseNamesAndCreatedDatesInList,
                              {"id", "total_students"}, idsAndStudentCounts,
                              {}, stringInSubobjectParams,
                              {}, intInSubArrayParams);
    courseNames.removeAll("");

    canvasCourses.clear();
    for(int i = 0; i < courseNames.size(); i++) {
        canvasCourses.append({courseNames.at(i), ids.at(i), studentCounts.at(i), QDateTime::fromString(courseCreatedDates.at(i), Qt::ISODate)});
    }
    std::sort(canvasCourses.begin(), canvasCourses.end(), [](const CanvasCourse &courseA, const CanvasCourse &courseB){return (courseA.creationDate > courseB.creationDate);});

    return canvasCourses;
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

    // First get the name, email address, and canvasID# of all students in the class
    QStringList studentNames;
    QStringList studentEmails;
    QList<int> ids;
    QStringList x;
    QList<int> y;
    QList<QStringList*> studentNamesandEmailsInList = {&studentNames, &studentEmails};
    QList<QList<int>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<int>*> intInSubArrayParams = {&y};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/users?enrollment_type[]=student",
                              {"sortable_name", "email"}, studentNamesandEmailsInList,
                              {"id"}, idsInList,
                              {}, stringInSubobjectParams,
                              {}, intInSubArrayParams);
    QStringList firstNames, lastNames;
    for(const auto &studentName : studentNames) {
        auto names = studentName.split(',');
        firstNames << (names.at(1).isEmpty()? "" : names.at(1).trimmed());
        lastNames << (names.at(0).isEmpty()? "" : names.at(0).trimmed());
    }

    // Next get the name and CanvasID# of every section in the class
    QStringList sectionNames;
    QList<QStringList*> sectionNamesInList = {&sectionNames};
    QList<int> sectionIDs;
    QList<QList<int>*> sectionIdsInList = {&sectionIDs};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/sections",
                              {"name"}, sectionNamesInList,
                              {"id"}, sectionIdsInList,
                              {}, stringInSubobjectParams,
                              {}, intInSubArrayParams);

    // Now match up each student in the class to their section by downloading
    // the roster for each section and using student CanvasID# to form match
    QStringList studentSections(ids.size());
    QList<int> idsInThisSection;
    QList<QList<int>*> idsInThisSectionInList = {&idsInThisSection};
    for(int i = 0; i < sectionIDs.size(); i++) {
        idsInThisSection.clear();
        getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/sections/"
                                  + QString::number(sectionIDs.at(i)) + "?include[]=students",
                                  {}, stringInSubobjectParams,
                                  {}, intInSubArrayParams,
                                  {}, stringInSubobjectParams,
                                  {"students/id"}, idsInThisSectionInList);
        for(int idNum = 0; idNum < ids.size(); idNum++) {
            if(idsInThisSection.contains(ids.at(idNum))) {
                studentSections[idNum] = sectionNames.at(i);
            }
        }
    }

    // Finally, package everything together as a list of studentrecords
    roster.clear();
    roster.reserve(studentNames.size());
    for(int i = 0; i < studentNames.size(); i++) {
        StudentRecord student;
        student.firstname = firstNames.at(i);
        student.lastname = lastNames.at(i);
        student.LMSID = ids.at(i);
        student.email = studentEmails.at(i);
        student.section = studentSections.at(i);
        roster << student;
    }

    return roster;
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
    if(quizID.isEmpty()) {
        return false;
    }
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
    QList<int> y;
    QList<QStringList*> titlesInList = {&titles};
    QList<QList<int>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<int>*> intInSubArrayParams = {&y};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes",
                              {"title"}, titlesInList,
                              {"id"}, idsInList,
                              {}, stringInSubobjectParams,
                              {}, intInSubArrayParams);

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
    QList<int> y;
    QList<QStringList*> stringParams = {&x};
    QList<QList<int>*> intParams = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&filename};
    QList<QList<int>*> intInSubArrayParams = {&y};
    // check every two seconds--a file object (including a download URL) is added to the json results when it is ready
    do {
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
        filename.clear();
        ids.clear();
        getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports",
                                  {}, stringParams,
                                  {"id"}, intParams,
                                  {"file/filename"}, stringInSubobjectParams,
                                  {}, intInSubArrayParams);
    } while(filename.first().isEmpty());
    const QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::TempLocation), quizName.simplified().replace(' ','_') + ".csv");
    // sometimes still a delay, so attempt to download every two seconds
    while(!downloadFile(URL, filepath.absoluteFilePath())) {
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
    }

    return filepath.absoluteFilePath();
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
        postToCanvasGetSingleResult(url, postData,
                                    {}, {stringParams},
                                    {"id"}, intParams,
                                    {"file/url"}, stringInSubobjectParams);
    } while(quizReportFileURL.first().isEmpty());

    return {quizReportFileURL.first()};
}

void CanvasHandler::getPaginatedCanvasResults(const QString &initialURL, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                         const QStringList &intParams, QList<QList<int>*> &intVals,
                                                                         const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals,
                                                                         const QStringList &intInSubArrayParams, QList<QList<int>*> &intInSubArrayVals) {
    QEventLoop loop;
    QNetworkReply *reply = nullptr;
    QString url = baseURL+initialURL, replyHeader;
    QByteArray replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;
    int numPages = 0;

    do {
        reply = OAuthFlow->get(url);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if(reply->bytesAvailable() == 0) {
            //qDebug() << "no reply";
            reply->deleteLater();
            return;
        }

        replyHeader = reply->rawHeader("Link");
        replyBody = reply->readAll();
        //qDebug() << replyBody;
        json_doc = QJsonDocument::fromJson(replyBody);
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
            const QJsonObject json_obj = value.toObject();
            for(int i = 0; i < stringParams.size(); i++) {
                *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
            }
            for(int i = 0; i < intParams.size(); i++) {
                *(intVals[i]) << json_obj[intParams.at(i)].toInt();
            }
            for(int i = 0; i < stringInSubobjectParams.size(); i++) {
                const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
                const QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
                *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
            }
            for(int i = 0; i < intInSubArrayParams.size(); i++) {
                const QStringList subarrayAndParamName = intInSubArrayParams.at(i).split('/');   // "subarray_name/int_paramater_name"
                const QJsonArray array = json_obj[subarrayAndParamName.at(0)].toArray();
                for(const auto &item : array) {
                    QJsonObject subobj = item.toObject();
                    *(intInSubArrayVals[i]) << subobj[subarrayAndParamName.at(1)].toInt();
                }
            }
        }
        static const QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
        const QRegularExpressionMatch nextURLMatch = nextURL.match(replyHeader);
        url = nextURLMatch.captured(1);
    }
    while(!url.isNull() && ++numPages < NUM_PAGES_TO_LOAD);

    reply->deleteLater();
}

void CanvasHandler::postToCanvasGetSingleResult(const QString &URL, const QByteArray &postData,
                                                const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                const QStringList &intParams, QList<QList<int>*> &intVals,
                                                const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals) {
    const auto replyBody = httpRequest(Method::post, baseURL + URL, postData);

    if(replyBody.isEmpty()) {
        return;
    }

    //qDebug() << replyBody;
    const QJsonDocument json_doc = QJsonDocument::fromJson(replyBody);
    QJsonArray json_array;
    if(json_doc.isArray()) {
        json_array = json_doc.array();
    }
    else if(json_doc.isObject()) {
        json_array << json_doc.object();
    }
    else {
        //empty or null
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
}

bool CanvasHandler::downloadFile(const QUrl &URL, const QString &filepath) {
    const auto replyBody = httpRequest(Method::get, URL);
    if(replyBody.isEmpty()) {
        //qDebug() << "empty reply";
        return false;
    }
    //qDebug() << replyBody.first(std::min(200, int(replyBody.size())));
    QFile file(filepath);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out << replyBody;
    return true;
}

// For testing: sets token manually
void CanvasHandler::authenticateWithManualToken(const QString &token) {
    OAuthFlow->setToken(token);
}

QStringList CanvasHandler::askUserForManualURLandToken(const QString &currentAccountName, const QString &currentURL, const QString &currentToken) {
    auto *getCanvasInfoDialog = new QDialog;
    getCanvasInfoDialog->setWindowTitle(tr("Connect to Canvas"));
    getCanvasInfoDialog->setWindowIcon(QIcon(":/icons_new/canvas.png"));
    getCanvasInfoDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    auto *instructions = new QLabel(tr("Completing the steps below would allow gruepr to interact with your Canvas course using a self-generated access token.\n"
                                    "This feature is not officially allowed by Canvas and is included for testing purposes only.\n"
                                    "This information will not be sent anywhere and should not be shared with anyone.\n\n"
                                    "You only have to perform the following steps once.\n"
                            "1) enter your institution's canvas URL in the second field below.\n"
                            "2) create a token so that gruepr can access your Canvas account. You can generally do this by:\n"
                            "   »  Log into Canvas,\n"
                            "   »  click \"Account\" in the left menu\n"
                            "   »  click \"Settings\", \n"
                            "   »  scroll to Approved Integration,\n"
                            "   »  click \"+ New Access Token\",\n"
                            "   »  fill in \"gruepr\" for the Purpose field and keep the expiration date blank,\n"
                            "   »  click \"Generate Token\", and\n"
                            "   »  copy your freshly generated token and paste it into the third field below.\n"
                            "Depending on your institution's settings, you may have to request that "
                            "your token be generated by the campus Canvas administrators.\n\n"), getCanvasInfoDialog);
    instructions->setStyleSheet(LABEL10PTSTYLE);
    auto *accountnameLabel = new QLabel(tr("Using Canvas account:") + "<br>" + currentAccountName, getCanvasInfoDialog);
    accountnameLabel->setStyleSheet(LABEL12PTSTYLE);

    auto *canvasAccountName = new QLineEdit(currentAccountName, getCanvasInfoDialog);
    canvasAccountName->setPlaceholderText(tr("Account nickname (optional)"));
    canvasAccountName->setStyleSheet(LINEEDITSTYLE);
    auto *canvasURL = new QLineEdit(currentURL, getCanvasInfoDialog);
    canvasURL->setPlaceholderText(tr("Your Canvas URL (e.g., https://exampleschool.instructure.com)"));
    canvasURL->setStyleSheet(LINEEDITSTYLE);
    auto *canvasToken = new QLineEdit(currentToken, getCanvasInfoDialog);
    canvasToken->setPlaceholderText(tr("User-generated Canvas token"));
    canvasToken->setStyleSheet(LINEEDITSTYLE);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, getCanvasInfoDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Proceed"));
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    auto *showInputsButton = buttonBox->button(QDialogButtonBox::Cancel);
    showInputsButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, getCanvasInfoDialog, &QDialog::accept);

    connect(canvasURL, &QLineEdit::textChanged, buttonBox, [buttonBox, canvasURL, canvasToken]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   !(canvasURL->text().isEmpty()) && !(canvasToken->text().isEmpty()));});
    connect(canvasToken, &QLineEdit::textChanged, buttonBox, [buttonBox, canvasURL, canvasToken]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   !(canvasURL->text().isEmpty()) && !(canvasToken->text().isEmpty()));});

    auto *vLayout = new QVBoxLayout;
    vLayout->addWidget(instructions);
    vLayout->addWidget(accountnameLabel);
    vLayout->addWidget(canvasAccountName);
    vLayout->addWidget(canvasURL);
    vLayout->addWidget(canvasToken);
    vLayout->addWidget(buttonBox);
    getCanvasInfoDialog->setLayout(vLayout);

    if(!currentURL.isEmpty() && !currentToken.isEmpty()) {
        // show simple dialog to proceed with already existing URL and token
        showInputsButton->setText(tr("Change account"));
        showInputsButton->setIcon(QIcon(":/icons_new/downButton.png"));
        showInputsButton->setLayoutDirection(Qt::RightToLeft);   // icon on right side
        showInputsButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        instructions->hide();
        canvasAccountName->hide();
        canvasURL->hide();
        canvasToken->hide();
        auto *receiver = new QObject(getCanvasInfoDialog);
        connect(showInputsButton, &QPushButton::clicked, receiver,
                [getCanvasInfoDialog, instructions, accountnameLabel, canvasAccountName, canvasURL, canvasToken, showInputsButton, buttonBox, receiver]
                { instructions->show();
                  accountnameLabel->hide();
                  canvasAccountName->show();
                  canvasURL->show();
                  canvasToken->show();
                  showInputsButton->setIcon(QIcon());
                  showInputsButton->setText(tr("Cancel"));
                  getCanvasInfoDialog->adjustSize();
                  connect(buttonBox, &QDialogButtonBox::rejected, getCanvasInfoDialog, &QDialog::reject);
                  receiver->deleteLater();  // makes sure this lambda only runs once
                });
    }
    else {
        accountnameLabel->hide();
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        showInputsButton->setText(tr("Cancel"));
        connect(buttonBox, &QDialogButtonBox::rejected, getCanvasInfoDialog, &QDialog::reject);
    }

    auto result = getCanvasInfoDialog->exec();
    getCanvasInfoDialog->deleteLater();

    if(result == QDialog::Rejected) {
        return {};
    }

    QString url = canvasURL->text().trimmed();
    if(!url.startsWith("https://") && !url.startsWith("http://")) {
        url.prepend("https://");
    }
    QString accountName = canvasAccountName->text().trimmed();
    if(accountName.isEmpty() && !url.isEmpty()) {
        // for a default nickname, grab the first part of the url (i.e., "example" in https://example.instructure.com)
        const int startPos = url.indexOf('/') + 2;    // move to just after the "//"
        const int endPos = url.indexOf('.');
        accountName = url.mid(startPos, endPos - startPos);
        accountName[0] = accountName[0].toUpper();
    }
    const QString token = canvasToken->text().trimmed();

    return {accountName, url, token};
}

void CanvasHandler::setBaseURL(const QString &baseAPIURL) {
    baseURL = baseAPIURL;
}

QString CanvasHandler::getScopes() const {
    return SCOPES;
}

QString CanvasHandler::getClientID() const {
    return CLIENT_ID;
}

QString CanvasHandler::getClientSecret() const {
    return CLIENT_SECRET;
}

QString CanvasHandler::getClientAuthorizationUrl() const {
    return authenticateURL;
}

QString CanvasHandler::getClientAccessTokenUrl() const {
    return accessTokenURL;
}

QString CanvasHandler::getActionDialogIcon() const {
    return ICON;
}

QPixmap CanvasHandler::icon() {
    return {ICON};
}

QString CanvasHandler::getActionDialogLabel() const {
    return tr("Communicating with Canvas...");
}

std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> CanvasHandler::getModifyParametersFunction() const {
    //*********************************
    //copied from Google, almost definitely needs to be updated
    return [](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            // needed to get refresh_token from Google Cloud
            parameters->insert("access_type", "offline");
        }
        else if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            // Percent-decode the "code" parameter so Google can match it
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->replace("code", QUrl::fromPercentEncoding(code));
        }
    };
}
