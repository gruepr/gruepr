#include "attributeWidget.h"
#include <QApplication>

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    theGrid = new QGridLayout(this);
    theGrid->setContentsMargins(0,0,0,0);
    theGrid->setHorizontalSpacing(4);
    theGrid->setVerticalSpacing(2);
    setLayout(theGrid);
    int row = 0, column = 0;

    questionLabel = new QLabel(this);
    questionLabel->setStyleSheet(QString(LABELSTYLE).replace("10pt", "12pt"));
    questionLabel->setTextFormat(Qt::RichText);
    questionLabel->setWordWrap(true);
    theGrid->addWidget(questionLabel, row++, column, 1, -1);

    responsesLabel = new QLabel(this);
    responsesLabel->setStyleSheet(LABELSTYLE);
    responsesLabel->setTextFormat(Qt::RichText);
    responsesLabel->setWordWrap(true);
    responsesLabel->setIndent(10);
    responsesFrame = new QFrame(this);
    responsesFrame->setStyleSheet("QFrame {background-color: " TRANSPARENT "; border: 1px solid; border-color: " AQUAHEX "; padding: 2 px;}");
    responsesFrame->setLineWidth(1);
    responsesFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
    auto *hlay = new QHBoxLayout(responsesFrame);
    hlay->addWidget(responsesLabel);
    theGrid->addWidget(responsesFrame, row++, column, 1, -1);

    weightPreLabel = new QLabel(tr("Weight:"), this);
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
    homogenLabel = new QLabel(tr("Homogeneous Values"), this);
    homogenLabel->setToolTip(HOMOGENTOOLTIP);
    theGrid->addWidget(homogenLabel, row, column++);
    homogeneous = new SwitchButton(this, false);
    homogeneous->setToolTip(HOMOGENTOOLTIP);
    theGrid->addWidget(homogeneous, row++, column);

    requiredIncompatsButton = new QPushButton(tr("Set Rules"), this);
    requiredIncompatsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    theGrid->addWidget(requiredIncompatsButton, row++, 0, 1, -1);

    theGrid->setRowStretch(row, 1);
}


void AttributeWidget::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    if(attribute >= dataOptions->numAttributes)
    {
        questionLabel->setText(tr("N/A"));
        responsesLabel->setText(tr("N/A"));
        weight->setEnabled(false);
        homogeneous->setEnabled(false);
        requiredIncompatsButton->setEnabled(false);
        return;
    }

    updateQuestionAndResponses(attribute, dataOptions);

    if(dataOptions->attributeVals[attribute].size() == 1)
    {
        teamingOptions->attributeWeights[attribute] = 0;
        weight->setEnabled(false);
        weight->setToolTip(ONLYONETOOLTIP);
        homogeneous->setEnabled(false);
        homogeneous->setToolTip(ONLYONETOOLTIP);
        requiredIncompatsButton->setEnabled(false);
        requiredIncompatsButton->setToolTip(ONLYONETOOLTIP);
    }
    else
    {
        weight->setEnabled(true);
        weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
        homogeneous->setEnabled(true);
        homogeneous->setToolTip(HOMOGENTOOLTIP);
        requiredIncompatsButton->setEnabled(true);
        requiredIncompatsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    }
    weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    homogeneous->setValue(teamingOptions->desireHomogeneous[attribute]);
}

void AttributeWidget::updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts)
{
    const auto type = dataOptions->attributeType[attribute];

    QString questionText = "<html>" + dataOptions->attributeQuestionText.at(attribute);
    if((type == DataOptions::AttributeType::multicategorical) || (type == DataOptions::AttributeType::multiordered))
    {
        questionText += "<br><i>Multiple responses allowed.</i>";
    }
    questionText += "</html>";

    static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    QRegularExpressionMatch match;
    int responseNum = 0;
    bool first = true;
    QString responsesText = "<html>";    //hanging indent
    for(const auto &response : qAsConst(dataOptions->attributeQuestionResponses[attribute]))
    {
        if(!first) {
            responsesText += "<br>";
        }
        first = false;
        responsesText += "<b>";
        if((type == DataOptions::AttributeType::ordered) || (type == DataOptions::AttributeType::multiordered))
        {
            // show response with starting number
            match = startsWithInteger.match(response);
            responsesText += match.captured(1) + "</b>" + response.mid(match.capturedLength(1));
        }
        else if((type == DataOptions::AttributeType::categorical) || (type == DataOptions::AttributeType::multicategorical))
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            responsesText += (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
            responsesText += "</b>. " + response;
        }
        else
        {
            // timezone, show response with GMT
            QString timezoneName;
            float hours=0, minutes=0, offsetFromGMT=0;
            if(DataOptions::parseTimezoneInfoFromText(response, timezoneName, hours, minutes, offsetFromGMT))
            {
                QString GMTtext = QString("[GMT %1%2:%3]").arg(hours >= 0 ? "+" : "").arg(static_cast<int>(hours)).arg(static_cast<int>(minutes), 2, 10, QChar('0'));
                responsesText += GMTtext + "</b> " + timezoneName;
            }
            else
            {
                responsesText += (responseNum < 26 ?
                                              QString(char(responseNum + 'A')) :
                                              QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
                responsesText += "</b>. " + response;
            }
        }
        responsesText += "  (" +
                                 QString::number(responseCounts.empty() ?
                                                     (dataOptions->attributeQuestionResponseCounts[attribute].at(response)) :
                                                     (responseCounts.at(response)))
                                 + " " + tr("students") + ")";
        responseNum++;
    }
    responsesText += "</html>";

    questionLabel->setText(questionText);
    responsesLabel->setText(responsesText);
}
