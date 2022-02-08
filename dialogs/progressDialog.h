#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include "boxwhiskerplot.h"
#include <QCheckBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>

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
    const QSize SHOWPROGRESSICONSIZE = QSize(20,20);
    const QSize STOPNOWICONSIZE = QSize(30,30);
    const QSize OKICONSIZE = QSize(25,25);
};

#endif // PROGRESSDIALOG_H
