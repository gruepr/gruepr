#ifndef CHECKABLECOMBOBOX_H
#define CHECKABLECOMBOBOX_H

#include <QComboBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

// A QComboBox where each item has a checkbox.
// Multiple items can be checked. The display text shows checked items joined with a separator.

class CheckableComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit CheckableComboBox(QWidget *parent = nullptr);

    // Add items and optionally check some of them
    void addItems(const QStringList &items);
    void setCheckedItems(const QStringList &items);
    QStringList checkedItems() const;

    // The text shown when the combo box is collapsed
    QString displayText() const;

signals:
    void checkedItemsChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void hidePopup() override;

private:
    class CheckBoxDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    };

    QStandardItemModel *itemModel = nullptr;
    bool ignoreHide = false;

    void onItemPressed(const QModelIndex &index);
};

#endif // CHECKABLECOMBOBOX_H
