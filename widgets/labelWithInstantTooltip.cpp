#include "labelWithInstantTooltip.h"
#include <QMouseEvent>
#include <QToolTip>

void LabelWithInstantTooltip::setToolTipText(const QString &text)
{
    toolTipText = text;
}

void LabelWithInstantTooltip::enterEvent(QEnterEvent *event)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this, this->rect(), DISPLAYTIME);
    QLabel::enterEvent(event);
}

void LabelWithInstantTooltip::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    QLabel::leaveEvent(event);
}

void LabelWithInstantTooltip::mouseReleaseEvent(QMouseEvent *event)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this, this->rect(), DISPLAYTIME);
    event->accept();
}
