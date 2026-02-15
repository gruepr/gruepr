#ifndef DROPCSVFRAME_H
#define DROPCSVFRAME_H

#include <QFrame>

class DropCSVFrame : public QFrame
{
    Q_OBJECT

public:
    DropCSVFrame(QWidget *parent = nullptr);
    void dragEnterEvent(QDragEnterEvent *event = nullptr) override;
    void dropEvent(QDropEvent *event = nullptr) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

signals:
    void itemDropped(const QString &filePathString);
};

#endif // DROPCSVFRAME_H
