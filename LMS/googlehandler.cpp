#include "googlehandler.h"
#include "googlesecrets.h"
#include <QAbstractButton>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

GoogleHandler::GoogleHandler(QWidget *parent) : LMS(parent), parent(parent) {
    initOAuth2();
    OAuthFlow->setAuthorizationUrl(QString(AUTHENTICATEURL));
    OAuthFlow->setAccessTokenUrl(QString(ACCESSTOKENURL));

    auto code_verifier = (QUuid::createUuid().toString(QUuid::WithoutBraces) +
                          QUuid::createUuid().toString(QUuid::WithoutBraces)).toLatin1(); // 43 <= length <= 128
    auto code_challenge = QCryptographicHash::hash(code_verifier, QCryptographicHash::Sha256).
                                                                  toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);


    const QSettings settings;
    const QString refreshToken = settings.value("GoogleRefreshToken", "").toString();
    accountName = settings.value("GoogleAccountName", "").toString();

    if(!refreshToken.isEmpty()) {
        refreshTokenExists = true;
        OAuthFlow->setRefreshToken(refreshToken);
    }
}

GoogleHandler::~GoogleHandler() {
    QSettings settings;
    const QString refreshToken = OAuthFlow->refreshToken();
    if(!refreshToken.isEmpty()) {
        settings.setValue("GoogleRefreshToken", refreshToken);
    }
    if(!accountName.isEmpty()) {
        settings.setValue("GoogleAccountName", accountName);
    }
}

bool GoogleHandler::authenticate() {
    // if refreshToken is found, try to use it to get accessTokens without re-granting permission
    if(refreshTokenExists) {
        auto *loginDialog = new QMessageBox(parent);
        loginDialog->setModal(true);
        loginDialog->setStyleSheet(LABEL10PTSTYLE);
        loginDialog->setIconPixmap(icon().scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        loginDialog->setStandardButtons(QMessageBox::Cancel);
        loginDialog->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);

        bool changingAccounts = false;
        if(!accountName.isEmpty()){
            loginDialog->setText(tr("Connecting to Google with account ") + accountName + "...\n" + tr("Press Cancel now to use a different account"));
            QTimer::singleShot(UI_DISPLAY_DELAYTIME * 2, loginDialog, &QMessageBox::accept);
            loginDialog->exec();
            changingAccounts = (loginDialog->result() == QMessageBox::Cancel);
        }

        if(changingAccounts) {
            authenticated = false;
            refreshTokenExists = false;
            OAuthFlow->setRefreshToken("");
        }
        else {
            loginDialog->setText(tr("Connecting to Google..."));
            loginDialog->open();
            connect(this, &LMS::replyReceived, loginDialog, [&loginDialog, this](const QString &reply) {
                loginDialog->setText(loginDialog->text() + "\n" + tr("Response received..."));
                const auto json_doc = QJsonDocument::fromJson(reply.toUtf8());
                const auto id_token = json_doc["id_token"].toString().split('.');  //id_token is JWT-encoded user info
                if(id_token.size() == 3) {
                    const auto id_token_JWTPayload = QJsonDocument::fromJson(QByteArray::fromBase64(id_token.at(1).toUtf8())).object();
                    accountName = id_token_JWTPayload["email"].toString();
                }
            });
            connect(this, &LMS::granted, loginDialog, &QMessageBox::accept);
            connect(this, &LMS::denied, loginDialog, [&loginDialog]() {
                loginDialog->setText(loginDialog->text() + "\n" + tr("Google is requesting that you re-authorize gruepr."));
                QEventLoop loop;
                QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
                loop.exec();
                loginDialog->accept();
            });

            LMS::authenticate();

            if(loginDialog->exec() == QMessageBox::Cancel) {
                loginDialog->deleteLater();
                return false;
            }
        }
        loginDialog->deleteLater();
    }

    // didn't have a refreshToken, the refreshToken didn't work, or user wants to change accounts: time to (re)authorize
    if(!authenticated) {
        auto *loginDialog2 = new QMessageBox(parent);
        loginDialog2->setModal(true);
        loginDialog2->setStyleSheet(LABEL10PTSTYLE);
        loginDialog2->setIconPixmap(icon().scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        loginDialog2->setText(QString() + tr("The next step will open a browser window so you can sign in with Google.\n\n"
                                            "  » Your computer may ask whether gruepr can access the network. "
                                            "This access is needed so that gruepr and Google can communicate.\n\n"
                                            "  » In the browser, Google will ask whether you authorize gruepr "
                                            "to access the files gruepr creates on your Google Drive. "
                                            "This access is needed so that the survey responses can now be downloaded.\n\n"
                                            "  » All data associated with this survey, including the questions asked and "
                                            "responses received, exist in your Google Drive only. "
                                            "No data from or about this survey will ever be stored or sent anywhere else."));
        loginDialog2->setStandardButtons(QMessageBox::Ok|QMessageBox::Cancel);
        auto *okButton = loginDialog2->button(QMessageBox::Ok);
        const int height = okButton->height();
        QPixmap loginpic(":/icons_new/google_signin_button.png");
        loginpic = loginpic.scaledToHeight(int(1.5f * float(height)), Qt::SmoothTransformation);
        okButton->setText("");
        okButton->setIconSize(loginpic.rect().size());
        okButton->setIcon(loginpic);
        okButton->adjustSize();
        loginDialog2->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);

        if(loginDialog2->exec() == QMessageBox::Cancel) {
            loginDialog2->deleteLater();
            return false;
        }

        loginDialog2->setText(tr("Please use your browser to log in to Google and then return here."));
        loginDialog2->setStandardButtons(QMessageBox::Cancel);
        loginDialog2->open();
        connect(this, &LMS::replyReceived, loginDialog2, [&loginDialog2, this](const QString &reply) {
            loginDialog2->setText(loginDialog2->text() + "\n" + tr("Response received..."));
            const auto json_doc = QJsonDocument::fromJson(reply.toUtf8());
            const auto id_token = json_doc["id_token"].toString().split('.');  //id_token is JWT-encoded user info
            if(id_token.size() == 3) {
                const auto id_token_JWTPayload = QJsonDocument::fromJson(QByteArray::fromBase64(id_token.at(1).toUtf8())).object();
                accountName = id_token_JWTPayload["email"].toString();
            }
        });
        connect(this, &LMS::granted, loginDialog2, &QMessageBox::accept);

        LMS::authenticate();

        if(loginDialog2->exec() == QMessageBox::Cancel) {
            loginDialog2->deleteLater();
            return false;
        }

        loginDialog2->deleteLater();
    }

    return true;
}

