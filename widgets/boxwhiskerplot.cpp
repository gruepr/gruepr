#include "boxwhiskerplot.h"
#include "gruepr_globals.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <algorithm>


BoxWhiskerPlot::BoxWhiskerPlot(const QString &title, const QString &xAxisTitle, const QString &yAxisTitle, QWidget *parent)
    : QWidget(parent), titleText(title), xAxisTitle(xAxisTitle), yAxisTitle(yAxisTitle)
{
    xAxisRange[axismin] = 0;
    xAxisRange[axismax] = DATAWIDTH / PLOTFREQUENCY;

    setMinimumSize(300, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}


void BoxWhiskerPlot::loadNextVals(const float *const vals, const int *const orderedIndex, int count, const bool unpenalizedGenomePresent)
{
    // adds a new distribution to the plot; vals is not sorted, but the indexes in sorted order are given in orderedIndex
    const int NUM_VALS_NEEDED_FOR_BOX_AND_WHISKER = 5;
    const int IGNORE_LOWEST_X_PERCENT_DATA = 5;

    count = count - (count * IGNORE_LOWEST_X_PERCENT_DATA / 100);

    BoxData box{};
    if (count >= NUM_VALS_NEEDED_FOR_BOX_AND_WHISKER) {
        box.lowerExtreme  = vals[orderedIndex[count]];
        box.lowerQuartile = median(vals, orderedIndex, count / 2, count);
        box.median        = median(vals, orderedIndex, 0, count);
        box.upperQuartile = median(vals, orderedIndex, 0, count / 2);
        box.upperExtreme  = vals[orderedIndex[0]];
    }
    box.unpenalized = unpenalizedGenomePresent;
    dataPoints.append(box);

    // shift x-axis if we are at the right edge
    if (dataPoints.count() > xAxisRange[axismax]) {
        xAxisRange[axismin] = std::max(xAxisRange[axismin], dataPoints.count() - 1 + (UPDATECHUNKSIZE - DATAWIDTH) / PLOTFREQUENCY);
        xAxisRange[axismax] = std::max(xAxisRange[axismax], dataPoints.count() - 1 + (UPDATECHUNKSIZE / PLOTFREQUENCY));
    }

    // update y-axis range
    yAxisRange[axismin] = std::min(yAxisRange[axismin], box.lowerExtreme);
    yAxisRange[axismax] = std::max(yAxisRange[axismax], box.upperExtreme);

    // apply "nice numbers" to y-axis: round range outward to clean tick values
    if (yAxisRange[axismax] > yAxisRange[axismin]) {
        const float range = yAxisRange[axismax] - yAxisRange[axismin];
        const float rawStep = range / 5.0F;
        const float magnitude = qPow(10.0F, qFloor(std::log10(rawStep)));
        const float residual = rawStep / magnitude;
        float niceStep;
        if (residual <= 1.5F) { niceStep = 1.0F * magnitude; }
        else if (residual <= 3.0F) { niceStep = 2.0F * magnitude; }
        else if (residual <= 7.0F) { niceStep = 5.0F * magnitude; }
        else { niceStep = 10.0F * magnitude; }
        yAxisRange[axismin] = qFloor(yAxisRange[axismin] / niceStep) * niceStep;
        yAxisRange[axismax] = qCeil(yAxisRange[axismax] / niceStep) * niceStep;
    }

    update();
}


void BoxWhiskerPlot::paintEvent(QPaintEvent * /*event*/)
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
    const int xRange = xAxisRange[axismax] - xAxisRange[axismin];
    if (xRange > 0) {
        painter.setFont(labelsFont);
        painter.setPen(QPen(Qt::black, 1));
        for (int i = xAxisRange[axismin]; i <= xAxisRange[axismax]; i++) {
            const int x = plotRect.left() + static_cast<int>(static_cast<double>(i - xAxisRange[axismin]) / xRange * plotRect.width());
            painter.drawLine(x, plotRect.bottom(), x, plotRect.bottom() + 4);

            const QString label = QString::number(i * PLOTFREQUENCY);
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

    // --- draw the box-and-whisker data ---
    if (dataPoints.isEmpty() || yRange <= 0 || xRange <= 0) {
        return;
    }

    const double slotWidth = static_cast<double>(plotRect.width()) / xRange;
    const double boxWidth = slotWidth * 0.6;

    painter.setClipRect(plotRect);

    for (int i = 0; i < dataPoints.count(); i++) {
        const BoxData &box = dataPoints[i];

        // x center of this box in widget coordinates
        const double xCenter = plotRect.left() + (static_cast<double>(i - xAxisRange[axismin]) / xRange) * plotRect.width();

        // skip if off-screen
        if (xCenter + boxWidth / 2 < plotRect.left() || xCenter - boxWidth / 2 > plotRect.right()) {
            continue;
        }

        // helper to convert a data value to y pixel
        auto yPos = [&](float val) -> int {
            return plotRect.bottom() - static_cast<int>((val - yAxisRange[axismin]) / yRange * plotRect.height());
        };

        const int yLower   = yPos(box.lowerExtreme);
        const int yQ1      = yPos(box.lowerQuartile);
        const int yMedian  = yPos(box.median);
        const int yQ3      = yPos(box.upperQuartile);
        const int yUpper   = yPos(box.upperExtreme);
        const int xLeft    = static_cast<int>(xCenter - boxWidth / 2);
        const int xRight   = static_cast<int>(xCenter + boxWidth / 2);
        const int xCenterI = static_cast<int>(xCenter);

        // fill the box (Q1 to Q3)
        const QColor fillColor = QColor::fromString(box.unpenalized ? AQUAHEX : STARFISHHEX);
        painter.setBrush(fillColor);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawRect(QRect(xLeft, yQ3, xRight - xLeft, yQ1 - yQ3));

        // median line
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(xLeft, yMedian, xRight, yMedian);

        // whiskers (vertical lines from box to extremes)
        painter.setPen(QPen(Qt::black, 1));
        painter.drawLine(xCenterI, yQ3, xCenterI, yUpper);   // upper whisker
        painter.drawLine(xCenterI, yQ1, xCenterI, yLower);   // lower whisker

        // whisker caps
        const int capHalfWidth = static_cast<int>(boxWidth / 4);
        painter.drawLine(xCenterI - capHalfWidth, yUpper, xCenterI + capHalfWidth, yUpper);
        painter.drawLine(xCenterI - capHalfWidth, yLower, xCenterI + capHalfWidth, yLower);
    }

    painter.setClipping(false);
}


float BoxWhiskerPlot::median(const float *const vals, const int *const orderedIndex, const int begin, const int end)
{
    const int count = end - begin;
    if ((count % 2) != 0) {
        return vals[orderedIndex[count / 2 + begin]];
    }

    const float right = vals[orderedIndex[count / 2 + begin]];
    const float left = vals[orderedIndex[count / 2 - 1 + begin]];
    return (right + left) / 2.0F;
}
