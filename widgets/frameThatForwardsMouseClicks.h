#ifndef FRAMETHATFORWARDSMOUSECLICKS_H
#define FRAMETHATFORWARDSMOUSECLICKS_H

// a subclassed frame that passes the mousepressevent to its children and ignores

#include <QFrame>

class FrameThatForwardsMouseClicks : public QFrame
{
    Q_OBJECT

public:
    FrameThatForwardsMouseClicks(QWidget *parent = nullptr) : QFrame(parent) {};

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void clicked();
};

#endif // LABELTHATFORWARDSMOUSECLICKS_H
