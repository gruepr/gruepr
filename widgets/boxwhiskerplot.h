#ifndef BOXWHISKERPLOT_H
#define BOXWHISKERPLOT_H

// a box-and-whisker plot

#include <QWidget>
#include <QList>

class BoxWhiskerPlot : public QWidget
{
    Q_OBJECT

public:
    BoxWhiskerPlot(const QString &title = "", const QString &xAxisTitle = "", const QString &yAxisTitle = "", QWidget *parent = nullptr);
    void loadNextVals(const float *const vals, const int *const orderedIndex, int count, const bool unpenalizedGenomePresent);
    inline static const int PLOTFREQUENCY = 5;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float median(const float *const vals, const int *const orderedIndex, const int begin, const int end);

    struct BoxData {
        float lowerExtreme;
        float lowerQuartile;
        float median;
        float upperQuartile;
        float upperExtreme;
        bool unpenalized;
    };
    QList<BoxData> dataPoints;

    QString titleText;
    QString xAxisTitle;
    QString yAxisTitle;

    long long xAxisRange[2] = {0, 1};
    float yAxisRange[2] = {0, 1};
    enum {axismin, axismax};
    inline static const int DATAWIDTH = 60;
    inline static const int UPDATECHUNKSIZE = 20;

    // layout margins for the plot area within the widget
    inline static const int MARGIN_LEFT = 60;
    inline static const int MARGIN_RIGHT = 20;
    inline static const int MARGIN_TOP = 30;
    inline static const int MARGIN_BOTTOM = 50;
};

#endif // BOXWHISKERPLOT_H
