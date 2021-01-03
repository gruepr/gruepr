#ifndef CATEGORICALSPINBOX
#define CATEGORICALSPINBOX

// a subclassed QSpinBox that replaces numerical values with categorical attribute responses in display

#include <QSpinBox>


class CategoricalSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    CategoricalSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {}
    void setCategoricalValues(const QStringList &categoricalValues);
    QString textFromValue(int value) const;
    int valueFromText(const QString &text) const;
    QValidator::State validate (QString &input, int &pos) const;

private:
    QStringList categoricalValues;
};


#endif // CATEGORICALSPINBOX
