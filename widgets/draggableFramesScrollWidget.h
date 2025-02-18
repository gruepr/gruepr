#ifndef DRAGGABLEFRAMESSCROLLWIDGET_H
#define DRAGGABLEFRAMESSCROLLWIDGET_H

#include <QVBoxLayout>
#include <QList>
#include "draggableQFrame.h"

class DraggableFramesScrollWidget : public QWidget {
    Q_OBJECT
public:
    explicit DraggableFramesScrollWidget(QWidget *parent = nullptr);

    void setListOfDraggableFrames(const QList<DraggableQFrame*>& list);
    QList<DraggableQFrame*>& getListOfDraggableFrames();

private:
    QLayout *layout;
    QList<DraggableQFrame*> listOfDraggableFrames;
};

#endif // DRAGGABLEFRAMESSCROLLWIDGET_H
