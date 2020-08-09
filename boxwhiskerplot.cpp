#include "boxwhiskerplot.h"

BoxWhiskerPlot::BoxWhiskerPlot(const QString &title, const QString &xAxisTitle, const QString &yAxisTitle, QWidget *parent)
{
    (void)parent;
    QFont titleFont("Oxygen Mono");
    setTheme(QtCharts::QChart::ChartThemeBlueIcy);
    legend()->setVisible(false);

    dataSeries = new QtCharts::QBoxPlotSeries();
    this->addSeries(dataSeries);

    setTitle(title);

    axisX = new QtCharts::QCategoryAxis;
    this->addAxis(axisX, Qt::AlignBottom);
    dataSeries->attachAxis(axisX);
    xAxisRange[0] = 0;
    xAxisRange[1] = dataWidth/plotFrequency;
    axisX->setRange(xAxisRange[0], xAxisRange[1]);
    for(int label = 0; label <= xAxisRange[1]; label++)
    {
        axisX->append(QString::number(label*plotFrequency), label);
    }
    axisX->setLabelsPosition(QtCharts::QCategoryAxis::AxisLabelsPositionOnValue);
    axisX->setTitleFont(titleFont);
    axisX->setTitleText(xAxisTitle);

    axisY = new QtCharts::QValueAxis;
    this->addAxis(axisY, Qt::AlignLeft);
    dataSeries->attachAxis(axisY);
    axisY->setRange(yAxisRange[0], yAxisRange[1]);
    axisY->setTitleFont(titleFont);
    axisY->setTitleText(yAxisTitle);
}


void BoxWhiskerPlot::loadNextVals(const QVector<float> &vals, const int *const orderedIndex)
{
    //adds a new distribution to the graph window; QVector vals is not sorted, but the indexes in sorted order is given in orderedIndex
    const int numValsNeededForBoxAndWhisker = 5;
    const int ignoreLowestXPercentOfData = 5;  //drop outliers at low end
    int count = vals.count() - (vals.count()*ignoreLowestXPercentOfData/100);

    if(count >= numValsNeededForBoxAndWhisker)
    {
        nextVals[0] = vals.at(orderedIndex[count]);                 //lower extreme (min)
        nextVals[1] = median(vals, orderedIndex, count/2, count);   //lower quartile
        nextVals[2] = median(vals, orderedIndex, 0, count);         //median
        nextVals[3] = median(vals, orderedIndex, 0, count/2);       //upper quartile
        nextVals[4] = vals.at(orderedIndex[0]);                     //upper extreme (max)
    }

    auto *set = new QtCharts::QBoxSet(nextVals[0], nextVals[1], nextVals[2], nextVals[3], nextVals[4]);
    if(set != nullptr)
    {
        dataSeries->append(set);
    }

    //shift x-axis by updateChunkSize if we are at the right edge of graph
    if(dataSeries->count() > xAxisRange[1])
    {
        xAxisRange[0] = std::max(xAxisRange[0], dataSeries->count() - 1 + (updateChunkSize - dataWidth)/plotFrequency);
        xAxisRange[1] = std::max(xAxisRange[1], dataSeries->count() - 1 + (updateChunkSize/plotFrequency));
        //remove labels getting shifted off the axis (working backwards)
        for(int label = ((updateChunkSize/plotFrequency) - 1); label >= 0; label--)
        {
            axisX->remove(axisX->categoriesLabels().at(label));
        }
        //add labels getting shifted on to the axis
        for(int label = dataSeries->count() - 1; label <= xAxisRange[1]; label++)
        {
            axisX->append(QString::number(label*plotFrequency), label);
        }
        axisX->setRange(xAxisRange[0], xAxisRange[1]);
        axisX->setStartValue(xAxisRange[0]);
    }

    yAxisRange[0] = std::min(yAxisRange[0], nextVals[0]);
    yAxisRange[1] = std::max(yAxisRange[1], nextVals[4]);
    axisY->setRange(yAxisRange[0], yAxisRange[1]);
    axisY->applyNiceNumbers();
}


float BoxWhiskerPlot::median(const QVector<float> &vals, const int *const orderedIndex, const int begin, const int end)
{
    int count = end - begin;
    if ((count % 2) != 0)
    {
        return vals.at(orderedIndex[count/2 + begin]);
    }

    float right = vals.at(orderedIndex[count/2 + begin]);
    float left = vals.at(orderedIndex[count/2 - 1 + begin]);
    return (right + left) / 2.0F;
}
