#include "switchButton.h"
#include "gruepr_globals.h"
#include <QPainter>

SwitchButton::SwitchButton(QWidget* parent, bool startingValue, Style style)
    : QWidget(parent)
    , _value(startingValue)
    , _duration(100)
    , _enabled(true)
{
    _bordercolor = GRUEPRMEDBLUE;

    _tol = 0;
    _background = new SwitchBackground(this);
    _labeloff = new QLabel(this);
    _labelon = new QLabel(this);
    _circle = new SwitchCircle(this);
    __btn_move = new QPropertyAnimation(this);
    __back_resize = new QPropertyAnimation(this);

    __btn_move->setTargetObject(_circle);
    __btn_move->setPropertyName("pos");
    __back_resize->setTargetObject(_background);
    __back_resize->setPropertyName("size");

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QFont font("DM Sans");
    _labeloff->setFont(font);
    _labelon->setFont(font);
    _labeloff->setText("Off");
    _labelon->setText("On");
    _labeloff->move(31, 5);
    _labelon->move(15, 5);
    setFixedSize(QSize(60, 24));
    if (style == Style::YESNO) {
        _labeloff->setText("No");
        _labelon->setText("Yes");
        _labeloff->move(33, 5);
        _labelon->move(12, 5);
        setFixedSize(QSize(60, 24));
    }
    else if (style == Style::BOOL) {
        _labeloff->setText("False");
        _labelon->setText("True");
        _labeloff->move(37, 5);
        _labelon->move(12, 5);
        setFixedSize(QSize(75, 24));
    }
    else if (style == Style::EMPTY) {
        _labeloff->setText("");
        _labelon->setText("");
        _labeloff->move(31, 5);
        _labelon->move(12, 5);
        setFixedSize(QSize(45, 24));
    }

    _labeloff->setStyleSheet("color: #" + QString(GRUEPRMEDBLUEHEX) + ";");
    _labelon->setStyleSheet("color: #" + QString(GRUEPRVERYLIGHTBLUEHEX) + ";");


    if(_value) {
        _background->resize(width() - 4, 20);
        _background->move(2, 2);
        _labelon->show();
        _labeloff->hide();
        _circle->move(width() - 22, 2);
    }
    else {
        _background->resize(20, 20);
        _background->move(2, 2);
        _labelon->hide();
        _labeloff->show();
        _circle->move(2, 2);
    }
}

SwitchButton::~SwitchButton()
{
    delete _circle;
    delete _background;
    delete _labeloff;
    delete _labelon;
    delete __btn_move;
    delete __back_resize;
}

void SwitchButton::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);

    if (_enabled) {
        painter->setBrush(_bordercolor);
        painter->drawRoundedRect(0, 0, width(), height(), 12, 12);
        painter->setBrush(GRUEPRVERYLIGHTBLUE);
        painter->drawRoundedRect(1, 1, width() - 2, height() - 2, 10, 10);
    }
    else {
        painter->setBrush(QColor(190, 190, 190));
        painter->drawRoundedRect(0, 0, width(), height(), 12, 12);
        painter->setBrush(QColor(QColor(230, 230, 230)));
        painter->drawRoundedRect(1, 1, width() - 2, height() - 2, 10, 10);
    }
    painter->end();
}

void SwitchButton::mousePressEvent(QMouseEvent*)
{
    if (!_enabled) {
        return;
    }

    __btn_move->stop();
    __back_resize->stop();

    __btn_move->setDuration(_duration);
    __back_resize->setDuration(_duration);

    int hback = 20;
    QSize initial_size(hback, hback);
    QSize final_size(width() - 4, hback);

    int xi = 2;
    int y  = 2;
    int xf = width() - 22;

    if (_value) {
        final_size = QSize(hback, hback);
        initial_size = QSize(width() - 4, hback);

        xi = xf;
        xf = 2;

        _labelon->hide();
        _labeloff->show();
    }
    else {
        _labelon->show();
        _labeloff->hide();
    }

    __btn_move->setStartValue(QPoint(xi, y));
    __btn_move->setEndValue(QPoint(xf, y));

    __back_resize->setStartValue(initial_size);
    __back_resize->setEndValue(final_size);

    __btn_move->start();
    __back_resize->start();

    // Assigning new current value
    _value = !_value;
    emit valueChanged(_value);
}

void SwitchButton::setEnabled(bool flag)
{
    _enabled = flag;
    _circle->setEnabled(flag);
    _background->setEnabled(flag);
    QWidget::setEnabled(flag);
}

void SwitchButton::setValue(bool flag)
{
    if (flag == value()) {
        return;
    }
    else {
        _value = flag;
        _update();
        setEnabled(_enabled);
        emit valueChanged(_value);
    }
}

bool SwitchButton::value() const
{
    return _value;
}

void SwitchButton::_update()
{
    int hback = 20;
    QSize final_size(width() - 4, hback);

    int y = 2;
    int xf = width() - 22;

    if (_value) {
        final_size = QSize(hback, hback);
        xf = 2;

        _labelon->show();
        _labeloff->hide();
    }
    else {
        _labelon->hide();
        _labeloff->show();
    }

    _circle->move(QPoint(xf, y));
    _background->resize(final_size);
}

SwitchButton::SwitchBackground::SwitchBackground(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(20);

    _bluebrush = GRUEPRMEDBLUE;
    _graybrush = QColor(190, 190, 190);

    _enabled = true;
}
SwitchButton::SwitchBackground::~SwitchBackground()
{
}
void SwitchButton::SwitchBackground::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);
    if (_enabled) {
        painter->setBrush(_bluebrush);
        painter->drawRoundedRect(0, 0, width(), height(), 10, 10);
    }
    else {
        painter->setBrush(_graybrush);
        painter->drawRoundedRect(0, 0, width(), height(), 10, 10);
    }
    painter->end();
}
void SwitchButton::SwitchBackground::setEnabled(bool flag)
{
    _enabled = flag;
}

SwitchButton::SwitchCircle::SwitchCircle(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(20, 20);

    _bluebrush = GRUEPRMEDBLUE;
    _whitebrush = GRUEPRVERYLIGHTBLUE;
    _graybrush = QColor(230, 230, 230);

    _enabled = true;
}
SwitchButton::SwitchCircle::~SwitchCircle()
{
}
void SwitchButton::SwitchCircle::paintEvent(QPaintEvent*)
{
    QPainter* painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::NoPen);
    painter->setPen(pen);

    if (_enabled) {
        painter->setBrush(_bluebrush);
        painter->drawEllipse(0, 0, 20, 20);
        painter->setBrush(_whitebrush);
        painter->drawEllipse(2, 2, 16, 16);
    }
    else {
        painter->setBrush(_graybrush.darker(150));
        painter->drawEllipse(0, 0, 20, 20);
        painter->setBrush(_graybrush);
        painter->drawEllipse(2, 2, 16, 16);
    }

    painter->end();
}
void SwitchButton::SwitchCircle::setEnabled(bool flag)
{
    _enabled = flag;
}
