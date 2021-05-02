#include "pushButtonWithMouseEnter.h"
#include <QGuiApplication>


//////////////////
// QPushButton that signals mouse enter events
//////////////////
PushButtonWithMouseEnter::PushButtonWithMouseEnter(const QIcon &icon, const QString &text, QWidget *parent)
    :QPushButton (icon, text, parent)
{
    this->setFlat(true);
    this->setMouseTracking(true);
    this->setIconSize(QSize(20,20));
}

void PushButtonWithMouseEnter::enterEvent(QEvent *event)
{
    emit mouseEntered();
    event->ignore();
}

void PushButtonWithMouseEnter::leaveEvent(QEvent *event)
{
    emit mouseLeft();
    event->ignore();
}
