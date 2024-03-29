#include "comboBoxWithElidedContents.h"
#include <QStylePainter>


//////////////////
// QComboBox that paints with elided contents
//////////////////
ComboBoxWithElidedContents::ComboBoxWithElidedContents(const QString &minWidthText, QWidget *parent)
    :QComboBox(parent)
{
    minText = minWidthText;
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
}

QSize ComboBoxWithElidedContents::sizeHint() const
{
    return minimumSizeHint();
}

QSize ComboBoxWithElidedContents::minimumSizeHint() const
{
    return {(QFontMetrics(this->font())).boundingRect(minText).width()+PIXELS_TO_ADD_TO_MIN_TEXT, QComboBox::minimumSizeHint().height()};
}

void ComboBoxWithElidedContents::paintEvent(QPaintEvent *event)
{
    (void)event;
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_ComboBox, opt);

    const QRect textRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    opt.currentText = p.fontMetrics().elidedText(opt.currentText, Qt::ElideMiddle, textRect.width());
    p.drawControl(QStyle::CE_ComboBoxLabel, opt);
}
