#include "googlehandler.h"
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
#include <QVBoxLayout>

GoogleHandler::GoogleHandler(QWidget *parent) : LMS(parent), parent(parent) {
    initOAuth2();

    const QSettings settings;
    const QByteArray key = "gruepr";

    QByteArray encToken = settings.value("GoogleRefreshToken", "").toByteArray();
    if(!encToken.isEmpty()) {
        for (int i = 0; i < encToken.size(); ++i) {
            encToken[i] ^= key[i%key.size()];
        }
        OAuthFlow->setRefreshToken(encToken);
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
        QByteArray encToken = OAuthFlow->refreshToken().toUtf8();
        for (int i = 0; i < encToken.size(); ++i) {
            encToken[i] ^= key[i%key.size()];
        }
        settings.setValue("GoogleRefreshToken", encToken);
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
            const auto id_token_JWTPayload = QJsonDocument::fromJson(QByteArray::fromBase64(id_token.at(1).toUtf8())).object();
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
        QJsonObject item;
        item["title"] = question.text.simplified();
        switch(question.type) {
        case Question::QuestionType::shorttext:
        case Question::QuestionType::longtext: {
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
            QJsonObject questionItem;
            QJsonObject questionBody;
            questionBody["required"] = false;
            QJsonObject kind;
            kind["type"] = (question.type == Question::QuestionType::dropdown ? "DROP_DOWN" :
                                (question.type == Question::QuestionType::radiobutton ? "RADIO" : "CHECKBOX"));
            kind["shuffle"] = false;
            QJsonArray responseOptions;
            for(const auto &option : question.options) {
                if((question.type == Question::QuestionType::dropdown) && (option.simplified().isEmpty())) { // google doesn't like blank options in a dropdown
                    continue;
                }
                QJsonObject responseOption;
                responseOption["value"] = option.simplified();
                responseOption["isOther"] = false;
                responseOptions << responseOption;
            }
            kind["options"] = responseOptions;
            questionBody["choiceQuestion"] = kind;
            questionItem["question"] = questionBody;
            item["questionItem"] = questionItem;
            break;}
        case Question::QuestionType::schedule: {
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
        return {};
    }
    revisionID = revisionIDinList.constFirst();
    if(revisionID.isEmpty()) {
        return {};
    }

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

    return {title, formID, currTime, QUrl(surveySubmissionURL)};
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
        out << response.toObject()["lastSubmittedTime"].toString();

        //pull out the answer(s) to each question in order, joining the answers with a semicolon if >1
        const QJsonObject answers = response.toObject()["answers"].toObject();
        for(const auto &question : std::as_const(questions)) {
            const QJsonArray nestedAnswers = answers[question.ID]["textAnswers"]["answers"].toArray();
            allValuesInField.clear();
            allValuesInField.reserve(nestedAnswers.size());
            for(const auto &nestedAnswer : std::as_const(nestedAnswers)) {
                const auto answerObject = nestedAnswer.toObject();
                allValuesInField << answerObject["value"].toString().replace('"', '\'');  // stray quotation marks in field can confuse the csv parser --> replace with apostrophe
            }
            out << ",\"" << allValuesInField.join(question.type == GoogleFormQuestion::Type::schedule? ';' : ',') << "\"";
        }

        out << Qt::endl;
    }

    file.close();
    return filepath.absoluteFilePath();
}


////////////////////////////////////////////

void GoogleHandler::postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData,
                                                const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals)
{
    OAuthFlow->setContentType(QAbstractOAuth::ContentType::Json);
    const auto replyBody = httpRequest(Method::post, URL, postData);

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

QString GoogleHandler::getScopes() const {
    return SCOPES;
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
    auto code_verifier = (QUuid::createUuid().toString(QUuid::WithoutBraces) +
                          QUuid::createUuid().toString(QUuid::WithoutBraces)).toLatin1(); // 43 <= length <= 128
    auto code_challenge = QCryptographicHash::hash(code_verifier, QCryptographicHash::Sha256).
                          toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    return [code_verifier, code_challenge](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            // needed to get refresh_token from Google Cloud
            parameters->insert("access_type", "offline");
            // PKCE
            parameters->insert("code_challenge", code_challenge);
            parameters->insert("code_challenge_method", "S256");
        }
        else if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            // PKCE
            parameters->insert("code_verifier", code_verifier);
            // Percent-decode the "code" parameter so Google can match it
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->replace("code", QUrl::fromPercentEncoding(code));
        }
    };
}
