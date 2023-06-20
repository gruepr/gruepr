#include "surveyMakerQuestion.h"
#include "gruepr_globals.h"
#include "dialogs/customResponseOptionsDialog.h"

SurveyMakerQuestionWithSwitch::SurveyMakerQuestionWithSwitch(QWidget *parent, const QString &textLabel, bool startingValue)
    : QFrame{parent}
    , _enabled(true)
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SurveyMakerQuestionWithSwitch::valueChange);

    label = new QLabel;
    setLabel(textLabel);

    setStyleSheet("background-color: #" BUBBLYHEX "; color: #" DEEPWATERHEX ";");

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
        setStyleSheet("background-color: #" BUBBLYHEX "; color: #" DEEPWATERHEX ";");
        label->setText(label->text().replace("bebebe", DEEPWATERHEX));
    }
    else {
        setStyleSheet("background-color: #e6e6e6; color: #bebebe;");
        label->setText(label->text().replace(DEEPWATERHEX, "bebebe"));
    }
}

void SurveyMakerQuestionWithSwitch::setEnabled(bool flag)
{
    _enabled = flag;
    switchButton->setEnabled(flag);
    for(auto *widget : this->findChildren<QWidget *>())
    {
        widget->setEnabled(flag);
    }
    QWidget::setEnabled(flag);
}

void SurveyMakerQuestionWithSwitch::setLabel(const QString &text)
{
    label->setText("<span style=\"color: #" DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + text + "</span>");
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
    label = new QLabel;
    setNumber(questionNum);
    deleteButton = new QPushButton;
    deleteButton->setStyleSheet(DELBUTTONSTYLE);
    deleteButton->setText(tr("Delete"));
    deleteButton->setIcon(QIcon(":/icons_new/trashButton.png"));

    questionLabel = new QLabel(tr("Question"));
    questionLabel->setStyleSheet(LABELSTYLE);
    questionPlainTextEdit = new QPlainTextEdit;
    questionPlainTextEdit->setStyleSheet(PLAINTEXTEDITSTYLE);
    questionPlainTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    questionPlainTextEdit->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the textedit
    questionPlainTextEdit->installEventFilter(new MouseWheelBlocker(questionPlainTextEdit));
    questionPlainTextEdit->setPlaceholderText(tr("Type your question..."));
    auto *doc = questionPlainTextEdit->document();
    QFontMetrics font(doc->defaultFont());
    auto margins = questionPlainTextEdit->contentsMargins();
    int nHeight = font.lineSpacing() * 2 + (doc->documentMargin() + questionPlainTextEdit->frameWidth()) * 2 + margins.top() + margins.bottom() + 6;
    questionPlainTextEdit->setFixedHeight(nHeight);
    responsesLabel = new QLabel(tr("Response type"));
    responsesLabel->setStyleSheet(LABELSTYLE);
    responsesComboBox = new ComboBoxWithElidedContents("1. Very high / 2. Above average / 3. Average");
    responsesComboBox->setEditable(false);
    responsesComboBox->setStyleSheet(COMBOBOXSTYLE);
    responsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    responsesComboBox->installEventFilter(new MouseWheelBlocker(responsesComboBox));
    responsesComboBox->addItem(tr("Choose an option..."), QStringList({""}));
    responsesComboBox->setItemData(0, "", Qt::ToolTipRole);
    responsesComboBox->insertSeparator(1);
    QStringList responseOptions = QString(RESPONSE_OPTIONS).split(';');
    numOfResponseOptions = responseOptions.size();
    int item = 0;
    for(const auto &responseOption : responseOptions)
    {
        responsesComboBox->addItem(responseOption, responseOption.split(" / "));
        responsesComboBox->setItemData(item + 2, responseOption, Qt::ToolTipRole);
        item++;
    }
    responsesComboBox->setItemData(item + 1, QStringList({""}));    // reset the "Custom options..." item data and tooltip
    responsesComboBox->setItemData(item + 1, "", Qt::ToolTipRole);
    multiAllowed = new QCheckBox(tr("Allow students to select multiple options"));
    multiAllowed->setStyleSheet(CHECKBOXSTYLE);

    connect(deleteButton, &QPushButton::clicked, this, &SurveyMakerMultichoiceQuestion::deleteRequest);
    connect(questionPlainTextEdit, &QPlainTextEdit::textChanged, this, &SurveyMakerMultichoiceQuestion::questionChange);
    connect(responsesComboBox, &QComboBox::activated, this, &SurveyMakerMultichoiceQuestion::responsesComboBoxActivated);
    connect(multiAllowed, &QCheckBox::toggled, this, &SurveyMakerMultichoiceQuestion::multiChange);

    setStyleSheet("background-color: #" BUBBLYHEX "; color: #" DEEPWATERHEX ";");

    layout = new QGridLayout(this);
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
}

