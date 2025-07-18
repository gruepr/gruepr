#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "dataOptions.h"
#include "teamingOptions.h"
#include <widgets/frameThatForwardsMouseClicks.h>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class ResponseLabelBarGraph;

class AttributeWidget : public QWidget
{
Q_OBJECT

public:
    explicit AttributeWidget(QWidget *parent = nullptr);

    void setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions);
    void updateResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts={});

    QPushButton *setRequiredValuesButton = nullptr;
    QPushButton *setIncompatibleValuesButton = nullptr;
    FrameThatForwardsMouseClicks *diverseCard = nullptr;
    FrameThatForwardsMouseClicks *similarCard = nullptr;
    QRadioButton *similarButton = nullptr;
    QRadioButton *diverseButton = nullptr;

private:
    QVBoxLayout *responsesLayout = nullptr;
    QList<QWidget*> responseRows;

    inline static const int BARGRAPHWIDTH = 40;
    inline static const QString DIVERSETOOLTIP = QObject::tr("Teammates will have a range of responses to this question.");
    inline static const QString SIMILARTOOLTIP = QObject::tr("Teammates will have similar responses to this question.");
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
