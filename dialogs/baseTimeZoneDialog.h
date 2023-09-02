#ifndef BASETIMEZONEDIALOG_H
#define BASETIMEZONEDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

class baseTimezoneDialog : public QDialog
{
    Q_OBJECT

public:
    baseTimezoneDialog(QWidget *parent = nullptr);
    float baseTimezoneVal = 0;

private:
    QGridLayout *theGrid = nullptr;
    QLabel *explanation = nullptr;
    QComboBox *timezones = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
};


#endif // BASETIMEZONEDIALOG_H
