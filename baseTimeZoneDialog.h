#ifndef BASETIMEZONEDIALOG_H
#define BASETIMEZONEDIALOG_H

#include <QDialog>
#include <QComboBox>
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
    QGridLayout *theGrid;
    QLabel *explanation;
    QComboBox *timezones;
    QDialogButtonBox *buttonBox;
};


#endif // BASETIMEZONEDIALOG_H
