#ifndef REMINDERDIALOG_H
#define REMINDERDIALOG_H

#include "gruepr_globals.h"
#include "studentRecord.h"
#include <QDialog>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSet>
#include <QTableWidget>

class CanvasHandler;

/**
 * @brief The ReminderDialog class reminds the students who haven't responded to a survey or a peer review: by copying their email
 * addresses, by saving a mail merge file with each student's own message, or by sending each student a private message in Canvas.
 */
class ReminderDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief The Recipient struct is one student who hasn't responded.
     */
    struct Recipient
    {
        long long ID = -1;                  // the caller's own ID for the student (e.g., in a peer review), to report who was sent a message
        QString firstname;
        QString lastname;
        QString email;
        QString team;                       // empty if there are no teams
        QString link;                       // where this student responds
        long long canvasID = -1;            // the student's Canvas ID, if known

        QString fullName() const {return (firstname + " " + lastname).simplified();}
    };

    /**
     * @brief ReminderDialog
     * @param title The title of the survey or peer review.
     * @param kind What it is, as it's named in the message (e.g., "survey" or "peer review").
     * @param recipients The students who haven't responded.
     * @param canvasCourseID The Canvas course whose students the recipients are, to send messages in (-1 if not known: the course is asked
     * for when messages are sent, and the students are matched to its roster).
     * @param canvas A connection to Canvas that is already signed in, to use rather than signing in again (or nullptr).
     * @param parent The parent window.
     */
    ReminderDialog(const QString &title, const QString &kind, const QList<Recipient> &recipients, const long long canvasCourseID = -1,
                   CanvasHandler *canvas = nullptr, QWidget *parent = nullptr);
    ~ReminderDialog() override = default;
    ReminderDialog(const ReminderDialog&) = delete;
    ReminderDialog operator= (const ReminderDialog&) = delete;
    ReminderDialog(ReminderDialog&&) = delete;
    ReminderDialog& operator= (ReminderDialog&&) = delete;

    /**
     * @brief messagedIDs The students (by the IDs they were given) who were sent a Canvas message.
     */
    QList<long long> messagedIDs() const {return messaged;}

    /**
     * @brief setNote Shows a note under the summary at the top (e.g., that some responses couldn't be matched to the roster).
     */
    void setNote(const QString &note);

    /**
     * @brief remindAboutSurvey Asks which survey made by gruepr's form builder (a Canvas quiz, or a Google Form) and who should respond to it,
     * works out who hasn't responded, and opens a ReminderDialog for them.
     * @param parent The window to open on top of.
     */
    static void remindAboutSurvey(QWidget *parent);

    /**
     * @brief infoMessage Shows a message about something that went fine (e.g., everyone has responded), with an OK button.
     */
    static void infoMessage(QWidget *parent, const QString &message);

private:
    void copyEmails();
    void saveMailMerge();
    void sendCanvasMessages();
    bool chooseCanvasCourse();      // asks for the course to send messages in, and matches the students to its roster
    CanvasHandler *connectToCanvas();
    void refreshTable();
    QList<int> checkedRows() const;
    bool confirmMissingLinks(const QList<int> &rows);     // if the message has a [link] that would be blank for any of these students, asks whether to go on
    QString personalized(const QString &text, const Recipient &recipient) const;    // the text, with [first name], [team], [link], and [title] filled in

    static void remindAboutCanvasSurvey(QWidget *parent);
    static void remindAboutGoogleSurvey(QWidget *parent);
    static long long chooseCanvasCourse(CanvasHandler *canvas, const QString &prompt, QWidget *parent, QString *courseName = nullptr);
    static int chooseFromList(const QString &windowTitle, const QString &prompt, const QStringList &items, const int selected, QWidget *parent);
    static QList<StudentRecord> loadCanvasRoster(CanvasHandler *canvas, const long long courseID, QWidget *parent);

    const QString title;
    const QString kind;
    QList<Recipient> recipients;
    QSet<int> sentRows;                     // the rows of the students who were sent a Canvas message here
    long long canvasCourseID = -1;
    CanvasHandler *canvas = nullptr;        // (made, as a child of this dialog, if not given)
    QList<long long> messaged;
    bool hasTeams = false;

    QLabel *summaryLabel = nullptr;
    QLabel *noteLabel = nullptr;
    QTableWidget *studentsTable = nullptr;
    QLineEdit *subjectLineEdit = nullptr;
    QPlainTextEdit *messageTextEdit = nullptr;

    enum Column {nameColumn, teamColumn, emailColumn, canvasColumn};

    inline static const char TABLESTYLE[] = "QTableView{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: " DEEPWATERHEX ";}"
                                            "QTableView::item:selected{background-color: " OPENWATERHEX "; color: white;}"
                                            "QTableView::indicator {background-color: white; width: 12px; height: 12px; border: 2px solid " DEEPWATERHEX ";}"
                                            "QTableView::indicator:checked {background-color: white; image: url(:/icons_new/Checkmark.png);}"
                                            "QHeaderView::section{background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; "
                                                                 "font-size: 10pt; padding: 4px; border: none;}";
};

#endif // REMINDERDIALOG_H
