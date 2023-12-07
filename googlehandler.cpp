#include "googlehandler.h"
#include "googlesecrets.h"
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

GoogleHandler::GoogleHandler() {
    manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->setTransferTimeout(TIMEOUT_TIME);
    google = new QOAuth2AuthorizationCodeFlow(QString(AUTHENTICATEURL), QString(ACCESSTOKENURL), manager, this);

    QString refreshToken;
    const QSettings settings;
    refreshToken = settings.value("GoogleRefreshToken", "").toString();

    if(!refreshToken.isEmpty()) {
        refreshTokenExists = true;
        google->setRefreshToken(refreshToken);
    }
}

GoogleHandler::~GoogleHandler() {
    QSettings settings;
    settings.setValue("GoogleRefreshToken", google->refreshToken());

    delete google->replyHandler();
    delete manager;
    delete google;
}

GoogleForm GoogleHandler::createSurvey(const Survey *const survey) {
    //create a Google Form--only the title and document_title can be set in this step
    QString url = "https://forms.googleapis.com/v1/forms";
    google->setContentType(QAbstractOAuth::ContentType::Json);
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
    postToGoogleGetSingleResult(url, QJsonDocument(form).toJson(), {"formId", "responderUri", "revisionId"}, stringParams, {}, stringInSubobjectParams);
    const QString formID = formIDInList.constFirst();
    const QString surveySubmissionURL = submissionURL.constFirst();
    QString revisionID = revisionIDinList.constFirst();
    if(formID.isEmpty()) {
        return {};
    }

    //"update" the form by adding each of the questions one-at-a-time and then updating the description
    //in future version of Forms API, can hopefully also update additional forms settings: setAcceptingResponses(true).setCollectEmail(false).setLimitOneResponsePerUser(false).setShowLinkToRespondAgain(false)
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
            kind["type"] = (question.type == Question::QuestionType::dropdown ? "DROP_DOWN" : (question.type == Question::QuestionType::radiobutton ? "RADIO" : "CHECKBOX"));
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
            item["description"] = "You may need to scroll to see all columns (" + survey->schedTimeNames.first() + " to " + survey->schedTimeNames.last() + ").";
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
    postToGoogleGetSingleResult(url, QJsonDocument(requestBody).toJson(), {}, stringParams, {"writeControl/requiredRevisionId"}, stringInSubobjectParams);
    revisionID = revisionIDinList.constFirst();

    if(revisionID.isEmpty()) {
        return {};
    }
    return {title, formID, QDateTime::currentDateTime().toString(Qt::ISODate), QUrl(surveySubmissionURL)};
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
        if(!id.isEmpty()) {
            formsList.append({name, id, createdTime, responderURL});
        }
    }
    settings.endArray();

