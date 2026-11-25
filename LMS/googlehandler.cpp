#include "googlehandler.h"
#include "secureStore.h"
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>
#include <QUuid>
#include <QVBoxLayout>

GoogleHandler::GoogleHandler(QWidget *parent) : LMS(parent), parent(parent) {
    initOAuth2();

    QSettings settings;
    const QByteArray key = "gruepr";

    QByteArray refreshToken;
    if(settings.contains("GoogleRefreshToken")) {
        // older versions of gruepr saved the token XOR'ed in QSettings; move it to the secure store
        refreshToken = settings.value("GoogleRefreshToken", "").toByteArray();
        for (int i = 0; i < refreshToken.size(); ++i) {
            refreshToken[i] ^= key[i%key.size()];
        }
        if(!refreshToken.isEmpty()) {
            SecureStore::write("GoogleRefreshToken", refreshToken);
        }
        settings.remove("GoogleRefreshToken");
    }
    else {
        refreshToken = SecureStore::read("GoogleRefreshToken");
    }
    if(!refreshToken.isEmpty()) {
        OAuthFlow->setRefreshToken(refreshToken);
    }

    QByteArray encName = settings.value("GoogleAccountName", "").toByteArray();
    for (int i = 0; i < encName.size(); ++i) {
        encName[i] ^= key[i%key.size()];
    }
    accountName = encName;
}

GoogleHandler::~GoogleHandler() {
    QSettings settings;
    const QByteArray key = "gruepr";

    if(!OAuthFlow->refreshToken().isEmpty()) {
        SecureStore::write("GoogleRefreshToken", OAuthFlow->refreshToken().toUtf8());
    }

    if(!accountName.isEmpty()) {
        QByteArray encName = accountName.toUtf8();
        for (int i = 0; i < encName.size(); ++i) {
            encName[i] ^= key[i%key.size()];
        }
        settings.setValue("GoogleAccountName", encName);
    }
}

bool GoogleHandler::authenticate() {
    QDialog *loginDialog = actionDialog(parent);

    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::replyDataReceived, this, [this](const QByteArray &reply) {
        //Grab the account name (email address) out of the OpenID reply
        const auto json_doc = QJsonDocument::fromJson(reply);
        const auto id_token = json_doc["id_token"].toString().split('.');  //id_token is JWT-encoded user info
        if(id_token.size() == 3) {
            const auto id_token_JWTPayload = QJsonDocument::fromJson(QByteArray::fromBase64(id_token.at(1).toUtf8(), QByteArray::Base64UrlEncoding)).object();
            accountName = id_token_JWTPayload["email"].toString();
        }
    });
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
            actionDialogLabel->setText(actionDialogLabel->text() + "<br>" + tr("Google is requesting that you re-authorize gruepr."));
            loginDialog->adjustSize();
            OAuthFlow->setRefreshToken("");
            QEventLoop loop;
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            loginDialog->accept();
        }
        else {
            //Unknown connection error, will need to start over
            actionDialogLabel->setText(actionDialogLabel->text() + "<br>" + tr("There is an error connecting to Google.<br>Plese retry later."));
            loginDialog->adjustSize();
            QEventLoop loop;
            QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
            loop.exec();
            loginDialog->reject();
        }
    });

    if(!OAuthFlow->refreshToken().isEmpty()) {
        //RefreshToken is found, try to use it to get accessTokens without having to re-grant permission
        bool changingAccounts = false;
        if(!accountName.isEmpty()){
            actionDialogLabel->setText(tr("Using Google account:") + "<br>" + accountName);
            actionDialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            actionDialogButtons->button(QDialogButtonBox::Ok)->setText(tr("Proceed"));
            actionDialogButtons->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
            actionDialogButtons->button(QDialogButtonBox::Ok)->adjustSize();
            connect(actionDialogButtons->button(QDialogButtonBox::Ok), &QPushButton::clicked, loginDialog, &QDialog::accept);
            actionDialogButtons->button(QDialogButtonBox::Cancel)->setText(tr("Change account"));
            actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
            actionDialogButtons->button(QDialogButtonBox::Cancel)->adjustSize();
            connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);
            loginDialog->show();
            loginDialog->adjustSize();
            changingAccounts = (loginDialog->exec() == QDialog::Rejected);
        }

        if(changingAccounts) {
            OAuthFlow->setRefreshToken("");
        }
        else {
            actionDialogLabel->setText(tr("Connecting to Google..."));
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
    }

    if(!authenticated()) {
        // didn't have a refreshToken, the refreshToken didn't work, or user wants to change accounts: time to (re)authorize
        actionDialogLabel->setText(tr("The next step will open a browser window so you can sign in with Google "
                                      "and then authorize gruepr to access the surveys that it creates on your Google Drive. "
                                      "You will need to select all checkboxes to provide this authorization.<br><br>"
                                      "ALL data associated with your survey will exist in your Google Drive only. "
                                      "No data will ever be stored or sent anywhere else."));
        actionDialogLabel->setWordWrap(true);
        actionDialogButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);
        actionDialogButtons->setStyleSheet("");
        actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        auto *okButton = actionDialogButtons->button(QDialogButtonBox::Ok);
        QPixmap loginpic(":/icons_new/google_signin_button.png");
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
        loginpic = loginpic.scaledToHeight(actionDialogButtons->button(QDialogButtonBox::Cancel)->height() * 5 / 4, Qt::SmoothTransformation);
#else
        loginpic = loginpic.scaledToHeight(okButton->height(), Qt::SmoothTransformation);
