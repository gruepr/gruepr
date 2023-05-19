#include "surveyMakerQuestionWithSwitch.h"
#include "gruepr_globals.h"
#include <QPainter>

SurveyMakerQuestionWithSwitch::SurveyMakerQuestionWithSwitch(QWidget *parent, QString textLabel, bool startingValue)
    : QFrame{parent}
    , _enabled(true)
    , _extraWidgetsIndex(0)
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SurveyMakerQuestionWithSwitch::valueChange);

    label = new QLabel;
    setLabel(textLabel);

    auto palette = this->palette();
    setAutoFillBackground(true);
    palette.setColor(QPalette::Window, GRUEPRVERYLIGHTBLUE);
    palette.setColor(QPalette::WindowText, GRUEPRDARKBLUE);
    setPalette(palette);

    layout = new QGridLayout(this);
    layout->addWidget(label, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(switchButton, 0, Qt::AlignRight | Qt::AlignVCenter);
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
    auto palette = this->palette();
    setAutoFillBackground(true);
    if (_enabled) {
        palette.setColor(QPalette::Window, GRUEPRVERYLIGHTBLUE);
        palette.setColor(QPalette::WindowText, GRUEPRDARKBLUE);
        label->setText(label->text().replace("bebebe", GRUEPRDARKBLUEHEX));
    }
    else {
        palette.setColor(QPalette::Window, QColor(230, 230, 230));
        palette.setColor(QPalette::WindowText, QColor(190, 190, 190));
        label->setText(label->text().replace(GRUEPRDARKBLUEHEX, "bebebe"));
    }
    setPalette(palette);

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

bool SurveyMakerQuestionWithSwitch::value()
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
