#include "draggableFramesScrollWidget.h"

DraggableFramesScrollWidget::DraggableFramesScrollWidget(QWidget *parent)
    : QWidget(parent) {

}

//Pass by reference and cannot modify itself
void DraggableFramesScrollWidget::setListOfDraggableFrames(const QList<DraggableQFrame*>& newFrames) {
    // Remove old frames and clear layout
    while (listOfDraggableFrames.length() != 1 and !listOfDraggableFrames.isEmpty()) {
        DraggableQFrame* frame = listOfDraggableFrames.takeFirst();
        layout->removeWidget(frame);
        delete frame;  // Free memory
    }

    // Add new frames
    listOfDraggableFrames = newFrames;
    for (DraggableQFrame* frame : listOfDraggableFrames) {
        frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(frame);
    }
    if (!listOfDraggableFrames.isEmpty()){
        DraggableQFrame* frame = listOfDraggableFrames.takeFirst();
        layout->removeWidget(frame);
        delete frame;  // Free memory
    }
    layout->update();
}

QList<DraggableQFrame*>& DraggableFramesScrollWidget::getListOfDraggableFrames(){
    return this->listOfDraggableFrames;
}
