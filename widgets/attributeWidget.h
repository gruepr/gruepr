#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "comboBoxWithElidedContents.h"
#include "dataOptions.h"
#include "teamingOptions.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

class AttributeWidget : public QWidget
{
Q_OBJECT

public:
    explicit AttributeWidget(QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);
    void updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts={});

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
    int index = 0;

    const QString HOMOGENTOOLTIP = tr("If selected, all of the students on a team will have a similar response to this question.\n"
                                      "If unselected, the students on a team will have a wide range of responses to this question.");
    const QString REQUIREDTOOLTIP = tr("<html>Indicate response(s) where each team should have at least one student with that value.<br>"
                                       "Examples:<br>Every team should have (at least) one student who is trained to use the library database.<br>"
                                       "Every team should have (at least) one student who said they want to be a team leader.</html>");
    const QString INCOMPATTOOLTIP = tr("<html>Indicate responses that should prevent students from being on the same team.<br>"
                                       "Examples:<br>No team should have an English major with a History major.<br>"
                                       "No team should have two (or more) students who said they want to be a team leader.</html>");
    const QString ONLYONETOOLTIP = tr("With only one response value, this attribute cannot be used for teaming");
};

#endif // ATTRIBUTEWIDGET_H
