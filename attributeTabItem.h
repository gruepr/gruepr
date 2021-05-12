#ifndef ATTRIBUTETABITEM_H
#define ATTRIBUTETABITEM_H

#include <QWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "comboBoxWithElidedContents.h"
#include "gruepr_structs_and_consts.h"

class attributeTabItem : public QWidget
{
Q_OBJECT

public:
    enum TabType {gruepr, surveyMaker};

    explicit attributeTabItem(TabType tabType = gruepr, QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);

    QTextEdit *attributeText = nullptr;
    QDoubleSpinBox *weight = nullptr;
    QCheckBox *homogeneous = nullptr;
    QPushButton *incompatsButton = nullptr;
    ComboBoxWithElidedContents *attributeResponses = nullptr;

signals:

private:
    QGridLayout *theGrid = nullptr;
    QLabel *weightLabel = nullptr;
};

#endif // ATTRIBUTETABITEM_H
