#include "comboBoxWithElidedContents.h"
#include <QStylePainter>


//////////////////
// QComboBox that paints with elided contents
//////////////////
ComboBoxWithElidedContents::ComboBoxWithElidedContents(QWidget *parent)
    :QComboBox(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

QSize ComboBoxWithElidedContents::sizeHint() const
{
    return minimumSizeHint();
}

QSize ComboBoxWithElidedContents::minimumSizeHint() const
{
    return {(QFontMetrics(this->font())).boundingRect("Very high / Above average / Average / Below average / Very low").width()+15, QComboBox::minimumSizeHint().height()};
}

void ComboBoxWithElidedContents::paintEvent(QPaintEvent *event)
{
    (void)event;
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_ComboBox, opt);

    QRect textRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    opt.currentText = p.fontMetrics().elidedText(opt.currentText, Qt::ElideMiddle, textRect.width());
    p.drawControl(QStyle::CE_ComboBoxLabel, opt);
}
