#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "dataOptions.h"
#include "teamingOptions.h"
#include "widgets/switchButton.h"
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

class AttributeWidget : public QWidget
{
Q_OBJECT

public:
    explicit AttributeWidget(QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);
    void updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts={});

    QLabel *questionLabel = nullptr;
    QLabel *responsesLabel = nullptr;
    QFrame *responsesFrame = nullptr;
    QDoubleSpinBox *weight = nullptr;
    SwitchButton *homogeneous = nullptr;
    QLabel *homogenLabel = nullptr;
    QPushButton *requiredIncompatsButton = nullptr;

signals:
    void closeRequested(int index);

private:
    QGridLayout *theGrid = nullptr;
    QLabel *weightPreLabel = nullptr;
    int index = 0;

    const QString HOMOGENTOOLTIP = tr("If selected, all of the students on a team will have a similar response to this question.\n"
                                      "If unselected, the students on a team will have a wide range of responses to this question.");
    const QString REQUIREDINCOMPATTOOLTIP = tr("<html>Create rules for required responses (each team must have at least one student with that response)<br>"
                                                "or incompatible responses (values that should prevent two students from being on the same team). <br><br>"
                                               "Examples:<ui><li>Every team should have (at least) one student who is trained to use the library database.</li>"
                                                "<li>Every team should have (at least) one student who said they want to be a team leader.</li>"
                                                "<li>No team should have an English major with a History major.</li>"
                                                "<li>No team should have two (or more) students who said they want to be a team leader.</li></ul></html>");
    const QString ONLYONETOOLTIP = tr("With only one response value, this attribute cannot be used for teaming");
};

#endif // ATTRIBUTEWIDGET_H
