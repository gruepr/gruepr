#ifndef ATTRIBUTEDIVERSITYSLIDER_H
#define ATTRIBUTEDIVERSITYSLIDER_H
#include <QSlider>
#include <teamingOptions.h>

class AttributeDiversitySlider : public QSlider
{
    Q_OBJECT
public:
    AttributeDiversitySlider(QWidget *parent = nullptr);
    static TeamingOptions::AttributeDiversity getAttributeDiversityFromSliderIndex(int sliderIndex);
    static int getSliderIndexFromAttributeDiversity(TeamingOptions::AttributeDiversity attributeDiversity);

};
#endif // ATTRIBUTEDIVERSITYSLIDER_H
