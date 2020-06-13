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
    BoxWhiskerPlot(QString title = "", QString xAxisTitle = "", QString yAxisTitle = "");
    void loadNextVals(QVector<float> vals);
    int dataWidth = 40;
    const int updateChunkSize = 10;

public slots:
    void updatePlot();

private:
    float median(QVector<float> vals, int begin, int end);
    QtCharts::QBoxPlotSeries *dataSeries;
    QtCharts::QBarCategoryAxis *axisX;
    QtCharts::QValueAxis *axisY;
    float nextVals[5] = {0,0,0,0,0};
    float yAxisRange[2] = {0, 1};
};

#endif // BOXWHISKERPLOT_H
