#include "customsplitter.h"
#include "widgets/customsplitterhandle.h"

CustomSplitter::CustomSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent) {}


QSplitterHandle* CustomSplitter::createHandle()
{
    return new CustomSplitterHandle(orientation(), this);
}
