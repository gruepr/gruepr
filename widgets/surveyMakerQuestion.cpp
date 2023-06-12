#include "surveyMakerQuestion.h"
#include "gruepr_globals.h"

SurveyMakerQuestionWithSwitch::SurveyMakerQuestionWithSwitch(QWidget *parent, const QString &textLabel, bool startingValue)
    : QFrame{parent}
    , _enabled(true)
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SurveyMakerQuestionWithSwitch::valueChange);

    label = new QLabel;
    setLabel(textLabel);

    setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");

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
        setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");
        label->setText(label->text().replace("bebebe", GRUEPRDARKBLUEHEX));
    }
    else {
        setStyleSheet("background-color: #e6e6e6; color: #bebebe;");
        label->setText(label->text().replace(GRUEPRDARKBLUEHEX, "bebebe"));
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
    label->setText("<span style=\"color: #" GRUEPRDARKBLUEHEX "; font-family:'DM Sans'; font-size:12pt\">" + text + "</span>");
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
    questionLineEdit = new QLineEdit;
    questionLineEdit->setStyleSheet(LINEEDITSTYLE);
    questionLineEdit->setPlaceholderText(tr("Type your question..."));
    responsesLabel = new QLabel(tr("Response type"));
    responsesLabel->setStyleSheet(LABELSTYLE);
    responsesComboBox = new ComboBoxWithElidedContents("1. Very high / 2. Above average / 3. Average");
    responsesComboBox->setEditable(false);
    responsesComboBox->setStyleSheet(COMBOBOXSTYLE);
    responsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    responsesComboBox->installEventFilter(new MouseWheelBlocker(responsesComboBox));
    responsesComboBox->addItem(tr("Choose an option..."));
    responsesComboBox->setItemData(0, "");
    responsesComboBox->insertSeparator(1);
    QStringList responseOptions = QString(RESPONSE_OPTIONS).split(';');
    QStringList specificOptions;
    int item = 0;
    for(const auto &responseOption : responseOptions)
    {
        responsesComboBox->addItem(responseOption);
        specificOptions = responseOption.split('/');
        for(auto &specificOption : specificOptions)
        {
            specificOption = specificOption.trimmed();
        }
        responsesComboBox->setItemData(item + 2, specificOptions);
        responsesComboBox->setItemData(item + 2, specificOptions, Qt::ToolTipRole);
        item++;
    }
    multiAllowed = new QCheckBox(tr("Allow students to select multiple options"));

    connect(deleteButton, &QPushButton::clicked, this, &SurveyMakerMultichoiceQuestion::deleteRequest);
    connect(questionLineEdit, &QLineEdit::textChanged, this, &SurveyMakerMultichoiceQuestion::questionChange);
    connect(responsesComboBox, &QComboBox::currentTextChanged, this, &SurveyMakerMultichoiceQuestion::responsesChange);
    connect(multiAllowed, &QCheckBox::clicked, this, &SurveyMakerMultichoiceQuestion::multiChange);

    setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");

    layout = new QGridLayout(this);
    int row = 0;
    layout->addWidget(label, row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(deleteButton, row++, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(questionLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(questionLineEdit, row++, 0, 1, -1, Qt::AlignVCenter);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(responsesLabel, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(responsesComboBox, row++, 0, 1, -1, Qt::AlignVCenter);
    layout->addWidget(multiAllowed, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->setColumnStretch(0, 1);
}

void SurveyMakerMultichoiceQuestion::setNumber(const int questionNum)
{
    label->setText("<span style=\"color: #" GRUEPRDARKBLUEHEX "; font-family:'DM Sans'; font-size:12pt\">" + tr("Question ") + QString::number(questionNum) + "</span>");
    this->questionNum = questionNum;
}

void SurveyMakerMultichoiceQuestion::deleteRequest()
{
    emit deleteRequested();
}

void SurveyMakerMultichoiceQuestion::questionChange(const QString &newQuestion)
{
    emit questionChanged(newQuestion);
}

void SurveyMakerMultichoiceQuestion::setQuestion(const QString &newQuestion)
{
    questionLineEdit->setText(newQuestion);
}

QString SurveyMakerMultichoiceQuestion::getQuestion() const
{
    return questionLineEdit->text();
}

void SurveyMakerMultichoiceQuestion::responsesChange(const QString &newResponses)
{
    if(responsesComboBox->currentIndex() == 0) {
        emit responsesChanged(QStringList());
        emit responsesAsStringChanged(tr("Options") + ": ---");
    }
    else {
        emit responsesChanged(newResponses.split(" / "));
        emit responsesAsStringChanged(tr("Options") + ": " + newResponses + (getMulti()? "\n   {Multiple responses allowed}" : ""));
    }
}

void SurveyMakerMultichoiceQuestion::multiChange(const bool newMulti)
{
    emit multiChanged(newMulti);
    responsesChange(responsesComboBox->currentText());
}

void SurveyMakerMultichoiceQuestion::setResponses(const QStringList &newResponses)
{
    if(newResponses == QStringList({""})) {
        responsesComboBox->setCurrentIndex(0);
    }
    else {
        responsesComboBox->setCurrentText(newResponses.join(" / "));
    }
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


//////////////////////////////////////////////////////////////////////////////////////////////////


SurveyMakerPreviewSection::SurveyMakerPreviewSection(const int pageNum, const QString &titleText, const int numQuestions, QWidget *parent)
    : QFrame{parent}
{
    setStyleSheet("background-color: #" GRUEPRVERYLIGHTBLUEHEX "; color: #" GRUEPRDARKBLUEHEX ";");

    layout = new QGridLayout(this);

    title = new QLabel("<span style=\"color: #" GRUEPRDARKBLUEHEX "; font-family:'DM Sans'; font-size:14pt\">" + titleText + "</span>");
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

        questionBottomLabel << new QLabel;
        questionBottomLabel.last()->setStyleSheet(LABELSTYLE);
        layout->addWidget(questionBottomLabel.last(), row++, 0, 1, -1);
        questionBottomLabel.last()->hide();
    }
}

void SurveyMakerPreviewSection::addWidget(QWidget *widget)
{
    layout->addWidget(widget, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
}
