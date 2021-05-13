#ifndef COMBOBOXWITHELIDEDCONTENTS
#define COMBOBOXWITHELIDEDCONTENTS

// a subclassed combobox that paints with elided contents

#include <QComboBox>


class ComboBoxWithElidedContents : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxWithElidedContents(const QString &minWidthText, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString minText;
    const int PIXELS_TO_ADD_TO_MIN_TEXT = 15;
};

#endif // COMBOBOXWITHELIDEDCONTENTS
