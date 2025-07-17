#ifndef CUSTOMSPLITTERHANDLE_H
#define CUSTOMSPLITTERHANDLE_H

#include <QSplitter>
#include <QToolButton>

class CustomSplitterHandle : public QSplitterHandle
{

public:
    CustomSplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *event);
    void toggleCollapse();

private:
    QToolButton *button;
    bool collapsed = false;
    int savedSize = 200;  // Default fallback width
};

#endif // CUSTOMSPLITTERHANDLE_H
