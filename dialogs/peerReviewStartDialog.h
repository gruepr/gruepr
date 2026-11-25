#ifndef PEERREVIEWSTARTDIALOG_H
#define PEERREVIEWSTARTDIALOG_H

#include "peerReview.h"
#include <QDialog>
#include <QPushButton>
#include <QTableWidget>

/**
 * @brief The PeerReviewStartDialog class lists the saved peer reviews and offers to start a new one, start a new round of a saved one,
 * load a saved one's results, or delete a saved one.
 */
class PeerReviewStartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PeerReviewStartDialog(QWidget *parent = nullptr);
    ~PeerReviewStartDialog() override = default;
    PeerReviewStartDialog(const PeerReviewStartDialog&) = delete;
    PeerReviewStartDialog operator= (const PeerReviewStartDialog&) = delete;
    PeerReviewStartDialog(PeerReviewStartDialog&&) = delete;
    PeerReviewStartDialog& operator= (PeerReviewStartDialog&&) = delete;

    enum class Action {newPeerReview, newRound, loadResults};
    Action action = Action::newPeerReview;
    QString chosenFileName;         // the saved peer review chosen for a new round or for loading results

private:
    void refreshList();
    void updateButtons();
    void choose(const Action chosenAction, const int row = -1);     // row: the saved peer review's row in the table
    void deleteReview(const int row);
    QPushButton *rowButton(const QString &iconFileName, const QString &toolTip);   // a small icon button in a row of the table

    QTableWidget *reviewsTable = nullptr;
    QPushButton *loadResultsButton = nullptr;
    QList<PeerReview::StoredPeerReview> storedReviews;
    inline static const int NEWROUNDCOLUMN = 4;
    inline static const int DELETECOLUMN = 5;
};

#endif // PEERREVIEWSTARTDIALOG_H
