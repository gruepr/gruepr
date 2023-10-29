#include "labelThatForwardsMouseClicks.h"
#include <QMouseEvent>

void LabelThatForwardsMouseClicks::mousePressEvent(QMouseEvent* event)
{
    emit mousePressed(event);
    event->ignore();
}
