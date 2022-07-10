#ifndef ATTRIBUTETABITEM_H
#define ATTRIBUTETABITEM_H

#include "comboBoxWithElidedContents.h"
#include "dataOptions.h"
#include "gruepr_globals.h"
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

    explicit attributeTabItem(TabType tabType = gruepr, int tabNum = 0, QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);
    void updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts={});
    void setTabNum(int tabNum);

    QTextEdit *attributeText = nullptr;
    QDoubleSpinBox *weight = nullptr;
    QCheckBox *homogeneous = nullptr;
    QCheckBox *allowMultipleResponses = nullptr;
    QPushButton *requiredButton = nullptr;
    QPushButton *incompatsButton = nullptr;
    ComboBoxWithElidedContents *attributeResponses = nullptr;
    QPushButton *closeButton = nullptr;

signals:
    void closeRequested(int index);

private:
    QGridLayout *theGrid = nullptr;
    QLabel *weightPreLabel = nullptr;
    QLabel *weightPostLabel = nullptr;
    int index = 0;
};

#endif // ATTRIBUTETABITEM_H
