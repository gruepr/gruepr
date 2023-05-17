#include "switchItem.h"
#include "gruepr_globals.h"

SwitchItem::SwitchItem(QWidget *parent, QString textLabel, bool startingValue)
    : QFrame{parent}
{
    switchButton = new SwitchButton(startingValue);
    connect(switchButton, &SwitchButton::valueChanged, this, &SwitchItem::valueChange);

    label = new QLabel;
    setLabel(textLabel);

    auto palette = this->palette();
    setAutoFillBackground(true);
    palette.setColor(QPalette::Window, GRUEPRVERYLIGHTBLUE);
    palette.setColor(QPalette::WindowText, GRUEPRMEDBLUE);
    setPalette(palette);

    layout = new QHBoxLayout(this);
    layout->addWidget(label, 0, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(switchButton, 0, Qt::AlignRight | Qt::AlignVCenter);
}

SwitchItem::~SwitchItem()
{
    delete label;
    delete switchButton;
    delete layout;
}

void SwitchItem::valueChange(bool newvalue)
{
    emit valueChanged(newvalue);
}

void SwitchItem::setLabel(QString text)
{
    label->setText("<span style=\"color: #" + QString(GRUEPRMEDBLUEHEX) + "; font-family:'DM Sans'\">" + text + "</span>");
}

void SwitchItem::setValue(bool value)
{
    switchButton->setValue(value);
}

bool SwitchItem::value()
{
    return switchButton->value();
}

/*
void SwitchItem::mousePressEvent(QMouseEvent *)
{

}
*/
