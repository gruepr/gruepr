#ifndef COMBOBOXWITHELIDEDCONTENTS
#define COMBOBOXWITHELIDEDCONTENTS

// a subclassed combobox that paints with elided contents

#include <QComboBox>


class ComboBoxWithElidedContents : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxWithElidedContents(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // COMBOBOXWITHELIDEDCONTENTS
