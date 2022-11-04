#ifndef COMBOBOXTHATPASSESSCROLLWHEEL
#define COMBOBOXTHATPASSESSCROLLWHEEL

// a subclassed combobox that passes the scrollwheel to the parent and ignores

#include <QComboBox>


class ComboBoxThatPassesScrollwheel : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxThatPassesScrollwheel(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
};

#endif // COMBOBOXTHATPASSESSCROLLWHEEL
