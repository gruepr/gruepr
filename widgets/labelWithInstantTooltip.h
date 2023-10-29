#ifndef LABELWITHINSTANTTOOLTIP_H
#define LABELWITHINSTANTTOOLTIP_H

#include <QLabel>

class LabelWithInstantTooltip : public QLabel
{
    Q_OBJECT
public:
    LabelWithInstantTooltip(const QString &labelText = "", QWidget *parent = nullptr) : QLabel(labelText, parent) {};
    LabelWithInstantTooltip(QWidget *parent = nullptr) : QLabel(parent) {};

    void setToolTipText(const QString &text);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString toolTipText;

    inline static const int DISPLAYTIME = 60000;    //amount of time to show the tooltip (in ms)
};

#endif // LABELWITHINSTANTTOOLTIP_H
