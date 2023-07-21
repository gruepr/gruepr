#ifndef LABELWITHINSTANTTOOLTIP_H
#define LABELWITHINSTANTTOOLTIP_H

#include <QLabel>

class LabelWithInstantTooltip : public QLabel
{
    Q_OBJECT
public:
    LabelWithInstantTooltip(const QString &labelText = "", QWidget *parent = nullptr);

    void setToolTipText(const QString &text);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString toolTipText;
};

#endif // LABELWITHINSTANTTOOLTIP_H
