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

SurveyMakerQuestionWithSwitch::~SurveyMakerQuestionWithSwitch()
{
    delete label;
    delete switchButton;
    delete layout;
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
    questionLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    questionLineEdit = new QLineEdit;
    questionLineEdit->setStyleSheet(SURVEYMAKERLINEEDITSTYLE);
    questionLineEdit->setPlaceholderText(tr("Type your question..."));
    responsesLabel = new QLabel(tr("Response type"));
    responsesLabel->setStyleSheet(SURVEYMAKERLABELSTYLE);
    responsesComboBox = new ComboBoxWithElidedContents("1. Very high / 2. Above average / 3. Average");
    responsesComboBox->setStyleSheet(SURVEYMAKERCOMBOBOXSTYLE);
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

SurveyMakerMultichoiceQuestion::~SurveyMakerMultichoiceQuestion()
{
    delete layout;
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
    emit questionChanged((newQuestion.isEmpty())? ("[" + tr("Question ") + QString::number(questionNum) + "]") : newQuestion);
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
    responsesComboBox->setCurrentText(newResponses.join(" / "));
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
