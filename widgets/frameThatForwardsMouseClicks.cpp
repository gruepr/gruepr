#include "frameThatForwardsMouseClicks.h"
#include <QMouseEvent>

void FrameThatForwardsMouseClicks::mousePressEvent(QMouseEvent* event)
{
    emit clicked();
    event->ignore();
}
