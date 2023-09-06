#ifndef TEAMMATESRULESDIALOG_H
#define TEAMMATESRULESDIALOG_H

#include <QDialog>

namespace Ui {
class TeammatesRulesDialog;
}

class TeammatesRulesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TeammatesRulesDialog(QWidget *parent = nullptr);
    ~TeammatesRulesDialog();
    TeammatesRulesDialog(const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog operator= (const TeammatesRulesDialog&) = delete;
    TeammatesRulesDialog(TeammatesRulesDialog&&) = delete;
    TeammatesRulesDialog& operator= (TeammatesRulesDialog&&) = delete;


private:
    Ui::TeammatesRulesDialog *ui;

    void Ok();
    void clearAllValues();
};

#endif // TEAMMATESRULESDIALOG_H
