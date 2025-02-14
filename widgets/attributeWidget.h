#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "dataOptions.h"
#include "teamingOptions.h"
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

class AttributeWidget : public QWidget
{
Q_OBJECT

public:
    explicit AttributeWidget(QWidget *parent = nullptr);
    //This function sets the values for a particular attribute (multiple choice) question
    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);
    void updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts={});

    QLabel *questionLabel = nullptr;
    QLabel *responsesLabel = nullptr;
    QDoubleSpinBox *weight = nullptr;
    QSlider *attribute_diversity_slider = nullptr;
    QPushButton *requiredIncompatsButton = nullptr;

private:

    inline static const QString HOMOGENTOOLTIP = QObject::tr("If selected, all of the students on a team will have a similar response to this question.\n"
                                                             "If unselected, the students on a team will have a wide range of responses to this question.");
    inline static const QString REQUIREDINCOMPATTOOLTIP = QObject::tr("<html>Create rules for required responses (each team must have at least one student with that response)<br>"
                                                                      "or incompatible responses (values that should prevent two students from being on the same team). <br><br>"
                                                                      "Examples:<ui><li>Every team should have (at least) one student who is trained to use the library database.</li>"
                                                                      "<li>Every team should have (at least) one student who said they want to be a team leader.</li>"
                                                                      "<li>No team should have an English major with a History major.</li>"
                                                                      "<li>No team should have two (or more) students who said they want to be a team leader.</li></ul></html>");
    inline static const QString ONLYONETOOLTIP = QObject::tr("With only one response value, this question cannot be used for teaming");
};

#endif // ATTRIBUTEWIDGET_H
