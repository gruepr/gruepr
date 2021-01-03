#ifndef PUSHBUTTONWITHMOUSEENTER
#define PUSHBUTTONWITHMOUSEENTER

// a subclassed QPushButton that passes mouse enter events to its parent

#include <QPushButton>


class PushButtonWithMouseEnter : public QPushButton
{
    Q_OBJECT

public:
    PushButtonWithMouseEnter(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

protected:
    void enterEvent(QEvent *event);

signals:
    void mouseEntered();
};

#endif // PUSHBUTTONWITHMOUSEENTER
