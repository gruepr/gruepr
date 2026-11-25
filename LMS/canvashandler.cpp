#include "canvashandler.h"
#include "secureStore.h"
#include <algorithm>
#include <QDesktopServices>
#include <QDir>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

CanvasHandler::CanvasHandler(QWidget *parent) : LMS(parent), parent(parent) {
    initOAuth2();

    QSettings settings;
    const QByteArray key = "gruepr";

    QByteArray refreshToken;
    if(settings.contains("CanvasRefreshToken")) {
        // older versions of gruepr saved the token XOR'ed in QSettings; move it to the secure store
        refreshToken = settings.value("CanvasRefreshToken", "").toByteArray();
        for (int i = 0; i < refreshToken.size(); ++i) {
            refreshToken[i] ^= key[i%key.size()];
        }
        if(!refreshToken.isEmpty()) {
            SecureStore::write("CanvasRefreshToken", refreshToken);
        }
        settings.remove("CanvasRefreshToken");
    }
    else {
        refreshToken = SecureStore::read("CanvasRefreshToken");
    }
    if(!refreshToken.isEmpty()) {
        OAuthFlow->setRefreshToken(refreshToken);
    }

    setBaseURL(baseAPIURL);
}

CanvasHandler::~CanvasHandler() {
    if(!OAuthFlow->refreshToken().isEmpty()) {
        SecureStore::write("CanvasRefreshToken", OAuthFlow->refreshToken().toUtf8());
    }
}