GoogleForm GoogleHandler::createSurvey(const Survey *const survey) {
    //create a Google Form--only the title and document_title can be set in this step
    QString url = "https://forms.googleapis.com/v1/forms";
    OAuthFlow->setContentType(QAbstractOAuth::ContentType::Json);
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
    QStringList userData = replybody.split("__USERDATA__");
    if(userData.size() != 3) {
        return {};
    }
    QString data = userData.at(1);
    data.replace(R"(\x3d)", R"(=)").replace(R"(\x26)", R"( )").replace(R"(\/)", R"(/)");

    QStringList urlsInReply = data.split(' ');
    // parse the URLs from the reply
    QString editURL, responseURL, csvURL;
    for(const auto &urlInReply : qAsConst(urlsInReply)) {
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
    for(const auto &form : qAsConst(formsList)) {
        formNames.append(form.name);
    }

    return formNames;
}

QString GoogleHandler::downloadSurveyResult(const QString &surveyName) {
    //get the ID
    QString ID;
    for(const auto &form : qAsConst(formsList)) {
        if(form.name == surveyName) {
            ID = form.ID;
        }
    }
    if(ID.isEmpty()) {
        return {};
    }

    //download the form itself into a JSON so that we can get the questions
    QString url = "https://forms.googleapis.com/v1/forms/" + ID;
    QEventLoop loop;
    auto *reply = OAuthFlow->get(url);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0) {
        //qDebug() << "no reply";
        delete reply;
        return {};
    }
    QString replyBody = reply->readAll();
    //qDebug() << replyBody;
    reply->deleteLater();
    QJsonDocument json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
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
    for(const auto &question : qAsConst(questions)) {
        out << ",\"" << question.text << "\"";
    }
    out << Qt::endl;

    //now download into a big JSON all of the responses (actually it's just the first 5000 responses! if >5000, will need to work with paginated results!)
    url = "https://forms.googleapis.com/v1/forms/" + ID + "/responses";
    reply = OAuthFlow->get(url);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0) {
        //qDebug() << "no reply";
        delete reply;
        file.close();
        return filepath.absoluteFilePath();
    }
    replyBody = reply->readAll();
    //qDebug() << replyBody;
    reply->deleteLater();
    json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
    //pull out each response and save as a row in the file, save the submitted time as a time stamp, and get the question answer(s)
    const QJsonArray responses = json_doc["responses"].toArray();
    for(const auto &response : qAsConst(responses)) {
        out << response.toObject()["lastSubmittedTime"].toString();

        //pull out the answer(s) to each question in order, joining the answers with a semicolon if >1
        const QJsonObject answers = response.toObject()["answers"].toObject();
        for(const auto &question : qAsConst(questions)) {
            const QJsonArray nestedAnswers = answers[question.ID]["textAnswers"]["answers"].toArray();
            QStringList allValues;
            allValues.reserve(nestedAnswers.size());
            for(const auto &nestedAnswer : qAsConst(nestedAnswers)) {
                const auto answerObject = nestedAnswer.toObject();
                allValues << answerObject["value"].toString();
            }
            out << ",\"" << allValues.join(question.type == GoogleFormQuestion::Type::schedule? ';' : ',') << "\"";
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
    QEventLoop loop;
    auto *reply = OAuthFlow->post(URL, postData);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0) {
        //qDebug() << "no reply";
        delete reply;
        return;
    }

    const QString replyBody = reply->readAll();
    //qDebug() << replyBody;
    const QJsonDocument json_doc = QJsonDocument::fromJson(replyBody.toUtf8());
    QJsonArray json_array;
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
        for(int i = 0; i < stringInSubobjectParams.size(); i++) {
            const QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
            QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
            *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
        }
    }

    reply->deleteLater();
}

QString GoogleHandler::getScopes() const {
    return "openid email "
           "https://www.googleapis.com/auth/drive.file";
}

QString GoogleHandler::getClientID() const {
    return CLIENT_ID;
}

QString GoogleHandler::getClientSecret() const {
    return CLIENT_SECRET;
}

QString GoogleHandler::getActionDialogIcon() const {
    return ":/icons_new/google.png";
}

QString GoogleHandler::getActionDialogLabel() const {
    return tr("Communicating with Google...");
}

std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> GoogleHandler::getModifyParametersFunction() const {
    return [this](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            // needed to get refresh_token from Google Cloud
            parameters->insert("access_type", "offline");
            parameters->insert("code_challenge", code_challenge);
            parameters->insert("code_challenge_method", "S256");
        }
        else if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            parameters->insert("code_verifier", code_verifier);
            // Percent-decode the "code" parameter so Google can match it
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->replace("code", QUrl::fromPercentEncoding(code));
        }
    };
}

QPixmap GoogleHandler::icon() {
    return {":/icons_new/google.png"};
}
