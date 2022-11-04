#include "comboBoxThatPassesScrollwheel.h"
#include <QWheelEvent>


//////////////////
// QComboBox that passes the scrollwheel to the parent and ignores
//////////////////
ComboBoxThatPassesScrollwheel::ComboBoxThatPassesScrollwheel(QWidget *parent)
    :QComboBox(parent)
{
}

void ComboBoxThatPassesScrollwheel::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}