// below requires access to (restricted) google drive scope; it lists all forms in the user's drive, not just those create by this app--note the "trashed=false" setting seems not to work
/*
    QList<QStringList*> stringInArrayParams = {&ids, &names, &createdTimes};
    QUrl fileURL("https://www.googleapis.com/drive/v3/files");
    QUrlQuery terms;
    terms.addQueryItem("q", "mimeType='application/vnd.google-apps.form'");
    terms.addQueryItem("trashed", "false");
    terms.addQueryItem("orderBy", "createdTime desc");
    terms.addQueryItem("fields", "files(id,name,createdTime)");
    fileURL.setQuery(terms);
    getFileList(fileURL.toString(QUrl::None), {"files/id", "files/name", "files/createdTime"}, stringInArrayParams);
*/

    std::sort(formsList.begin(), formsList.end(), [](const GoogleForm &lhs, const GoogleForm &rhs)
                                                  {return (QDateTime::fromString(lhs.createdTime,Qt::ISODate) < QDateTime::fromString(rhs.createdTime,Qt::ISODate));});
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
    auto *reply = google->get(url);
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
            questions.append({object["questionItem"]["question"]["questionId"].toString(), object["title"].toString(), GoogleFormQuestion::Type::notSchedule});
        }
        else if(object.contains("questionGroupItem")) {
            const QJsonArray questionsInGroup = object["questionGroupItem"]["questions"].toArray();
            for(const auto &questionInGroup : questionsInGroup) {
                const auto questionObject = questionInGroup.toObject();
                const QJsonObject rowQuestion = questionObject["rowQuestion"].toObject();
                questions.append({questionObject["questionId"].toString(), object["title"].toString() + "[" + rowQuestion["title"].toString() + "]", GoogleFormQuestion::Type::schedule});
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

    //now download into a big JSON all of the responses (actually it's just the first 5000 responses! if there are >5000, will need to work with paginated results!)
    url = "https://forms.googleapis.com/v1/forms/" + ID + "/responses";
    reply = google->get(url);
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

// function below gives list of Google Forms on Google Drive; requires direct access to Google Drive, which is a restricted scope and requires high security
/*
void GoogleHandler::getFileList(const QString &initialURL, const QStringList &stringInArrayParams, QList<QStringList*> &stringInArrayVals)
{
    QEventLoop loop;
    QNetworkReply *reply = nullptr;
    QString url = initialURL, replyHeader, replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;
    int numPages = 0;

    do {
        reply = google->get(url);

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
            for(int i = 0; i < stringInArrayParams.size(); i++) {
                QStringList subArrayAndParamName = stringInArrayParams.at(i).split('/');   // "array_name/string_paramater_name"
                QJsonArray subarray = json_obj[subArrayAndParamName.at(0)].toArray();
                for(const auto &subvalue : qAsConst(subarray)) {
                    *(stringInArrayVals[i]) << subvalue[subArrayAndParamName.at(1)].toString();
                }
            }
        }
        //next 3 lines work with do/while in case more than one page of results exist. Currently based off Canvas pagination; would need to be updated to work here
        static QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
        QRegularExpressionMatch nextURLMatch = nextURL.match(replyHeader);
        url = nextURLMatch.captured(1);
    }
    while(!url.isNull() && ++numPages < NUM_PAGES_TO_LOAD);

    reply->deleteLater();
}
*/

void GoogleHandler::postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                                const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals)
{
    QEventLoop loop;
    auto *reply = google->post(URL, postData);
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

void GoogleHandler::authenticate() {
    google->setScope("https://www.googleapis.com/auth/drive.file");

    const QUrl redirectUri = QString(REDIRECT_URI);
    const auto port = static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT));

    google->setClientIdentifier(CLIENT_ID);
    google->setClientIdentifierSharedKey(CLIENT_SECRET);

    google->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        // Percent-decode the "code" parameter so Google can match it
        if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            const QByteArray code = parameters->value("code").toByteArray();
            parameters->insert("code", QUrl::fromPercentEncoding(code));
        }
    });

    auto *replyHandler = new grueprOAuthHttpServerReplyHandler(port, this);
    google->setReplyHandler(replyHandler);

    connect(google, &QOAuth2AuthorizationCodeFlow::granted, this, [this](){authenticated = true;
                                                                           emit granted();});
    connect(replyHandler, &grueprOAuthHttpServerReplyHandler::error, this, [this](const QString &error){
                                                                                    authenticated = false;
                                                                                    google->setRefreshToken("");
                                                                                    refreshTokenExists = false;
                                                                                    emit denied();});

    if(refreshTokenExists) {
        google->refreshAccessToken();
    }
    else {
        QAbstractOAuth2::connect(google, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);
        google->grant();
    }
}

QDialog* GoogleHandler::actionDialog(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *busyDialog = new QDialog(parent);
    busyDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
    actionDialogIcon = new QLabel(busyDialog);
    actionDialogIcon->setPixmap(QPixmap(":/icons_new/google.png").scaled(GOOGLEICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionDialogLabel = new QLabel(busyDialog);
    actionDialogLabel->setStyleSheet(LABELSTYLE);
    actionDialogLabel->setText(tr("Communicating with Google..."));
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

void GoogleHandler::actionComplete(QDialog *busyDialog) {
    QApplication::restoreOverrideCursor();
    busyDialog->accept();
    busyDialog->deleteLater();
}



void grueprOAuthHttpServerReplyHandler::networkReplyFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    }

    QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
};
