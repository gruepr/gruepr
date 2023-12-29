#include "progressDialog.h"
#include "gruepr_globals.h"
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QTimer>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show progress in optimization
/////////////////////////////////////////////////////////////////////////////////////////////////////////

progressDialog::progressDialog(const QString &currSection, QChartView *chart, QWidget *parent)
    :QDialog (parent)
{
    //Set up window
    setWindowTitle(currSection.isEmpty() ? tr("Grueping...") : tr("Grueping ") + currSection + "...");
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint);
    setModal(true);
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    statusText = new QLabel(tr("Status: Optimizing..."), this);
    statusText->setStyleSheet(QString(LABEL14PTSTYLE).replace("font-size", "font-weight: bold; font-size"));
    layout->addWidget(statusText);

    explanationText = new QLabel(tr("Generation 0 - Top Score = 0"), this);
    explanationText->setStyleSheet(LABEL10PTSTYLE);
    layout->addWidget(explanationText);

    progressBar = new QProgressBar(this);
    progressBar->setStyleSheet(PROGRESSBARSTYLE);
    progressBar->setMaximumHeight(8);
    progressBar->setTextVisible(false);
    progressBar->setRange(0, PROGRESSBARMAX);
    progressBar->setValue(0);
    layout->addWidget(progressBar);

    actionText = new QLabel(tr("Please wait while your grueps are created!"), this);
    actionText->setStyleSheet(QString(LABEL14PTSTYLE).replace("color: " DEEPWATERHEX ";", "color: " OPENWATERHEX ";"));
    layout->addWidget(actionText);

    auto *buttonBox = new QDialogButtonBox(this);

    if(chart != nullptr) {
        layout->addWidget(chart);
        chart->hide();
        graphShown = false;


        showStatsButton = buttonBox->addButton(QDialogButtonBox::Reset);
        showStatsButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        showStatsButton->setText(tr("Show progress"));
        showStatsButton->setIcon(QIcon(":/icons_new/downButton.png"));
        showStatsButton->setLayoutDirection(Qt::RightToLeft);   // icon on right side
        connect(showStatsButton, &QPushButton::clicked, this, [this, chart] {statsButtonPushed(chart);});
    }

    onlyStopManually = new QCheckBox("Continue optimizing until end is pressed", this);
    onlyStopManually->setStyleSheet(CHECKBOXSTYLE);
    layout->addWidget(onlyStopManually);

    stopHere = buttonBox->addButton(QDialogButtonBox::Ok);
    stopHere->setStyleSheet(SMALLBUTTONSTYLE);
    stopHere->setText(tr("End optimization"));
    QPixmap whiteStop = QPixmap(":/icons_new/close.png").scaled(ICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&whiteStop);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteStop.rect(), QColor("white"));
    painter.end();
    stopHere->setIcon(whiteStop);
    stopHere->setToolTip(tr("Stop the optimization process immediately and show the best set of teams found so far."));
    connect(stopHere, &QPushButton::clicked, this, [this] {emit letsStop();});
    layout->addWidget(buttonBox);

    countdownToClose = new QTimer(this);
    adjustSize();
}

void progressDialog::setText(const QString &text, int generation, float score, bool autostopInProgress)
{
    explanationText->setText(tr("Generation ") + QString::number(generation) + " - " + tr("Top Score = ") + QString::number(score));
    QString action = text;
    if(autostopInProgress && !onlyStopManually->isChecked()) {
        action += tr("\nOptimization will end in ") + QString::number(secsLeftToClose) + tr(" seconds.");
        score = PROGRESSBARMAX - ((PROGRESSBARMAX - score) * secsLeftToClose / SECSINCOUNTDOWNTIMER);
    }
    actionText->setText(action);
    progressBar->setValue(score);

    if(autostopInProgress && !onlyStopManually->isChecked()) {
        statusText->setText(tr("Status: Finalizing..."));
    }
    else {
        statusText->setText(tr("Status: Optimizing..."));
    }
}

void progressDialog::highlightStopButton()
{
    stopHere->setFocus();

    if(countdownToClose->isActive() || onlyStopManually->isChecked()) {
        return;
    }

    connect(countdownToClose, &QTimer::timeout, this, &progressDialog::updateCountdown);
    countdownToClose->start(std::chrono::seconds(1));
}

void progressDialog::updateCountdown()
{
    if(onlyStopManually->isChecked()) {
        secsLeftToClose = SECSINCOUNTDOWNTIMER;
        return;
    }

    secsLeftToClose--;
    static const QRegularExpression stopTime(tr("stop in ") + "\\d*");
    explanationText->setText(explanationText->text().replace(stopTime, tr("stop in ") +  QString::number(std::max(0, secsLeftToClose))));
    if(secsLeftToClose == 0) {
        progressBar->setValue(PROGRESSBARMAX);
        stopHere->animateClick();
    }
}

void progressDialog::reject()
{
    // If closing the window with click on close or hitting 'Esc', stop the optimization, too
    stopHere->animateClick();
    QDialog::reject();
}

void progressDialog::statsButtonPushed(QChartView *chart)
{
    graphShown = !graphShown;

    int height = 0, width = 0;
    const int numHorizontalAxisMarkers = 14;
    QIcon icon;
    QString butText;
    if(graphShown) {
        chart->show();
        height = CHARTHEIGHT;
        width = numHorizontalAxisMarkers * QFontMetrics(QFont("Oxygen Mono", QFont("Oxygen Mono").pointSize() - 2)).horizontalAdvance("XX  ");
        icon = QIcon(":/icons_new/upButton.png");
        butText = "Hide progress";
    }
    else {
        chart->hide();
        icon = QIcon(":/icons_new/downButton.png");
        butText = "Show progress";
    }
    chart->setMinimumSize(QSize(width, height));
    showStatsButton->setIcon(icon);
    showStatsButton->setText(butText);
    adjustSize();
}

progressDialog::~progressDialog()
{
    countdownToClose->stop();
}
