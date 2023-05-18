#ifndef SWITCHITEM_H
#define SWITCHITEM_H

#include "switchButton.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>

class SwitchItem : public QFrame
{
    Q_OBJECT

public:
    explicit SwitchItem(QWidget *parent = nullptr, QString textLabel = "", bool startingValue = false);
    ~SwitchItem() override;

    void setLabel(QString text);
    void setValue(bool value);
    bool value();

    void addWidget(QWidget *widget);

    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

signals:
    void valueChanged(bool newvalue);

private slots:
    void valueChange(bool newvalue);

private:
    QLabel *label;
    SwitchButton *switchButton;
    QGridLayout *layout;

    bool _enabled;
    int _extraWidgetsIndex;
};

#endif // SWITCHITEM_H
