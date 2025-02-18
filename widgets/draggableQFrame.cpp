#include "draggableQFrame.h"
#include <QDrag>
#include <QMimeData>
#include "qevent.h"
#include "qscrollarea.h"
#include "widgets/draggableFramesScrollWidget.h"
#include <QLayout>

DraggableQFrame::DraggableQFrame(QWidget *parent): QFrame(parent) {
    setAcceptDrops(true);
}
void DraggableQFrame::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton){
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString::number(reinterpret_cast<quintptr>(this)));
        drag->setMimeData(mimeData);
        drag->setPixmap(grab());  // Capture widget appearance
        drag->exec(Qt::MoveAction);
    }
}

void DraggableQFrame::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void DraggableQFrame::dropEvent(QDropEvent *event) {
    // Get the widget ID (pointer stored in mime data)
    QString widgetID = event->mimeData()->text();
    DraggableQFrame *draggedFrame = reinterpret_cast<DraggableQFrame*>(widgetID.toULongLong());

    if (draggedFrame) {
        // Find the index of both frames in the layout
        int draggedPriorityOrder = draggedFrame->getPriorityOrder();
        int targetPriorityOrder = this->getPriorityOrder();

        // Swap positions in layout

        this->setPriorityOrder(draggedPriorityOrder);
        draggedFrame->setPriorityOrder(targetPriorityOrder);

        // a layout that stores the objects in a list <DraggableQFrame> then depending on the order, outputs!
        // Insert the widgets back at their new positions
        event->acceptProposedAction();
        emit frameSwapRequested(draggedPriorityOrder, targetPriorityOrder);  // Emitting the signal
    }
}

int DraggableQFrame::getPriorityOrder(){
    return this->priorityOrder;
}
void DraggableQFrame::setPriorityOrder(int priorityOrder){
    this->priorityOrder = priorityOrder;
}