#endif
        okButton->setText("");
        okButton->setIconSize(loginpic.rect().size());
        okButton->setIcon(loginpic);
        okButton->adjustSize();
        loginDialog->show();
        loginDialog->adjustSize();

        if(loginDialog->exec() == QDialog::Rejected) {
            actionComplete(loginDialog);
            return false;
        }

        actionDialogLabel->setText(tr("Please use your browser to log in<br>to Google and then return here."));
        actionDialogLabel->setWordWrap(false);
        actionDialogButtons->setStandardButtons(QDialogButtonBox::Cancel);
        actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        loginDialog->show();
        loginDialog->adjustSize();
        connect(actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, loginDialog, &QDialog::reject);

        OAuthFlow->setModifyParametersFunction(getModifyParametersFunction());
        LMS::authenticate();

        if(loginDialog->exec() == QDialog::Rejected) {
            actionComplete(loginDialog);
            return false;
        }
    }

    return true;
}

GoogleHandler::GoogleForm GoogleHandler::createSurvey(const Survey *const survey) {
    //create a Google Form--only the title and document_title can be set in this step
    QString url = "https://forms.googleapis.com/v1/forms";
    const QString title = survey->title.isEmpty() ? QDateTime::currentDateTime().toString("hh:mm dd MMMM yyyy") : survey->title.simplified();
    QJsonObject info;
    info["title"] = title;
    info["document_title"] = title;
    QJsonObject form;
    form["info"] = info;
    QStringList formIDInList, submissionURL, revisionIDinList;
    QStringList y;
    QList<QStringList*> stringParams = {&formIDInList, &submissionURL, &revisionIDinList};
    QList<QStringList*> stringInSubobjectParams = {&y};
    postToGoogleGetSingleResult(url, QJsonDocument(form).toJson(),
                                {"formId", "responderUri", "revisionId"}, stringParams,
                                {}, stringInSubobjectParams);
    if(formIDInList.isEmpty() || submissionURL.isEmpty() || revisionIDinList.isEmpty()) {
        return {};
    }
    const QString formID = formIDInList.constFirst();
    const QString surveySubmissionURL = submissionURL.constFirst();
    QString revisionID = revisionIDinList.constFirst();
    if(formID.isEmpty()) {
        return {};
    }

    //"update" the form by adding each of the questions one-at-a-time and then updating the description
    //in future version of Forms API, can hopefully also update additional forms settings: setAcceptingResponses(true).
    //                                                                                     setCollectEmail(false).
    //                                                                                     setLimitOneResponsePerUser(false).
    //                                                                                     setShowLinkToRespondAgain(false)
    url = "https://forms.googleapis.com/v1/forms/" + formID + ":batchUpdate";
    QJsonObject requestBody;
    requestBody["includeFormInResponse"] = false;
    QJsonObject writeControl;
    writeControl["targetRevisionId"] = revisionID;
    requestBody["writeControl"] = writeControl;
    QJsonArray requests;
    //create each question
    int questionNum = 0;
    for(const auto &question : survey->questions) {
        //create one question
        QJsonObject newQuestion;
        QJsonObject location;
        location["index"] = questionNum;
        newQuestion["location"] = location;
        const QString questionAddendum = question.type == Question::QuestionType::freeresponsenumber? (QString("  (") + WRITEANUMBER) + ".)" : "";
        QJsonObject item;
        item["title"] = question.text.simplified() + questionAddendum;
        bool skipThisQuestion = question.text.simplified().isEmpty();
        switch(question.type) {
        case Question::QuestionType::shorttext:
        case Question::QuestionType::longtext:
        case Question::QuestionType::freeresponsenumber: {  // Google Forms API doesn't have a native number input - this will be free text!
            QJsonObject questionItem;
            QJsonObject questionBody;
            questionBody["required"] = false;
            QJsonObject kind;
            kind["paragraph"] = (question.type == Question::QuestionType::longtext);
            questionBody["textQuestion"] = kind;
            questionItem["question"] = questionBody;
            item["questionItem"] = questionItem;
            break;}
        case Question::QuestionType::dropdown:
        case Question::QuestionType::radiobutton:
        case Question::QuestionType::checkbox: {
            const QStringList filteredOptions = dedupedNonBlankOptions(question.options);
            if(filteredOptions.isEmpty()) {
                skipThisQuestion = true;
                break;
            }
            QJsonObject questionItem;
            QJsonObject questionBody;
            questionBody["required"] = false;
            QJsonObject kind;
            kind["type"] = (question.type == Question::QuestionType::dropdown ? "DROP_DOWN" :
                                (question.type == Question::QuestionType::radiobutton ? "RADIO" : "CHECKBOX"));
            kind["shuffle"] = false;
            QJsonArray responseOptions;
            for(const auto &option : filteredOptions) {
                QJsonObject responseOption;
                responseOption["value"] = option;
                responseOption["isOther"] = false;
                responseOptions << responseOption;
            }
            kind["options"] = responseOptions;
            questionBody["choiceQuestion"] = kind;
            questionItem["question"] = questionBody;
            item["questionItem"] = questionItem;
            break;}
        case Question::QuestionType::schedule: {
            if(survey->schedTimeNames.isEmpty() || survey->schedDayNames.isEmpty()) {
                skipThisQuestion = true;
                break;
            }
            item["description"] = "You may need to scroll to see all columns (" + survey->schedTimeNames.first() +
                                                                         " to " + survey->schedTimeNames.last() + ").";
            QJsonObject questionGroupItem;
            QJsonObject grid;
            grid["shuffleQuestions"] = false;
            QJsonObject choiceQuestion;
            choiceQuestion["type"] = "CHECKBOX";
            QJsonArray times;
            for(const auto &timeName : survey->schedTimeNames) {
                QJsonObject time;
                time["value"] = timeName;
                time["isOther"] = false;
                times << time;
            }
            choiceQuestion["options"] = times;
            grid["columns"] = choiceQuestion;
            QJsonArray questions;
            for(const auto &dayName : survey->schedDayNames) {
                QJsonObject questionBody;
                questionBody["required"] = false;
                QJsonObject row;
                row["title"] = dayName.simplified();
                questionBody["rowQuestion"] = row;
                questions << questionBody;
            }
            questionGroupItem["questions"] = questions;
            questionGroupItem["grid"] = grid;
            item["questionGroupItem"] = questionGroupItem;
            break;}
        case Question::QuestionType::rankedchoice: {
            const QStringList filteredOptions = dedupedNonBlankOptions(question.options);
            if(filteredOptions.isEmpty()) {
                skipThisQuestion = true;
                break;
            }
            skipThisQuestion = false;
            // Create k separate dropdown questions, one per rank
            for(int rank = 0; rank < question.numRankedChoices; rank++) {
                if(rank > 0) {
                    // Push the previous rank via the same path the outer loop uses
                    newQuestion["item"] = item;
                    QJsonObject createItem;
                    createItem["createItem"] = newQuestion;
                    requests << createItem;
                    questionNum++;

                    // Reset for next rank
                    newQuestion = QJsonObject();
                    QJsonObject nextLocation;
                    nextLocation["index"] = questionNum;
                    newQuestion["location"] = nextLocation;
                    item = QJsonObject();
                }
                item["title"] = question.text.simplified() + " [" + (rank == 0? QString(RANKYOURFIRSTCHOICE) :
                                    (QString(RANKYOURCHOICE) + " " + QString::number(rank + 1))) + "]";
                QJsonObject questionItem;
                QJsonObject questionBody;
                questionBody["required"] = false;
                QJsonObject kind;
                kind["type"] = "DROP_DOWN";
                kind["shuffle"] = false;
                QJsonArray responseOptions;
                for(const auto &option : filteredOptions) {
                    QJsonObject responseOption;
                    responseOption["value"] = option;
                    responseOption["isOther"] = false;
                    responseOptions << responseOption;
                }
                kind["options"] = responseOptions;
                questionBody["choiceQuestion"] = kind;
                questionItem["question"] = questionBody;
                item["questionItem"] = questionItem;
            }
            break;}
        }
        if(skipThisQuestion) {
            continue;
        }
        newQuestion["item"] = item;
        QJsonObject createItem;
        createItem["createItem"] = newQuestion;
        requests << createItem;
        questionNum++;
    }

    //update the description
    info.remove("document_title");
    info["description"] = SURVEYINSTRUCTIONS;
    QJsonObject UpdateFormInfoRequest;
    UpdateFormInfoRequest["info"] = info;
    UpdateFormInfoRequest["updateMask"] = "description";
    QJsonObject updateItem;
    updateItem["updateFormInfo"] = UpdateFormInfoRequest;
    requests << updateItem;

    requestBody["requests"] = requests;

    stringParams = {&y};
    stringInSubobjectParams = {&revisionIDinList};
    revisionIDinList.clear();
    postToGoogleGetSingleResult(url, QJsonDocument(requestBody).toJson(),
                                {}, stringParams,
                                {"writeControl/requiredRevisionId"}, stringInSubobjectParams);
    if(revisionIDinList.isEmpty()) {
        deleteOrphanedForm(formID);
        return {};
    }
    revisionID = revisionIDinList.constFirst();
    if(revisionID.isEmpty()) {
        deleteOrphanedForm(formID);
        return {};
    }

    //publish the form so that responders can access it (required for forms created by API after June 2026)
    url = "https://forms.googleapis.com/v1/forms/" + formID + ":setPublishSettings";
    QJsonObject publishBody;
    QJsonObject publishState;
    publishState["isPublished"] = true;
    publishState["isAcceptingResponses"] = true;
    QJsonObject publishSettings;
    publishSettings["publishState"] = publishState;
    publishBody["publishSettings"] = publishSettings;
    const auto publishReplyBody = httpRequest(Method::post, url, QJsonDocument(publishBody).toJson(), "application/json");
    const bool published = lastErrorMessage.isEmpty() && !publishReplyBody.isEmpty();

    // append this survey to the saved values
    QSettings settings;
    const int currentArraySize = settings.beginReadArray("GoogleForm");
    settings.endArray();
    settings.beginWriteArray("GoogleForm");
    settings.setArrayIndex(currentArraySize);
    settings.setValue("name", title);
    settings.setValue("ID", formID);
    auto currTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    settings.setValue("createdTime", currTime);
    settings.setValue("responderURL", QUrl(surveySubmissionURL));
    settings.setValue("accountName", accountName);
    settings.endArray();

    return {title, formID, currTime, QUrl(surveySubmissionURL), published};
}

