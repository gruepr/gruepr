#include "attributeWidget.h"
#include "attributeDiversitySlider.cpp"
#include <QApplication>
#include <QFrame>
#include <QGridLayout>

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    auto *theGrid = new QGridLayout(this);
    theGrid->setContentsMargins(0,0,0,0);
    theGrid->setHorizontalSpacing(4);
    theGrid->setVerticalSpacing(2);
    setLayout(theGrid);
    int row = 0, column = 0;

    questionLabel = new QLabel(this);
    questionLabel->setStyleSheet(LABEL12PTSTYLE);
    questionLabel->setTextFormat(Qt::RichText);
    questionLabel->setWordWrap(true);
    theGrid->addWidget(questionLabel, row++, column, 1, -1);

    responsesLabel = new QLabel(this);
    responsesLabel->setStyleSheet(LABEL10PTSTYLE);
    responsesLabel->setTextFormat(Qt::RichText);
    responsesLabel->setWordWrap(true);
    responsesLabel->setIndent(10);
    auto *responsesFrame = new QFrame(this);
    responsesFrame->setStyleSheet("QFrame {background-color: " TRANSPARENT "; border: 1px solid; border-color: " AQUAHEX "; padding: 2 px;}");
    responsesFrame->setLineWidth(1);
    responsesFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
    auto *hlay = new QHBoxLayout(responsesFrame);
    hlay->addWidget(responsesLabel);
    theGrid->addWidget(responsesFrame, row++, column, 1, -1);

    auto *weightPreLabel = new QLabel(tr("Weight:"), this);
    theGrid->addWidget(weightPreLabel, row, column++, Qt::AlignLeft);
    weight = new QDoubleSpinBox(this);
    weight->setFocusPolicy(Qt::StrongFocus);
    weight->installEventFilter(new MouseWheelBlocker(weight));
    weight->setDecimals(1);
    weight->setMinimum(0);
    weight->setMaximum(TeamingOptions::MAXWEIGHT);
    weight->setSuffix("  /  " + QString::number(TeamingOptions::MAXWEIGHT) + "   ");
    weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
    weight->setValue(1);
    theGrid->addWidget(weight, row, column++);
    theGrid->setColumnStretch(column++, 1);

    auto *sliderLabel = new QLabel(tr("Attribute Diversity"), this);
    sliderLabel->setToolTip(HOMOGENTOOLTIP);
    theGrid->addWidget(sliderLabel, row, column++);
    attribute_diversity_slider = new AttributeDiversitySlider(this);
    attribute_diversity_slider->setToolTip(HOMOGENTOOLTIP);
    QLabel *sliderLabel1 = new QLabel("Diverse", this);
    QLabel *sliderLabel2 = new QLabel("Ignore", this);
    QLabel *sliderLabel3 = new QLabel("Similar", this);
    theGrid->addWidget(attribute_diversity_slider, row++, column, 1, 3);
    // Align labels under the slider evenly
    theGrid->addWidget(sliderLabel1, row, column++, Qt::AlignLeft);
    theGrid->addWidget(sliderLabel2, row, column++, Qt::AlignLeft);
    theGrid->addWidget(sliderLabel3, row++, column, Qt::AlignLeft);

    requiredIncompatsButton = new QPushButton(tr("Set Rules"), this);
    requiredIncompatsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    theGrid->addWidget(requiredIncompatsButton, row, 0, 1, -1);
}


void AttributeWidget::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    if(attribute >= dataOptions->numAttributes) {
        questionLabel->setText(tr("N/A"));
        responsesLabel->setText(tr("N/A"));
        weight->setEnabled(false);
        attribute_diversity_slider->setEnabled(false);
        requiredIncompatsButton->setEnabled(false);
        return;
    }

    updateQuestionAndResponses(attribute, dataOptions);

    if(dataOptions->attributeVals[attribute].size() == 1) {
        teamingOptions->attributeWeights[attribute] = 0;
        weight->setEnabled(false);
        weight->setToolTip(ONLYONETOOLTIP);
        attribute_diversity_slider->setEnabled(false);
        attribute_diversity_slider->setToolTip(ONLYONETOOLTIP);
        requiredIncompatsButton->setEnabled(false);
        requiredIncompatsButton->setToolTip(ONLYONETOOLTIP);
    }
    else {
        weight->setEnabled(true);
        weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
        attribute_diversity_slider->setEnabled(true);
        attribute_diversity_slider->setToolTip(HOMOGENTOOLTIP);
        requiredIncompatsButton->setEnabled(true);
        requiredIncompatsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    }
    weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    //Convert AttributeDiversity to Slider Index then set slider value
    attribute_diversity_slider->setValue(AttributeDiversitySlider::getSliderIndexFromAttributeDiversity(teamingOptions->attributeDiversity[attribute]));
}

void AttributeWidget::updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts)
{
    const auto type = dataOptions->attributeType[attribute];

    QString questionText = "<html>" + dataOptions->attributeQuestionText.at(attribute);
    if((type == DataOptions::AttributeType::multicategorical) || (type == DataOptions::AttributeType::multiordered)) {
        questionText += "<br><i>Multiple responses allowed.</i>";
    }
    questionText += "</html>";
    questionLabel->setText(questionText);

    static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    QRegularExpressionMatch match;
    int responseNum = 0;
    bool first = true;
    QString responsesText = "<html>";
    for(const auto &response : qAsConst(dataOptions->attributeQuestionResponses[attribute])) {
        if(!first) {
            responsesText += "<br>";
        }
        first = false;
        responsesText += "<b>";
        if((type == DataOptions::AttributeType::ordered) || (type == DataOptions::AttributeType::multiordered)) {
            // show response with starting number
            match = startsWithInteger.match(response);
            responsesText += match.captured(1) + "</b>" + response.mid(match.capturedLength(1));
        }
        else if((type == DataOptions::AttributeType::categorical) || (type == DataOptions::AttributeType::multicategorical)) {
            // show response with a preceding letter (letter repeated for responses after 26)
            responsesText += (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
            responsesText += "</b>. " + response;
        }
        else {
            // timezone, show response with GMT
            QString timezoneName;
            float hours=0, minutes=0, offsetFromGMT=0;
            if(DataOptions::parseTimezoneInfoFromText(response, timezoneName, hours, minutes, offsetFromGMT)) {
                const QString GMTtext = QString("[GMT %1%2:%3]").arg(hours >= 0 ? "+" : "").arg(int(hours)).arg(int(minutes), 2, 10, QChar('0'));
                responsesText += GMTtext + "</b> " + timezoneName;
            }
            else {
                responsesText += (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
                responsesText += "</b>. " + response;
            }
        }
        responsesText += "  (" + QString::number(responseCounts.empty() ?
                                     (dataOptions->attributeQuestionResponseCounts[attribute].at(response)) :
                                     (responseCounts.at(response))) +
                         " " + tr("students") + ")";
        responseNum++;
    }
    responsesText += "</html>";
    responsesLabel->setText(responsesText);
}