bool CanvasHandler::authenticate() {

    //***************************************************
    //IN BETA--GETS USER'S API TOKEN MANUALLY
    QSettings savedSettings;
    const QByteArray key = "gruepr";    // used to prevent saving the URL as plaintext (and to read tokens saved by older versions)
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
    if(savedSettings.contains("canvasToken")) {
        // older versions of gruepr saved the token in QSettings; move it to the secure store
        if(!savedCanvasToken.isEmpty()) {
            SecureStore::write("canvasToken", savedCanvasToken.toUtf8());
        }
        savedSettings.remove("canvasToken");
    }
    else {
        savedCanvasToken = QString::fromUtf8(SecureStore::read("canvasToken"));
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
    savedSettings.setValue("canvasNickname", savedCanvasNickname);
    savedSettings.setValue("canvasURL", encURL);
    SecureStore::write("canvasToken", savedCanvasToken.toUtf8());

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
    QList<long long> ids, studentCounts;
    QStringList x;
    QList<long long> y;
    QList<QStringList*> courseNamesAndCreatedDatesInList = {&courseNames, &courseCreatedDates};
    QList<QList<long long>*> idsAndStudentCounts = {&ids, &studentCounts};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<long long>*> integerInSubArrayParams = {&y};

    getPaginatedCanvasResults("/api/v1/courses?include[]=total_students",
                              {"name", "created_at"}, courseNamesAndCreatedDatesInList,
                              {"id", "total_students"}, idsAndStudentCounts,
                              {}, stringInSubobjectParams,
                              {}, integerInSubArrayParams);
    canvasCourses.clear();
    for(int i = 0; i < courseNames.size(); i++) {
        if(courseNames.at(i).isEmpty()) {
            continue;   // e.g., date-restricted courses come back without a name; skip them
        }
        canvasCourses.append({courseNames.at(i), ids.at(i), int(studentCounts.at(i)), QDateTime::fromString(courseCreatedDates.at(i), Qt::ISODate)});
    }
    std::sort(canvasCourses.begin(), canvasCourses.end(), [](const CanvasCourse &courseA, const CanvasCourse &courseB){return (courseA.creationDate > courseB.creationDate);});

    return canvasCourses;
}

// Retrieves the number of students in the given course
int CanvasHandler::getStudentCount(long long courseID) {
    for(const auto &course : std::as_const(canvasCourses)) {
        if(course.ID == courseID) {
            return course.numStudents;
        }
    }
    return 0;
}

// Retrieves the StudentRoster in the given course
QList<StudentRecord> CanvasHandler::getStudentRoster(long long courseID, const std::function<void(int stepsDone, int numSteps)> &progress) {
    // Each request is a step: the students, the sections, then each section's students (so the number of steps is known once the sections are)
    if(progress) {
        progress(0, 0);
    }

    // First get the name, email address, and canvasID# of all students in the class
    QStringList studentNames;
    QStringList studentEmails;
    QList<long long> ids;
    QStringList x;
    QList<long long> y;
    QList<QStringList*> studentNamesandEmailsInList = {&studentNames, &studentEmails};
    QList<QList<long long>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<long long>*> integerInSubArrayParams = {&y};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/users?enrollment_type[]=student",
                              {"sortable_name", "email"}, studentNamesandEmailsInList,
                              {"id"}, idsInList,
                              {}, stringInSubobjectParams,
                              {}, integerInSubArrayParams);
    QStringList firstNames, lastNames;
    for(const auto &studentName : studentNames) {
        const auto names = studentName.split(',');
        lastNames << (names.isEmpty() ? "" : names.at(0).trimmed());
        firstNames << (names.size() > 1 ? names.at(1).trimmed() : "");
    }

    // Next get the name and CanvasID# of every section in the class
    QStringList sectionNames;
    QList<QStringList*> sectionNamesInList = {&sectionNames};
    QList<long long> sectionIDs;
    QList<QList<long long>*> sectionIdsInList = {&sectionIDs};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/sections",
                              {"name"}, sectionNamesInList,
                              {"id"}, sectionIdsInList,
                              {}, stringInSubobjectParams,
                              {}, integerInSubArrayParams);

    const int numSteps = 2 + int(sectionIDs.size());
    if(progress) {
        progress(2, numSteps);
    }

    // Now match up each student in the class to their section by downloading
    // the roster for each section and using student CanvasID# to form match
    QStringList studentSections(ids.size());
    QList<long long> idsInThisSection;
    QList<QList<long long>*> idsInThisSectionInList = {&idsInThisSection};
    for(int i = 0; i < sectionIDs.size(); i++) {
        idsInThisSection.clear();
        getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/sections/"
                                  + QString::number(sectionIDs.at(i)) + "?include[]=students",
                                  {}, stringInSubobjectParams,
                                  {}, integerInSubArrayParams,
                                  {}, stringInSubobjectParams,
                                  {"students/id"}, idsInThisSectionInList);
        for(int idNum = 0; idNum < ids.size(); idNum++) {
            if(idsInThisSection.contains(ids.at(idNum))) {
                studentSections[idNum] = sectionNames.at(i);
            }
        }
        if(progress) {
            progress(3 + i, numSteps);
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

bool CanvasHandler::createSurvey(long long courseID, const Survey *const survey,
                                 const std::function<void(int questionNum, int stepsDone, int numSteps)> &progress) {
    // each request is a step: one for the survey, then one for each question, except that a ranked choice question is posted as a
    // question for each rank, and a schedule question as a question for each day (plus an introduction, if there's more than one day)
    int numSteps = 1;
    for(const auto &question : survey->questions) {
        if(question.type == Question::QuestionType::rankedchoice) {
            numSteps += std::max(1, question.numRankedChoices);
        }
        else if(question.type == Question::QuestionType::schedule) {
            numSteps += ((survey->schedDayNames.size() > 1)? int(survey->schedDayNames.size()) + 1 : 1);
        }
        else {
            numSteps++;
        }
    }
    int stepsDone = 0;
    int surveyQuestionNum = -1;
    auto stepDone = [&progress, &stepsDone, &surveyQuestionNum, numSteps]() {
        stepsDone++;
        if(progress) {
            progress(surveyQuestionNum, stepsDone, numSteps);
        }
    };
    if(progress) {
        progress(surveyQuestionNum, stepsDone, numSteps);
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
    QList<long long> quizID;
    QStringList x;
    QList<QStringList*> stringParams = {&quizURL, &mobileQuizURL};
    QList<QList<long long>*> integerParams = {&quizID};
    QList<QStringList*> stringInSubobjectParams = {&x};
    postToCanvasGetSingleResult(url, postData, {"html_url", "mobile_url"}, stringParams, {"id"}, integerParams, {}, stringInSubobjectParams);
    stepDone();
    if(quizID.isEmpty()) {
        return false;
    }
    const long long surveyID = quizID.constFirst();

    //add each question
    int questionNum = 0;
    QStringList newQuestionText;
    QList<long long> questionID;
    for(const auto &question : survey->questions) {
        surveyQuestionNum++;
        if(progress) {
            progress(surveyQuestionNum, stepsDone, numSteps);
        }

        //create one question
        url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(surveyID) + "/questions";
        newQuestionText.clear();
        questionID.clear();
        stringParams = {&newQuestionText};
        integerParams = {&questionID};
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
        case Question::QuestionType::freeresponsenumber:
            query.addQueryItem("question[question_type]", "numerical_question");
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
        case Question::QuestionType::rankedchoice: {
            for(int rank = 0; rank < question.numRankedChoices; rank++) {
                if(rank > 0) {
                    // Post the previous rank's question, then start a new one
                    postData = query.toString(QUrl::FullyEncoded).toUtf8();
                    postToCanvasGetSingleResult(url, postData, {"question_text"}, stringParams,
                                                {"id"}, integerParams, {}, stringInSubobjectParams);
                    stepDone();
                    allGood = allGood && !newQuestionText.isEmpty() && !newQuestionText.constFirst().isEmpty();
                    questionNum++;
                    newQuestionText.clear();
                    questionID.clear();
                    query.clear();
                    query.addQueryItem("question[question_name]", "Question " + QString::number(questionNum + 1));
                    query.addQueryItem("question[position]", QString::number(questionNum + 1));
                }
                query.addQueryItem("question[question_type]", "multiple_dropdowns_question");
                query.addQueryItem("question[question_text]",
                                   question.text + " <strong> [" + (rank == 0? QString(RANKYOURFIRSTCHOICE) : (QString(RANKYOURCHOICE) + " "
                                       + QString::number(rank + 1))) + "]</strong>  [options]");
                int optionNum = 0;
                for(const auto &option : question.options) {
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][blank_id]", "options");
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_text]", option);
                    query.addQueryItem("question[answers][" + QString::number(optionNum) + "][answer_weight]", "100");
                    optionNum++;
                }
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
                scheduleIntroStatement += "</strong>. " + SCHEDULEQUESTIONINTRO3;
                query.addQueryItem("question[question_text]", scheduleIntroStatement);
                // post the question and then add a question for each day (final day will get posted outside of switch/case)
                for(const auto &dayName : survey->schedDayNames) {
                    postData = query.toString(QUrl::FullyEncoded).toUtf8();
                    postToCanvasGetSingleResult(url, postData, {"question_text"}, stringParams, {"id"}, integerParams, {}, stringInSubobjectParams);
                    stepDone();
                    allGood = allGood && !newQuestionText.isEmpty() && !newQuestionText.constFirst().isEmpty();
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
        postToCanvasGetSingleResult(url, postData, {"question_text"}, stringParams, {"id"}, integerParams, {}, stringInSubobjectParams);
        stepDone();
        allGood = allGood && !newQuestionText.isEmpty() && !newQuestionText.constFirst().isEmpty();
        questionNum++;
    }

    return allGood;
}

QList<CanvasHandler::CanvasQuiz> CanvasHandler::getQuizList(long long courseID) {
    QStringList titles;
    QList<long long> ids;
    QStringList x;
    QList<long long> y;
    QList<QStringList*> titlesInList = {&titles};
    QList<QList<long long>*> idsInList = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&x};
    QList<QList<long long>*> integerInSubArrayParams = {&y};
    getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes",
                              {"title"}, titlesInList,
                              {"id"}, idsInList,
                              {}, stringInSubobjectParams,
                              {}, integerInSubArrayParams);

    QList<CanvasQuiz> quizzes;
    for(int i = 0; i < titles.size(); i++) {
        quizzes.append({titles.at(i), ids.at(i)});
    }
    return quizzes;
}

QString CanvasHandler::downloadQuizResult(long long courseID, long long quizID, const QString &quizName,
                                          const std::function<void(ReportStage stage, int percentDone, int secondsWaited)> &progress) {
    // all of the waiting below, including in getQuizResultsURL(), shares one time limit and can be canceled by the user
    QElapsedTimer waitTimer;
    waitTimer.start();
    auto stopWaiting = [this, &waitTimer]() {
        if(downloadCanceled || (waitTimer.elapsed() > MAX_REPORT_WAIT_TIME)) {
            lastErrorMessage = (downloadCanceled? tr("Download canceled.") : tr("Canvas did not finish preparing the survey results in time."));
            return true;
        }
        return false;
    };

    // tells the caller how long it's been and, while Canvas prepares the results, how much of them Canvas says it has prepared
    // (Canvas is asked only while it is still preparing them)
    QString progressURL;
    int percentDone = -1;
    auto showProgress = [this, &progress, &progressURL, &percentDone, &waitTimer](const ReportStage stage, const bool askCanvas) {
        if(!progress) {
            return;
        }
        if(askCanvas) {
            percentDone = reportPercentDone(progressURL);
        }
        progress(stage, (stage == ReportStage::preparing)? percentDone : -1, int(waitTimer.elapsed() / 1000));
    };
    showProgress(ReportStage::preparing, false);

    const QUrl URL = getQuizResultsURL(courseID, quizID, waitTimer, [&progressURL, &showProgress](const QString &newProgressURL){
        if(!newProgressURL.isEmpty()) {
            progressURL = newProgressURL;
        }
        showProgress(ReportStage::preparing, true);
    });
    if(URL.isEmpty()) {
        return {};
    }
    if(percentDone >= 0) {
        percentDone = 100;      // the report is made (its file is just being readied for downloading)
    }

    // wait until the results file is ready
    QEventLoop loop;
    QStringList x;
    QList<long long> ids;
    QStringList filename;
    QList<long long> y;
    QList<QStringList*> stringParams = {&x};
    QList<QList<long long>*> integerParams = {&ids};
    QList<QStringList*> stringInSubobjectParams = {&filename};
    QList<QList<long long>*> integerInSubArrayParams = {&y};
    // check every two seconds--a file object (including a download URL) is added to the json results when it is ready
    do {
        if(stopWaiting()) {
            return {};
        }
        showProgress(ReportStage::preparing, false);
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
        filename.clear();
        ids.clear();
        getPaginatedCanvasResults("/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports",
                                  {}, stringParams,
                                  {"id"}, integerParams,
                                  {"file/filename"}, stringInSubobjectParams,
                                  {}, integerInSubArrayParams);
    } while(filename.isEmpty() || filename.first().isEmpty());
    static const QRegularExpression unallowedChars(R"([#&&{}\\/\<>*?$!'":@+`|=])");
    const QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::TempLocation), quizName.simplified().replace(' ','_').replace(unallowedChars, "_") + ".csv");
    // sometimes still a delay, so attempt to download every two seconds
    showProgress(ReportStage::downloading, false);
    while(!downloadFile(URL, filepath.absoluteFilePath())) {
        if(stopWaiting()) {
            return {};
        }
        showProgress(ReportStage::downloading, false);
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
    }

    return filepath.absoluteFilePath();
}

// Creates a teamset
bool CanvasHandler::createTeams(long long courseID, const QString &setName, const QStringList &teamNames, const QList<QList<StudentRecord>> &teams,
                                const std::function<void(int teamNum, int stepsDone, int numSteps)> &progress) {
    bool allGood = true;

    // each request is a step: one for the teamset, then one for each team and one for each student on it
    int numSteps = 1 + int(teamNames.size());
    for(int i = 0; i < teamNames.size(); i++) {
        numSteps += int(teams.at(i).size());
    }
    int stepsDone = 0;
    int teamNum = 0;
    auto stepDone = [&progress, &stepsDone, &teamNum, numSteps]() {
        stepsDone++;
        if(progress) {
            progress(teamNum, stepsDone, numSteps);
        }
    };
    if(progress) {
        progress(0, 0, numSteps);
    }

    //create the teamset
    QString url = "/api/v1/courses/" + QString::number(courseID) + "/group_categories";
    QUrlQuery query;
    query.addQueryItem("name", setName);
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QStringList groupCategoryName;
    QList<long long> groupCategoryID;
    QStringList x;
    QList<QStringList*> stringParams = {&groupCategoryName};
    QList<QList<long long>*> integerParams = {&groupCategoryID};
    QList<QStringList*> stringInSubobjectParams = {&x};
    postToCanvasGetSingleResult(url, postData, {"name"}, stringParams, {"id"}, integerParams, {}, stringInSubobjectParams);
    stepDone();

    if(groupCategoryID.empty()) {
        allGood = false;
        return allGood;
    }

    //create the teams
    QStringList groupName;
    QList<long long> groupID;
    QStringList workflowState;
    for(int i = 0; i < teamNames.size(); i++) {
        teamNum = i;
        if(progress) {
            progress(teamNum, stepsDone, numSteps);
        }

        //create one team
        url = "/api/v1/group_categories/" + QString::number(groupCategoryID[0]) + "/groups";
        query.clear();
        query.addQueryItem("name", teamNames.at(i));
        postData = query.toString(QUrl::FullyEncoded).toUtf8();
        groupName.clear();
        groupID.clear();
        stringParams = {&groupName};
        integerParams = {&groupID};
        postToCanvasGetSingleResult(url, postData, {"name"}, stringParams, {"id"}, integerParams, {}, stringInSubobjectParams);
        stepDone();

        if(groupID.empty()) {
            allGood = false;
            stepsDone += int(teams.at(i).size());      // (this team's students won't be added)
            continue;
        }

        //add each student on the team
        for(const auto &student : teams.at(i)) {
            url = "/api/v1/groups/" + QString::number(groupID[0]) + "/memberships";
            query.clear();
            query.addQueryItem("user_id", QString::number(student.LMSID));
            postData = query.toString(QUrl::FullyEncoded).toUtf8();
            QList<long long> membershipID, newUserID;
            workflowState.clear();
            stringParams = {&workflowState};
            integerParams = {&membershipID, &newUserID};
            postToCanvasGetSingleResult(url, postData, {"workflow_state"}, stringParams, {"id", "user_id"}, integerParams, {}, stringInSubobjectParams);
            stepDone();
            allGood = allGood && !workflowState.empty() && !newUserID.empty() &&
                      (workflowState.constFirst() == "accepted") && (newUserID.constFirst() == student.LMSID);
        }
    }

    return allGood;
}

// Creates a peer review's quizzes, one per team
bool CanvasHandler::createPeerReviewQuizzes(const long long courseID, PeerReview *const peerReview,
                                            const std::function<void(int teamNum, int stepsDone, int numSteps)> &progress) {
    const QString coursePath = "/api/v1/courses/" + QString::number(courseID);
    peerReview->canvasQuizzes.clear();
    peerReview->canvasQuizzes.resize(peerReview->teams.size());
    peerReview->canvasAnswerLabels.clear();
    peerReview->formItems.clear();
    peerReview->canvasAssignmentGroupID = -1;
    bool groupMadeHere = false;

    // undoes a partly-made peer review (keeping the error message that explains why); the assignment group is deleted only if it was just made
    auto deleteCreated = [this, &coursePath, peerReview, &groupMadeHere]() {
        const QString errorMessage = lastErrorMessage;
        for(auto &canvasQuiz : peerReview->canvasQuizzes) {
            if(canvasQuiz.quizID != -1) {
                httpRequest(Method::del, baseURL + coursePath + "/quizzes/" + QString::number(canvasQuiz.quizID));
            }
            canvasQuiz = {};
        }
        if(groupMadeHere && (peerReview->canvasAssignmentGroupID != -1)) {
            httpRequest(Method::del, baseURL + coursePath + "/assignment_groups/" + QString::number(peerReview->canvasAssignmentGroupID));
        }
        peerReview->canvasAssignmentGroupID = -1;
        peerReview->formItems.clear();
        peerReview->canvasAnswerLabels.clear();
        lastErrorMessage = errorMessage;
        return false;
    };

    // the course's peer review assignment group, which holds every peer review's team quizzes so they are together in the gradebook
    bool groupsOK = false;
    const QJsonArray groups = getAllPages(coursePath + "/assignment_groups", &groupsOK);
    if(!groupsOK) {
        return false;
    }
    for(const auto &group : groups) {
        if(group.toObject()["name"].toString().simplified().compare(PEERREVIEWASSIGNMENTGROUP, Qt::CaseInsensitive) == 0) {
            peerReview->canvasAssignmentGroupID = group.toObject()["id"].toInteger(-1);
            break;
        }
    }
    if(peerReview->canvasAssignmentGroupID == -1) {
        const QJsonObject group = jsonRequest(Method::post, coursePath + "/assignment_groups", QJsonObject{{"name", PEERREVIEWASSIGNMENTGROUP}}).toObject();
        peerReview->canvasAssignmentGroupID = group["id"].toInteger(-1);
        if(peerReview->canvasAssignmentGroupID == -1) {
            return false;
        }
        groupMadeHere = true;
    }

    // plain text (with blank lines between paragraphs) as html paragraphs
    auto paragraphs = [](const QString &text) {
        QString html;
        for(const auto &paragraph : text.split(QRegularExpression(R"(\n\s*\n)"), Qt::SkipEmptyParts)) {
            html += "<p>" + paragraph.trimmed().toHtmlEscaped().replace("\n", "<br>") + "</p>";
        }
        return html;
    };

    // how a question is asked in a team's quiz
    enum class Layout {dropdownForEach, numberForEach, questionForEach, single};
    auto layoutOf = [](const PeerReviewQuestion &question) {
        const bool hasOptions = !question.formLabels().isEmpty();
        if(!question.isAboutTeammates()) {
            return Layout::single;                  // about yourself, or asked once
        }
        if(hasOptions && !question.multi) {
            return Layout::dropdownForEach;         // one question with a dropdown for each team member
        }
        if(!hasOptions && ((question.kind == PeerReviewQuestion::Kind::numberEntry) || (question.kind == PeerReviewQuestion::Kind::contribution))) {
            return Layout::numberForEach;           // one question with a box for a number for each team member
        }
        return Layout::questionForEach;             // a separate question for each team member (checkboxes, or an essay)
    };

    // the progress, counted in requests to Canvas: for each team with a quiz, one per question (one per member for a question asked
    // separately about each), plus five more (create the quiz, get its assignment, show it to the team, publish it, and set its grading)
    int numSteps = 0;
    int stepsDone = 0;
    for(const auto &team : peerReview->teams) {
        if(std::none_of(team.members.cbegin(), team.members.cend(), [](const PeerReviewStudent &member){return member.LMSID > 0;})) {
            continue;
        }
        numSteps += 5;
        for(const auto &question : peerReview->questions) {
            numSteps += (layoutOf(question) == Layout::questionForEach)? int(team.members.size()) : 1;
        }
    }
    auto stepDone = [&progress, &stepsDone, numSteps](const int teamNum) {
        stepsDone++;
        if(progress) {
            progress(teamNum, stepsDone, numSteps);
        }
    };

    for(int teamNum = 0; teamNum < peerReview->teams.size(); teamNum++) {
        const auto &team = peerReview->teams.at(teamNum);
        QStringList memberNames;        // with a number added to teammates who have the same name, as in the Google Form
        QList<long long> memberIDs;
        QJsonArray canvasStudentIDs;
        for(int memberNum = 0; memberNum < team.members.size(); memberNum++) {
            // (square brackets would be taken as a dropdown's place in a question, so they become parentheses)
            memberNames << peerReview->memberLabel(teamNum, memberNum).replace('[', '(').replace(']', ')').toHtmlEscaped();
            memberIDs << team.members.at(memberNum).ID;
            if(team.members.at(memberNum).LMSID > 0) {
                canvasStudentIDs << team.members.at(memberNum).LMSID;
            }
        }
        if(canvasStudentIDs.isEmpty()) {
            continue;   // none of this team is in the course
        }
        if(progress) {
            progress(teamNum, stepsDone, numSteps);
        }
        auto &canvasQuiz = peerReview->canvasQuizzes[teamNum];

        //create the quiz
        QJsonObject quiz;
        quiz["title"] = peerReview->title + " - " + team.name;
        quiz["description"] = paragraphs(peerReview->instructions) + "<p><strong>" + tr("Your team:") + "</strong> " + memberNames.join(", ") + "</p>";
        quiz["quiz_type"] = "graded_survey";
        quiz["points_possible"] = 1;
        quiz["allowed_attempts"] = -1;          // students can change their answers until the quiz is locked; the latest is kept
        quiz["scoring_policy"] = "keep_latest";
        quiz["only_visible_to_overrides"] = true;
        quiz["published"] = false;
        quiz["assignment_group_id"] = peerReview->canvasAssignmentGroupID;
        const QJsonObject createdQuiz = jsonRequest(Method::post, coursePath + "/quizzes", QJsonObject{{"quiz", quiz}}).toObject();
        canvasQuiz.quizID = createdQuiz["id"].toInteger(-1);
        canvasQuiz.url = createdQuiz["html_url"].toString();
        if(canvasQuiz.quizID == -1) {
            return deleteCreated();
        }
        stepDone(teamNum);
        const QString quizPath = coursePath + "/quizzes/" + QString::number(canvasQuiz.quizID);

        //add each question; each is worth 0 points, since the quiz's point is for completing it
        int position = 1;
        // adds one question, keeping what each of its options is, and returns the ID Canvas gave it (-1 if it couldn't be added)
        auto addQuestion = [this, &quizPath, &position, peerReview, &stepDone, teamNum](QJsonObject question, const QString &questionName) {
            question["question_name"] = questionName;
            question["position"] = position++;
            question["points_possible"] = 0;
            const QJsonObject created = jsonRequest(Method::post, quizPath + "/questions", QJsonObject{{"question", question}}).toObject();
            const long long questionID = created["id"].toInteger(-1);
            if(questionID != -1) {
                // (Canvas's option IDs are unique only within a question, so each is kept with its question's ID)
                for(const auto &answer : created["answers"].toArray()) {
                    const QJsonObject answerObject = answer.toObject();
                    peerReview->canvasAnswerLabels.insert(QString::number(questionID) + ":" + QString::number(answerObject["id"].toInteger()),
                                                          answerObject["text"].toString());
                }
                stepDone(teamNum);
            }
            return questionID;
        };
        // the options of a question, for each of the named blanks (or for the whole question if there are no blanks)
        auto answersFor = [](const QStringList &options, const QStringList &blankIDs) {
            QJsonArray answers;
            for(const auto &blankID : (blankIDs.isEmpty()? QStringList{""} : blankIDs)) {
                for(const auto &option : options) {
                    QJsonObject answer{{"answer_text", option}, {"answer_weight", 100}};
                    if(!blankID.isEmpty()) {
                        answer["blank_id"] = blankID;
                    }
                    answers << answer;
                }
            }
            return answers;
        };
        // a row for each team member, each with a blank (a dropdown or a box) in it
        QStringList blankIDs;
        QString rows;
        for(int memberNum = 0; memberNum < memberNames.size(); memberNum++) {
            blankIDs << "m" + QString::number(memberNum + 1);
            rows += "<p>" + memberNames.at(memberNum) + ": [" + blankIDs.last() + "]</p>";
        }

        for(int questionNum = 0; questionNum < peerReview->questions.size(); questionNum++) {
            const auto &peerReviewQuestion = peerReview->questions.at(questionNum);
            const Layout layout = layoutOf(peerReviewQuestion);
            const QStringList labels = peerReviewQuestion.formLabels();
            const QString questionName = tr("Question") + " " + QString::number(questionNum + 1);
            // (a question asked separately about each team member has no line with the student's own name, but a question about them)
            QString notes = peerReviewQuestion.formDescription();
            if(layout == Layout::questionForEach) {
                notes.replace(PEERREVIEWSKIPYOURSELF, PEERREVIEWSKIPYOURSELFQUESTION);
            }
            const QString questionText = "<p><strong>" + peerReviewQuestion.text.simplified().toHtmlEscaped() + "</strong></p>" +
                                         "<p><em>" + notes.toHtmlEscaped().replace("\n", "<br>") + "</em></p>";

            if((layout == Layout::dropdownForEach) || (layout == Layout::numberForEach)) {
                const bool dropdowns = (layout == Layout::dropdownForEach);
                const long long questionID = addQuestion(QJsonObject{{"question_type", dropdowns? "multiple_dropdowns_question" : "fill_in_multiple_blanks_question"},
                                                                     {"question_text", questionText + rows},
                                                                     {"answers", answersFor(dropdowns? labels : QStringList{"0"}, blankIDs)}}, questionName);
                if(questionID == -1) {
                    return deleteCreated();
                }
                for(int memberNum = 0; memberNum < memberNames.size(); memberNum++) {
                    peerReview->formItems.append({QString::number(questionID) + "/" + blankIDs.at(memberNum), questionNum, teamNum, memberIDs.at(memberNum)});
                }
            }
            else if(layout == Layout::questionForEach) {
                for(int memberNum = 0; memberNum < memberNames.size(); memberNum++) {
                    QJsonObject question{{"question_text", "<p><strong>" + memberNames.at(memberNum) + "</strong></p>" + questionText}};
                    if(!labels.isEmpty()) {
                        question["question_type"] = "multiple_answers_question";
                        question["answers"] = answersFor(labels, {});
                    }
                    else {
                        question["question_type"] = "essay_question";
                    }
                    const long long questionID = addQuestion(question, questionName + " (" + QString::number(memberNum + 1) + ")");
                    if(questionID == -1) {
                        return deleteCreated();
                    }
                    peerReview->formItems.append({QString::number(questionID), questionNum, teamNum, memberIDs.at(memberNum)});
                }
            }
            else {
                QJsonObject question{{"question_text", questionText}};
                if(!labels.isEmpty()) {
                    question["question_type"] = peerReviewQuestion.multi? "multiple_answers_question" : "multiple_choice_question";
                    question["answers"] = answersFor(labels, {});
                }
                else if(peerReviewQuestion.kind == PeerReviewQuestion::Kind::numberEntry) {
                    question["question_type"] = "numerical_question";
                    question["answers"] = QJsonArray{QJsonObject{{"numerical_answer_type", "range_answer"}, {"answer_range_start", -1000000000},
                                                                 {"answer_range_end", 1000000000}, {"answer_weight", 100}}};
                }
                else {
                    question["question_type"] = "essay_question";
                }
                const long long questionID = addQuestion(question, questionName);
                if(questionID == -1) {
                    return deleteCreated();
                }
                const long long aboutID = (peerReviewQuestion.about == PeerReviewQuestion::About::self)? PeerReviewFormItem::ABOUTGIVER :
                                                                                                       PeerReviewFormItem::ABOUTNOONE;
                peerReview->formItems.append({QString::number(questionID), questionNum, teamNum, aboutID});
            }
        }

        //show the quiz only to the team's students, then publish it (a quiz's questions reach students only once it's published)
        canvasQuiz.assignmentID = jsonRequest(Method::get, quizPath).toObject()["assignment_id"].toInteger(-1);
        if(canvasQuiz.assignmentID == -1) {
            return deleteCreated();
        }
        stepDone(teamNum);
        const QString assignmentPath = coursePath + "/assignments/" + QString::number(canvasQuiz.assignmentID);
        const QJsonObject teamOverride = jsonRequest(Method::post, assignmentPath + "/overrides",
                                                     QJsonObject{{"assignment_override", QJsonObject{{"student_ids", canvasStudentIDs}, {"title", team.name}}}}).toObject();
        canvasQuiz.overrideID = teamOverride["id"].toInteger(-1);
        if(canvasQuiz.overrideID == -1) {
            return deleteCreated();
        }
        stepDone(teamNum);
        if(jsonRequest(Method::put, quizPath, QJsonObject{{"quiz", QJsonObject{{"published", true}}}}).isNull()) {
            return deleteCreated();
        }
        stepDone(teamNum);

        //graded complete/incomplete (after publishing, since publishing a quiz updates its assignment), and maybe left out of the final grade
        const QJsonObject grading{{"grading_type", "pass_fail"}, {"omit_from_final_grade", !peerReview->countTowardFinalGrade}};
        if(jsonRequest(Method::put, assignmentPath, QJsonObject{{"assignment", grading}}).isNull()) {
            return deleteCreated();
        }
        stepDone(teamNum);
    }

    peerReview->platform = PeerReview::Platform::canvasQuizzes;
    peerReview->canvasCourseID = courseID;
    peerReview->canvasCourseURL = baseURL + "/courses/" + QString::number(courseID);
    peerReview->acceptingResponses = true;
    return true;
}

// Downloads every submission to a peer review's team quizzes
QJsonArray CanvasHandler::downloadPeerReviewSubmissions(const PeerReview &peerReview, bool *ok,
                                                        const std::function<void(int quizzesDone, int numQuizzes)> &progress) {
    *ok = true;
    QJsonArray submissions;
    const int numQuizzes = int(std::count_if(peerReview.canvasQuizzes.cbegin(), peerReview.canvasQuizzes.cend(),
                                             [](const PeerReview::CanvasTeamQuiz &canvasQuiz){return canvasQuiz.assignmentID != -1;}));
    int quizzesDone = 0;
    for(const auto &canvasQuiz : peerReview.canvasQuizzes) {
        if(canvasQuiz.assignmentID == -1) {
            continue;
        }
        if(progress) {
            progress(quizzesDone++, numQuizzes);
        }
        bool pageOK = false;
        const QJsonArray quizSubmissions = getAllPages("/api/v1/courses/" + QString::number(peerReview.canvasCourseID) + "/assignments/" +
                                                       QString::number(canvasQuiz.assignmentID) + "/submissions?include[]=submission_history&include[]=user",
                                                       &pageOK);
        if(!pageOK) {
            *ok = false;
            return {};
        }
        for(const auto &submission : quizSubmissions) {
            if(submission.toObject()["workflow_state"].toString() != "unsubmitted") {
                submissions << submission;
            }
        }
    }
    return submissions;
}

// Locks or unlocks a peer review's team quizzes, through each one's team-only override (whose dates are the ones that apply to the team)
bool CanvasHandler::setPeerReviewQuizzesLocked(const PeerReview &peerReview, const bool locked,
                                               const std::function<void(int quizzesDone, int numQuizzes, bool undoing)> &progress) {
    auto setLock = [this, &peerReview](const PeerReview::CanvasTeamQuiz &canvasQuiz, const bool lock) {
        const QJsonValue lockAt = lock? QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate)) : QJsonValue(QJsonValue::Null);
        const QString url = "/api/v1/courses/" + QString::number(peerReview.canvasCourseID) + "/assignments/" +
                            QString::number(canvasQuiz.assignmentID) + "/overrides/" + QString::number(canvasQuiz.overrideID);
        return !jsonRequest(Method::put, url, QJsonObject{{"assignment_override", QJsonObject{{"lock_at", lockAt}}}}).isNull();
    };
    const int numQuizzes = int(std::count_if(peerReview.canvasQuizzes.cbegin(), peerReview.canvasQuizzes.cend(),
                                             [](const PeerReview::CanvasTeamQuiz &canvasQuiz){return canvasQuiz.quizID != -1;}));

    // all of the team quizzes are changed, or none of them: if one can't be, the ones already changed are changed back
    QList<const PeerReview::CanvasTeamQuiz *> changed;
    for(const auto &canvasQuiz : peerReview.canvasQuizzes) {
        if(canvasQuiz.quizID == -1) {
            continue;   // a team with no quiz
        }
        if(progress) {
            progress(int(changed.size()), numQuizzes, false);
        }
        const bool canBeChanged = (canvasQuiz.assignmentID != -1) && (canvasQuiz.overrideID != -1);
        if(!canBeChanged || !setLock(canvasQuiz, locked)) {
            if(!canBeChanged) {
                lastErrorMessage = tr("A team's quiz has no record of the setting that shows it only to the team, so it can't be locked or unlocked.");
            }
            const QString errorMessage = lastErrorMessage;
            for(int changedNum = 0; changedNum < changed.size(); changedNum++) {
                if(progress) {
                    progress(changedNum, int(changed.size()), true);
                }
                setLock(*changed.at(changedNum), !locked);
            }
            lastErrorMessage = errorMessage;
            return false;
        }
        changed << &canvasQuiz;
    }
    return true;
}

// Retrieves a course's group sets
QList<CanvasHandler::CanvasGroupSet> CanvasHandler::getGroupSets(const long long courseID, bool *ok) {
    QList<CanvasGroupSet> groupSets;
    const QJsonArray items = getAllPages("/api/v1/courses/" + QString::number(courseID) + "/group_categories", ok);
    for(const auto &item : items) {
        const QJsonObject groupSet = item.toObject();
        groupSets.append({groupSet["name"].toString(), groupSet["id"].toInteger()});
    }
    return groupSets;
}

// Retrieves the groups in a group set, then the members of each one
QList<CanvasHandler::CanvasGroup> CanvasHandler::getGroups(const long long groupSetID, bool *ok,
                                                           const std::function<void(int groupNum, int numGroups)> &progress) {
    QList<CanvasGroup> groups;
    QList<bool> isEmpty;
    const QJsonArray items = getAllPages("/api/v1/group_categories/" + QString::number(groupSetID) + "/groups", ok);
    if(!*ok) {
        return {};
    }
    for(const auto &item : items) {
        const QJsonObject group = item.toObject();
        groups.append({group["name"].toString(), group["id"].toInteger(), {}});
        isEmpty << (group["members_count"].toInt(-1) == 0);     // an empty group needs no request for its members
    }

    const int numGroups = int(groups.size());
    for(int groupNum = 0; groupNum < numGroups; groupNum++) {
        if(progress) {
            progress(groupNum, numGroups);
        }
        if(isEmpty.at(groupNum)) {
            continue;
        }
        const QJsonArray members = getAllPages("/api/v1/groups/" + QString::number(groups.at(groupNum).ID) + "/users", ok);
        if(!*ok) {
            return {};
        }
        for(const auto &member : members) {
            groups[groupNum].memberIDs << member.toObject()["id"].toInteger();
        }
    }
    return groups;
}

// Posts a comment on a student's submission (with a file attached, if given), and returns the new comment's ID
long long CanvasHandler::postSubmissionComment(const long long courseID, const long long assignmentID, const long long userID, const QString &text,
                                               const QString &fileName, const QByteArray &fileContent, const QString &fileContentType) {
    // the new comment is found as the latest one by whoever is signed in, so first find out who that is
    if(ownUserID == -1) {
        ownUserID = jsonRequest(Method::get, "/api/v1/users/self").toObject()["id"].toInteger(-1);
        if(ownUserID == -1) {
            return -1;
        }
    }

    const QString submissionPath = "/api/v1/courses/" + QString::number(courseID) + "/assignments/" + QString::number(assignmentID) +
                                   "/submissions/" + QString::number(userID);
    QJsonObject comment{{"text_comment", text}};
    if(!fileContent.isEmpty()) {
        const long long fileID = uploadSubmissionCommentFile(submissionPath, fileName, fileContent, fileContentType);
        if(fileID == -1) {
            return -1;
        }
        comment["file_ids"] = QJsonArray{fileID};
    }
    QJsonValue submission = jsonRequest(Method::put, submissionPath, QJsonObject{{"comment", comment}});
    if(submission.isNull()) {
        return -1;
    }
    if(!submission.toObject()["submission_comments"].isArray()) {
        // (Canvas's reply has included the submission's comments, but in case it doesn't, they're asked for)
        submission = jsonRequest(Method::get, submissionPath + "?include[]=submission_comments");
    }
    long long commentID = -1;
    for(const auto &value : submission.toObject()["submission_comments"].toArray()) {
        const QJsonObject submissionComment = value.toObject();
        if(submissionComment["author_id"].toInteger(-2) == ownUserID) {
            commentID = std::max(commentID, submissionComment["id"].toInteger(-1));
        }
    }
    // (the comment was posted either way, so if Canvas didn't show it, that isn't a failure, but its ID isn't known)
    return std::max(commentID, 0LL);
}

// Sends a student a private message, as a new conversation in the course
long long CanvasHandler::sendMessage(const long long courseID, const long long userID, const QString &subject, const QString &body) {
    // group_conversation false keeps it private; force_new starts a conversation of its own, rather than adding to an earlier one with the student
    const QJsonValue reply = jsonRequest(Method::post, "/api/v1/conversations",
                                         QJsonObject{{"recipients", QJsonArray{QString::number(userID)}}, {"subject", subject}, {"body", body},
                                                     {"group_conversation", false}, {"force_new", true},
                                                     {"context_code", "course_" + QString::number(courseID)}});
    if(reply.isNull()) {
        return -1;
    }
    const long long conversationID = reply.toArray().at(0).toObject()["id"].toInteger(-1);
    if(conversationID == -1) {
        lastErrorMessage = tr("Canvas didn't show the new message.");
    }
    return conversationID;
}

// Retrieves the Canvas IDs of the students who have submitted a quiz
QList<long long> CanvasHandler::getQuizSubmitterIDs(const long long courseID, const long long quizID, bool *ok) {
    QList<long long> submitterIDs;
    const QJsonArray quizSubmissions = getAllPages("/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) +
                                                   "/submissions", ok, "quiz_submissions");
    for(const auto &value : quizSubmissions) {
        const QJsonObject quizSubmission = value.toObject();
        // a submission shows its latest attempt, which is "untaken" while a student who already submitted is partway through another
        // attempt; a later attempt number, or a score kept from an earlier attempt, shows they did submit
        const QString state = quizSubmission["workflow_state"].toString();
        const bool submittedBefore = (quizSubmission["attempt"].toInt(0) > 1) || !quizSubmission["kept_score"].isNull();
        if((state == "complete") || (state == "pending_review") || ((state == "untaken") && submittedBefore)) {
            submitterIDs << quizSubmission["user_id"].toInteger(-1);
        }
    }
    return submitterIDs;
}

// Retrieves who a quiz is assigned to, from its dates and the overrides that show it to particular students, sections, or groups
// (a section's or group's students are asked for by ID, so a student in more than one section is found in each of them)
CanvasHandler::QuizAssignees CanvasHandler::getQuizAssignees(const long long courseID, const long long quizID, bool *ok) {
    QuizAssignees assignees;
    const QJsonValue reply = jsonRequest(Method::get, "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) +
                                                      "/date_details");
    const QJsonObject details = reply.toObject();
    *ok = !reply.isNull() && (details.contains("visible_to_everyone") || details.contains("only_visible_to_overrides"));
    if(!*ok) {
        return assignees;
    }
    assignees.everyone = details.contains("visible_to_everyone")? details["visible_to_everyone"].toBool(true) :
                                                                  !details["only_visible_to_overrides"].toBool(false);
    if(assignees.everyone) {
        return assignees;
    }
    bool saysWho = false;
    for(const auto &value : details["overrides"].toArray()) {
        const QJsonObject assignmentOverride = value.toObject();
        if(assignmentOverride.contains("student_ids")) {
            saysWho = true;
            for(const auto &studentID : assignmentOverride["student_ids"].toArray()) {
                assignees.studentIDs.insert(studentID.toInteger(-1));
            }
        }
        QJsonArray members;
        bool membersOK = true;
        if(assignmentOverride.contains("course_section_id") && !assignmentOverride["course_section_id"].isNull()) {
            // (the same request getStudentRoster makes for a section's students; "students" is null if the section has none)
            const QJsonValue section = jsonRequest(Method::get, "/api/v1/courses/" + QString::number(courseID) + "/sections/" +
                                                                QString::number(assignmentOverride["course_section_id"].toInteger()) + "?include[]=students");
            membersOK = !section.isNull();
            members = section.toObject()["students"].toArray();
        }
        else if(assignmentOverride.contains("group_id") && !assignmentOverride["group_id"].isNull()) {
            members = getAllPages("/api/v1/groups/" + QString::number(assignmentOverride["group_id"].toInteger()) + "/users", &membersOK);
        }
        else {
            continue;
        }
        if(!membersOK) {
            *ok = false;
            return assignees;
        }
        saysWho = true;
        for(const auto &member : std::as_const(members)) {
            assignees.studentIDs.insert(member.toObject()["id"].toInteger(-1));
        }
    }
    // only for some students, but Canvas didn't say which ones
    if(!saysWho) {
        *ok = false;
    }
    return assignees;
}

// Deletes a comment from a student's submission
bool CanvasHandler::deleteSubmissionComment(const long long courseID, const long long assignmentID, const long long userID, const long long commentID) {
    return !jsonRequest(Method::del, "/api/v1/courses/" + QString::number(courseID) + "/assignments/" + QString::number(assignmentID) +
                                     "/submissions/" + QString::number(userID) + "/comments/" + QString::number(commentID)).isNull();
}

// Uploads a file to attach to a comment on a submission, in Canvas's three steps, and returns the new file's ID
long long CanvasHandler::uploadSubmissionCommentFile(const QString &submissionPath, const QString &fileName, const QByteArray &content,
                                                     const QString &contentType) {
    // 1. tell Canvas about the file; it answers with where to send it
    const QJsonObject slot = jsonRequest(Method::post, submissionPath + "/comments/files",
                                         QJsonObject{{"name", fileName}, {"size", qint64(content.size())}, {"content_type", contentType}}).toObject();
    const QString uploadURL = slot["upload_url"].toString();
    if(uploadURL.isEmpty()) {
        if(lastErrorMessage.isEmpty()) {
            lastErrorMessage = tr("Canvas didn't say where to upload the file.");
        }
        return -1;
    }

    // 2. send the file, after the fields Canvas gave, to that address, which is usually a separate file server, so the access token isn't sent
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    const QJsonObject uploadParams = slot["upload_params"].toObject();
    for(auto param = uploadParams.constBegin(); param != uploadParams.constEnd(); ++param) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + param.key() + "\"");
        part.setBody(param.value().toVariant().toString().toUtf8());
        multiPart->append(part);
    }
    // the file's name is given plainly in ASCII (accents dropped, any other letters made "_"), since a header can't hold other characters,
    // and also in full, percent-encoded (Canvas has the full name from step 1 anyway)
    QString asciiFileName;
    for(const QChar character : fileName.normalized(QString::NormalizationForm_D)) {
        if(character.category() == QChar::Mark_NonSpacing) {
            continue;
        }
        asciiFileName += ((character.unicode() < 128) && (character != '"') && (character != '\\'))? character : QChar('_');
    }
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    filePart.setRawHeader("Content-Disposition", "form-data; name=\"file\"; filename=\"" + asciiFileName.toLatin1() + "\"; filename*=UTF-8''" +
                                                 QUrl::toPercentEncoding(fileName));
    filePart.setBody(content);
    multiPart->append(filePart);
    QNetworkRequest request{QUrl(uploadURL)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);     // (any redirect is followed below)
    request.setTransferTimeout(POST_TIMEOUT_TIME);
    auto *reply = manager->post(request, multiPart);
    multiPart->setParent(reply);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray locationHeader = reply->rawHeader("Location");
    const QUrl location = locationHeader.isEmpty()? QUrl() : QUrl(uploadURL).resolved(QUrl(QString::fromUtf8(locationHeader)));
    const QJsonObject uploaded = QJsonDocument::fromJson(reply->readAll()).object();
    const QString errorString = reply->errorString();
    reply->deleteLater();
    if((httpStatus < 200) || (httpStatus >= 400)) {
        lastErrorMessage = tr("The file could not be uploaded:") + " " + errorString +
                           ((httpStatus != 0)? (" (" + tr("HTTP status") + " " + QString::number(httpStatus) + ")") : "");
        return -1;
    }

    // 3. the file server answers with the new file, or else with where Canvas confirms it (which is asked only of this Canvas site,
    //    since that request carries the access token)
    auto idOf = [](const QJsonValue &id) {
        return id.isString()? id.toString().toLongLong() : id.toInteger(-1);
    };
    long long fileID = idOf(uploaded["id"]);
    if(fileID <= 0) {
        const QUrl confirmURL = uploaded.contains("location")? QUrl(uploadURL).resolved(QUrl(uploaded["location"].toString())) : location;
        if(confirmURL.isValid() && !confirmURL.host().isEmpty() && (confirmURL.host().compare(QUrl(baseURL).host(), Qt::CaseInsensitive) == 0)) {
            const QString confirmPath = confirmURL.path() + (confirmURL.hasQuery()? ("?" + confirmURL.query(QUrl::FullyEncoded)) : "");
            fileID = idOf(jsonRequest(Method::get, confirmPath).toObject()["id"]);
        }
    }
    if(fileID <= 0) {
        if(lastErrorMessage.isEmpty()) {
            lastErrorMessage = tr("Canvas didn't confirm the uploaded file.");
        }
        return -1;
    }
    return fileID;
}

////////////////////////////////////////////

QJsonValue CanvasHandler::jsonRequest(const Method method, const QString &URL, const QJsonObject &body) {
    const QByteArray data = ((method == Method::get) || (method == Method::del))? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact);
    // a 403 is retried only if Canvas says it's rate-limiting; otherwise it's a refusal (e.g., a student who has left the course), which won't change
    const QByteArray replyBody = httpRequest(method, QUrl(baseURL + URL), data, data.isEmpty()? QByteArray() : QByteArray("application/json"), false);
    if(replyBody.isEmpty() && !lastErrorMessage.isEmpty()) {
        return QJsonValue(QJsonValue::Null);
    }
    const QJsonDocument replyJson = QJsonDocument::fromJson(replyBody);
    if(replyJson.isObject()) {
        return replyJson.object();
    }
    if(replyJson.isArray()) {
        return replyJson.array();
    }
    return QJsonObject();       // a successful reply with no content
}

QJsonArray CanvasHandler::getAllPages(const QString &initialURL, bool *ok, const QString &arrayKey) {
    *ok = false;
    QJsonArray items;
    QEventLoop loop;
    // ask for large pages (Canvas defaults to only 10 items per page); the "next" links Canvas returns keep this setting
    QString url = baseURL + initialURL + (initialURL.contains('?')? "&" : "?") + "per_page=100";
    int numPages = 0;
    do {
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + OAuthFlow->token().toUtf8());
        request.setTransferTimeout(POST_TIMEOUT_TIME);      // (a page of submissions with their history can be large)

        // each page is retried, after a growing delay, if the failure is likely temporary (rate-limiting, a server error, or a network error),
        // as LMS::httpRequest does
        int httpStatus = 0;
        QByteArray replyHeader, replyBody;
        QString errorString;
        for(int attempt = 1; attempt <= NUM_RETRIES_BEFORE_ABORT; attempt++) {
            auto *reply = manager->get(request);
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            replyHeader = reply->rawHeader("Link");
            replyBody = reply->readAll();
            errorString = reply->errorString();
            reply->deleteLater();
            const bool temporary = (httpStatus == 0) || (httpStatus == 429) || ((httpStatus >= 500) && (httpStatus < 600)) ||
                                   ((httpStatus == 403) && replyBody.toLower().contains("rate limit"));
            if(((httpStatus >= 200) && (httpStatus < 300)) || !temporary || (attempt == NUM_RETRIES_BEFORE_ABORT)) {
                break;
            }
            emit retrying(attempt + 1);
            QEventLoop delayLoop;
            QTimer::singleShot(RETRY_DELAY_TIME << (attempt - 1), &delayLoop, &QEventLoop::quit);     // 500ms, 1s, 2s, 4s
            delayLoop.exec();
        }
        const QJsonDocument replyJson = QJsonDocument::fromJson(replyBody);
        const bool hasList = arrayKey.isEmpty()? replyJson.isArray() : (replyJson.isObject() && replyJson.object()[arrayKey].isArray());
        if((httpStatus < 200) || (httpStatus >= 300) || !hasList) {
            lastErrorMessage = errorString + ((httpStatus != 0)? (" (" + tr("HTTP status") + " " + QString::number(httpStatus) + ")") : "");
            return {};
        }
        const QJsonArray pageItems = arrayKey.isEmpty()? replyJson.array() : replyJson.object()[arrayKey].toArray();
        for(const auto &item : pageItems) {
            items << item;
        }
        static const QRegularExpression nextURL(R"(<([^>]*)>;\s*rel="next")");
        url = nextURL.match(QString::fromUtf8(replyHeader)).captured(1);
    }
    while(!url.isEmpty() && ++numPages < NUM_PAGES_TO_LOAD);
    *ok = true;
    return items;
}

////////////////////////////////////////////

QUrl CanvasHandler::getQuizResultsURL(const long long courseID, const long long quizID, const QElapsedTimer &waitTimer,
                                      const std::function<void(const QString &progressURL)> &waiting) {
    const QString url = "/api/v1/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quizID) + "/reports";
    QUrlQuery query;
    query.addQueryItem("quiz_report[report_type]", "student_analysis");
    const QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();
    QStringList quizReportProgressURL;
    QList<long long> quizReportID;
    QStringList quizReportFileURL;
    QList<QStringList*> stringParams = {&quizReportProgressURL};
    QList<QList<long long>*> integerParams = {&quizReportID};
    QList<QStringList*> stringInSubobjectParams = {&quizReportFileURL};
    QEventLoop loop;
    // check every RELOAD_DELAY_TIME (i.e., 2 seconds)--a file object (including a download URL) is added to the json results when it is
    do {
        if(waiting) {
            waiting(quizReportProgressURL.isEmpty()? QString() : quizReportProgressURL.constFirst());
        }
        // (checked after waiting(), which may take a moment to ask Canvas how far along the report is)
        if(downloadCanceled || (waitTimer.elapsed() > MAX_REPORT_WAIT_TIME)) {
            lastErrorMessage = (downloadCanceled? tr("Download canceled.") : tr("Canvas did not finish preparing the survey results in time."));
            return {};
        }
        QTimer::singleShot(RELOAD_DELAY_TIME, &loop, &QEventLoop::quit);
        loop.exec();
        quizReportProgressURL.clear();
        quizReportID.clear();
        quizReportFileURL.clear();
        postToCanvasGetSingleResult(url, postData,
                                    {"progress_url"}, stringParams,
                                    {"id"}, integerParams,
                                    {"file/url"}, stringInSubobjectParams);
    } while(quizReportFileURL.isEmpty() || quizReportFileURL.first().isEmpty());

    return {quizReportFileURL.first()};
}

int CanvasHandler::reportPercentDone(const QString &progressURL) {
    // the progress record is asked for only from this Canvas site, since the request carries the access token
    const QString path = QUrl(progressURL).path();
    if(!path.startsWith("/api/v1/progress/") || downloadCanceled) {
        return -1;
    }
    // a single try, with no retrying and no error reported, since not knowing the progress only means the bar can't show it
    QNetworkRequest request(QUrl(baseURL + path));
    request.setRawHeader("Authorization", "Bearer " + OAuthFlow->token().toUtf8());
    request.setTransferTimeout(TIMEOUT_TIME);
    auto *reply = manager->get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const QJsonObject progressRecord = QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();
    const QJsonValue completion = progressRecord["completion"];
    return completion.isDouble()? std::clamp(int(completion.toDouble()), 0, 100) : -1;
}

void CanvasHandler::getPaginatedCanvasResults(const QString &initialURL, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                         const QStringList &integerParams, QList<QList<long long>*> &integerVals,
                                                                         const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals,
                                                                         const QStringList &integerInSubArrayParams, QList<QList<long long>*> &integerInSubArrayVals) {
    QEventLoop loop;
    // ask for large pages (Canvas defaults to only 10 items per page); the "next" links Canvas returns keep this setting
    QString url = baseURL + initialURL + (initialURL.contains('?')? "&" : "?") + "per_page=100";
    QString replyHeader;
    QByteArray replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;
    int numPages = 0;

    do {
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + OAuthFlow->token().toUtf8());
        auto *reply = manager->get(request);

        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if(reply->bytesAvailable() == 0) {
            //qDebug() << "no reply";
            reply->deleteLater();
            return;
        }

        replyHeader = reply->rawHeader("Link");
        replyBody = reply->readAll();
        reply->deleteLater();   // done with this page's reply; each page gets its own
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

        for(const auto &value : std::as_const(json_array)) {
            const QJsonObject json_obj = value.toObject();
            for(int i = 0; i < stringParams.size(); i++) {
                *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
            }
            for(int i = 0; i < integerParams.size(); i++) {
                *(integerVals[i]) << json_obj[integerParams.at(i)].toInteger();
            }
            for(int i = 0; i < stringInSubobjectParams.size(); i++) {
                const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
                const QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
                *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
            }
            for(int i = 0; i < integerInSubArrayParams.size(); i++) {
                const QStringList subarrayAndParamName = integerInSubArrayParams.at(i).split('/');   // "subarray_name/int_paramater_name"
                const QJsonArray array = json_obj[subarrayAndParamName.at(0)].toArray();
                for(const auto &item : array) {
                    QJsonObject subobj = item.toObject();
                    *(integerInSubArrayVals[i]) << subobj[subarrayAndParamName.at(1)].toInteger();
                }
            }
        }
        static const QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
        const QRegularExpressionMatch nextURLMatch = nextURL.match(replyHeader);
        url = nextURLMatch.captured(1);
    }
    while(!url.isNull() && ++numPages < NUM_PAGES_TO_LOAD);
}

void CanvasHandler::postToCanvasGetSingleResult(const QString &URL, const QByteArray &postData,
                                                const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                const QStringList &integerParams, QList<QList<long long>*> &integerVals,
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

    for(const auto &value : std::as_const(json_array)) {
        QJsonObject json_obj = value.toObject();
        for(int i = 0; i < stringParams.size(); i++) {
            *(stringVals[i]) << json_obj[stringParams.at(i)].toString("");
        }
        for(int i = 0; i < integerParams.size(); i++) {
            *(integerVals[i]) << json_obj[integerParams.at(i)].toInteger();
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
    if(!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return (file.write(replyBody) == replyBody.size());
}

// For testing: sets token manually
void CanvasHandler::authenticateWithManualToken(const QString &token) {
    OAuthFlow->setToken(token);
}

QStringList CanvasHandler::askUserForManualURLandToken(const QString &currentAccountName, const QString &currentURL, const QString &currentToken) {
    auto *getCanvasInfoDialog = new QDialog(parent);    // (with a parent, it opens on top of the window that asked to sign in)
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
                                                   !(canvasURL->text().isEmpty()) && canvasURL->text().contains('.') && !(canvasToken->text().isEmpty()));});
    connect(canvasToken, &QLineEdit::textChanged, buttonBox, [buttonBox, canvasURL, canvasToken]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   !(canvasURL->text().isEmpty()) && canvasURL->text().contains('.') && !(canvasToken->text().isEmpty()));});

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
    while(url.endsWith('/')) {
        url.chop(1);
    }
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

QSet<QByteArray> CanvasHandler::getScopes() const {
    QSet<QByteArray> result;
    for (const auto &s : QString(SCOPES).split(' ', Qt::SkipEmptyParts)) {
        result.insert(s.toUtf8());
    }
    return result;
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
