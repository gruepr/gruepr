#ifndef SWITCHITEM_H
#define SWITCHITEM_H

#include "switchButton.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

class SwitchItem : public QFrame
{
    Q_OBJECT

public:
    explicit SwitchItem(QWidget *parent = nullptr, QString textLabel = "", bool startingValue = false);
    ~SwitchItem() override;

    void setLabel(QString text);
    void setValue(bool value);
    bool value();

    //-- QWidget methods
    //void mousePressEvent(QMouseEvent *) override; // pass to switchButton?

signals:
    void valueChanged(bool newvalue);

private slots:
    void valueChange(bool newvalue);

private:
    QLabel *label;
    SwitchButton *switchButton;
    QHBoxLayout *layout;
};

#endif // SWITCHITEM_H
