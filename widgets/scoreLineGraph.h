#ifndef SCORELINEGRAPH_H
#define SCORELINEGRAPH_H

// a simple line graph tracking the best (max) score in each generation

#include <QWidget>
#include <QList>

class ScoreLineGraph : public QWidget
{
    Q_OBJECT

public:
    explicit ScoreLineGraph(const QString &title = "", const QString &xAxisTitle = "", const QString &yAxisTitle = "", QWidget *parent = nullptr);
    void addScore(float maxScore);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<float> scores;

    QString titleText;
    QString xAxisTitle;
    QString yAxisTitle;

    long long xAxisRange[2] = {0, 1};
    float yAxisRange[2] = {0, 1};
    enum {axismin, axismax};

    // very negative (penalized/infeasible) scores are pinned at this floor so the y-axis
    // autoranging stays focused on the meaningful 0-100 range instead of stretching to fit them
    inline static const float NEGATIVE_SCORE_FLOOR = -10.0F;
    inline static const int MIN_XAXIS_SPAN = 40;

    // layout margins for the plot area within the widget
    inline static const int MARGIN_LEFT = 60;
    inline static const int MARGIN_RIGHT = 20;
    inline static const int MARGIN_TOP = 30;
    inline static const int MARGIN_BOTTOM = 50;
};

#endif // SCORELINEGRAPH_H
