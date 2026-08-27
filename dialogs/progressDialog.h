#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(const QString &currSection = "", QWidget *chart = nullptr, QWidget *parent = nullptr);
    ~ProgressDialog() override;
    ProgressDialog(const ProgressDialog&) = delete;
    ProgressDialog operator= (const ProgressDialog&) = delete;
    ProgressDialog(ProgressDialog&&) = delete;
    ProgressDialog& operator= (ProgressDialog&&) = delete;

    void setText(const QString &text = "", int generation = 0, float score = 0, bool autostopInProgress = false);
    void highlightStopButton();
    bool continuingManually() const;

    // Passed as setText()'s "generation" when there are no more generations left to run and a bounded
    // final local search is instead refining the single best genome -- shows "Finalizing" in place of
    // a generation number.
    static constexpr int FINALIZING = -1;

private slots:
    void statsButtonPushed(QWidget *chart);
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
