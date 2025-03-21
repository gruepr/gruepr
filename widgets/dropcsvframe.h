#ifndef DROPCSVFRAME_H
#define DROPCSVFRAME_H

#include "qframe.h"
class DropCSVFrame : public QFrame
{
    Q_OBJECT
public:
    DropCSVFrame(QWidget *parent = nullptr);
    void dragEnterEvent(QDragEnterEvent *event = nullptr);
    void dropEvent(QDropEvent *event = nullptr);

signals:
    void itemDropped(const QString &filePathString);
};

#endif // DROPCSVFRAME_H
