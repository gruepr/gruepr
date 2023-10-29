#ifndef ATTRIBUTERULESDIALOG_H
#define ATTRIBUTERULESDIALOG_H

#include <QDialog>
#include "dataOptions.h"
#include "teamingOptions.h"
#include <QCheckBox>

namespace Ui {
class AttributeRulesDialog;
}

struct AttributeValue
{
    int value;
    QString response;
};

class AttributeRulesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttributeRulesDialog(const int attribute, const DataOptions &dataOptions, const TeamingOptions &teamingOptions, QWidget *parent = nullptr);
    ~AttributeRulesDialog() override;
    AttributeRulesDialog(const AttributeRulesDialog&) = delete;
    AttributeRulesDialog operator= (const AttributeRulesDialog&) = delete;
    AttributeRulesDialog(AttributeRulesDialog&&) = delete;
    AttributeRulesDialog& operator= (AttributeRulesDialog&&) = delete;

    QList<int> requiredValues;
    QList< QPair<int,int> > incompatibleValues;

private slots:
    void Ok();
    void clearAllValues();

private:
    Ui::AttributeRulesDialog *ui;
    QList<AttributeValue> attributeValues;
    int numPossibleValues;
    QString valuePrefix(int value, DataOptions::AttributeType attributeType);
    QList< QList<QCheckBox *> > incompCheckboxList;
    QList<QCheckBox *> reqCheckboxes;
};

#endif // ATTRIBUTERULESDIALOG_H