// function below sends the Google Form to a 'finalize script' to set some options not available currently in Forms API
// and to create a Sheet to hold the results. Requires opening a browser to give the script access to Google Drive and Sheets
/*
QStringList GoogleHandler::sendSurveyToFinalizeScript(const GoogleForm &form){
    // Finish the job with the script
    QUrl script(FINALIZESCRIPTURL);
    QUrlQuery titleAndID;
    titleAndID.addQueryItem("title", form.name);
    titleAndID.addQueryItem("id", form.ID);
    script.setQuery(titleAndID);

    QEventLoop loop;
    auto *reply = google->get(script.toString(QUrl::None));
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // expected data is all between the 2 __USERDATA__ signals
    QString replybody = ((reply->bytesAvailable() == 0) ? "" : reply->readAll());
    reply->deleteLater();
    QStringList userData = replybody.split("__USERDATA__");
    if(userData.size() != 3) {
        return {};
    }
    QString data = userData.at(1);
    data.replace(R"(\x3d)", R"(=)").replace(R"(\x26)", R"( )").replace(R"(\/)", R"(/)");

    QStringList urlsInReply = data.split(' ');
    // parse the URLs from the reply
    QString editURL, responseURL, csvURL;
    for(const auto &urlInReply : std::as_const(urlsInReply)) {
        if(urlInReply.contains("editURL")) {
            editURL = urlInReply.mid(urlInReply.indexOf('=') + 1);
        }
        if(urlInReply.contains("responseURL")) {
            responseURL = urlInReply.mid(urlInReply.indexOf('=') + 1);
        }
        if(urlInReply.contains("csvURL")) {
            csvURL = urlInReply.mid(urlInReply.indexOf('=') + 1);
        }
    }

    return {editURL, responseURL, csvURL};
}
*/

