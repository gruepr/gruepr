#ifndef CUSTOMSPLITTERHANDLE_H
#define CUSTOMSPLITTERHANDLE_H

#include <QSplitter>
#include <QToolButton>

class CustomSplitterHandle : public QSplitterHandle
{

public:
    CustomSplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
    void toggleCollapse();

private:
    QToolButton *button;
    bool collapsed = false;
    int savedSize = 200;  // Default fallback width
    enum class ArrowDirection {left, right};
    static QCursor makeArrowCursor(ArrowDirection direction);
};

#endif // CUSTOMSPLITTERHANDLE_H
