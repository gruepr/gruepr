#ifndef DRAGGABLEQFRAME_H
#define DRAGGABLEQFRAME_H

#include "qframe.h"
class DraggableQFrame : public QFrame

{
    Q_OBJECT
public:
    DraggableQFrame(QWidget *parent = nullptr);
    void mousePressEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    int getPriorityOrder();
    void setPriorityOrder(int priorityOrder);
signals:
    void frameSwapRequested(int draggedIndex, int targetIndex);  // Emitting the signal
private:
    int priorityOrder;
};

#endif // DRAGGABLEQFRAME_H