QStringList GoogleHandler::getSurveyList() {
    formsList.clear();

    QSettings settings;
    const int size = settings.beginReadArray("GoogleForm");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        const QString name = settings.value("name").toString();
        const QString id = settings.value("ID").toString();
        const QString createdTime = settings.value("createdTime").toString();
        const QString responderURL = settings.value("responderURL").toString();
        const QString account = settings.value("accountName").toString();
        if(!id.isEmpty() && (account.isEmpty() || account == accountName)) {
            formsList.append({name, id, createdTime, responderURL});
        }
    }
    settings.endArray();

    std::sort(formsList.begin(), formsList.end(), [](const GoogleForm &lhs, const GoogleForm &rhs)
                                                  {return (QDateTime::fromString(lhs.createdTime,Qt::ISODate) <
                                                           QDateTime::fromString(rhs.createdTime,Qt::ISODate));});
    QStringList formNames;
    formNames.reserve(formsList.size());
    for(const auto &form : std::as_const(formsList)) {
        formNames.append(form.name);
    }

    return formNames;
}

QString GoogleHandler::getSurveyResponderURL(const QString &surveyName) const {
    for(const auto &form : std::as_const(formsList)) {
        if(form.name == surveyName) {
            return form.responderURL.toString();
        }
    }
    return {};
}

