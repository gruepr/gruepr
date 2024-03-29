#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QChartView>
#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

class progressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit progressDialog(const QString &currSection = "", QChartView *chart = nullptr, QWidget *parent = nullptr);
    ~progressDialog() override;
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
    inline static const int PROGRESSBARMAX = 125;
};

#endif // PROGRESSDIALOG_H
