#include "peerReviewQuestion.h"
#include "gruepr_globals.h"
#include <QGridLayout>
#include <QRadioButton>

static const char NOTESTYLE[] = "QLabel {color: " DEEPWATERHEX "; font-size: 10pt; font-family: 'DM Sans'; font-style: italic; border: none;}";

PeerReviewQuestionSettings::PeerReviewQuestionSettings(QWidget *parent)
    : QWidget{parent}
{
    auto *aboutLabel = new QLabel(tr("Asked about"), this);
    aboutLabel->setStyleSheet(LABEL10PTSTYLE);
    aboutComboBox = new StyledComboBox(this);
    aboutComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    aboutComboBox->installEventFilter(new MouseWheelBlocker(aboutComboBox));
    setAboutChoices({PeerReviewQuestion::About::eachTeammate, PeerReviewQuestion::About::eachTeammateAndSelf,
                     PeerReviewQuestion::About::self, PeerReviewQuestion::About::once});

    auto *shareLabel = new QLabel(tr("Answers shared with"), this);
    shareLabel->setStyleSheet(LABEL10PTSTYLE);
    shareComboBox = new StyledComboBox(this);
    shareComboBox->setFocusPolicy(Qt::StrongFocus);
    shareComboBox->installEventFilter(new MouseWheelBlocker(shareComboBox));

    requiredCheckBox = new QCheckBox(tr("Require an answer"), this);
    requiredCheckBox->setChecked(requiredWanted);

    connect(aboutComboBox, &QComboBox::activated, this, &PeerReviewQuestionSettings::aboutChange);
    connect(shareComboBox, &QComboBox::activated, this, &PeerReviewQuestionSettings::settingsChanged);
    connect(requiredCheckBox, &QCheckBox::clicked, this, [this](bool checked){requiredWanted = checked; emit settingsChanged();});

    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    int row = 0;
    layout->addWidget(aboutLabel, row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(shareLabel, row++, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(aboutComboBox, row, 0, 1, 1, Qt::AlignVCenter);
    layout->addWidget(shareComboBox, row++, 1, 1, 1, Qt::AlignVCenter);
    layout->addWidget(requiredCheckBox, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);

    aboutChange();
}

void PeerReviewQuestionSettings::setAboutChoices(const QList<PeerReviewQuestion::About> &choices)
{
    const QVariant currentAbout = aboutComboBox->currentData();
    const bool currentValue = aboutComboBox->blockSignals(true);
    aboutComboBox->clear();
    for(const auto about : choices) {
        aboutComboBox->addItem(PeerReviewQuestion::aboutText(about), static_cast<int>(about));
    }
    const int index = aboutComboBox->findData(currentAbout);
    aboutComboBox->setCurrentIndex(std::max(0, index));
    aboutComboBox->setEnabled(choices.size() > 1);
    aboutComboBox->blockSignals(currentValue);
    if(shareComboBox != nullptr) {
        aboutChange();
    }
}

void PeerReviewQuestionSettings::aboutChange()
{
    // offer only the sharing choices that suit who the question is about, keeping the current choice if it's still offered
    const auto about = static_cast<PeerReviewQuestion::About>(aboutComboBox->currentData().toInt());
    const QVariant currentShare = shareComboBox->currentData();
    const bool currentValue = shareComboBox->blockSignals(true);
    shareComboBox->clear();
    for(const auto &sharingChoice : PeerReviewQuestion::sharingChoices(about)) {
        shareComboBox->addItem(PeerReviewQuestion::shareWithText(sharingChoice.first, about, sharingChoice.second),
                               sharingCode(sharingChoice.first, sharingChoice.second));
    }
    const int index = shareComboBox->findData(currentShare);
    shareComboBox->setCurrentIndex(std::max(0, index));
    shareComboBox->blockSignals(currentValue);

    updateRequiredCheckBox();
    emit settingsChanged();
}

void PeerReviewQuestionSettings::updateRequiredCheckBox()
{
    const auto about = static_cast<PeerReviewQuestion::About>(aboutComboBox->currentData().toInt());
    const bool currentValue = requiredCheckBox->blockSignals(true);
    if(about == PeerReviewQuestion::About::eachTeammate) {
        requiredCheckBox->setChecked(false);
        requiredCheckBox->setEnabled(false);
        requiredCheckBox->setToolTip(tr("Everyone on a team answers on the same page of the form, so each student skips the line with their own name.\n"
                                        "That's why an answer about each teammate can't be required."));
    }
    else {
        requiredCheckBox->setChecked(requiredWanted);
        requiredCheckBox->setEnabled(true);
        requiredCheckBox->setToolTip("");
    }
    requiredCheckBox->blockSignals(currentValue);
}

void PeerReviewQuestionSettings::setSettings(const PeerReviewQuestion &question)
{
    const bool currentValue = blockSignals(true);
    const int aboutIndex = aboutComboBox->findData(static_cast<int>(question.about));
    aboutComboBox->setCurrentIndex(std::max(0, aboutIndex));
    aboutChange();
    const int shareIndex = shareComboBox->findData(sharingCode(question.shareWith, question.showsWriterNames()));
    shareComboBox->setCurrentIndex(std::max(0, shareIndex));
    requiredWanted = question.required;
    updateRequiredCheckBox();
    blockSignals(currentValue);
}

void PeerReviewQuestionSettings::getSettings(PeerReviewQuestion &question) const
{
    question.about = static_cast<PeerReviewQuestion::About>(aboutComboBox->currentData().toInt());
    const int code = shareComboBox->currentData().toInt();
    question.shareWith = static_cast<PeerReviewQuestion::ShareWith>(code / 2);
    question.namesShown = ((code % 2) == 1);
    question.required = requiredWanted;
}

int PeerReviewQuestionSettings::sharingCode(const PeerReviewQuestion::ShareWith shareWith, const bool namesShown)
{
    return (static_cast<int>(shareWith) * 2) + (namesShown? 1 : 0);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PeerReviewRatedQuestion::PeerReviewRatedQuestion(int questionNum, QWidget *parent)
    : SurveyMakerAttributeQuestion(questionNum, parent)
{
    // the response options: no "free response number" (here that's its own type of question), and the peer review scales offered first
    for(int i = 0; i < responsesComboBox->count(); i++) {
        if(responsesComboBox->itemData(i).toStringList() == QStringList({FREERESPONSENUMBER})) {
            responsesComboBox->removeItem(i);
            numOfResponseOptions--;
            break;
        }
    }
    const QStringList peerReviewOptions = QString(PEERREVIEW_RESPONSE_OPTIONS).split(';');
    int item = 2;   // after "Choose an option..." and the separator
    for(const auto &peerReviewOption : peerReviewOptions) {
        responsesComboBox->insertItem(item, peerReviewOption, peerReviewOption.split(" / "));
        responsesComboBox->setItemData(item, peerReviewOption, Qt::ToolTipRole);
        item++;
    }
    numOfResponseOptions += int(peerReviewOptions.size());
    responsesLabel->setText(tr("Response options"));

    kindLabel = new QLabel(tr("Question type"), this);
    kindLabel->setStyleSheet(LABEL10PTSTYLE);
    kindComboBox = new StyledComboBox(this);
    kindComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    kindComboBox->installEventFilter(new MouseWheelBlocker(kindComboBox));
    for(const auto kind : kinds) {
        kindComboBox->addItem(PeerReviewQuestion::kindText(kind), static_cast<int>(kind));
    }
    kindComboBox->setItemData(int(kinds.indexOf(PeerReviewQuestion::Kind::likert)), tr("Ordered words, such as Always ... Never; scored"), Qt::ToolTipRole);
    kindComboBox->setItemData(int(kinds.indexOf(PeerReviewQuestion::Kind::numberScale)), tr("Numbers from low to high, such as 1 to 10; scored"), Qt::ToolTipRole);
    kindComboBox->setItemData(int(kinds.indexOf(PeerReviewQuestion::Kind::contribution)),
                              tr("How much of the work each person did, compared to an equal share"), Qt::ToolTipRole);
    kindComboBox->setItemData(int(kinds.indexOf(PeerReviewQuestion::Kind::choice)), tr("Options that aren't scored, such as roles; counted"), Qt::ToolTipRole);
    kindComboBox->setItemData(int(kinds.indexOf(PeerReviewQuestion::Kind::numberEntry)), tr("A number typed in, such as hours worked; averaged"), Qt::ToolTipRole);

    bestFirstCheckBox = new QCheckBox(tr("The first option is the best"), this);
    bestFirstCheckBox->setChecked(true);
    bestFirstCheckBox->setToolTip(tr("All of gruepr's built-in scales list the best option first. Uncheck this if the last option is the best."));

    scaleWidget = new QWidget(this);
    scaleWidget->setStyleSheet("QWidget{border-style:none;}");
    auto *scaleLayout = new QGridLayout(scaleWidget);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    auto *fromLabel = new QLabel(tr("From"), scaleWidget);
    fromLabel->setStyleSheet(LABEL10PTSTYLE);
    scaleLowSpinBox = new QSpinBox(scaleWidget);
    scaleLowSpinBox->setStyleSheet(SPINBOXSTYLE);
    scaleLowSpinBox->setRange(0, 1);            // the range allowed in a Google Forms linear scale
    scaleLowSpinBox->setValue(1);
    lowLabelLineEdit = new QLineEdit(scaleWidget);
    lowLabelLineEdit->setStyleSheet(LINEEDITSTYLE);
    lowLabelLineEdit->setPlaceholderText(tr("Label (optional), e.g., Poor"));
    auto *toLabel = new QLabel(tr("To"), scaleWidget);
    toLabel->setStyleSheet(LABEL10PTSTYLE);
    scaleHighSpinBox = new QSpinBox(scaleWidget);
    scaleHighSpinBox->setStyleSheet(SPINBOXSTYLE);
    scaleHighSpinBox->setRange(2, 10);
    scaleHighSpinBox->setValue(5);
    highLabelLineEdit = new QLineEdit(scaleWidget);
    highLabelLineEdit->setStyleSheet(LINEEDITSTYLE);
    highLabelLineEdit->setPlaceholderText(tr("Label (optional), e.g., Excellent"));
    scaleLayout->addWidget(fromLabel, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    scaleLayout->addWidget(scaleLowSpinBox, 0, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    scaleLayout->addWidget(lowLabelLineEdit, 0, 2, 1, 1, Qt::AlignVCenter);
    scaleLayout->addWidget(toLabel, 1, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    scaleLayout->addWidget(scaleHighSpinBox, 1, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    scaleLayout->addWidget(highLabelLineEdit, 1, 2, 1, 1, Qt::AlignVCenter);
    scaleLayout->setColumnStretch(2, 1);

    contributionStyleLabel = new QLabel(tr("Students answer with"), this);
    contributionStyleLabel->setStyleSheet(LABEL10PTSTYLE);
    contributionStyleComboBox = new StyledComboBox(this);
    contributionStyleComboBox->setFocusPolicy(Qt::StrongFocus);
    contributionStyleComboBox->installEventFilter(new MouseWheelBlocker(contributionStyleComboBox));
    contributionStyleComboBox->addItem(tr("A grid with a row for each person, from -100% to +100% of an equal share (-50%, -25%, -10%, ... +100%)"),
                                       static_cast<int>(PeerReviewQuestion::ContributionStyle::grid));
    contributionStyleComboBox->addItem(tr("A dropdown for each person, in 10% steps (e.g., \"Equal share + 10%\")"),
                                       static_cast<int>(PeerReviewQuestion::ContributionStyle::dropdown));
    contributionStyleComboBox->addItem(tr("Points for each person (100 = an equal share)"),
                                       static_cast<int>(PeerReviewQuestion::ContributionStyle::points));

    settings = new PeerReviewQuestionSettings(this);

    // put the question type above the response options, and the settings for each type below them
    auto *grid = qobject_cast<QGridLayout *>(layout());
    grid->removeWidget(responsesLabel);
    grid->removeWidget(responsesComboBox);
    grid->removeWidget(multiAllowed);
    int row = 5;
    grid->addWidget(kindLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(kindComboBox, row++, 0, 1, -1, Qt::AlignVCenter);
    grid->setRowMinimumHeight(row++, 10);
    grid->addWidget(responsesLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(responsesComboBox, row++, 0, 1, -1, Qt::AlignVCenter);
    grid->addWidget(multiAllowed, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(bestFirstCheckBox, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(scaleWidget, row++, 0, 1, -1, Qt::AlignVCenter);
    grid->addWidget(contributionStyleLabel, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(contributionStyleComboBox, row++, 0, 1, -1, Qt::AlignVCenter);
    grid->setRowMinimumHeight(row++, 10);
    grid->addWidget(settings, row++, 0, 1, -1, Qt::AlignVCenter);

    // any change updates the preview (the base question already does so for changes to the response options) and is signaled
    auto changed = [this]() {
        if(settingQuestion) {
            return;
        }
        updatePreviewWidget();
        emit peerReviewQuestionChanged();
    };
    connect(kindComboBox, &QComboBox::activated, this, &PeerReviewRatedQuestion::kindChange);
    connect(this, &SurveyMakerAttributeQuestion::questionChanged, this, [this](){if(!settingQuestion) {emit peerReviewQuestionChanged();}});
    connect(this, &SurveyMakerAttributeQuestion::responsesChanged, this, [this](){if(!settingQuestion) {emit peerReviewQuestionChanged();}});
    connect(this, &SurveyMakerAttributeQuestion::multiChanged, this, [this](){if(!settingQuestion) {emit peerReviewQuestionChanged();}});
    connect(bestFirstCheckBox, &QCheckBox::toggled, this, changed);
    connect(scaleLowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, changed);
    connect(scaleHighSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, changed);
    connect(lowLabelLineEdit, &QLineEdit::textChanged, this, changed);
    connect(highLabelLineEdit, &QLineEdit::textChanged, this, changed);
    connect(contributionStyleComboBox, &QComboBox::activated, this, changed);
    connect(settings, &PeerReviewQuestionSettings::settingsChanged, this, changed);

    clear();
}

PeerReviewQuestion::Kind PeerReviewRatedQuestion::currentKind() const
{
    return static_cast<PeerReviewQuestion::Kind>(kindComboBox->currentData().toInt());
}

void PeerReviewRatedQuestion::kindChange()
{
    const auto kind = currentKind();
    settingQuestion = true;
    if((kind == PeerReviewQuestion::Kind::likert) && (getResponses() == QStringList({""}))) {
        setResponses(QString(PEERREVIEW_RESPONSE_OPTIONS).split(';').constFirst().split(" / "));
    }
    if(kind == PeerReviewQuestion::Kind::contribution) {
        settings->setAboutChoices({PeerReviewQuestion::About::eachTeammateAndSelf});
    }
    else {
        settings->setAboutChoices({PeerReviewQuestion::About::eachTeammate, PeerReviewQuestion::About::eachTeammateAndSelf,
                                   PeerReviewQuestion::About::self, PeerReviewQuestion::About::once});
    }
    settingQuestion = false;
    updateVisibleSettings();
    updatePreviewWidget();
    emit peerReviewQuestionChanged();
}

void PeerReviewRatedQuestion::updateVisibleSettings()
{
    const auto kind = currentKind();
    const bool hasOptions = (kind == PeerReviewQuestion::Kind::likert) || (kind == PeerReviewQuestion::Kind::choice);
    responsesLabel->setVisible(hasOptions);
    responsesComboBox->setVisible(hasOptions);
    multiAllowed->setVisible(kind == PeerReviewQuestion::Kind::choice);
    bestFirstCheckBox->setVisible(kind == PeerReviewQuestion::Kind::likert);
    scaleWidget->setVisible(kind == PeerReviewQuestion::Kind::numberScale);
    contributionStyleLabel->setVisible(kind == PeerReviewQuestion::Kind::contribution);
    contributionStyleComboBox->setVisible(kind == PeerReviewQuestion::Kind::contribution);
}

void PeerReviewRatedQuestion::setPeerReviewQuestion(const PeerReviewQuestion &question)
{
    settingQuestion = true;
    setQuestion(question.text);
    kindComboBox->setCurrentIndex(std::max(0, kindComboBox->findData(static_cast<int>(question.kind))));
    setResponses(question.options.isEmpty()? QStringList({""}) : question.options);
    setMulti(question.multi);
    bestFirstCheckBox->setChecked(question.bestFirst);
    scaleLowSpinBox->setValue(question.scaleLow);
    scaleHighSpinBox->setValue(question.scaleHigh);
    lowLabelLineEdit->setText(question.lowLabel);
    highLabelLineEdit->setText(question.highLabel);
    contributionStyleComboBox->setCurrentIndex(std::max(0, contributionStyleComboBox->findData(static_cast<int>(question.contributionStyle))));
    if(question.kind == PeerReviewQuestion::Kind::contribution) {
        settings->setAboutChoices({PeerReviewQuestion::About::eachTeammateAndSelf});
    }
    else {
        settings->setAboutChoices({PeerReviewQuestion::About::eachTeammate, PeerReviewQuestion::About::eachTeammateAndSelf,
                                   PeerReviewQuestion::About::self, PeerReviewQuestion::About::once});
    }
    settings->setSettings(question);
    settingQuestion = false;

    updateVisibleSettings();
    updatePreviewWidget();
    emit peerReviewQuestionChanged();
}

PeerReviewQuestion PeerReviewRatedQuestion::getPeerReviewQuestion() const
{
    PeerReviewQuestion question;
    question.kind = currentKind();
    question.text = getQuestion().trimmed();
    if((question.kind == PeerReviewQuestion::Kind::likert) || (question.kind == PeerReviewQuestion::Kind::choice)) {
        const QStringList responses = getResponses();
        if(responses != QStringList({""})) {
            question.options = responses;
        }
    }
    question.multi = (question.kind == PeerReviewQuestion::Kind::choice) && getMulti();
    question.bestFirst = bestFirstCheckBox->isChecked();
    question.scaleLow = scaleLowSpinBox->value();
    question.scaleHigh = scaleHighSpinBox->value();
    question.lowLabel = lowLabelLineEdit->text().trimmed();
    question.highLabel = highLabelLineEdit->text().trimmed();
    question.contributionStyle = static_cast<PeerReviewQuestion::ContributionStyle>(contributionStyleComboBox->currentData().toInt());
    settings->getSettings(question);
    return question;
}

void PeerReviewRatedQuestion::clear()
{
    // a new question is a rating scale about each teammate and yourself, shared with the person it's about
    PeerReviewQuestion question;
    question.options = QString(PEERREVIEW_RESPONSE_OPTIONS).split(';').constFirst().split(" / ");
    setPeerReviewQuestion(question);
}

void PeerReviewRatedQuestion::setPreviewNames(const QStringList &names)
{
    previewNames = names;
    updatePreviewWidget();
}

void PeerReviewRatedQuestion::updatePreviewWidget()
{
    previewWidget->setUpdatesEnabled(false);
    grueprGlobal::clearLayout(previewLayout);
    PeerReviewPreview::fillPreview(previewLayout, getPeerReviewQuestion(), previewNames);
    previewWidget->setUpdatesEnabled(true);
    previewWidget->update();
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PeerReviewWrittenQuestion::PeerReviewWrittenQuestion(int questionNum, QWidget *parent)
    : SurveyMakerFreeResponseQuestion(questionNum, parent)
{
    settings = new PeerReviewQuestionSettings(this);
    auto *grid = qobject_cast<QGridLayout *>(layout());
    grid->addWidget(settings, 5, 0, 1, -1, Qt::AlignVCenter);

    connect(this, &SurveyMakerFreeResponseQuestion::questionChanged, this, [this](){if(!settingQuestion) {emit peerReviewQuestionChanged();}});
    connect(settings, &PeerReviewQuestionSettings::settingsChanged, this, [this]() {
        if(settingQuestion) {
            return;
        }
        updatePreviewWidget();
        emit peerReviewQuestionChanged();
    });

    clear();
}

void PeerReviewWrittenQuestion::setPeerReviewQuestion(const PeerReviewQuestion &question)
{
    settingQuestion = true;
    setQuestion(question.text);
    settings->setSettings(question);
    settingQuestion = false;

    updatePreviewWidget();
    emit peerReviewQuestionChanged();
}

PeerReviewQuestion PeerReviewWrittenQuestion::getPeerReviewQuestion() const
{
    PeerReviewQuestion question;
    question.kind = PeerReviewQuestion::Kind::written;
    question.text = getQuestion().trimmed();
    settings->getSettings(question);
    return question;
}

void PeerReviewWrittenQuestion::clear()
{
    // a new written question is about each teammate, shared with the person it's about, with no answer required
    PeerReviewQuestion question;
    question.kind = PeerReviewQuestion::Kind::written;
    question.about = PeerReviewQuestion::About::eachTeammate;
    question.shareWith = PeerReviewQuestion::ShareWith::personItsAbout;
    question.required = false;
    setPeerReviewQuestion(question);
}

void PeerReviewWrittenQuestion::setPreviewNames(const QStringList &names)
{
    previewNames = names;
    updatePreviewWidget();
}

void PeerReviewWrittenQuestion::updatePreviewWidget()
{
    previewWidget->setUpdatesEnabled(false);
    grueprGlobal::clearLayout(previewLayout);
    PeerReviewPreview::fillPreview(previewLayout, getPeerReviewQuestion(), previewNames);
    previewWidget->setUpdatesEnabled(true);
    previewWidget->update();
}


//////////////////////////////////////////////////////////////////////////////////////////////////


void PeerReviewPreview::fillPreview(QVBoxLayout *previewLayout, const PeerReviewQuestion &question, const QStringList &names)
{
    const QStringList rowNames = names.isEmpty()? QStringList({QObject::tr("Teammate 1"), QObject::tr("Teammate 2"), QObject::tr("Teammate 3")}) : names;
    const QStringList labels = question.formLabels();
    const bool requiredInForm = question.isRequiredInForm();

    auto *note = new QLabel(question.formDescription() + (requiredInForm? ("\n* " + QObject::tr("Required")) : ""));
    note->setStyleSheet(NOTESTYLE);
    note->setWordWrap(true);
    previewLayout->addWidget(note);
    previewLayout->addSpacing(4);

    if(question.isAboutTeammates() && question.usesGrid()) {
        // a grid: one row per team member, one column per option
        auto *grid = new QWidget;
        auto *gridLayout = new QGridLayout(grid);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setSpacing(4);
        for(int col = 0; col < labels.size(); col++) {
            auto *header = new QLabel(labels.at(col));
            header->setStyleSheet(LABEL10PTSTYLE);
            header->setWordWrap(true);
            header->setAlignment(Qt::AlignCenter);
            gridLayout->addWidget(header, 0, col + 1, 1, 1, Qt::AlignCenter | Qt::AlignBottom);
        }
        for(int row = 0; row < rowNames.size(); row++) {
            auto *name = new QLabel(rowNames.at(row));
            name->setStyleSheet(LABEL10PTSTYLE);
            gridLayout->addWidget(name, row + 1, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
            for(int col = 0; col < labels.size(); col++) {
                if(question.multi) {
                    gridLayout->addWidget(new QCheckBox, row + 1, col + 1, 1, 1, Qt::AlignCenter);
                }
                else {
                    gridLayout->addWidget(new QRadioButton, row + 1, col + 1, 1, 1, Qt::AlignCenter);
                }
            }
        }
        previewLayout->addWidget(grid);
    }
    else if(question.isAboutTeammates()) {
        // a separate question for each team member
        for(const auto &rowName : rowNames) {
            auto *name = new QLabel(rowName);
            name->setStyleSheet(LABEL10PTSTYLE);
            previewLayout->addWidget(name);
            if(!labels.isEmpty() && !question.multi) {
                auto *dropdown = new StyledComboBox;
                dropdown->addItems(labels);
                dropdown->setPlaceholderText(QObject::tr("Choose"));
                dropdown->setCurrentIndex(-1);
                previewLayout->addWidget(dropdown);
            }
            else if(!labels.isEmpty()) {
                for(const auto &label : labels) {
                    previewLayout->addWidget(new QCheckBox(label));
                }
            }
            else {
                auto *answer = new QLineEdit;
                answer->setStyleSheet(LINEEDITSTYLE);
                answer->setPlaceholderText((question.kind == PeerReviewQuestion::Kind::written)? QObject::tr("Your answer") : QObject::tr("Enter a number"));
                answer->setReadOnly(true);
                previewLayout->addWidget(answer);
            }
            previewLayout->addSpacing(4);
        }
    }
    else if(question.kind == PeerReviewQuestion::Kind::numberScale) {
        // a linear scale
        auto *scale = new QWidget;
        auto *scaleLayout = new QHBoxLayout(scale);
        scaleLayout->setContentsMargins(0, 0, 0, 0);
        if(!question.lowLabel.isEmpty()) {
            auto *lowLabel = new QLabel(question.lowLabel);
            lowLabel->setStyleSheet(LABEL10PTSTYLE);
            scaleLayout->addWidget(lowLabel);
        }
        for(int value = question.scaleLow; value <= question.scaleHigh; value++) {
            scaleLayout->addWidget(new QRadioButton(QString::number(value)));
        }
        if(!question.highLabel.isEmpty()) {
            auto *highLabel = new QLabel(question.highLabel);
            highLabel->setStyleSheet(LABEL10PTSTYLE);
            scaleLayout->addWidget(highLabel);
        }
        scaleLayout->addStretch(1);
        previewLayout->addWidget(scale);
    }
    else if(!labels.isEmpty()) {
        // a single multiple choice question
        if(question.multi) {
            for(const auto &label : labels) {
                previewLayout->addWidget(new QCheckBox(label));
            }
        }
        else {
            for(const auto &label : labels) {
                previewLayout->addWidget(new QRadioButton(label));
            }
        }
    }
    else {
        auto *answer = new QLineEdit;
        answer->setStyleSheet(LINEEDITSTYLE);
        answer->setPlaceholderText((question.kind == PeerReviewQuestion::Kind::written)? QObject::tr("Your answer") : QObject::tr("Enter a number"));
        answer->setReadOnly(true);
        previewLayout->addWidget(answer);
    }
}
