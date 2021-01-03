#include "categorialSpinBox.h"


//////////////////
// QSpinBox that replaces numerical values with categorical attribute responses in display
//////////////////
void CategoricalSpinBox::setCategoricalValues(const QStringList &categoricalValues)
{
    this->categoricalValues = categoricalValues;
}

QString CategoricalSpinBox::textFromValue(int value) const
{
    return ((value > 0) ? (value <= 26 ? QString(char(value + 'A' - 1)) :
                                         QString(char((value - 1)%26 + 'A')).repeated(1 + ((value-1)/26))) + " - " + categoricalValues.at(value - 1) : "0");
}

int CategoricalSpinBox::valueFromText(const QString &text) const
{
    return (categoricalValues.indexOf(text.split(" - ").last()) + 1);
}

QValidator::State CategoricalSpinBox::validate (QString &input, int &pos) const
{
    (void)input;
    (void)pos;
    return QValidator::Acceptable;
}
