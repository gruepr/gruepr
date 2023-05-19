#ifndef SURVEYMAKERQUESTIONWITHSWITCH_H
#define SURVEYMAKERQUESTIONWITHSWITCH_H

#include "switchButton.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>

class SurveyMakerQuestionWithSwitch : public QFrame
{
    Q_OBJECT

public:
    explicit SurveyMakerQuestionWithSwitch(QWidget *parent = nullptr, QString textLabel = "", bool startingValue = false);
    ~SurveyMakerQuestionWithSwitch() override;

    void setLabel(QString text);
    void setValue(bool value);
    bool value();

    void addWidget(QWidget *widget, int row, int column, bool wholeRow, Qt::Alignment alignment = Qt::Alignment());

    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;
    void setEnabled(bool);

signals:
    void valueChanged(bool newvalue);

private slots:
    void valueChange(bool newvalue);

private:
    QLabel *label = nullptr;
    SwitchButton *switchButton = nullptr;
    QGridLayout *layout = nullptr;

    bool _enabled;
    int _extraWidgetsIndex;
};

#endif // SURVEYMAKERQUESTIONWITHSWITCH_H
