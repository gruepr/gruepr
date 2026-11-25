#ifndef PEERREVIEWSAMPLEQUESTIONSDIALOG_H
#define PEERREVIEWSAMPLEQUESTIONSDIALOG_H

#include "peerReview.h"
#include <QDialog>

/**
 * @brief The PeerReviewSampleQuestionsDialog class shows a bank of sample peer review questions, each with its type and
 * settings filled in, any of which can be added to the peer review.
 */
class PeerReviewSampleQuestionsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief PeerReviewSampleQuestionsDialog
     * @param writtenQuestions Whether to show the written sample questions (true) or the others (false).
     * @param parent The parent window.
     */
    explicit PeerReviewSampleQuestionsDialog(const bool writtenQuestions, QWidget *parent = nullptr);
    ~PeerReviewSampleQuestionsDialog() override = default;
    PeerReviewSampleQuestionsDialog(const PeerReviewSampleQuestionsDialog&) = delete;
    PeerReviewSampleQuestionsDialog operator= (const PeerReviewSampleQuestionsDialog&) = delete;
    PeerReviewSampleQuestionsDialog(PeerReviewSampleQuestionsDialog&&) = delete;
    PeerReviewSampleQuestionsDialog& operator= (PeerReviewSampleQuestionsDialog&&) = delete;

    /**
     * @brief summary A one-line description of a question's type and settings.
     */
    static QString summary(const PeerReviewQuestion &question);

signals:
    void addQuestionRequested(const PeerReviewQuestion &question);
};

#endif // PEERREVIEWSAMPLEQUESTIONSDIALOG_H
