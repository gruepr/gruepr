#include "peerReviewSampleQuestionsDialog.h"
#include "gruepr_globals.h"
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QToolTip>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog showing sample peer review questions, each of which can be added to the peer review
/////////////////////////////////////////////////////////////////////////////////////////////////////////

PeerReviewSampleQuestionsDialog::PeerReviewSampleQuestionsDialog(const bool writtenQuestions, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(writtenQuestions? tr("Sample written questions") : tr("Sample rating and choice questions"));
    setStyleSheet(QString(WHITEDIALOGSTYLE) + SCROLLBARSTYLE);
    setSizeGripEnabled(true);
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);

    auto *layout = new QVBoxLayout(this);
    auto *topLabel = new QLabel(tr("Add any of these questions, then edit it however you like."), this);
    topLabel->setStyleSheet(LABEL10PTSTYLE);
    topLabel->setWordWrap(true);
    layout->addWidget(topLabel);

    auto *samplesWidget = new QWidget;
    auto *samplesLayout = new QVBoxLayout(samplesWidget);
    samplesLayout->setSpacing(10);
    const QList<PeerReviewQuestion> samples = PeerReview::sampleQuestions();
    for(const auto &sample : samples) {
        if((sample.kind == PeerReviewQuestion::Kind::written) != writtenQuestions) {
            continue;
        }
        auto *card = new QFrame;
        card->setStyleSheet(BLUEFRAME);
        auto *cardLayout = new QGridLayout(card);
        auto *text = new QLabel(sample.text, card);
        text->setStyleSheet(LABEL12PTSTYLE);
        text->setWordWrap(true);
        auto *details = new QLabel(summary(sample), card);
        details->setStyleSheet(LABEL10PTSTYLE);
        details->setWordWrap(true);
        auto *addButton = new QPushButton(tr("Add"), card);
        addButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        addButton->setIcon(QIcon(":/icons_new/addButton.png"));
        connect(addButton, &QPushButton::clicked, this, [this, sample, addButton](){
            emit addQuestionRequested(sample);
            QToolTip::showText(addButton->mapToGlobal(QPoint(0, 0)), tr("Question added"), addButton, QRect(), UI_DISPLAY_DELAYTIME);
        });
        cardLayout->addWidget(text, 0, 0, 1, 1);
        cardLayout->addWidget(details, 1, 0, 1, 1);
        cardLayout->addWidget(addButton, 0, 1, 2, 1, Qt::AlignRight | Qt::AlignVCenter);
        cardLayout->setColumnStretch(0, 1);
        samplesLayout->addWidget(card);
    }
    samplesLayout->addStretch(1);

    auto *samplesArea = new QScrollArea(this);
    samplesArea->setWidget(samplesWidget);
    samplesArea->setWidgetResizable(true);
    samplesArea->setStyleSheet("QScrollArea{border: none;}");
    layout->addWidget(samplesArea, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonBox->button(QDialogButtonBox::Close)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

QString PeerReviewSampleQuestionsDialog::summary(const PeerReviewQuestion &question)
{
    QString text = PeerReviewQuestion::kindText(question.kind);
    if((question.kind == PeerReviewQuestion::Kind::likert) || (question.kind == PeerReviewQuestion::Kind::choice)) {
        QStringList labels = question.formLabels();
        text += " (" + labels.join(" / ") + ")";
    }
    else if(question.kind == PeerReviewQuestion::Kind::numberScale) {
        text += " (" + QString::number(question.scaleLow) + " " + tr("to") + " " + QString::number(question.scaleHigh) + ")";
    }
    else if(question.kind == PeerReviewQuestion::Kind::contribution) {
        if(question.contributionStyle == PeerReviewQuestion::ContributionStyle::points) {
            text += " (" + tr("points") + ")";
        }
        else if(question.contributionStyle == PeerReviewQuestion::ContributionStyle::dropdown) {
            text += " (" + tr("dropdown") + ")";
        }
        else {
            text += " (" + tr("grid") + ")";
        }
    }
    text += "  " + QString(BULLET) + "  " + tr("About:") + " " + PeerReviewQuestion::aboutText(question.about).toLower();
    text += "  " + QString(BULLET) + "  " + tr("Shared with:") + " " + PeerReviewQuestion::shareWithText(question.shareWith, question.about,
                                                                                                             question.showsWriterNames()).toLower();
    return text;
}
