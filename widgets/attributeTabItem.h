#ifndef ATTRIBUTETABITEM_H
#define ATTRIBUTETABITEM_H

#include "comboBoxWithElidedContents.h"
#include "dataOptions.h"
#include "gruepr_consts.h"
#include "teamingOptions.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

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
    QCheckBox *allowMultipleResponses = nullptr;
    QPushButton *requiredButton = nullptr;
    QPushButton *incompatsButton = nullptr;
    ComboBoxWithElidedContents *attributeResponses = nullptr;

private:
    QGridLayout *theGrid = nullptr;
    QLabel *weightLabel = nullptr;
};

#endif // ATTRIBUTETABITEM_H
