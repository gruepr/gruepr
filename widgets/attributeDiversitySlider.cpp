#include "attributeDiversitySlider.h"
#include <QSlider>
#include <teamingOptions.cpp>
#include "gruepr_globals.h"
AttributeDiversitySlider::AttributeDiversitySlider(QWidget *parent): QSlider(Qt::Horizontal, parent) {
    setRange(0, 2);  // Set min-max values
    setTickPosition(QSlider::TicksBelow);
    setTickInterval(1);
    setValue(1);
    setStyleSheet("QSlider::handle:horizontal { background: " OPENWATERHEX "; width: 10px; height: 10px; border-radius: 5px; }");
}

TeamingOptions::AttributeDiversity AttributeDiversitySlider::getAttributeDiversityFromSliderIndex(int sliderIndex){
    if (sliderIndex == 0){
        return TeamingOptions::AttributeDiversity::HETEROGENOUS;
    } else if (sliderIndex == 1){
        return TeamingOptions::AttributeDiversity::IGNORED;
    } else if (sliderIndex == 2){
        return TeamingOptions::AttributeDiversity::HOMOGENOUS;
    } else {
        return TeamingOptions::AttributeDiversity::IGNORED;
    }
}

int AttributeDiversitySlider::getSliderIndexFromAttributeDiversity(TeamingOptions::AttributeDiversity attributeDiversity){
    if (attributeDiversity == TeamingOptions::AttributeDiversity::HETEROGENOUS){
        return 0;
    } else if (attributeDiversity == TeamingOptions::AttributeDiversity::IGNORED){
        return 1;
    } else if (attributeDiversity == TeamingOptions::AttributeDiversity::HOMOGENOUS){
        return 2;
    } else{
        return 1;
    }
}


