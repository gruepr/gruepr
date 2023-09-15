#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <QLabel>
#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#endif
#include <QWidget>

class SwitchButton : public QWidget
{
    Q_OBJECT

public:
    enum class Style
    {
        YESNO,
        ONOFF,
        BOOL,
        EMPTY
    };

public:
    explicit SwitchButton(QWidget* parent = nullptr, bool startingValue = false, Style style = Style::YESNO);
    explicit SwitchButton(bool startingValue = false) : SwitchButton(nullptr, startingValue) {};
    ~SwitchButton() override;

    //-- QWidget methods
    void mousePressEvent(QMouseEvent* event = nullptr) override;
    void paintEvent(QPaintEvent* event = nullptr) override;
    void setEnabled(bool);

    //-- Setters
    void setValue(bool);

    //-- Getters
    bool value() const;

signals:
    void valueChanged(bool newvalue);

private:
    class SwitchCircle;
    class SwitchBackground;
    void _update();

private:
    bool _value;
    int  _duration;

    QColor _bordercolor;
    int    _tol;

    // This order for definition is important (these widgets overlap)
    SwitchBackground* _background = nullptr;
    QLabel*           _labeloff = nullptr;
    QLabel*           _labelon = nullptr;
    SwitchCircle*     _circle = nullptr;

    bool _enabled;

#if (defined (Q_OS_WIN) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    QPropertyAnimation* __btn_move = nullptr;
    QPropertyAnimation* __back_resize = nullptr;
    QParallelAnimationGroup *__animationGroup = nullptr;
#endif
};

class SwitchButton::SwitchBackground : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SwitchBackground)

public:
    explicit SwitchBackground(QWidget* parent = nullptr);

    //-- QWidget methods
    void paintEvent(QPaintEvent* event = nullptr) override;
    void setEnabled(bool);

private:
    QColor          _bluebrush;
    QColor          _graybrush;
    bool _enabled;
};


class SwitchButton::SwitchCircle : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SwitchCircle)

public:
    explicit SwitchCircle(QWidget* parent = nullptr);

    //-- QWidget methods
    void paintEvent(QPaintEvent* event = nullptr) override;
    void setEnabled(bool);

private:
    QColor          _bluebrush;
    QColor          _whitebrush;
    QColor          _graybrush;
    bool _enabled;
};

#endif // SWITCHBUTTON_H
