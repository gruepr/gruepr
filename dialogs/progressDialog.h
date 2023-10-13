#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include "widgets/boxwhiskerplot.h"
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

class progressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit progressDialog(const QString &currSection = "", QChartView *chart = nullptr, QWidget *parent = nullptr);
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
    QVBoxLayout *layout = nullptr;
    QLabel *statusText = nullptr;
    QLabel *explanationText = nullptr;
    QProgressBar *progressBar = nullptr;
    QLabel *actionText = nullptr;
    QCheckBox *onlyStopManually = nullptr;
    QPushButton *stopHere = nullptr;
    QPushButton *showStatsButton = nullptr;
    QTimer *countdownToClose = nullptr;
    inline static const int SECSINCOUNTDOWNTIMER = 6;
    int secsLeftToClose = SECSINCOUNTDOWNTIMER;
    inline static const int CHARTHEIGHT = 400;
    inline static const QSize ICONSIZE = QSize(30,30);
};

#endif // PROGRESSDIALOG_H
