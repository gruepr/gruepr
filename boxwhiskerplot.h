#ifndef BOXWHISKERPLOT_H
#define BOXWHISKERPLOT_H

#include <QChartView>
#include <QBoxPlotSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

class BoxWhiskerPlot : public QtCharts::QChart
{
    Q_OBJECT

public:
    BoxWhiskerPlot(const QString &title = "", const QString &xAxisTitle = "", const QString &yAxisTitle = "");
    void loadNextVals(QVector<float> vals);
    const int plotFrequency = 5;

public slots:
    void updatePlot();

private:
    float median(const QVector<float> &vals, int begin, int end);
    QtCharts::QBoxPlotSeries *dataSeries;
    QtCharts::QBarCategoryAxis *axisX;
    QtCharts::QValueAxis *axisY;
    float nextVals[5] = {0,0,0,0,0};
    float yAxisRange[2] = {0, 1};
    const int dataWidth = 50;
    const int updateChunkSize = 20;
    const int xAxisTickPeriod = 2;
};

#endif // BOXWHISKERPLOT_H
