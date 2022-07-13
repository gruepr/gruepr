#include "googlehandler.h"
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
    manager = new QNetworkAccessManager;
    google = new QOAuth2AuthorizationCodeFlow(QString(AUTHENTICATEURL), QString(ACCESSTOKENURL), manager, this);
    google->networkAccessManager()->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

GoogleHandler::~GoogleHandler() {
    delete manager;
    delete google;
}

GoogleForm GoogleHandler::createSurvey(const Survey *const survey) {
    //create a Google Form
    QString url = "https://forms.googleapis.com/v1/forms";
    google->setContentType(QAbstractOAuth::ContentType::Json);
    QString title = survey->title.isEmpty() ? QDateTime::currentDateTime().toString("hh:mm dd MMMM yyyy") : survey->title;
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
    QString formID = formIDInList.constFirst();
    QString surveySubmissionURL = submissionURL.constFirst();
    QString revisionID = revisionIDinList.constFirst();
    if(formID.isEmpty()) {
        return {};
    }

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
        item["title"] = question.text;
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
            item["description"] = "You may need to scroll to see all columns (" +
                                  QString::number((survey->schedStartTime <= 12) ? survey->schedStartTime : (survey->schedStartTime - 12)) + ":00" + (survey->schedStartTime < 12 ? "am" : "pm") + " to " +
                                  QString::number((survey->schedEndTime <= 12) ? survey->schedEndTime : (survey->schedEndTime - 12)) + ":00" + (survey->schedEndTime < 12 ? "am" : "pm") + ").";
            QJsonObject questionGroupItem;
            QJsonObject grid;
            grid["shuffleQuestions"] = false;
            QJsonObject choiceQuestion;
            choiceQuestion["type"] = "CHECKBOX";
            QJsonArray times;
            for(int i = survey->schedStartTime; i <= survey->schedEndTime; i++) {
                QJsonObject time;
                time["value"] = QString::number((i <= 12) ? i : (i - 12)) + ":00" + (i < 12 ? "am" : "pm");
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
                row["title"] = dayName;
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
    requestBody["requests"] = requests;

    stringParams = {&y};
    stringInSubobjectParams = {&revisionIDinList};
    revisionIDinList.clear();
    postToGoogleGetSingleResult(url, QJsonDocument(requestBody).toJson(), {}, stringParams, {"writeControl/requiredRevisionId"}, stringInSubobjectParams);
    revisionID = revisionIDinList.constFirst();

    if(revisionID.isEmpty()) {
        return {};
    }
    return {title, formID, QDateTime::currentDateTime().toString(Qt::ISODate), ""};
}

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

QStringList GoogleHandler::getSurveyList() {
    formsList.clear();

    QSettings settings;
    int size = settings.beginReadArray("GoogleForm");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        QString id = settings.value("ID").toString();
        QString createdTime = settings.value("createdTime").toString();
        QString downloadURL = settings.value("downloadURL").toString();
        if(!downloadURL.isEmpty()) {
            formsList.append({name, id, createdTime, downloadURL});
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
    for(const auto &form : qAsConst(formsList)) {
        formNames.append(form.name);
    }

    return formNames;
}

QString GoogleHandler::downloadSurveyResult(const QString &formName) {
    QString URL;
    for(const auto &form : qAsConst(formsList)) {
        if(form.name == formName) {
            URL = form.downloadURL;
        }
    }
    if(URL.isEmpty()) {
        return {};
    }

    QEventLoop loop;
    auto *reply = google->get(URL);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if(reply->bytesAvailable() == 0)
    {
        //qDebug() << "no reply";
        delete reply;
        return {};
    }
    QByteArray replyBody = reply->readAll();
    //qDebug() << replyBody;
    reply->deleteLater();

    QRegularExpression unallowedChars(R"([#&&{}\/\<>*?$!'":@+`|=])");
    QFileInfo filepath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), formName.simplified().replace(unallowedChars, "_") + ".csv");
    QFile file(filepath.absoluteFilePath());
    file.open(QIODevice::WriteOnly);
    //qDebug() << file.errorString();
    QDataStream out(&file);
    out << replyBody;

    return filepath.absoluteFilePath();
}


////////////////////////////////////////////

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

    do
    {
        reply = google->get(url);

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
            for(int i = 0; i < stringInArrayParams.size(); i++)
            {
                QStringList subArrayAndParamName = stringInArrayParams.at(i).split('/');   // "array_name/string_paramater_name"
                QJsonArray subarray = json_obj[subArrayAndParamName.at(0)].toArray();
                for(const auto &subvalue : qAsConst(subarray))
                {
                    *(stringInArrayVals[i]) << subvalue[subArrayAndParamName.at(1)].toString();
                }
            }
        }
        //next 3 lines work with do/while in case more than one page of results exist. Currently based off Canvas pagination; would need to be updated to work here
        QRegularExpression nextURL(R"(^.*\<(.*?)\>; rel="next")");
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
    QString replyBody;
    QJsonDocument json_doc;
    QJsonArray json_array;

    auto *reply = google->post(URL, postData);

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
        for(int i = 0; i < stringInSubobjectParams.size(); i++)
        {
            QStringList subobjectAndParamName = stringInSubobjectParams.at(i).split('/');   // "subobject_name/string_paramater_name"
            QJsonObject object = json_obj[subobjectAndParamName.at(0)].toObject();
            *(stringInSubobjectVals[i]) << object[subobjectAndParamName.at(1)].toString();
        }
    }

    reply->deleteLater();
}

void GoogleHandler::authenticate(Permissions permissions) {
    if(permissions == Permissions::readonly) {
        google->setScope("https://www.googleapis.com/auth/drive.readonly");
    }
    else {
        google->setScope("https://www.googleapis.com/auth/forms.body "
                         "https://www.googleapis.com/auth/forms "
                         "https://www.googleapis.com/auth/spreadsheets "
                         "https://www.googleapis.com/auth/drive.file");
    }
    QAbstractOAuth2::connect(google, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, &QDesktopServices::openUrl);

    const QUrl redirectUri = QString(REDIRECT_URI);
    const auto port = static_cast<quint16>(redirectUri.port(REDIRECT_URI_PORT));

    google->setClientIdentifier(CLIENT_ID);
    google->setClientIdentifierSharedKey(CLIENT_SECRET);

    google->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QVariantMap *parameters) {
        // Percent-decode the "code" parameter so Google can match it
        if (stage == QAbstractOAuth::Stage::RequestingAccessToken) {
            QByteArray code = parameters->value("code").toByteArray();
            (*parameters)["code"] = QUrl::fromPercentEncoding(code);
        }
    });

    auto *replyHandler = new QOAuthHttpServerReplyHandler(port);
    google->setReplyHandler(replyHandler);

    google->grant();
    connect(google, &QOAuth2AuthorizationCodeFlow::granted, this, [this](){emit granted(); authenticated = true;/*    qDebug() << google->refreshToken();*/});
}

QMessageBox* GoogleHandler::busy(QWidget *parent) {
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *busyDialog = new QMessageBox(parent);
    QPixmap icon(":/icons/google.png");
    busyDialog->setIconPixmap(icon.scaled(GOOGLEICONSIZE));
    busyDialog->setText(tr("Communicating with Google..."));
    busyDialog->setStandardButtons(QMessageBox::NoButton);
    busyDialog->setModal(false);
    busyDialog->show();
    return busyDialog;
}

void GoogleHandler::notBusy(QMessageBox *busyDialog) {
    QApplication::restoreOverrideCursor();
    busyDialog->close();
    delete busyDialog;
}
