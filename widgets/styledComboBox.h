#ifndef STYLEDCOMBOBOX_H
#define STYLEDCOMBOBOX_H

#include "gruepr_globals.h"
#include <QComboBox>
#include <QAbstractItemView>
#include <QStyleFactory>

class StyledComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit StyledComboBox(QWidget *parent = nullptr) : QComboBox(parent)
    {
        setStyleSheet(COMBOBOXSTYLE);
        view()->setStyle(QStyleFactory::create("Fusion"));
        view()->setStyleSheet(SCROLLBARSTYLE);
    }

};

#endif // STYLEDCOMBOBOX_H
