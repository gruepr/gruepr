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
    progressDialog(const QString &currSection = "", QChartView *chart = nullptr, QWidget *parent = nullptr);
    ~progressDialog();
    progressDialog(const progressDialog&) = delete;
    progressDialog operator= (const progressDialog&) = delete;
    progressDialog(progressDialog&&) = delete;
    progressDialog& operator= (progressDialog&&) = delete;

    void setText(const QString &text = "", int generation = 0, float score = 0, bool autostopInProgress = false);
    void highlightStopButton();

private slots:
    void statsButtonPushed(QChartView *chart);
    void updateCountdown();
    void reject() override;

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
    inline static const int SECSINCOUNTDOWNTIMER = 5;
    int secsLeftToClose = SECSINCOUNTDOWNTIMER;
    inline static const int CHARTHEIGHT = 400;
    inline static const QSize SHOWPROGRESSICONSIZE = QSize(20,20);
    inline static const QSize STOPNOWICONSIZE = QSize(30,30);
    inline static const QSize OKICONSIZE = QSize(25,25);
};

#endif // PROGRESSDIALOG_H
