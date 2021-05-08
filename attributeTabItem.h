#ifndef ATTRIBUTETABITEM_H
#define ATTRIBUTETABITEM_H

#include <QWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "gruepr_structs_and_consts.h"

class attributeTabItem : public QWidget
{
Q_OBJECT

public:
    explicit attributeTabItem(QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);

    QDoubleSpinBox *weight = nullptr;
    QCheckBox *homogeneous = nullptr;
    QPushButton *incompatsButton = nullptr;

signals:

private:
    QGridLayout *theGrid = nullptr;
    QLabel *weightLabel = nullptr;
    QTextEdit *attributeText = nullptr;
};

#endif // ATTRIBUTETABITEM_H
