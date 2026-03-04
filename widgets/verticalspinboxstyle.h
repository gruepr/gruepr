#ifndef VERTICALSPINBOXSTYLE_H
#define VERTICALSPINBOXSTYLE_H

#include <QProxyStyle>
#include <QStyleOptionSpinBox>

class VerticalSpinBoxStyle : public QProxyStyle {
public:
    explicit VerticalSpinBoxStyle(QStyle *base = nullptr)
        : QProxyStyle(base) {}

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *w) const override
    {
        if (cc == CC_SpinBox) {
            const auto *so = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
            if (!so) return QProxyStyle::subControlRect(cc, opt, sc, w);

            QRect frame = QProxyStyle::subControlRect(cc, opt, SC_SpinBoxFrame, w);
            int btnW = 20;
            int btnH = frame.height() / 2;
            QRect btnArea(frame.right() - btnW, frame.top(), btnW, frame.height());

            if (sc == SC_SpinBoxUp)
                return QRect(btnArea.left(), btnArea.top(), btnW, btnH);
            if (sc == SC_SpinBoxDown)
                return QRect(btnArea.left(), btnArea.top() + btnH, btnW, btnH);
            if (sc == SC_SpinBoxEditField)
                return QRect(frame.left(), frame.top(), frame.width() - btnW, frame.height());
            if (sc == SC_SpinBoxFrame)
                return frame;
        }
        return QProxyStyle::subControlRect(cc, opt, sc, w);
    }
};

#endif // VERTICALSPINBOXSTYLE_H
