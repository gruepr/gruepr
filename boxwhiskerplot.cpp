#include "boxwhiskerplot.h"

BoxWhiskerPlot::BoxWhiskerPlot(const QString &title, const QString &xAxisTitle, const QString &yAxisTitle)
    : QtCharts::QChart()
{
    QFont titleFont("Oxygen Mono");
    setTheme(QtCharts::QChart::ChartThemeBlueIcy);
    legend()->setVisible(false);

    dataSeries = new QtCharts::QBoxPlotSeries();
    this->addSeries(dataSeries);

    setTitle(title);

    axisX = new QtCharts::QBarCategoryAxis;
    this->addAxis(axisX, Qt::AlignBottom);
    dataSeries->attachAxis(axisX);
    int ticklabel = 0;
    for(int i = 0; i <= dataWidth; i += plotFrequency)
    {
        if(ticklabel++ % xAxisTickPeriod == 0)
        {
            axisX->append(QString::number(i));
        }
        else
        {
            axisX->append(QString(' ').repeated(i));
        }
    }
    axisX->setRange(QString::number(1), QString::number(dataWidth));
    axisX->setTitleFont(titleFont);
    axisX->setTitleText(xAxisTitle);

    axisY = new QtCharts::QValueAxis;
    this->addAxis(axisY, Qt::AlignLeft);
    dataSeries->attachAxis(axisY);
    axisY->setRange(yAxisRange[0], yAxisRange[1]);
    axisY->setTitleFont(titleFont);
    axisY->setTitleText(yAxisTitle);
}


void BoxWhiskerPlot::updatePlot()
{
    auto *set = new QtCharts::QBoxSet(nextVals[0], nextVals[1], nextVals[2], nextVals[3], nextVals[4]);
    if(set != nullptr)
    {
        dataSeries->append(set);
    }

    if(((dataSeries->count() - 1) * plotFrequency)  == (axisX->max()).toInt())
    {
        int ticklabel = 0;
        for(int i = ((dataSeries->count() - 1) * plotFrequency); i <= (((dataSeries->count()) * plotFrequency) + updateChunkSize); i += plotFrequency)
        {
            if(ticklabel++ % xAxisTickPeriod == 0)
            {
                axisX->append(QString::number(i));
            }
            else
            {
                axisX->append(QString(' ').repeated(i));
            }
        }
    }
    axisX->setRange(QString::number(std::max(0, ((dataSeries->count() - 1) * plotFrequency) + updateChunkSize - dataWidth)),
                    QString::number(std::max(dataWidth, ((dataSeries->count() - 1) * plotFrequency) + updateChunkSize)));

    axisY->setRange(yAxisRange[0], yAxisRange[1]);
    axisY->applyNiceNumbers();
}


void BoxWhiskerPlot::loadNextVals(QVector<float> vals)
{
    int count = vals.count();
    const int numValsNeededForBoxAndWhisker = 5;
    const int ignoreBottomXPercentOfData = 10;

    if(count >= numValsNeededForBoxAndWhisker)
    {
        std::sort(vals.begin(), vals.end());
        nextVals[0] = median(vals, 0, count/(ignoreBottomXPercentOfData/2));     //lower extreme
        nextVals[1] = median(vals, 0, count/2);     //lower quartile
        nextVals[2] = median(vals, 0, count);       //median
        nextVals[3] = median(vals, count / 2 + (count % 2), count);     //upper quartile
        nextVals[4] = vals.last();                      //upper extreme (max)

        yAxisRange[0] = std::min(yAxisRange[0], nextVals[0]);     //min y-axis value of plot
        yAxisRange[1] = std::max(yAxisRange[1], nextVals[4]);     //max y-axis value of plot
    }
}


float BoxWhiskerPlot::median(const QVector<float> &vals, int begin, int end)
{
    int count = end - begin;
    if ((count % 2) != 0)
    {
        return vals.at(count/2 + begin);
    }

    float right = vals.at(count/2 + begin);
    float left = vals.at(count/2 - 1 + begin);
    return (right + left) / 2.0f;
}
