#include "labelThatForwardsMouseClicks.h"
#include <QMouseEvent>

LabelThatForwardsMouseClicks::LabelThatForwardsMouseClicks(QWidget *parent)
    :QLabel(parent)
{
}

void LabelThatForwardsMouseClicks::mousePressEvent(QMouseEvent* event)
{
    emit mousePressed(event);
    event->ignore();
}
