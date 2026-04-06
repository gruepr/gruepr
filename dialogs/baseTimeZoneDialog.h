#ifndef BASETIMEZONEDIALOG_H
#define BASETIMEZONEDIALOG_H

#include <QDialog>
#include "widgets/styledComboBox.h"

class baseTimezoneDialog : public QDialog
{
    Q_OBJECT

public:
    baseTimezoneDialog(QWidget *parent = nullptr);
    float baseTimezoneVal = 0;

private:
    StyledComboBox *timezones = nullptr;
};


#endif // BASETIMEZONEDIALOG_H
