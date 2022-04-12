#ifndef BOXWHISKERPLOT_H
#define BOXWHISKERPLOT_H

// a box-and-whisker plot based on QChart

#include <QBoxPlotSeries>
#include <QCategoryAxis>
#include <QChartView>
#include <QValueAxis>

class BoxWhiskerPlot : public QtCharts::QChart
{
    Q_OBJECT

public:
    BoxWhiskerPlot(const QString &title = "", const QString &xAxisTitle = "", const QString &yAxisTitle = "", QWidget *parent = nullptr);
    void loadNextVals(const QVector<float> &vals, const int *const orderedIndex, const bool unpenalizedGenomePresent);
    inline static const int PLOTFREQUENCY = 5;

private:
    float median(const QVector<float> &vals, const int *const orderedIndex, const int begin, const int end);
    QtCharts::QBoxPlotSeries *dataSeries;
    QtCharts::QCategoryAxis *axisX;
    QtCharts::QValueAxis *axisY;
    float nextVals[5] = {0,0,0,0,0};
    int xAxisRange[2] = {0, 1};
    float yAxisRange[2] = {0, 1};
    enum {axismin, axismax};
    inline static const int DATAWIDTH = 60;
    inline static const int UPDATECHUNKSIZE = 20;
};

#endif // BOXWHISKERPLOT_H
