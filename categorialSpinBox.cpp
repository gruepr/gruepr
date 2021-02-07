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
    if(whatTypeOfValue == letter)
    {
        return ((value > 0) ? (value <= 26 ? QString(char(value - 1 + 'A')) :
                                             QString(char((value - 1)%26 + 'A')).repeated(1 + ((value-1)/26))) + " - " + categoricalValues.at(value - 1) : "0");
    }
    return ((value > 0) ? categoricalValues.at(value - 1) : "0");
}

int CategoricalSpinBox::valueFromText(const QString &text) const
{
    if(whatTypeOfValue == letter)
    {
        return (categoricalValues.indexOf(text.split(" - ").last()) + 1);

    }
    return (categoricalValues.indexOf(text) + 1);
}

QValidator::State CategoricalSpinBox::validate (QString &input, int &pos) const
{
    (void)input;
    (void)pos;
    return QValidator::Acceptable;
}
