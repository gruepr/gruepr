#ifndef SURVEYMAKERQUESTIONWITHSWITCH_H
#define SURVEYMAKERQUESTIONWITHSWITCH_H

#include "switchButton.h"
#include <QFrame>
#include <QGridLayout>
#include <QLabel>

class SurveyMakerQuestionWithSwitch : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(bool value READ getValue WRITE setValue NOTIFY valueChanged)

public:
    explicit SurveyMakerQuestionWithSwitch(QWidget *parent = nullptr, const QString &textLabel = "", bool startingValue = false);
    ~SurveyMakerQuestionWithSwitch() override;

    void setLabel(const QString &text);
    void setValue(bool value);
    bool getValue() const;

    void addWidget(QWidget *widget, int row, int column, bool expandToRestOfRow, Qt::Alignment alignment = Qt::Alignment());

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
};

#endif // SURVEYMAKERQUESTIONWITHSWITCH_H
