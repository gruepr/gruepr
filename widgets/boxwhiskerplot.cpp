#include "boxwhiskerplot.h"
#include "gruepr_globals.h"


BoxWhiskerPlot::BoxWhiskerPlot(const QString &title, const QString &xAxisTitle, const QString &yAxisTitle, QWidget *parent)
{
    (void)parent;
    QFont titleFont("Oxygen Mono");
    QFont labelsFont(titleFont);
    labelsFont.setPointSize(titleFont.pointSize()-2);
    legend()->hide();

    dataSeries = new QBoxPlotSeries();
    this->addSeries(dataSeries);

    setTitle(title);

    axisX = new QCategoryAxis;
    this->addAxis(axisX, Qt::AlignBottom);
    dataSeries->attachAxis(axisX);
    xAxisRange[axismin] = 0;
    xAxisRange[axismax] = DATAWIDTH/PLOTFREQUENCY;
    axisX->setRange(xAxisRange[axismin], xAxisRange[axismax]);
    for(int label = 0; label <= xAxisRange[axismax]; label++)
    {
        axisX->append(QString::number(label*PLOTFREQUENCY), label);
    }
    axisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisX->setLabelsFont(labelsFont);
    axisX->setTitleFont(titleFont);
    axisX->setTitleText(xAxisTitle);

    axisY = new QValueAxis;
    this->addAxis(axisY, Qt::AlignLeft);
    dataSeries->attachAxis(axisY);
    axisY->setRange(yAxisRange[axismin], yAxisRange[axismax]);
    axisY->setLabelsFont(labelsFont);
    axisY->setTitleFont(titleFont);
    axisY->setTitleText(yAxisTitle);
}


void BoxWhiskerPlot::loadNextVals(const float *const vals, const int *const orderedIndex, int count, const bool unpenalizedGenomePresent)
{
    //adds a new distribution to the graph window; QList vals is not sorted, but the indexes in sorted order is given in orderedIndex
    const int NUM_VALS_NEEDED_FOR_BOX_AND_WHISKER = 5;
    const int IGNORE_LOWEST_X_PERCENT_DATA = 5;  //drop outliers at low end

    count = count - int(count*IGNORE_LOWEST_X_PERCENT_DATA/100);
    if(count >= NUM_VALS_NEEDED_FOR_BOX_AND_WHISKER)
    {
        nextVals[QBoxSet::LowerExtreme] = vals[orderedIndex[count]];
        nextVals[QBoxSet::LowerQuartile] = median(vals, orderedIndex, count/2, count);
        nextVals[QBoxSet::Median] = median(vals, orderedIndex, 0, count);
        nextVals[QBoxSet::UpperQuartile] = median(vals, orderedIndex, 0, count/2);
        nextVals[QBoxSet::UpperExtreme] = vals[orderedIndex[0]];
    }

    auto *set = new QBoxSet(nextVals[QBoxSet::LowerExtreme], nextVals[QBoxSet::LowerQuartile],
                                      nextVals[QBoxSet::Median], nextVals[QBoxSet::UpperQuartile],
                                      nextVals[QBoxSet::UpperExtreme]);
    if(set != nullptr)
    {
        set->setBrush(QBrush(QColor::fromString(unpenalizedGenomePresent? AQUAHEX : STARFISHHEX)));
        dataSeries->append(set);
    }

    //shift x-axis by updateChunkSize if we are at the right edge of graph
    if(dataSeries->count() > xAxisRange[axismax])
    {
        xAxisRange[axismin] = std::max(xAxisRange[axismin], dataSeries->count() - 1 + (UPDATECHUNKSIZE - DATAWIDTH)/PLOTFREQUENCY);
        xAxisRange[axismax] = std::max(xAxisRange[axismax], dataSeries->count() - 1 + (UPDATECHUNKSIZE/PLOTFREQUENCY));
        //remove labels getting shifted off the axis (working backwards)
        for(int label = ((UPDATECHUNKSIZE/PLOTFREQUENCY) - 1); label >= 0; label--)
        {
            axisX->remove(axisX->categoriesLabels().at(label));
        }
        //add labels getting shifted on to the axis
        for(int label = dataSeries->count() - 1; label <= xAxisRange[axismax]; label++)
        {
            axisX->append(QString::number(label*PLOTFREQUENCY), label);
        }
        axisX->setRange(xAxisRange[axismin], xAxisRange[axismax]);
        axisX->setStartValue(xAxisRange[axismin]);
    }

    yAxisRange[axismin] = std::min(yAxisRange[axismin], nextVals[QBoxSet::LowerExtreme]);
    yAxisRange[axismax] = std::max(yAxisRange[axismax], nextVals[QBoxSet::UpperExtreme]);
    axisY->setRange(yAxisRange[axismin], yAxisRange[axismax]);
    axisY->applyNiceNumbers();
}


float BoxWhiskerPlot::median(const float *const vals, const int *const orderedIndex, const int begin, const int end)
{
    int count = end - begin;
    if ((count % 2) != 0)
    {
        return vals[orderedIndex[count/2 + begin]];
    }

    float right = vals[orderedIndex[count/2 + begin]];
    float left = vals[orderedIndex[count/2 - 1 + begin]];
    return (right + left) / 2.0F;
}
