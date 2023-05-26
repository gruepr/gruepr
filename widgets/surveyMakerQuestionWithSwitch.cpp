#include "surveyMakerQuestionWithSwitch.h"
#include "gruepr_globals.h"

SurveyMakerQuestionWithSwitch::SurveyMakerQuestionWithSwitch(QWidget *parent, QString textLabel, bool startingValue)
    : QFrame{parent}
    , _enabled(true)
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SurveyMakerQuestionWithSwitch::valueChange);

    label = new QLabel;
    setLabel(textLabel);

    setStyleSheet("background-color: #" + QString(GRUEPRVERYLIGHTBLUEHEX) + "; color: #" + GRUEPRDARKBLUEHEX + ";");

    layout = new QGridLayout(this);
    layout->addWidget(label, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(switchButton, 0, 1, Qt::AlignRight | Qt::AlignVCenter);
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
        setStyleSheet("background-color: #" + QString(GRUEPRVERYLIGHTBLUEHEX) + "; color: #" + GRUEPRDARKBLUEHEX + ";");
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

void SurveyMakerQuestionWithSwitch::setLabel(QString text)
{
    label->setText("<span style=\"color: #" + QString(GRUEPRDARKBLUEHEX) + "; font-family:'DM Sans'; font-size:16pt\">" + text + "</span>");
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

void SurveyMakerQuestionWithSwitch::addWidget(QWidget *widget, int row, int column, bool wholeRow, Qt::Alignment horizontalAlignment)
{
    if(row <= 0) {
        return;
    }
    layout->addWidget(widget, row, column, 1, (wholeRow? -1 : 1), horizontalAlignment | Qt::AlignVCenter);
}
