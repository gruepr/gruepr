#include "progressDialog.h"
#include "gruepr_globals.h"
#include <QMovie>
#include <QPushButton>
#include <QTimer>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show progress in optimization
/////////////////////////////////////////////////////////////////////////////////////////////////////////

progressDialog::progressDialog(const QString &currSection, QChartView *chart, QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(currSection.isEmpty() ? tr("Grueping...") : tr("Grueping ") + currSection + "...");
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint);
    setSizeGripEnabled(true);
    setModal(true);
    theGrid = new QGridLayout(this);

    statusText = new QLabel(this);
    QFont defFont("Oxygen Mono");
    statusText->setFont(defFont);
    statusText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(statusText, 0, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);

    explanationIcon = new QLabel(this);
    explanationIcon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    auto *movie = new QMovie(":/icons/loading.gif", "GIF", this);
    movie->setParent(explanationIcon);
    explanationIcon->setMovie(movie);
    movie->start();

    explanationText = new QLabel(this);
    explanationText->setFont(defFont);
    explanationText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto *explanationBox = new QHBoxLayout;
    theGrid->addLayout(explanationBox, 1, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    explanationBox->addWidget(explanationIcon, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    explanationBox->addWidget(explanationText, 0, Qt::AlignLeft | Qt::AlignVCenter);
    explanationBox->addStretch(1);

    theGrid->setRowMinimumHeight(2, DIALOG_SPACER_ROWHEIGHT);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(line, 3, 0, 1, -1);

    if(chart != nullptr)
    {
        theGrid->addWidget(chart, 6, 0, 1, -1);
        chart->hide();
        graphShown = false;

        showStatsButton = new QPushButton(QIcon(":/icons/down_arrow.png"), "Show progress", this);
        showStatsButton->setIconSize(SHOWPROGRESSICONSIZE);
        showStatsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(showStatsButton, &QPushButton::clicked, this, [this, chart] {statsButtonPushed(chart);});
        theGrid->addWidget(showStatsButton, 4, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
        theGrid->setColumnStretch(1,1);
    }

    onlyStopManually = new QCheckBox("Continue optimizing\nuntil I manually stop it.", this);
    onlyStopManually->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    theGrid->addWidget(onlyStopManually, 4, 2, 1, 1, Qt::AlignRight | Qt::AlignVCenter);

    stopHere = new QPushButton(QIcon(":/icons/stop.png"), "Stop\nnow", this);
    stopHere->setIconSize(STOPNOWICONSIZE);
    stopHere->setToolTip(tr("Stop the optimization process immediately and show the best set of teams found so far."));
    stopHere->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(stopHere, &QPushButton::clicked, this, [this] {emit letsStop();});
    theGrid->addWidget(stopHere, 4, 3, 1, -1, Qt::AlignRight | Qt::AlignVCenter);

    countdownToClose = new QTimer(this);
    setText("");
    adjustSize();
}

void progressDialog::setText(const QString &text, int generation, float score, bool autostopInProgress)
{
    QString explanation = "<html>" + tr("Generation ") + QString::number(generation) + " - "
                          + tr("Top Score = ") + (score < 0? "<span style=\"font-family:'Arial'\"> - </span>": "") + QString::number(std::abs(score)) +
                          "<br><span style=\"color:" + (autostopInProgress? "green" : "black") + ";\">" + text;
    if(autostopInProgress && !onlyStopManually->isChecked())
    {
        explanation += "<br>" + tr("Optimization will stop in ") + QString::number(secsLeftToClose) + tr(" seconds.");
    }
    explanation += "</span></html>";
    explanationText->setText(explanation);

    if(autostopInProgress)
    {
        explanationIcon->setPixmap(QIcon(":/icons/ok.png").pixmap(OKICONSIZE));
    }

    if(autostopInProgress && !onlyStopManually->isChecked())
    {
        statusText->setText(tr("Status: Finalizing..."));
    }
    else
    {
        statusText->setText(tr("Status: Optimizing..."));
    }
}

void progressDialog::highlightStopButton()
{
    stopHere->setFocus();

    if(countdownToClose->isActive() || onlyStopManually->isChecked())
    {
        return;
    }

    connect(countdownToClose, &QTimer::timeout, this, &progressDialog::updateCountdown);
    countdownToClose->start(std::chrono::seconds(1));
}

void progressDialog::updateCountdown()
{
    if(onlyStopManually->isChecked())
    {
        secsLeftToClose = SECSINCOUNTDOWNTIMER;
        return;
    }

    secsLeftToClose--;
    explanationText->setText(explanationText->text().replace(QRegularExpression(tr("stop in ") + "\\d*"), tr("stop in ") +  QString::number(std::max(0, secsLeftToClose))));
    if(secsLeftToClose == 0)
    {
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
    if(graphShown)
    {
        chart->show();
        height = CHARTHEIGHT;
        width = numHorizontalAxisMarkers * QFontMetrics(QFont("Oxygen Mono", QFont("Oxygen Mono").pointSize() - 2)).horizontalAdvance("XX  ");
        icon = QIcon(":/icons/up_arrow.png");
        butText = "Hide progress";
    }
    else
    {
        chart->hide();
        icon = QIcon(":/icons/down_arrow.png");
        butText = "Show progress";
    }
    int chartRow, chartCol, x;
    theGrid->getItemPosition(theGrid->indexOf(chart), &chartRow, &chartCol, &x, &x);
    theGrid->setRowMinimumHeight(chartRow, height);
    theGrid->setColumnMinimumWidth(chartCol, width);
    showStatsButton->setIcon(icon);
    showStatsButton->setText(butText);
    adjustSize();
}

progressDialog::~progressDialog()
{
    countdownToClose->stop();
}
