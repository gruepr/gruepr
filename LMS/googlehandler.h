#ifndef GOOGLEHANDLER_H
#define GOOGLEHANDLER_H

#include "LMS.h"
#include "peerReview.h"
#include "survey.h"
#include <QJsonArray>


class GoogleHandler : public LMS
{
    Q_OBJECT

    struct GoogleFormQuestion {QString ID = ""; QString text = ""; enum class Type{notSchedule, schedule} type = Type::notSchedule;};
    struct GoogleForm {QString name = ""; QString ID = ""; QString createdTime = ""; QUrl responderURL = {}; bool published = false;
                       QString warning = "";};     // warning: something the user should know about a form that was otherwise created

public:
    GoogleHandler(QWidget *parent = nullptr);
    ~GoogleHandler() override;
    GoogleHandler(const GoogleHandler&) = delete;
    GoogleHandler operator= (const GoogleHandler&) = delete;
    GoogleHandler(GoogleHandler&&) = delete;
    GoogleHandler& operator= (GoogleHandler&&) = delete;

    bool authenticate() override;

    GoogleForm createSurvey(const Survey *const survey);
    QStringList getSurveyList();
    QString getSurveyResponderURL(const QString &surveyName) const;    // the link students use to fill out a survey in getSurveyList()
    QString downloadSurveyResult(const QString &surveyName);

    /**
     * @brief createPeerReviewForm Creates the Google Form for a peer review: a page with the name question, which sends each student to
     * their own team's page, and a page for each team with the questions about its members.
     * @param peerReview The peer review, which gets the form's ID and URL and the record of which form question asks what about whom.
     * @return The form's details (with an empty name if the form couldn't be created).
     */
    GoogleForm createPeerReviewForm(PeerReview *const peerReview);

    /**
     * @brief downloadPeerReviewResponses Downloads all of the responses to a peer review form.
     * @param formID The form's ID.
     * @param ok Set to true if the responses were downloaded.
     * @return The responses, as given by the Forms API.
     */
    QJsonArray downloadPeerReviewResponses(const QString &formID, bool *ok);

    /**
     * @brief setPeerReviewAcceptingResponses Stops or resumes a peer review form's accepting of responses.
     * @return True if the change was made.
     */
    bool setPeerReviewAcceptingResponses(const QString &formID, const bool accepting);

    /**
     * @brief createDriveFolder Creates a folder in the user's Google Drive.
     * @return The folder's ID (empty if it couldn't be created).
     */
    QString createDriveFolder(const QString &folderName);

    /**
     * @brief shareFeedbackDoc Creates a Google Doc of one student's feedback report and shares it with them as a viewer.
     * Google then emails the student a link to the Doc, along with the message.
     * @param folderID The Drive folder to put the Doc in.
     * @param docName The name of the Doc.
     * @param html The contents of the Doc.
     * @param email The student's email address.
     * @param message A message included in Google's email to the student.
     * @param docURL Set to the URL of the Doc, if it was created.
     * @return True if the Doc was created and shared.
     */
    bool shareFeedbackDoc(const QString &folderID, const QString &docName, const QString &html, const QString &email,
                          const QString &message, QString &docURL);

    QString getAccountName() const {return accountName;}

    static QPixmap icon();

private:
    void postToGoogleGetSingleResult(const QString &URL, const QByteArray &postData, const QStringList &stringParams, QList<QStringList*> &stringVals,
                                                                                     const QStringList &stringInSubobjectParams, QList<QStringList*> &stringInSubobjectVals);
    static QStringList dedupedNonBlankOptions(const QStringList &options);
    void deleteOrphanedForm(const QString &formID);
    static QJsonArray optionsArray(const QStringList &options);
    QJsonObject postBatchUpdate(const QString &formID, const QJsonArray &requests, const QString &targetRevisionID = "");
    inline static const int PEERREVIEWBATCHSIZE = 100;     // number of items created per request, small enough that each request is processed quickly

    QList<GoogleForm> formsList;
    QString accountName;

    QWidget *parent = nullptr;

    QSet<QByteArray> getScopes() const override;
    QString getClientID() const override;
    QString getClientSecret() const override;
    QString getClientAuthorizationUrl() const override;
    QString getClientAccessTokenUrl() const override;
    QString getActionDialogIcon() const override;
    QString getActionDialogLabel() const override;
    std::function<void(QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters)> getModifyParametersFunction() const override;

    inline static const char AUTHENTICATEURL[]{"https://accounts.google.com/o/oauth2/auth"};
    inline static const char ACCESSTOKENURL[]{"https://oauth2.googleapis.com/token"};
    inline static const char SCOPES[]{"openid "
                                      "email "
                                      "https://www.googleapis.com/auth/drive.file"};
    inline static const char ICON[]{":/icons_new/google.png"};
    inline static const char CLIENT_ID[]{SECRET_GOOGLE_CLIENT_ID};
    inline static const char CLIENT_SECRET[]{SECRET_GOOGLE_CLIENT_SECRET};
};

#endif // GOOGLEHANDLER_H
