#ifndef PEERREVIEWQUESTION_H
#define PEERREVIEWQUESTION_H

// the editors for the questions in the peer review wizard, built on the survey maker's questions

#include "peerReview.h"
#include "widgets/styledComboBox.h"
#include "widgets/surveyMakerQuestion.h"
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

/**
 * @brief The PeerReviewQuestionSettings class is the part of a peer review question's editor that sets who it is about,
 * who will see its answers, and whether an answer is required.
 */
class PeerReviewQuestionSettings : public QWidget
{
    Q_OBJECT

public:
    explicit PeerReviewQuestionSettings(QWidget *parent = nullptr);

    /**
     * @brief setAboutChoices Sets which choices are offered for who the question is about.
     */
    void setAboutChoices(const QList<PeerReviewQuestion::About> &choices);

    /**
     * @brief setSettings Shows the about, sharing, and required settings of a question.
     */
    void setSettings(const PeerReviewQuestion &question);

    /**
     * @brief getSettings Copies the about, sharing, and required settings into a question.
     */
    void getSettings(PeerReviewQuestion &question) const;

signals:
    void settingsChanged();

private:
    void aboutChange();
    void updateRequiredCheckBox();
    static int sharingCode(const PeerReviewQuestion::ShareWith shareWith, const bool namesShown);     // the sharing combobox's data for each choice
    StyledComboBox *aboutComboBox = nullptr;
    StyledComboBox *shareComboBox = nullptr;
    QCheckBox *requiredCheckBox = nullptr;
    bool requiredWanted = true;     // remembers the choice while the question is about each teammate, when an answer can't be required
};


//////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief The PeerReviewRatedQuestion class is the editor for a peer review question that is answered by choosing a rating,
 * an option, or a number (that is, every kind of question except a written one).
 */
class PeerReviewRatedQuestion : public SurveyMakerAttributeQuestion
{
    Q_OBJECT

public:
    explicit PeerReviewRatedQuestion(int questionNum, QWidget *parent = nullptr);

    void setPeerReviewQuestion(const PeerReviewQuestion &question);
    PeerReviewQuestion getPeerReviewQuestion() const;
    void clear();

    /**
     * @brief setPreviewNames Sets the names of the team members shown in the preview of the question.
     */
    void setPreviewNames(const QStringList &names);

    inline static const QList<PeerReviewQuestion::Kind> kinds = {PeerReviewQuestion::Kind::likert, PeerReviewQuestion::Kind::numberScale,
                                                                 PeerReviewQuestion::Kind::contribution, PeerReviewQuestion::Kind::choice,
                                                                 PeerReviewQuestion::Kind::numberEntry};

signals:
    void peerReviewQuestionChanged();

protected:
    void updatePreviewWidget() override;

private:
    void kindChange();
    void updateVisibleSettings();
    PeerReviewQuestion::Kind currentKind() const;

    QLabel *kindLabel = nullptr;
    StyledComboBox *kindComboBox = nullptr;
    QCheckBox *bestFirstCheckBox = nullptr;
    QWidget *scaleWidget = nullptr;
    QSpinBox *scaleLowSpinBox = nullptr;
    QSpinBox *scaleHighSpinBox = nullptr;
    QLineEdit *lowLabelLineEdit = nullptr;
    QLineEdit *highLabelLineEdit = nullptr;
    QLabel *contributionStyleLabel = nullptr;
    StyledComboBox *contributionStyleComboBox = nullptr;
    PeerReviewQuestionSettings *settings = nullptr;
    QStringList previewNames;
    bool settingQuestion = false;       // true while setPeerReviewQuestion() is filling in the editor, so changes aren't signaled one by one
};


//////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief The PeerReviewWrittenQuestion class is the editor for a peer review question that is answered in writing.
 */
class PeerReviewWrittenQuestion : public SurveyMakerFreeResponseQuestion
{
    Q_OBJECT

public:
    explicit PeerReviewWrittenQuestion(int questionNum, QWidget *parent = nullptr);

    void setPeerReviewQuestion(const PeerReviewQuestion &question);
    PeerReviewQuestion getPeerReviewQuestion() const;
    void clear();
    void setPreviewNames(const QStringList &names);

signals:
    void peerReviewQuestionChanged();

protected:
    void updatePreviewWidget() override;

private:
    PeerReviewQuestionSettings *settings = nullptr;
    QStringList previewNames;
    bool settingQuestion = false;
};


//////////////////////////////////////////////////////////////////////////////////////////////////


namespace PeerReviewPreview {
    /**
     * @brief fillPreview Fills a layout with a preview of how a question looks in the form, for a team with the given members.
     */
    void fillPreview(QVBoxLayout *previewLayout, const PeerReviewQuestion &question, const QStringList &names);
}

#endif // PEERREVIEWQUESTION_H
