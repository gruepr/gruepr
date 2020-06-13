#include "boxwhiskerplot.h"

BoxWhiskerPlot::BoxWhiskerPlot(QString title, QString xAxisTitle, QString yAxisTitle)
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
    for(int i = 1; i <= dataWidth + updateChunkSize; i++)
    {
        if(i % updateChunkSize == 0)
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
    if(set)
    {
        dataSeries->append(set);
    }

    if(!((axisX->categories()).contains(QString::number(updateChunkSize * (1 + (dataSeries->count() / updateChunkSize))))))
    {
        for(int i = 1 + (updateChunkSize * (dataSeries->count() / updateChunkSize)); i <= updateChunkSize * (1 + (dataSeries->count() / updateChunkSize)); i++)
        {
            if(i % updateChunkSize == 0)
            {
                axisX->append(QString::number(i));
            }
            else
            {
                axisX->append(QString(' ').repeated(i));
            }
        }
    }
    axisX->setRange(QString::number(std::max(0, updateChunkSize * (1 + (dataSeries->count() / updateChunkSize)) - dataWidth )),
                    QString::number(std::max(dataWidth, updateChunkSize * (1 + (dataSeries->count() / updateChunkSize)) )));

    axisY->setRange(yAxisRange[0], yAxisRange[1]);
    axisY->applyNiceNumbers();
}


void BoxWhiskerPlot::loadNextVals(QVector<float> vals)
{
    if(vals.count() >= 5)
    {
        std::sort(vals.begin(), vals.end());
        int count = vals.count();
        nextVals[0] = median(vals, 0, count/5);     //lower extreme (lower decile)
        nextVals[1] = median(vals, 0, count/2);     //lower quartile
        nextVals[2] = median(vals, 0, count);       //median
        nextVals[3] = median(vals, count / 2 + (count % 2), count);     //upper quartile
        nextVals[4] = vals.last();                      //upper extreme (max)

        yAxisRange[0] = std::min(yAxisRange[0], nextVals[0]);     //min y-axis value of plot
        yAxisRange[1] = std::max(yAxisRange[1], nextVals[4]);     //max y-axis value of plot
    }
}


float BoxWhiskerPlot::median(QVector<float> vals, int begin, int end)
{
    int count = end - begin;
    if (count % 2)
    {
        return vals.at(count/2 + begin);
    }
    else
    {
        float right = vals.at(count/2 + begin);
        float left = vals.at(count/2 - 1 + begin);
        return (right + left) / 2.0;
    }
}
