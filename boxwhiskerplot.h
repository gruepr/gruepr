#ifndef BOXWHISKERPLOT_H
#define BOXWHISKERPLOT_H

#include <QChartView>
#include <QBoxPlotSeries>
#include <QCategoryAxis>
#include <QValueAxis>

class BoxWhiskerPlot : public QtCharts::QChart
{
    Q_OBJECT

public:
    BoxWhiskerPlot(const QString &title = "", const QString &xAxisTitle = "", const QString &yAxisTitle = "");
    void loadNextVals(const QVector<float> &vals, const int *orderedIndex);
    const int plotFrequency = 5;

private:
    float median(const QVector<float> &vals, const int *orderedIndex, int begin, int end);
    QtCharts::QBoxPlotSeries *dataSeries;
    QtCharts::QCategoryAxis *axisX;
    QtCharts::QValueAxis *axisY;
    float nextVals[5] = {0,0,0,0,0};
    int xAxisRange[2] = {0, 1};
    float yAxisRange[2] = {0, 1};
    const int dataWidth = 60;
    const int updateChunkSize = 20;
};

#endif // BOXWHISKERPLOT_H
