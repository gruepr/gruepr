#ifndef DROPFILEFRAME_H
#define DROPFILEFRAME_H

#include <QFrame>

class DropFileFrame : public QFrame
{
    Q_OBJECT

public:
    DropFileFrame(QWidget *parent = nullptr);
    void dragEnterEvent(QDragEnterEvent *event = nullptr) override;
    void dropEvent(QDropEvent *event = nullptr) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

signals:
    void itemDropped(const QString &filePathString);
};

#endif // DROPFILEFRAME_H
