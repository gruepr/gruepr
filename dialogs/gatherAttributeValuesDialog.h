#ifndef GATHERATTRIBUTEVALUESDIALOG_H
#define GATHERATTRIBUTEVALUESDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include "dataOptions.h"
#include "teamingOptions.h"

class gatherAttributeValuesDialog : public QDialog
{
    Q_OBJECT

public:
    enum GatherType{required, incompatible};

    gatherAttributeValuesDialog(const int attribute, const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions, const GatherType gathertype,
                                QWidget *parent = nullptr);
    ~gatherAttributeValuesDialog();

    QVector<int> requiredValues;
    QVector< QPair<int,int> > incompatibleValues;

private slots:
    void addValues();
    void clearAllValues();

private:
    void updateExplanation();
    GatherType gatherType;
    int numPossibleValues;
    QGridLayout *theGrid;
    QLabel *attributeQuestion;
    QLabel *selectOneExplanation;
    QLabel *selectMultipleExplanation;
    QRadioButton *selectOneValues;
    QPushButton *selectOneResponses;
    QButtonGroup *selectOneValuesGroup;
    QCheckBox *selectMultipleValues;
    QPushButton *selectMultipleResponses;
    QPushButton *addValuesButton;
    QPushButton *resetValuesButton;
    QDialogButtonBox *buttonBox;
    QLabel *explanation;
};

#endif // GATHERATTRIBUTEVALUESDIALOG_H
