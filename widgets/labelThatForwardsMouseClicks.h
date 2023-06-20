#ifndef LABELTHATFORWARDSMOUSECLICKS_H
#define LABELTHATFORWARDSMOUSECLICKS_H

// a subclassed label that passes the mousepressevent to the parent and ignores

#include <QLabel>

class LabelThatForwardsMouseClicks : public QLabel
{
    Q_OBJECT

public:
    LabelThatForwardsMouseClicks(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void mousePressed(QMouseEvent* event);
};

#endif // LABELTHATFORWARDSMOUSECLICKS_H
