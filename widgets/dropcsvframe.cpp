#include "dropcsvframe.h"
#include "gruepr_globals.h"
#include <QDropEvent>
#include <QMimeData>

DropCSVFrame::DropCSVFrame(QWidget *parent) :
    QFrame(parent)
{
    setAcceptDrops(true);
    setStyleSheet("QWidget {background-color: " OPENWATERHEX "; color: white; font-family:'DM Sans'; font-size: 12pt; "
                  "border-style: solid; border-width: 2px; border-radius: 5px; border-color: white; padding: 10px;}"
                  "QLabel { border: none; padding: 0px;}");
}


void DropCSVFrame::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        setStyleSheet(DRAGENTERDROPFRAME);
    }
}
void DropCSVFrame::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event); //suppress warnings about unused event parameter
    setStyleSheet(DROPFRAME);
}

void DropCSVFrame::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    QString filePathString;

    for(const QUrl &url : std::as_const(urls)) {
        filePathString += url.toLocalFile();
    }
    emit itemDropped(filePathString);
    setStyleSheet(DROPFRAME);
    //qDebug() << "Dropped url" << filePathString;
}
