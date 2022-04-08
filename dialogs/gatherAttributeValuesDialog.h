#ifndef GATHERATTRIBUTEVALUESDIALOG_H
#define GATHERATTRIBUTEVALUESDIALOG_H

#include "dataOptions.h"
#include "teamingOptions.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

struct AttributeValue
{
    int value;
    QString response;
};

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
    QString valuePrefix(int value);
    void updateExplanation();
    GatherType gatherType;
    DataOptions::AttributeType attributeType;
    QVector<AttributeValue> attributeValues;
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

    inline static const QChar BULLET = QChar(0x2022);
    inline static const QChar DOUBLEARROW = QChar(0x27f7);
};

#endif // GATHERATTRIBUTEVALUESDIALOG_H
