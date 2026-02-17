#include "customsplitterhandle.h"
#include "gruepr_globals.h"
#include <QPainter>

CustomSplitterHandle::CustomSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
    : QSplitterHandle(orientation, parent)
{
    button = new QToolButton(this);
    button->setAutoRaise(true);
    button->setCursor(makeArrowCursor(ArrowDirection::left));
    button->setFixedSize(16, 48);
    button->setStyleSheet(
        "QToolButton { background-color: white; border: 1px solid #ccc; color: black; padding: 4px; }"
        "QToolButton:hover { background-color: " BUBBLYHEX "; }"
        );

    button->setIcon(QIcon(":/icons_new/left-collapse.png"));
    button->move((width() - button->width()) / 2, (height() - button->height()) / 2);
    connect(button, &QToolButton::clicked, this, &CustomSplitterHandle::toggleCollapse);
}

void CustomSplitterHandle::resizeEvent(QResizeEvent *event)
{
    QSplitterHandle::resizeEvent(event);
    // Center the button in the handle
    button->move((width() - button->width()) / 2, (height() - button->height()) / 2);
    // QWidget *leftWidget = splitter()->widget(0);

    // if (leftWidget->width() == 0){
    //     collapsed = true;
    //     button->setIcon(QIcon(":/icons_new/right-collapse.png"));
    // } else {
    //     collapsed = false;
    //     button->setIcon(QIcon(":/icons_new/left-collapse.png"));
    // }
}


void CustomSplitterHandle::toggleCollapse()
{
    auto *splitter = qobject_cast<QSplitter*>(parent());

    QList<int> sizes = splitter->sizes();
    savedSize = splitter->size().width()/2;

    if (!collapsed) {
        sizes[0] = 0;
        sizes[1] = splitter->size().width();
        button->setIcon(QIcon(":/icons_new/right-collapse.png"));
        button->setCursor(makeArrowCursor(ArrowDirection::right));
    }
    else {
        sizes[0] = splitter->size().width();
        sizes[1] = 0;
        button->setIcon(QIcon(":/icons_new/left-collapse.png"));
        button->setCursor(makeArrowCursor(ArrowDirection::left));
    }

    collapsed = !collapsed;
    splitter->setSizes(sizes);
}

void CustomSplitterHandle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();
    const QPoint center = r.center();

    const auto bgColor = QColor("lightgray");
    painter.fillRect(rect(), bgColor);

    painter.setBrush(QColor(0x505050));
    painter.setPen(Qt::NoPen);

    // Draw 3 dots vertically or horizontally depending on orientation
    if (orientation() == Qt::Vertical) {
        // int y = center.y();
        // int xStart = center.x() - 10;
        // for (int i = 0; i < 3; ++i)
        //     painter.drawEllipse(QPoint(xStart + i * 6, y), 2, 2);
    }
    else {
        int x = width() / 2;

        const int yStartTop = height() / 4 - 12;
        for (int i = 0; i < 4; ++i) {
            painter.drawEllipse(QPoint(x, yStartTop + i * 6), 2, 2);
        }
        x = center.x();
        const int yStartBottom = (3 * height() / 4) - 12;
        for (int i = 0; i < 4; ++i) {
            painter.drawEllipse(QPoint(x, yStartBottom + i * 6), 2, 2);
        }
    }
}

QCursor CustomSplitterHandle::makeArrowCursor(ArrowDirection direction)
{
    const int size = 24;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(DEEPWATERHEX), 2));
    p.setBrush(QColor(AQUAHEX));

    QPolygon arrow;
    if (direction == ArrowDirection::left) {
        arrow << QPoint(4, size / 2)
        << QPoint(size - 4, 4)
        << QPoint(size - 4, size - 4);
    } else {
        arrow << QPoint(size - 4, size / 2)
        << QPoint(4, 4)
        << QPoint(4, size - 4);
    }
    p.drawPolygon(arrow);
    p.end();

    const int hotX = (direction == ArrowDirection::left) ? 4 : size - 4;
    return QCursor(pix, hotX, size / 2);
}
