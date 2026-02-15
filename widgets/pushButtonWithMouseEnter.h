#ifndef PUSHBUTTONWITHMOUSEENTER
#define PUSHBUTTONWITHMOUSEENTER

// a subclassed QPushButton that passes mouse enter events to its parent

#include <QEnterEvent>
#include <QPushButton>


class PushButtonWithMouseEnter : public QPushButton
{
    Q_OBJECT

public:
    PushButtonWithMouseEnter(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void mouseEntered();
    void mouseLeft();

private:
    inline static const QSize ICONSIZE = QSize(20,20);
};

#endif // PUSHBUTTONWITHMOUSEENTER
