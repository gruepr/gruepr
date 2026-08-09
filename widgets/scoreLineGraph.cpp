#include "scoreLineGraph.h"
#include "gruepr_globals.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <algorithm>


ScoreLineGraph::ScoreLineGraph(const QString &title, const QString &xAxisTitle, const QString &yAxisTitle, QWidget *parent)
    : QWidget(parent), titleText(title), xAxisTitle(xAxisTitle), yAxisTitle(yAxisTitle)
{
    xAxisRange[axismin] = 0;
    xAxisRange[axismax] = MIN_XAXIS_SPAN;
    yAxisRange[axismin] = 0;
    yAxisRange[axismax] = 100;

    setMinimumSize(300, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}


void ScoreLineGraph::addScore(float maxScore)
{
    // pin very negative (penalized/infeasible) scores at a floor so the y-axis stays focused on 0-100
    const float pinnedScore = std::max(maxScore, NEGATIVE_SCORE_FLOOR);
    scores.append(pinnedScore);

    // expand x-axis to fit all generations seen so far
    xAxisRange[axismax] = std::max<long long>(xAxisRange[axismax], scores.count() - 1);

    // expand y-axis only as needed; it starts pinned to the meaningful 0-100 range
    yAxisRange[axismin] = std::min(yAxisRange[axismin], pinnedScore);
    yAxisRange[axismax] = std::max(yAxisRange[axismax], pinnedScore);

    update();
}


void ScoreLineGraph::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // fonts matching the original
    const QFont titleFont("Oxygen Mono", 10);
    QFont labelsFont("Oxygen Mono");
    labelsFont.setPointSize(titleFont.pointSize() - 2);
    const QFontMetrics labelsFM(labelsFont);
    const QFontMetrics titleFM(titleFont);

    // plot area
    const QRect plotRect(MARGIN_LEFT, MARGIN_TOP, width() - MARGIN_LEFT - MARGIN_RIGHT, height() - MARGIN_TOP - MARGIN_BOTTOM);
    if (plotRect.width() <= 0 || plotRect.height() <= 0) {
        return;
    }

    // background
    painter.fillRect(rect(), Qt::white);

    // title
    if (!titleText.isEmpty()) {
        painter.setFont(titleFont);
        painter.setPen(Qt::black);
        painter.drawText(QRect(0, 2, width(), MARGIN_TOP - 4), Qt::AlignHCenter | Qt::AlignVCenter, titleText);
    }

    // --- axes ---
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());   // x-axis
    painter.drawLine(plotRect.topLeft(), plotRect.bottomLeft());       // y-axis

    // y-axis ticks and labels
    const float yRange = yAxisRange[axismax] - yAxisRange[axismin];
    if (yRange > 0) {
        const float rawStep = yRange / 5.0F;
        const float magnitude = qPow(10.0F, qFloor(std::log10(rawStep)));
        const float residual = rawStep / magnitude;
        float niceStep;
        if (residual <= 1.5F) { niceStep = 1.0F * magnitude; }
        else if (residual <= 3.0F) { niceStep = 2.0F * magnitude; }
        else if (residual <= 7.0F) { niceStep = 5.0F * magnitude; }
        else { niceStep = 10.0F * magnitude; }

        painter.setFont(labelsFont);
        painter.setPen(QPen(Qt::black, 1));
        for (float v = yAxisRange[axismin]; v <= yAxisRange[axismax] + niceStep * 0.001F; v += niceStep) {
            const int y = plotRect.bottom() - static_cast<int>((v - yAxisRange[axismin]) / yRange * plotRect.height());
            painter.drawLine(plotRect.left() - 4, y, plotRect.left(), y);

            const QString label = QString::number(static_cast<double>(v), 'g', 4);
            const QRect labelRect(0, y - labelsFM.height() / 2, MARGIN_LEFT - 8, labelsFM.height());
            painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, label);

            // grid line
            painter.setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
            painter.drawLine(plotRect.left() + 1, y, plotRect.right(), y);
            painter.setPen(QPen(Qt::black, 1));
        }
    }

    // x-axis ticks and labels
    const int xRange = int(xAxisRange[axismax] - xAxisRange[axismin]);
    if (xRange > 0) {
        const int numTicks = 10;
        const int tickStep = std::max(1, xRange / numTicks);
        painter.setFont(labelsFont);
        painter.setPen(QPen(Qt::black, 1));
        for (long long i = xAxisRange[axismin]; i <= xAxisRange[axismax]; i += tickStep) {
            const int x = plotRect.left() + static_cast<int>(static_cast<double>(i - xAxisRange[axismin]) / xRange * plotRect.width());
            painter.drawLine(x, plotRect.bottom(), x, plotRect.bottom() + 4);

            const QString label = QString::number(i);
            const QRect labelRect(x - 20, plotRect.bottom() + 6, 40, labelsFM.height());
            painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, label);
        }
    }

    // axis titles
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    // x-axis title
    painter.drawText(QRect(MARGIN_LEFT, height() - titleFM.height() - 2, plotRect.width(), titleFM.height()),
                     Qt::AlignHCenter | Qt::AlignBottom, xAxisTitle);
    // y-axis title (rotated)
    painter.save();
    painter.translate(titleFM.height(), MARGIN_TOP + plotRect.height() / 2);
    painter.rotate(-90);
    painter.drawText(QRect(-plotRect.height() / 2, -titleFM.height(), plotRect.height(), titleFM.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter, yAxisTitle);
    painter.restore();

    // --- draw the score line ---
    if (scores.isEmpty() || yRange <= 0 || xRange <= 0) {
        return;
    }

    painter.setClipRect(plotRect);

    auto pointAt = [&](int generation, float score) -> QPointF {
        const double x = plotRect.left() + (static_cast<double>(generation) - xAxisRange[axismin]) / xRange * plotRect.width();
        const double y = plotRect.bottom() - (score - yAxisRange[axismin]) / yRange * plotRect.height();
        return {x, y};
    };

    QPolygonF polyline;
    polyline.reserve(scores.count());
    for (int generation = 0; generation < scores.count(); generation++) {
        polyline << pointAt(generation, scores[generation]);
    }

    painter.setPen(QPen(QColor::fromString(AQUAHEX), 2));
    painter.drawPolyline(polyline);

    painter.setClipping(false);
}
