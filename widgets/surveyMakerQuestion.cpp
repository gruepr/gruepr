#include "surveyMakerQuestion.h"
#include "gruepr_globals.h"
#include "dialogs/customResponseOptionsDialog.h"
#include <QCheckBox>
#include <QRadioButton>

SurveyMakerQuestionWithSwitch::SurveyMakerQuestionWithSwitch(QWidget *parent, const QString &textLabel, bool startingValue)
    : QFrame{parent}
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SurveyMakerQuestionWithSwitch::valueChange);

    label = new QLabel(this);
    setLabel(textLabel);

    setStyleSheet(QString("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX ";}") + SCROLLBARSTYLE);

    layout = new QGridLayout(this);
    layout->addWidget(label, 0, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(switchButton, 0, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    layout->setColumnStretch(0, 1);
}

void SurveyMakerQuestionWithSwitch::mousePressEvent(QMouseEvent *event)
{
    switchButton->mousePressEvent(event);
}

void SurveyMakerQuestionWithSwitch::paintEvent(QPaintEvent*)
{
    setAutoFillBackground(true);
    if (_enabled) {
        setStyleSheet(QString("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX ";}") + SCROLLBARSTYLE);
        label->setText(label->text().replace("#bebebe", DEEPWATERHEX));
    }
    else {
        setStyleSheet(QString("QFrame{background-color: #e6e6e6; color: #bebebe;}") + SCROLLBARSTYLE);
        label->setText(label->text().replace(DEEPWATERHEX, "#bebebe"));
    }
}

void SurveyMakerQuestionWithSwitch::setEnabled(bool flag)
{
    _enabled = flag;
    switchButton->setEnabled(flag);
    for(auto *widget : this->findChildren<QWidget *>()) {
        widget->setEnabled(flag);
    }
    QWidget::setEnabled(flag);
}

void SurveyMakerQuestionWithSwitch::setLabel(const QString &text)
{
    label->setText("<span style=\"color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + text + "</span>");
}

void SurveyMakerQuestionWithSwitch::valueChange(bool newvalue)
{
    emit valueChanged(newvalue);
}

void SurveyMakerQuestionWithSwitch::setValue(bool value)
{
    switchButton->setValue(value);
}

bool SurveyMakerQuestionWithSwitch::getValue() const
{
    return switchButton->value();
}

void SurveyMakerQuestionWithSwitch::addWidget(QWidget *widget, int row, int column, bool expandToRestOfRow, Qt::Alignment horizontalAlignment)
{
    if((row <= 0) || (column > 1)) {
        return;
    }
    widget->setAttribute(Qt::WA_NoMousePropagation, true);
    layout->addWidget(widget, row, column, 1, (expandToRestOfRow? -1 : 1), horizontalAlignment | Qt::AlignVCenter);
}

void SurveyMakerQuestionWithSwitch::moveWidget(QWidget *widget, int newRow, int newColumn, bool expandToRestOfRow, Qt::Alignment horizontalAlignment)
{
    if((newRow <= 0) || (newColumn > 1)) {
        return;
    }
    layout->removeWidget(widget);
    layout->addWidget(widget, newRow, newColumn, 1, (expandToRestOfRow? -1 : 1), horizontalAlignment | Qt::AlignVCenter);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


SurveyMakerMultichoiceQuestion::SurveyMakerMultichoiceQuestion(int questionNum, QWidget *parent)
    : QFrame{parent}
{
    label = new QLabel(this);
    setNumber(questionNum);
    deleteButton = new QPushButton(this);
    deleteButton->setStyleSheet(DELBUTTONSTYLE);
    deleteButton->setText(tr("Delete"));
    deleteButton->setIcon(QIcon(":/icons_new/trashButton.png"));

    auto *questionLabel = new QLabel(tr("Question"), this);
    questionLabel->setStyleSheet(LABELSTYLE);
    questionPlainTextEdit = new QPlainTextEdit;
    questionPlainTextEdit->setStyleSheet(PLAINTEXTEDITSTYLE);
    questionPlainTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    questionPlainTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    resizeQuestionPlainTextEdit();
    questionPlainTextEdit->setPlaceholderText(tr("Type your question..."));
    responsesLabel = new QLabel(tr("Response type"), this);
    responsesLabel->setStyleSheet(LABELSTYLE);
    responsesComboBox = new ComboBoxWithElidedContents("1. Very high / 2. Above average / 3. Average", this);
    responsesComboBox->setEditable(false);
    responsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    responsesComboBox->installEventFilter(new MouseWheelBlocker(responsesComboBox));
    responsesComboBox->addItem(tr("Choose an option..."), QStringList({""}));
    responsesComboBox->setItemData(0, "", Qt::ToolTipRole);
    responsesComboBox->insertSeparator(1);
    QStringList responseOptions = QString(RESPONSE_OPTIONS).split(';');
    numOfResponseOptions = responseOptions.size();
    int item = 0;
    for(const auto &responseOption : responseOptions) {
        responsesComboBox->addItem(responseOption, responseOption.split(" / "));
        responsesComboBox->setItemData(item + 2, responseOption, Qt::ToolTipRole);
        item++;
    }
    responsesComboBox->setItemData(item + 1, QStringList({""}));    // reset the "Custom options..." item data and tooltip
    responsesComboBox->setItemData(item + 1, "", Qt::ToolTipRole);
    multiAllowed = new QCheckBox(tr("Allow students to select multiple options"), this);

    connect(deleteButton, &QPushButton::clicked, this, &SurveyMakerMultichoiceQuestion::deleteRequest);
    connect(questionPlainTextEdit, &QPlainTextEdit::textChanged, this, &SurveyMakerMultichoiceQuestion::questionChange);
    connect(responsesComboBox, &QComboBox::activated, this, &SurveyMakerMultichoiceQuestion::responsesComboBoxActivated);
    connect(multiAllowed, &QCheckBox::toggled, this, &SurveyMakerMultichoiceQuestion::multiChange);

    setStyleSheet(QString() + "QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX ";}" + SCROLLBARSTYLE);

    auto *layout = new QGridLayout(this);
    int row = 0;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(deleteButton, row++, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(questionLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(questionPlainTextEdit, row++, 0, 1, -1, Qt::AlignVCenter);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(responsesLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(responsesComboBox, row++, 0, 1, -1, Qt::AlignVCenter);
    layout->addWidget(multiAllowed, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->setColumnStretch(0, 1);

    previewWidget = new QWidget(this);
    previewWidget->setStyleSheet(QString() + "QWidget{border-style:none;}" + SCROLLBARSTYLE);
    previewLayout = new QVBoxLayout;
    previewLayout->setSpacing(0);
    previewLayout->setContentsMargins(20, 0, 0, 0);
    previewWidget->setLayout(previewLayout);
}

void SurveyMakerMultichoiceQuestion::resizeEvent(QResizeEvent *event)
{
    resizeQuestionPlainTextEdit();
    QWidget::resizeEvent(event);
}

void SurveyMakerMultichoiceQuestion::setNumber(const int questionNum)
{
    label->setText("<span style=\"color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + tr("Question ") + QString::number(questionNum) + "</span>");
}

void SurveyMakerMultichoiceQuestion::deleteRequest()
{
    emit deleteRequested();
}

void SurveyMakerMultichoiceQuestion::questionChange()
{
    resizeQuestionPlainTextEdit();
    emit questionChanged(questionPlainTextEdit->toPlainText());
}

void SurveyMakerMultichoiceQuestion::setQuestion(const QString &newQuestion)
{
    questionPlainTextEdit->setPlainText(newQuestion);
}

QString SurveyMakerMultichoiceQuestion::getQuestion() const
{
    return questionPlainTextEdit->toPlainText();
}

void SurveyMakerMultichoiceQuestion::multiChange(const bool newMulti)
{
    emit multiChanged(newMulti);

    QStringList newResponses = responsesComboBox->currentData().toStringList();
    emit responsesChanged(newResponses);

    updatePreviewWidget();
}

void SurveyMakerMultichoiceQuestion::setResponses(const QStringList &newResponses)
{
    if(newResponses == QStringList({""})) {
        responsesComboBox->setCurrentIndex(0);
    }
    else {
        QString newResponsesAsString = newResponses.join(" / ");
        if(responsesComboBox->findText(newResponsesAsString, Qt::MatchFixedString) != -1) {
            responsesComboBox->setCurrentText(newResponsesAsString);
        }
        else {
            responsesComboBox->setItemText(numOfResponseOptions + 1, newResponsesAsString);
            responsesComboBox->setItemData(numOfResponseOptions + 1, newResponses);
            responsesComboBox->setItemData(numOfResponseOptions + 1, newResponsesAsString, Qt::ToolTipRole);
            responsesComboBox->setCurrentText(newResponsesAsString);

            responsesComboBox->removeItem(numOfResponseOptions + 2);
            responsesComboBox->addItem(tr("Custom options..."), QStringList({""}));
        }
    }

    emit responsesChanged(newResponses);

    updatePreviewWidget();
}

QStringList SurveyMakerMultichoiceQuestion::getResponses() const
{
    return responsesComboBox->currentData().toStringList();
}

void SurveyMakerMultichoiceQuestion::setMulti(const bool newMulti)
{
    multiAllowed->setChecked(newMulti);
}

bool SurveyMakerMultichoiceQuestion::getMulti() const
{
    return multiAllowed->isChecked();
}

void SurveyMakerMultichoiceQuestion::responsesComboBoxActivated(int index)
{
    static int prevIndex = 0;
    static QStringList currentCustomOptions;
    //see if custom options being enabled
    if(responsesComboBox->currentText() == tr("Custom options...")) {
        auto *window = new customResponseOptionsDialog(currentCustomOptions, this);

        // If user clicks OK, use these options
        int reply = window->exec();
        if(reply == QDialog::Accepted) {
            bool currentValue = responsesComboBox->blockSignals(true);
            currentCustomOptions = window->options;
            responsesComboBox->setItemText(numOfResponseOptions + 1, currentCustomOptions.join(" / "));
            responsesComboBox->setItemData(numOfResponseOptions + 1, currentCustomOptions);
            responsesComboBox->setItemData(numOfResponseOptions + 1, currentCustomOptions.join(" / "), Qt::ToolTipRole);
            prevIndex = numOfResponseOptions + 1;

            responsesComboBox->removeItem(numOfResponseOptions + 2);
            responsesComboBox->addItem(tr("Custom options..."), QStringList({""}));
            responsesComboBox->blockSignals(currentValue);
        }
        else {
            bool currentValue = responsesComboBox->blockSignals(true);
            responsesComboBox->setCurrentIndex(prevIndex);
            responsesComboBox->blockSignals(currentValue);
        }

        delete window;
    }
    else {
        prevIndex = index;
    }

    // Put list of options back to just built-ins plus "Custom options"
    if(index < numOfResponseOptions) {
        responsesComboBox->removeItem(numOfResponseOptions + 2);
        responsesComboBox->removeItem(numOfResponseOptions + 1);
        responsesComboBox->addItem(tr("Custom options..."));
    }

    QStringList newResponses = responsesComboBox->currentData().toStringList();
    emit responsesChanged(newResponses);

    updatePreviewWidget();
}

void SurveyMakerMultichoiceQuestion::resizeQuestionPlainTextEdit()
{
    // make the edit box at least 2 rows of text tall but as big as needed to show all text
    const auto *const doc = questionPlainTextEdit->document();
    const QFontMetrics font(doc->defaultFont());
    const auto margins = questionPlainTextEdit->contentsMargins();
    const int height = (font.lineSpacing() * std::max(2.0, doc->size().height())) +
                       (doc->documentMargin() + questionPlainTextEdit->frameWidth()) * 2 + margins.top() + margins.bottom();
    questionPlainTextEdit->setFixedHeight(height);
}

void SurveyMakerMultichoiceQuestion::updatePreviewWidget()
{
    previewWidget->setUpdatesEnabled(false);
    QLayoutItem *child;
    while ((child = previewLayout->takeAt(0)) != nullptr) {
        delete child->widget(); // delete the widget
        delete child;   // delete the layout item
    }

    if(responsesComboBox->currentIndex() != 0) {
        if(multiAllowed->isChecked()) {
            //multiallowed, so preview is list of checkboxes
            auto *topLabel = new QLabel(SELECTMULT);
            topLabel->setStyleSheet(LABELSTYLE);
            topLabel->setWordWrap(true);
            previewLayout->addWidget(topLabel);
            QStringList responses = responsesComboBox->currentData().toStringList();
            QList<QCheckBox*> re;
            re.reserve(responses.size());
            for(const auto &response : responses) {
                re << new QCheckBox(response);
                previewLayout->addWidget(re.last());
            }
            re.first()->setChecked(true);
        }
        else if(responsesComboBox->currentData().toStringList().size() <= 9) {
            //select one and <=15 options, so radiobuttons
            auto *topLabel = new QLabel(SELECTONE);
            topLabel->setStyleSheet(LABELSTYLE);
            topLabel->setWordWrap(true);
            previewLayout->addWidget(topLabel);
            QStringList responses = responsesComboBox->currentData().toStringList();
            QList<QRadioButton*> re;
            re.reserve(responses.size());
            for(const auto &response : responses) {
                re << new QRadioButton(response);
                previewLayout->addWidget(re.last());
            }
            re.first()->setChecked(true);
        }
        else {
            //select one and >=10 options, so dropdown with explainer label
            auto list = responsesComboBox->currentData().toStringList();
            auto *re = new QComboBox;
            re->setStyleSheet(COMBOBOXSTYLE);
            re->addItems(list);
            previewLayout->addWidget(re);
            auto *bottomLabel = new QLabel(tr("Options") + ": " + list.join(" / "));
            bottomLabel->setStyleSheet(LABELSTYLE);
            bottomLabel->setWordWrap(true);
            previewLayout->addWidget(bottomLabel);
        }
    }
    previewWidget->setUpdatesEnabled(true);
    previewWidget->update();
}


//////////////////////////////////////////////////////////////////////////////////////////////////


SurveyMakerPreviewSection::SurveyMakerPreviewSection(const int pageNum, const QString &titleText, const int numQuestions, QWidget *parent)
    : QFrame{parent}
{
    setStyleSheet(QString() + "QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX ";}" + SCROLLBARSTYLE);

    layout = new QGridLayout(this);

    title = new QLabel("<span style=\"color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size:14pt\">" + titleText + "</span>", this);
    //center the title for just the Survey Title section
    if(pageNum == 0) {
        layout->addWidget(title, row, 0, 1, 1, Qt::AlignCenter | Qt::AlignVCenter);
    }
    else {
        layout->addWidget(title, row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    }

    auto *editButton = new QPushButton(this);
    editButton->setStyleSheet(QString(DELBUTTONSTYLE).replace("10pt", "12pt"));
    editButton->setIcon(QIcon(":/icons_new/edit.png"));
    editButton->setText(tr("Edit") + " " + (pageNum == 0? tr("title") : tr("section")));
    connect(editButton, &QPushButton::clicked, this, [this, pageNum](){emit editRequested(pageNum);});
    layout->addWidget(editButton, row++, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);

    layout->setColumnStretch(0, 1);

    for(int i = 0; i < numQuestions; i++) {
        preQuestionSpacer << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addItem(preQuestionSpacer.last(), row++, 0, 1, -1);

        questionLabel << new QLabel(this);
        questionLabel.last()->setStyleSheet(LABELSTYLE);
        questionLabel.last()->setWordWrap(true);
        layout->addWidget(questionLabel.last(), row++, 0, 1, -1);
        questionLabel.last()->hide();

        questionLineEdit << new QLineEdit(this);
        questionLineEdit.last()->setStyleSheet(LINEEDITSTYLE);
        layout->addWidget(questionLineEdit.last(), row++, 0, 1, -1);
        questionLineEdit.last()->hide();

        questionComboBox << new QComboBox(this);
        questionComboBox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        questionComboBox.last()->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
        questionComboBox.last()->installEventFilter(new MouseWheelBlocker(questionComboBox.last()));
        layout->addWidget(questionComboBox.last(), row++, 0, 1, -1, Qt::AlignLeft);
        questionComboBox.last()->hide();

        questionGroupBox << new QGroupBox(this);
        questionGroupLayout << new QVBoxLayout;
        questionGroupLayout.last()->setSpacing(6);
        questionGroupLayout.last()->setContentsMargins(20, 0, 0, 0);
        questionGroupBox.last()->setLayout(questionGroupLayout.last());
        questionGroupBox.last()->setFlat(true);
        questionGroupBox.last()->setStyleSheet("QGroupBox{border: none;}");
        layout->addWidget(questionGroupBox.last(), row++, 0, 1, -1);
        questionGroupBox.last()->hide();
    }
}

void SurveyMakerPreviewSection::addWidget(QWidget *widget)
{
    layout->addWidget(widget, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
}

void SurveyMakerPreviewSection::setTitle(const QString &newTitle)
{
    title->setText("<span style=\"color: " DEEPWATERHEX "; font-family:'DM Sans'; font-size:14pt\">" + newTitle + "</span>");
}