QString GoogleHandler::downloadSurveyResult(const QString &surveyName) {
    //get the ID
    QString ID;
    for(const auto &form : std::as_const(formsList)) {
        if(form.name == surveyName) {
            ID = form.ID;
        }
    }
    if(ID.isEmpty()) {
        return {};
    }

    //download the form itself into a JSON so that we can get the questions
    QString url = "https://forms.googleapis.com/v1/forms/" + ID;
    //qDebug() << url;
    auto replyBody = httpRequest(Method::get, url);
    if(replyBody.isEmpty()) {
        return {};
    }
    //qDebug() << replyBody.first(std::min(200, int(replyBody.size())));
    QJsonDocument json_doc = QJsonDocument::fromJson(replyBody);
    //pull out each question and save the question ID and text
    const QJsonArray items = json_doc["items"].toArray();
    QList<GoogleFormQuestion> questions;
    questions.reserve(items.size());
    for(const auto &item : items) {
        const QJsonObject object = item.toObject();
        if(object.contains("questionItem")) {
            questions.append({object["questionItem"]["question"]["questionId"].toString(), object["title"].toString(),
                              GoogleFormQuestion::Type::notSchedule});
        }
        else if(object.contains("questionGroupItem")) {
            const QJsonArray questionsInGroup = object["questionGroupItem"]["questions"].toArray();
            for(const auto &questionInGroup : questionsInGroup) {
                const auto questionObject = questionInGroup.toObject();
                const QJsonObject rowQuestion = questionObject["rowQuestion"].toObject();
                questions.append({questionObject["questionId"].toString(), object["title"].toString() + "[" + rowQuestion["title"].toString() + "]",
                                  GoogleFormQuestion::Type::schedule});
            }
        }
    }

    //prepare a file for the results
    static const QRegularExpression unallowedChars(R"([#&&{}\/\<>*?$!'":@+`|=])");
    const QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::TempLocation), surveyName.simplified().replace(unallowedChars, "_") + ".csv");
    QFile file(filepath.absoluteFilePath());
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    //qDebug() << file.errorString();
    QTextStream out(&file);

    //save the header row
    out << "Timestamp";
    for(const auto &question : std::as_const(questions)) {
        out << ",\"" << question.text << "\"";
    }
    out << Qt::endl;

    //now download into a big JSON all of the responses (actually it's just the first 5000 responses! if >5000, will need to work with paginated results!)
    url = "https://forms.googleapis.com/v1/forms/" + ID + "/responses";
    //qDebug() << url;
    replyBody = httpRequest(Method::get, url);
    if(replyBody.isEmpty()) {
        return {};
    }
    //qDebug() << replyBody.first(std::min(200, int(replyBody.size())));
    json_doc = QJsonDocument::fromJson(replyBody);
    //pull out each response and save as a row in the file, save the submitted time as a time stamp, and get the question answer(s)
    const QJsonArray responses = json_doc["responses"].toArray();
    QStringList allValuesInField;
    for(const auto &response : std::as_const(responses)) {
        out << response.toObject().value("lastSubmittedTime").toString();

        //pull out the answer(s) to each question in order, joining the answers with a semicolon if >1
        const QJsonObject answers = response.toObject().value("answers").toObject();
        for(const auto &question : std::as_const(questions)) {
            const QJsonArray nestedAnswers = answers[question.ID]["textAnswers"]["answers"].toArray();
            allValuesInField.clear();
            allValuesInField.reserve(nestedAnswers.size());
            for(const auto &nestedAnswer : std::as_const(nestedAnswers)) {
                const auto answerObject = nestedAnswer.toObject();
                allValuesInField << answerObject["value"].toString()
                                        .replace('"', '\'').replace('\n', ' ').replace('\r', ' ');  // strip stray quotation marks, newlines
            }
            out << ",\"" << allValuesInField.join(question.type == GoogleFormQuestion::Type::schedule? ';' : ',') << "\"";
        }

        out << Qt::endl;
    }

    file.close();
    return filepath.absoluteFilePath();
}

GoogleHandler::GoogleForm GoogleHandler::createPeerReviewForm(PeerReview *const peerReview) {
    //create a Google Form--only the title and document_title can be set in this step
    QString url = "https://forms.googleapis.com/v1/forms";
    const QString title = peerReview->title.isEmpty() ? QDateTime::currentDateTime().toString("hh:mm dd MMMM yyyy") : peerReview->title.simplified();
    QJsonObject info;
    info["title"] = title;
    info["document_title"] = title;
    QJsonObject form;
    form["info"] = info;
    QStringList formIDInList, submissionURL, revisionIDinList;
    QStringList y;
    QList<QStringList*> stringParams = {&formIDInList, &submissionURL, &revisionIDinList};
    QList<QStringList*> stringInSubobjectParams = {&y};
    postToGoogleGetSingleResult(url, QJsonDocument(form).toJson(),
                                {"formId", "responderUri", "revisionId"}, stringParams,
                                {}, stringInSubobjectParams);
    if(formIDInList.isEmpty() || submissionURL.isEmpty() || revisionIDinList.isEmpty()) {
        return {};
    }
    const QString formID = formIDInList.constFirst();
    const QString surveySubmissionURL = submissionURL.constFirst();
    const QString revisionID = revisionIDinList.constFirst();
    if(formID.isEmpty()) {
        return {};
    }

    //plan every item of the form: the name question, then for each team a page with its questions and a final "submit" question
    //(the form's pages can't be set to submit when they end, so the "submit" question is needed to keep a student from going on to the next team's page)
    struct ItemPlan {enum class Type{name, page, question, other} type = Type::other; int teamNum = -1; int questionNum = -1; QList<long long> aboutIDs;};
    QList<ItemPlan> itemPlans;
    QJsonArray requests;
    auto addItem = [&itemPlans, &requests](const QJsonObject &item, const ItemPlan &itemPlan) {
        QJsonObject location;
        location["index"] = int(requests.size());
        QJsonObject newItem;
        newItem["item"] = item;
        newItem["location"] = location;
        QJsonObject createItem;
        createItem["createItem"] = newItem;
        requests << createItem;
        itemPlans << itemPlan;
    };
    // builds the body of a question
    auto questionBody = [](const bool required, const QString &kindName, const QJsonObject &kind) {
        QJsonObject body;
        body["required"] = required;
        body[kindName] = kind;
        QJsonObject questionItem;
        questionItem["question"] = body;
        return questionItem;
    };
    auto choiceKind = [](const QString &type, const QStringList &options) {
        QJsonObject kind;
        kind["type"] = type;
        kind["shuffle"] = false;
        kind["options"] = optionsArray(options);
        return kind;
    };
    auto textKind = [](const bool paragraph) {
        QJsonObject kind;
        kind["paragraph"] = paragraph;
        return kind;
    };

    //the name question; its answers are sent to their team's page once the pages exist
    const QList<QPair<QString, long long>> nameOptions = peerReview->nameOptions();
    QStringList names;
    for(const auto &nameOption : nameOptions) {
        names << nameOption.first;
    }
    QJsonObject nameItem;
    nameItem["title"] = PEERREVIEWNAMEQUESTION;
    nameItem["questionItem"] = questionBody(true, "choiceQuestion", choiceKind("DROP_DOWN", names));
    addItem(nameItem, {ItemPlan::Type::name});

    for(int teamNum = 0; teamNum < peerReview->teams.size(); teamNum++) {
        const auto &team = peerReview->teams.at(teamNum);
        QStringList memberNames;        // with a number added to teammates who have the same name, so each row and question is distinct
        QList<long long> memberIDs;
        for(int memberNum = 0; memberNum < team.members.size(); memberNum++) {
            memberNames << peerReview->memberLabel(teamNum, memberNum);
            memberIDs << team.members.at(memberNum).ID;
        }

        QJsonObject pageItem;
        pageItem["title"] = team.name;
        pageItem["description"] = tr("Your team:") + " " + memberNames.join(", ");
        pageItem["pageBreakItem"] = QJsonObject();
        addItem(pageItem, {ItemPlan::Type::page, teamNum});

        for(int questionNum = 0; questionNum < peerReview->questions.size(); questionNum++) {
            const auto &question = peerReview->questions.at(questionNum);
            const QStringList labels = question.formLabels();
            const bool required = question.isRequiredInForm();
            QJsonObject item;
            item["title"] = question.text.simplified();
            item["description"] = question.formDescription();

            if(question.isAboutTeammates() && question.usesGrid()) {
                //a grid with a row for each team member
                QJsonArray rows;
                for(const auto &memberName : std::as_const(memberNames)) {
                    QJsonObject row;
                    row["title"] = memberName;
                    QJsonObject rowBody;
                    rowBody["required"] = required;
                    rowBody["rowQuestion"] = row;
                    rows << rowBody;
                }
                QJsonObject grid;
                grid["shuffleQuestions"] = false;
                grid["columns"] = choiceKind(question.multi? "CHECKBOX" : "RADIO", labels);
                QJsonObject questionGroupItem;
                questionGroupItem["questions"] = rows;
                questionGroupItem["grid"] = grid;
                item["questionGroupItem"] = questionGroupItem;
                addItem(item, {ItemPlan::Type::question, teamNum, questionNum, memberIDs});
            }
            else if(question.isAboutTeammates()) {
                //the question with its notes, then a separate question for each team member
                item["textItem"] = QJsonObject();
                addItem(item, {ItemPlan::Type::other, teamNum});
                for(int memberNum = 0; memberNum < memberNames.size(); memberNum++) {
                    QJsonObject memberItem;
                    memberItem["title"] = memberNames.at(memberNum);
                    if(!labels.isEmpty()) {
                        memberItem["questionItem"] = questionBody(required, "choiceQuestion", choiceKind(question.multi? "CHECKBOX" : "DROP_DOWN", labels));
                    }
                    else {
                        memberItem["questionItem"] = questionBody(required, "textQuestion", textKind(question.kind == PeerReviewQuestion::Kind::written));
                    }
                    addItem(memberItem, {ItemPlan::Type::question, teamNum, questionNum, {memberIDs.at(memberNum)}});
                }
            }
            else {
                //a single question, about yourself or asked once
                if(question.kind == PeerReviewQuestion::Kind::numberScale) {
                    QJsonObject scale;
                    scale["low"] = question.scaleLow;
                    scale["high"] = question.scaleHigh;
                    if(!question.lowLabel.isEmpty()) {
                        scale["lowLabel"] = question.lowLabel;
                    }
                    if(!question.highLabel.isEmpty()) {
                        scale["highLabel"] = question.highLabel;
                    }
                    item["questionItem"] = questionBody(required, "scaleQuestion", scale);
                }
                else if(!labels.isEmpty()) {
                    const QString type = question.multi? "CHECKBOX" : ((labels.size() <= 6)? "RADIO" : "DROP_DOWN");
                    item["questionItem"] = questionBody(required, "choiceQuestion", choiceKind(type, labels));
                }
                else {
                    item["questionItem"] = questionBody(required, "textQuestion", textKind(question.kind == PeerReviewQuestion::Kind::written));
                }
                const long long aboutID = (question.about == PeerReviewQuestion::About::self)? PeerReviewFormItem::ABOUTGIVER :
                                                                                               PeerReviewFormItem::ABOUTNOONE;
                addItem(item, {ItemPlan::Type::question, teamNum, questionNum, {aboutID}});
            }
        }

        //the "submit" question that ends the team's page
        QJsonObject submitOption;
        submitOption["value"] = PEERREVIEWSUBMITOPTION;
        submitOption["goToAction"] = "SUBMIT_FORM";
        QJsonObject submitKind;
        submitKind["type"] = "RADIO";
        submitKind["options"] = QJsonArray{submitOption};
        QJsonObject submitItem;
        submitItem["title"] = PEERREVIEWSUBMITQUESTION;
        submitItem["questionItem"] = questionBody(true, "choiceQuestion", submitKind);
        addItem(submitItem, {ItemPlan::Type::other, teamNum});
    }

    //create the items, a batch at a time, keeping the IDs given to each item and its question(s)
    QList<QPair<QString, QStringList>> createdIDs;      // itemId, questionIds
    for(int first = 0; first < requests.size(); first += PEERREVIEWBATCHSIZE) {
        QJsonArray batch;
        for(int request = first; request < std::min(int(requests.size()), first + PEERREVIEWBATCHSIZE); request++) {
            batch << requests.at(request);
        }
        const QJsonObject reply = postBatchUpdate(formID, batch, (first == 0)? revisionID : "");
        const QJsonArray replies = reply["replies"].toArray();
        if(replies.size() != batch.size()) {
            deleteOrphanedForm(formID);
            return {};
        }
        for(const auto &createReply : replies) {
            const QJsonObject createItem = createReply.toObject()["createItem"].toObject();
            QStringList questionIDs;
            const QJsonArray questionIDsArray = createItem["questionId"].toArray();
            for(const auto &questionID : questionIDsArray) {
                questionIDs << questionID.toString();
            }
            createdIDs.append({createItem["itemId"].toString(), questionIDs});
        }
    }

    //now that the team pages exist, send each name to its team's page
    QHash<int, QString> pageIDOfTeam;
    QString nameItemID, nameQuestionID;
    for(int itemNum = 0; itemNum < itemPlans.size(); itemNum++) {
        if(itemPlans.at(itemNum).type == ItemPlan::Type::page) {
            pageIDOfTeam.insert(itemPlans.at(itemNum).teamNum, createdIDs.at(itemNum).first);
        }
        else if(itemPlans.at(itemNum).type == ItemPlan::Type::name) {
            nameItemID = createdIDs.at(itemNum).first;
            nameQuestionID = createdIDs.at(itemNum).second.value(0);
        }
    }
    QJsonArray nameOptionsWithPages;
    for(const auto &nameOption : nameOptions) {
        QJsonObject option;
        option["value"] = nameOption.first;
        option["goToSectionId"] = pageIDOfTeam.value(peerReview->teamOf(nameOption.second));
        nameOptionsWithPages << option;
    }
    // jumping to a page is documented for radio buttons, and probably works for a dropdown; if the dropdown is refused, use radio buttons
    bool namesSentToPages = false;
    for(const QString &type : {QString("DROP_DOWN"), QString("RADIO")}) {
        QJsonObject nameKind;
        nameKind["type"] = type;
        nameKind["options"] = nameOptionsWithPages;
        QJsonObject nameBody;
        nameBody["questionId"] = nameQuestionID;
        nameBody["required"] = true;
        nameBody["choiceQuestion"] = nameKind;
        QJsonObject nameQuestionItem;
        nameQuestionItem["question"] = nameBody;
        QJsonObject updatedNameItem;
        updatedNameItem["itemId"] = nameItemID;
        updatedNameItem["title"] = PEERREVIEWNAMEQUESTION;
        updatedNameItem["questionItem"] = nameQuestionItem;
        QJsonObject location;
        location["index"] = 0;
        QJsonObject updateItemRequest;
        updateItemRequest["item"] = updatedNameItem;
        updateItemRequest["location"] = location;
        updateItemRequest["updateMask"] = "questionItem.question.choiceQuestion";
        QJsonObject updateItem;
        updateItem["updateItem"] = updateItemRequest;
        const QJsonObject reply = postBatchUpdate(formID, {updateItem});
        if(reply.contains("replies") || reply.contains("writeControl")) {
            namesSentToPages = true;
            break;
        }
    }
    if(!namesSentToPages) {
        deleteOrphanedForm(formID);
        return {};
    }

    //update the description
    info.remove("document_title");
    info["description"] = peerReview->instructions.isEmpty()? QString(PEERREVIEWINSTRUCTIONS) : peerReview->instructions;
    QJsonObject updateFormInfoRequest;
    updateFormInfoRequest["info"] = info;
    updateFormInfoRequest["updateMask"] = "description";
    QJsonObject updateFormInfo;
    updateFormInfo["updateFormInfo"] = updateFormInfoRequest;
    const QJsonObject infoReply = postBatchUpdate(formID, {updateFormInfo});
    if(!infoReply.contains("replies") && !infoReply.contains("writeControl")) {
        deleteOrphanedForm(formID);
        return {};
    }

    //if wanted, require signing in so the respondent's email is collected; this is sent by itself so that, if it is refused,
    //the form is kept (without requiring sign-in) and the instructor is told why
    QString warning;
    if(peerReview->requireSignIn) {
        QJsonObject settings;
        settings["emailCollectionType"] = "VERIFIED";
        QJsonObject updateSettingsRequest;
        updateSettingsRequest["settings"] = settings;
        updateSettingsRequest["updateMask"] = "emailCollectionType";
        QJsonObject updateSettings;
        updateSettings["updateSettings"] = updateSettingsRequest;
        const QJsonObject settingsReply = postBatchUpdate(formID, {updateSettings});
        if(!settingsReply.contains("replies") && !settingsReply.contains("writeControl")) {
            peerReview->requireSignIn = false;
            warning = tr("Google did not allow this form to require signing in, so students will respond without signing in.") +
                      (lastErrorMessage.isEmpty()? "" : ("<br><code>" + lastErrorMessage.toHtmlEscaped() + "</code>"));
        }
    }

    //publish the form so that responders can access it (required for forms created by API after June 2026)
    url = "https://forms.googleapis.com/v1/forms/" + formID + ":setPublishSettings";
    QJsonObject publishBody;
    QJsonObject publishState;
    publishState["isPublished"] = true;
    publishState["isAcceptingResponses"] = true;
    QJsonObject publishSettings;
    publishSettings["publishState"] = publishState;
    publishBody["publishSettings"] = publishSettings;
    const auto publishReplyBody = httpRequest(Method::post, url, QJsonDocument(publishBody).toJson(), "application/json");
    const bool published = lastErrorMessage.isEmpty() && !publishReplyBody.isEmpty();

    //record in the peer review which form question asks which question about whom
    peerReview->formItems.clear();
    for(int itemNum = 0; itemNum < itemPlans.size(); itemNum++) {
        const auto &itemPlan = itemPlans.at(itemNum);
        if(itemPlan.type != ItemPlan::Type::question) {
            continue;
        }
        const QStringList &questionIDs = createdIDs.at(itemNum).second;
        for(int i = 0; i < std::min(questionIDs.size(), itemPlan.aboutIDs.size()); i++) {
            peerReview->formItems.append({questionIDs.at(i), itemPlan.questionNum, itemPlan.teamNum, itemPlan.aboutIDs.at(i)});
        }
    }
    peerReview->formID = formID;
    peerReview->responderURL = surveySubmissionURL;
    peerReview->accountName = accountName;
    peerReview->formPublished = published;
    peerReview->acceptingResponses = published;
    peerReview->nameQuestionID = nameQuestionID;

    return {title, formID, QDateTime::currentDateTime().toString(Qt::ISODate), QUrl(surveySubmissionURL), published, warning};
}

QJsonArray GoogleHandler::downloadPeerReviewResponses(const QString &formID, bool *ok) {
    *ok = false;

    //download the responses a page at a time
    QJsonArray allResponses;
    QString pageToken;
    do {
        QString url = "https://forms.googleapis.com/v1/forms/" + formID + "/responses?pageSize=5000";
        if(!pageToken.isEmpty()) {
            url += "&pageToken=" + QString::fromLatin1(QUrl::toPercentEncoding(pageToken));
        }
        const auto replyBody = httpRequest(Method::get, url);
        if(!lastErrorMessage.isEmpty()) {
            return {};
        }
        const QJsonDocument json_doc = QJsonDocument::fromJson(replyBody);
        const QJsonArray responses = json_doc["responses"].toArray();
        for(const auto &response : responses) {
            allResponses << response;
        }
        pageToken = json_doc["nextPageToken"].toString();
    } while(!pageToken.isEmpty());

    *ok = true;
    return allResponses;
}

bool GoogleHandler::setPeerReviewAcceptingResponses(const QString &formID, const bool accepting) {
    const QString url = "https://forms.googleapis.com/v1/forms/" + formID + ":setPublishSettings";
    QJsonObject publishBody;
    QJsonObject publishState;
    publishState["isPublished"] = true;
    publishState["isAcceptingResponses"] = accepting;
    QJsonObject publishSettings;
    publishSettings["publishState"] = publishState;
    publishBody["publishSettings"] = publishSettings;
    const auto replyBody = httpRequest(Method::post, url, QJsonDocument(publishBody).toJson(), "application/json");
    return lastErrorMessage.isEmpty() && !replyBody.isEmpty();
}

QString GoogleHandler::createDriveFolder(const QString &folderName) {
    QJsonObject folder;
    folder["name"] = folderName;
    folder["mimeType"] = "application/vnd.google-apps.folder";
    const auto replyBody = httpRequest(Method::post, QUrl("https://www.googleapis.com/drive/v3/files"),
                                       QJsonDocument(folder).toJson(), "application/json");
    return QJsonDocument::fromJson(replyBody)["id"].toString();
}

bool GoogleHandler::shareFeedbackDoc(const QString &folderID, const QString &docName, const QString &html, const QString &email,
                                     const QString &message, QString &docURL) {
    docURL.clear();

    //upload the report as html, which Google Drive converts into a Google Doc
    const QByteArray boundary = "gruepr_" + QUuid::createUuid().toByteArray(QUuid::WithoutBraces);
    QJsonObject metadata;
    metadata["name"] = docName;
    metadata["mimeType"] = "application/vnd.google-apps.document";
    if(!folderID.isEmpty()) {
        metadata["parents"] = QJsonArray{folderID};
    }
    QByteArray body = "--" + boundary + "\r\n"
                      "Content-Type: application/json; charset=UTF-8\r\n\r\n" +
                      QJsonDocument(metadata).toJson(QJsonDocument::Compact) + "\r\n"
                      "--" + boundary + "\r\n"
                      "Content-Type: text/html; charset=UTF-8\r\n\r\n" +
                      html.toUtf8() + "\r\n"
                      "--" + boundary + "--";
    const auto uploadReplyBody = httpRequest(Method::post, QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
                                             body, "multipart/related; boundary=" + boundary);
    const QString docID = QJsonDocument::fromJson(uploadReplyBody)["id"].toString();
    if(docID.isEmpty()) {
        return false;
    }
    docURL = "https://docs.google.com/document/d/" + docID + "/edit";

    //share it with the student as a viewer; Google emails them the link
    // (the message is percent-encoded by hand, since QUrlQuery leaves a '+' as is, which Google would read as a space)
    QUrl url("https://www.googleapis.com/drive/v3/files/" + docID + "/permissions");
    QString query = "sendNotificationEmail=true";
    if(!message.isEmpty()) {
        query += "&emailMessage=" + QString::fromLatin1(QUrl::toPercentEncoding(message));
    }
    url.setQuery(query);
    QJsonObject permission;
    permission["role"] = "reader";
    permission["type"] = "user";
    permission["emailAddress"] = email;
    const auto permissionReplyBody = httpRequest(Method::post, url, QJsonDocument(permission).toJson(), "application/json", false);
    return lastErrorMessage.isEmpty() && !permissionReplyBody.isEmpty();
}


////////////////////////////////////////////

void GoogleHandler::postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData,
                                                const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals)
{
    const auto replyBody = httpRequest(Method::post, URL, postData, "application/json");

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
        for(int i = 0; i < stringInSubobjectParams.size(); i++) {
            const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
            QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
            *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
        }
    }
}

QStringList GoogleHandler::dedupedNonBlankOptions(const QStringList &options) {
    QStringList result;
    for(const auto &option : options) {
        const QString simplifiedOption = option.simplified();
        if(!simplifiedOption.isEmpty()) {
            result << simplifiedOption;
        }
    }
    result.removeDuplicates();
    return result;
}

void GoogleHandler::deleteOrphanedForm(const QString &formID) {
    const QString originalErrorMessage = lastErrorMessage;
    httpRequest(Method::del, "https://www.googleapis.com/drive/v3/files/" + formID);
    lastErrorMessage = originalErrorMessage;   // a failed cleanup should not change the error shown to the user
}

QJsonArray GoogleHandler::optionsArray(const QStringList &options) {
    QJsonArray responseOptions;
    for(const auto &option : dedupedNonBlankOptions(options)) {
        QJsonObject responseOption;
        responseOption["value"] = option;
        responseOption["isOther"] = false;
        responseOptions << responseOption;
    }
    return responseOptions;
}

QJsonObject GoogleHandler::postBatchUpdate(const QString &formID, const QJsonArray &requests, const QString &targetRevisionID) {
    QJsonObject requestBody;
    requestBody["includeFormInResponse"] = false;
    if(!targetRevisionID.isEmpty()) {
        QJsonObject writeControl;
        writeControl["targetRevisionId"] = targetRevisionID;
        requestBody["writeControl"] = writeControl;
    }
    requestBody["requests"] = requests;
    const auto replyBody = httpRequest(Method::post, "https://forms.googleapis.com/v1/forms/" + formID + ":batchUpdate",
                                       QJsonDocument(requestBody).toJson(), "application/json");
    return QJsonDocument::fromJson(replyBody).object();
}

QSet<QByteArray> GoogleHandler::getScopes() const {
    QSet<QByteArray> result;
    for (const auto &s : QString(SCOPES).split(' ', Qt::SkipEmptyParts)) {
        result.insert(s.toUtf8());
    }
    return result;
}

QString GoogleHandler::getClientID() const {
    return CLIENT_ID;
}

QString GoogleHandler::getClientSecret() const {
    return CLIENT_SECRET;
}

QString GoogleHandler::getClientAuthorizationUrl() const {
    return AUTHENTICATEURL;
}

QString GoogleHandler::getClientAccessTokenUrl() const {
    return ACCESSTOKENURL;
}

QString GoogleHandler::getActionDialogIcon() const {
    return ICON;
}

QPixmap GoogleHandler::icon() {
    return {ICON};
}

QString GoogleHandler::getActionDialogLabel() const {
    return tr("Communicating with Google...");
}

std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> GoogleHandler::getModifyParametersFunction() const {
    auto code_verifier = std::make_shared<QByteArray>();

    return [code_verifier](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            // Generate fresh PKCE for each authorization attempt
            *code_verifier = (QUuid::createUuid().toString(QUuid::WithoutBraces) +
                              QUuid::createUuid().toString(QUuid::WithoutBraces)).toLatin1();
            auto code_challenge = QCryptographicHash::hash(*code_verifier, QCryptographicHash::Sha256).
                                  toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

            parameters->replace("access_type", "offline");
            parameters->replace("code_challenge", code_challenge);
            parameters->replace("code_challenge_method", "S256");
        }
        else if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            parameters->replace("code_verifier", *code_verifier);
            // Percent-decode the "code" parameter so Google can match it
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->replace("code", QUrl::fromPercentEncoding(code));
        }
    };
}
