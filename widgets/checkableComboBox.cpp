#include "checkableComboBox.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStylePainter>

CheckableComboBox::CheckableComboBox(QWidget *parent)
    : QComboBox(parent)
{
    itemModel = new QStandardItemModel(this);
    setModel(itemModel);
    setItemDelegate(new CheckBoxDelegate(this));

    // Keep the popup open when an item is clicked (toggling its check state)
    connect(view(), &QAbstractItemView::pressed, this, &CheckableComboBox::onItemPressed);
}

void CheckableComboBox::addItems(const QStringList &items)
{
    for (const QString &text : items) {
        auto *item = new QStandardItem(text);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        itemModel->appendRow(item);
    }
}

void CheckableComboBox::setCheckedItems(const QStringList &items)
{
    for (int i = 0; i < itemModel->rowCount(); ++i) {
        auto *item = itemModel->item(i);
        item->setData(items.contains(item->text()) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
    repaint();
    emit checkedItemsChanged();
}

QStringList CheckableComboBox::checkedItems() const
{
    QStringList result;
    for (int i = 0; i < itemModel->rowCount(); ++i) {
        const auto *item = itemModel->item(i);
        if (item->checkState() == Qt::Checked) {
            result << item->text();
        }
    }
    return result;
}

QString CheckableComboBox::displayText() const
{
    const QStringList allCheckedItems = checkedItems();
    if (allCheckedItems.isEmpty()) {
        return tr("(none selected)");
    }
    QString joinedItems = allCheckedItems.join('|');
    return joinedItems.replace(joinedItems.lastIndexOf('|'), 1, " or ").replace('|', ", ");
}

void CheckableComboBox::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // Draw the combo box frame and arrow
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // Draw the multi-line text ourselves in the label area
    const QRect textRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextWordWrap, displayText());
}

void CheckableComboBox::hidePopup()
{
    // Prevent the popup from hiding when clicking items (we want it to stay open for multi-select).
    // Only hide when clicking outside the popup.
    if (ignoreHide) {
        ignoreHide = false;
        return;
    }
    QComboBox::hidePopup();
}

void CheckableComboBox::onItemPressed(const QModelIndex &index)
{
    auto *item = itemModel->itemFromIndex(index);
    if (item == nullptr) {
        return;
    }

    // Toggle check state
    item->setData(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);

    // Prevent popup from closing
    ignoreHide = true;

    repaint();
    emit checkedItemsChanged();
}

// ============================================================
// Custom delegate: draws checkbox + text for each item
// ============================================================

void CheckableComboBox::CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
{
    painter->save();

    const QStyle *style = option.widget ? option.widget->style() : QApplication::style();

    // Draw background (handles hover highlighting)
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

    // Calculate checkbox rect
    QStyleOptionButton checkOpt;
    checkOpt.rect = QRect(option.rect.left() + 4,
                          option.rect.top() + (option.rect.height() - 16) / 2,
                          16, 16);
    checkOpt.state = option.state & ~QStyle::State_HasFocus;
    const auto checkState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    checkOpt.state |= (checkState == Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
    style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOpt, painter, option.widget);

    // Draw text to the right of the checkbox
    const QRect textRect(checkOpt.rect.right() + 6, option.rect.top(),
                         option.rect.width() - checkOpt.rect.width() - 10, option.rect.height());
    const QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(option.palette.color(
        (option.state & QStyle::State_MouseOver) ? QPalette::HighlightedText : QPalette::Text));
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    painter->restore();
}

bool CheckableComboBox::CheckBoxDelegate::editorEvent(QEvent */*event*/, QAbstractItemModel */*model*/,
                                                      const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/)
{
    // Let the default handling work — the pressed signal on the view handles toggling
    return false;
}