void SurveyMakerMultichoiceQuestion::setNumber(const int questionNum)
{
    label->setText("<span style=\"color: #" DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + tr("Question ") + QString::number(questionNum) + "</span>");
    this->questionNum = questionNum;
}

void SurveyMakerMultichoiceQuestion::deleteRequest()
{
    emit deleteRequested();
}

void SurveyMakerMultichoiceQuestion::questionChange()
{
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
    emit responsesAsStringChanged(tr("Options") + ": " + (newResponses == QStringList({""})? "---" : newResponses.join(" / ")) + (getMulti()? "\n   {Multiple responses allowed}" : ""));
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
    emit responsesAsStringChanged(tr("Options") + ": " + (newResponses == QStringList({""})? "---" : newResponses.join(" / ")) + (getMulti()? "\n   {Multiple responses allowed}" : ""));
}

QStringList SurveyMakerMultichoiceQuestion::getResponses() const
{
    return responsesComboBox->currentText().split(" / ");
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
    if(responsesComboBox->currentText() == tr("Custom options..."))
    {
        auto *window = new customResponseOptionsDialog(currentCustomOptions, this);

        // If user clicks OK, use these options
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
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
        else
        {
            bool currentValue = responsesComboBox->blockSignals(true);
            responsesComboBox->setCurrentIndex(prevIndex);
            responsesComboBox->blockSignals(currentValue);
        }

        delete window;
    }
    else
    {
        prevIndex = index;
    }

    // Put list of options back to just built-ins plus "Custom options"
    if(index < numOfResponseOptions)
    {
        responsesComboBox->removeItem(numOfResponseOptions + 2);
        responsesComboBox->removeItem(numOfResponseOptions + 1);
        responsesComboBox->addItem(tr("Custom options..."));
    }

    QStringList newResponses = responsesComboBox->currentData().toStringList();
    emit responsesChanged(newResponses);
    emit responsesAsStringChanged(tr("Options") + ": " + (newResponses == QStringList({""})? "---" : newResponses.join(" / ")) + (getMulti()? "\n   {Multiple responses allowed}" : ""));
}


//////////////////////////////////////////////////////////////////////////////////////////////////


SurveyMakerPreviewSection::SurveyMakerPreviewSection(const int pageNum, const QString &titleText, const int numQuestions, QWidget *parent)
    : QFrame{parent}
{
    setStyleSheet("background-color: #" BUBBLYHEX "; color: #" DEEPWATERHEX ";");

    layout = new QGridLayout(this);

    title = new QLabel("<span style=\"color: #" DEEPWATERHEX "; font-family:'DM Sans'; font-size:14pt\">" + titleText + "</span>");
    editButton = new QPushButton;
    editButton->setStyleSheet(DELBUTTONSTYLE);
    editButton->setText("🖉" + tr("Edit section"));
    connect(editButton, &QPushButton::clicked, this, [this, pageNum](){emit editRequested(pageNum);});

    layout->addWidget(title, row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(editButton, row++, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);

    layout->setColumnStretch(0, 1);

    for(int i = 0; i < numQuestions; i++) {
        preQuestionSpacer << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addItem(preQuestionSpacer.last(), row++, 0, 1, -1);

        questionLabel << new QLabel;
        questionLabel.last()->setStyleSheet(LABELSTYLE);
        layout->addWidget(questionLabel.last(), row++, 0, 1, -1);
        questionLabel.last()->hide();

        questionLineEdit << new QLineEdit;
        questionLineEdit.last()->setReadOnly(true);
        questionLineEdit.last()->setStyleSheet(LINEEDITSTYLE);
        layout->addWidget(questionLineEdit.last(), row++, 0, 1, -1);
        questionLineEdit.last()->hide();

        questionComboBox << new QComboBox;
        questionComboBox.last()->setStyleSheet(COMBOBOXSTYLE);
        questionComboBox.last()->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
        questionComboBox.last()->installEventFilter(new MouseWheelBlocker(questionComboBox.last()));
        layout->addWidget(questionComboBox.last(), row++, 0, 1, -1);
        questionComboBox.last()->hide();

        questionGroupBox << new QGroupBox;
        questionGroupLayout << new QVBoxLayout;
        questionGroupBox.last()->setLayout(questionGroupLayout.last());
        questionGroupBox.last()->setFlat(true);
        questionGroupBox.last()->setStyleSheet("border: none;");
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
    title->setText("<span style=\"color: #" DEEPWATERHEX "; font-family:'DM Sans'; font-size:14pt\">" + newTitle + "</span>");
}
