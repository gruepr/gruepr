#ifndef CATEGORICALSPINBOX
#define CATEGORICALSPINBOX

// a subclassed QSpinBox that replaces numerical values with categorical attribute responses in display

#include <QSpinBox>
#include <QLineEdit>


class CategoricalSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    enum typeOfValue{letter, numerical};
    CategoricalSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {lineEdit()->setReadOnly(true);}
    void setWhatTypeOfValue(typeOfValue tOV) {whatTypeOfValue = tOV;};
    void setCategoricalValues(const QStringList &categoricalValues);
    QString textFromValue(int value) const;
    int valueFromText(const QString &text) const;
    QValidator::State validate (QString &input, int &pos) const;

private:
    QStringList categoricalValues;
    typeOfValue whatTypeOfValue = letter;
    void stepBy(int steps);
};


#endif // CATEGORICALSPINBOX
