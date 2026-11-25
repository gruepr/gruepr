#ifndef FEEDBACKRELEASEDIALOG_H
#define FEEDBACKRELEASEDIALOG_H

#include "peerReview.h"
#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QPdfWriter>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBrowser>

/**
 * @brief The FeedbackReleaseDialog class is where the instructor provides feedback to students: previewing each student's report
 * as they will see it, adding a personal note, holding back a student's report, and then saving the reports as PDF files or as a
 * mail merge file, sending them as Google Docs shared with each student, or (for a peer review run as Canvas quizzes) posting each one
 * as a comment on the student's team quiz. (Teammates' comments are edited in the results window.)
 */
class FeedbackReleaseDialog : public QDialog
{
    Q_OBJECT

public:
    FeedbackReleaseDialog(PeerReview &peerReview, const PeerReviewResults &results, QWidget *parent = nullptr);
    ~FeedbackReleaseDialog() override = default;
    FeedbackReleaseDialog(const FeedbackReleaseDialog&) = delete;
    FeedbackReleaseDialog operator= (const FeedbackReleaseDialog&) = delete;
    FeedbackReleaseDialog(FeedbackReleaseDialog&&) = delete;
    FeedbackReleaseDialog& operator= (FeedbackReleaseDialog&&) = delete;

    /**
     * @brief framed Puts a text box inside a white frame with a border, so it clearly looks like a box to type in.
     * @return The frame, to be added to a layout in place of the text box.
     */
    static QFrame *framed(QPlainTextEdit *textEdit, const int height, QWidget *parent);

private:
    void refreshStudentList();
    void refreshCurrentStudent();
    void refreshPreview();
    void savePDFs();
    void saveMailMerge();
    void shareGoogleDocs();

    /**
     * @brief postCanvasComments Posts each checked student's report as a comment on their team quiz in Canvas, in plain text with a PDF of
     * the report attached. A student who already has their report as a comment gets the current report in a new comment, and the
     * earlier one is deleted, since Canvas can't change a comment's attachment.
     */
    void postCanvasComments();

    static void printReport(const QString &html, QPdfWriter &writer);     // lays out a report on letter-size pages, as in the saved PDF files

    enum class Method {pdf, mailMerge, googleDoc, canvasComment};
    static QString methodName(const Method method);         // the name of a way of saving or sending the reports, as recorded in the history
    static QString doneText(const QString &method);         // what was done, as shown in the students table (e.g., "PDF saved" for "PDF")

    long long currentStudentID() const;
    QList<long long> studentsToDeliver() const;
    bool confirmDelivery(const QList<long long> &studentIDs, const Method method);
    void logDelivery(const long long studentID, const QString &method, const QString &detail);

    PeerReview &peerReview;
    const PeerReviewResults &results;

    QPlainTextEdit *introTextEdit = nullptr;
    QSpinBox *minimumSpinBox = nullptr;
    QCheckBox *teamAverageCheckBox = nullptr;
    QCheckBox *factorCheckBox = nullptr;
    QTableWidget *studentsTable = nullptr;
    QTextBrowser *reportPreview = nullptr;
    QPlainTextEdit *noteTextEdit = nullptr;
    QCheckBox *overrideCheckBox = nullptr;
    bool refreshing = false;                        // true while the table and the note are being filled, so their changes aren't taken as edits

    inline static const char TABLESTYLE[] = "QTableView{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: " DEEPWATERHEX ";}"
                                            "QTableView::item:selected{background-color: " OPENWATERHEX "; color: white;}"
                                            "QTableView::indicator {background-color: white; width: 12px; height: 12px; border: 2px solid " DEEPWATERHEX ";}"
                                            "QTableView::indicator:checked {background-color: white; image: url(:/icons_new/Checkmark.png);}"
                                            "QHeaderView::section{background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; "
                                                                 "font-size: 10pt; padding: 4px; border: none;}";
    inline static const char TEXTBOXFRAMESTYLE[] = "QFrame#textBoxFrame {background-color: white; border: 1px solid " DEEPWATERHEX "; border-radius: 3px;}";
    inline static const char TEXTBOXSTYLE[] = "QPlainTextEdit {background-color: white; color: " DEEPWATERHEX "; border: none; "
                                              "font-family: 'DM Sans'; font-size: 12pt;}";
};

#endif // FEEDBACKRELEASEDIALOG_H
