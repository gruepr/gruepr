#ifndef PEERREVIEWRESULTSDIALOG_H
#define PEERREVIEWRESULTSDIALOG_H

#include "peerReview.h"
#include "widgets/styledComboBox.h"
#include <memory>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextBrowser>

/**
 * @brief The PeerReviewResultsDialog class shows the results of a peer review: who has responded, each student's results,
 * the answers to each question, the contribution results, and the comments to be shared. From here feedback is provided to the students.
 */
class PeerReviewResultsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief PeerReviewResultsDialog
     * @param peerReview The peer review, which is saved as its settings change and as new responses are downloaded.
     * @param parent The parent window.
     */
    explicit PeerReviewResultsDialog(PeerReview &peerReview, QWidget *parent = nullptr);
    ~PeerReviewResultsDialog() override = default;
    PeerReviewResultsDialog(const PeerReviewResultsDialog&) = delete;
    PeerReviewResultsDialog operator= (const PeerReviewResultsDialog&) = delete;
    PeerReviewResultsDialog(PeerReviewResultsDialog&&) = delete;
    PeerReviewResultsDialog& operator= (PeerReviewResultsDialog&&) = delete;

private:
    void downloadResponses();
    void toggleAcceptingResponses();
    void editAdjustmentFactor();
    void exportSummarySpreadsheet();
    void provideFeedback();
    void remindStudents();

    void refreshResults();
    void refreshStatus();
    void refreshResponsesTab();
    void refreshStudentsTab();
    void refreshStudentDetails();
    void refreshQuestionTab();
    void refreshContributionTab();
    void refreshCommentsTab();
    void editComment(const int row);

    QStringList scoreLines(const int questionNum, const PeerReviewResults::StudentResult &studentResult) const;   // the teammates' average, then the self rating
    QString answerText(const PeerReviewAnswer &answer) const;
    QString ratingCell(const PeerReviewQuestion &question, const double value) const;      // a table cell with the value, tinted by where it falls on the scale
    QString studentName(const long long studentID) const;
    QList<const PeerReviewStudent *> missingStudents() const;
    static void setUpSortableHeader(QTableWidget *table);   // a header like the main window's: not bold, with icons showing the columns can be sorted
    static void updateSortIcons(QTableWidget *table);

    PeerReview &peerReview;
    std::unique_ptr<PeerReviewResults> results;

    QLabel *statusLabel = nullptr;
    QPushButton *acceptingButton = nullptr;
    QTabWidget *tabs = nullptr;
    QLabel *responsesSummaryLabel = nullptr;
    QTableWidget *responsesTable = nullptr;
    QLabel *unmatchedLabel = nullptr;
    QTableWidget *studentsTable = nullptr;
    QTextBrowser *studentDetails = nullptr;
    StyledComboBox *questionComboBox = nullptr;
    QTextBrowser *questionView = nullptr;
    QTextBrowser *contributionView = nullptr;
    int contributionTabIndex = -1;
    QTableWidget *commentsTable = nullptr;
    QList<PeerReviewAnswer> sharedComments;         // the comments in the comments table (each row holds its index in this list)
    int commentsTabIndex = -1;

    inline static const char TABLESTYLE[] = "QTableView{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: " DEEPWATERHEX ";}"
                                            "QTableView::item:selected{background-color: " OPENWATERHEX "; color: white;}"
                                            "QHeaderView::section{background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; "
                                                                 "font-size: 10pt; padding: 4px; border: none;}"
                                            "QHeaderView::down-arrow{image: url(:/icons_new/downButton_white.png); width: 12px; "
                                                                    "subcontrol-origin: padding; subcontrol-position: bottom left;}"
                                            "QHeaderView::up-arrow{image: url(:/icons_new/upButton_white.png); width: 12px; "
                                                                  "subcontrol-origin: padding; subcontrol-position: top left;}";
    inline static const char BROWSERSTYLE[] = "QTextBrowser{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: black; "
                                              "border: 1px solid lightGray;}";
};

#endif // PEERREVIEWRESULTSDIALOG_H
