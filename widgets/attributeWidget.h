#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "dataOptions.h"
#include "qradiobutton.h"
#include "teamingOptions.h"
#include <widgets/frameThatForwardsMouseClicks.h>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class AttributeCriterion;


class AttributeWidget : public QWidget
{
Q_OBJECT

public:
explicit AttributeWidget(int attribute, const DataOptions *const incomingDataOptions, TeamingOptions *const incomingTeamingOptions,
                             AttributeCriterion *incomingCriterion, DataOptions::AttributeType attributeType, QWidget *parent = nullptr);

    void setValues(bool updateResponsesToo = true);

    QPushButton *setRequiredValuesButton = nullptr;
    QPushButton *setIncompatibleValuesButton = nullptr;
    FrameThatForwardsMouseClicks *diverseCard = nullptr;
    FrameThatForwardsMouseClicks *similarCard = nullptr;
    FrameThatForwardsMouseClicks *averageCard = nullptr;
    QRadioButton *diverseButton = nullptr;
    QRadioButton *similarButton = nullptr;
    QRadioButton *averageButton = nullptr;
    QDoubleSpinBox *minimumSpinBox = nullptr;
    QDoubleSpinBox *maximumSpinBox = nullptr;

private:
    const int attribute;
    const DataOptions::AttributeType attributeType;
    const DataOptions *const dataOptions;
    TeamingOptions *const teamingOptions;
    AttributeCriterion *criterion = nullptr;

    QVBoxLayout *responsesLayout = nullptr;
    QList<QWidget*> responseRows;
    void updateResponses(const std::map<QString, int> &responseCounts={});

    inline static const int BARGRAPHWIDTH = 40;
    inline static const QString DIVERSETOOLTIP = QObject::tr("Teammates will have a range of responses to this question.");
    inline static const QString SIMILARTOOLTIP = QObject::tr("Teammates will have similar responses to this question.");
    inline static const QString AVERAGETOOLTIP = QObject::tr("The team's average response will be close to the class average.");
    inline static const QString REQUIREDTOOLTIP = QObject::tr("<html>Enforce that each team have at least one student with a particular response.<br><br>"
                                                              "Examples:<ui><li>Every team should have (at least) one student who is trained to use the library database.</li>"
                                                              "<li>Every team should have (at least) one student who said they want to be a team leader.</li></ul></html>");
    inline static const QString INCOMPATTOOLTIP = QObject::tr("<html>Prevent teams from having students with incompatible responses. <br><br>"
                                                              "Examples:<ui><li>No team should have an English major with a History major.</li>"
                                                              "<li>No team should have more than one student who said they want to be a team leader.</li></ul></html>");
    inline static const QString ONLYONETOOLTIP = QObject::tr("With only one response value, this question cannot be used for teaming");
};


class ResponseLabelBarGraph : public QWidget
{
public:
    ResponseLabelBarGraph(int value, int maxValue, int barWidth, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    int m_value;
    int m_maxValue;
    int m_barWidth;
};

#endif // ATTRIBUTEWIDGET_H
