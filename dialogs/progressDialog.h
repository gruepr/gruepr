#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include "boxwhiskerplot.h"

class progressDialog : public QDialog
{
    Q_OBJECT

public:
    progressDialog(QtCharts::QChartView *chart = nullptr, QWidget *parent = nullptr);
    ~progressDialog();

    void setText(const QString &text = "", int generation = 0, float score = 0, bool autostopInProgress = false);
    void highlightStopButton();

private slots:
    void statsButtonPushed(QtCharts::QChartView *chart);
    void updateCountdown();
    void reject();

signals:
    void letsStop();

private:
    bool graphShown;
    QGridLayout *theGrid;
    QLabel *statusText;
    QLabel *explanationText;
    QLabel *explanationIcon;
    QCheckBox *onlyStopManually;
    QPushButton *stopHere;
    QPushButton *showStatsButton;
    QTimer *countdownToClose;
    const int SECSINCOUNTDOWNTIMER = 5;
    int secsLeftToClose = SECSINCOUNTDOWNTIMER;
    const int CHARTHEIGHT = 400;
};

#endif // PROGRESSDIALOG_H
