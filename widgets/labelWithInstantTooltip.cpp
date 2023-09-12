#include "labelWithInstantTooltip.h"
#include <QMouseEvent>
#include <QToolTip>

LabelWithInstantTooltip::LabelWithInstantTooltip(const QString &labelText, QWidget *parent)
    :QLabel(labelText, parent)
{
}

LabelWithInstantTooltip::LabelWithInstantTooltip(QWidget *parent)
    :QLabel(parent)
{
}


void LabelWithInstantTooltip::enterEvent(QEnterEvent *event)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this, this->rect(), 60000);
    QLabel::enterEvent(event);
}

void LabelWithInstantTooltip::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    QLabel::leaveEvent(event);
}

void LabelWithInstantTooltip::mouseReleaseEvent(QMouseEvent *event)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this, this->rect(), 60000);
    event->accept();
}


void LabelWithInstantTooltip::setToolTipText(const QString &text)
{
    toolTipText = text;
}
