#ifndef BASETIMEZONEDIALOG_H
#define BASETIMEZONEDIALOG_H

#include <QDialog>
#include <QComboBox>

class baseTimezoneDialog : public QDialog
{
    Q_OBJECT

public:
    baseTimezoneDialog(QWidget *parent = nullptr);
    float baseTimezoneVal = 0;

private:
    QComboBox *timezones = nullptr;
};


#endif // BASETIMEZONEDIALOG_H
