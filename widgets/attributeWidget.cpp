#include "attributeWidget.h"

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    theGrid = new QGridLayout(this);
    theGrid->setHorizontalSpacing(4);
    theGrid->setVerticalSpacing(2);
    setLayout(theGrid);
    int row = 0, column = 0;

    attributeText = new QTextEdit(this);
    attributeText->setFontFamily("DM Sans");
    attributeText->setFrameStyle(QFrame::Box | QFrame::Plain);
    attributeText->setContextMenuPolicy(Qt::NoContextMenu);
    attributeText->setAcceptDrops(false);
    attributeText->setReadOnly(true);
    attributeText->setUndoRedoEnabled(false);
    attributeText->setEnabled(true);
    attributeText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    attributeText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    theGrid->addWidget(attributeText, row++, column, 1, -1);

    weightPreLabel = new QLabel(tr("Weight:"), this);
    theGrid->addWidget(weightPreLabel, row, column++, Qt::AlignLeft);

    weight = new QDoubleSpinBox(this);
    weight->setButtonSymbols(QAbstractSpinBox::NoButtons);
    weight->setDecimals(1);
    weight->setMinimum(0);
    weight->setMaximum(TeamingOptions::MAXWEIGHT);
    weight->setSuffix("  /  " + QString::number(TeamingOptions::MAXWEIGHT) + "   ");
    weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
    weight->setValue(1);
    homogeneous = new QCheckBox(tr("Prefer\nHomogeneous"), this);
    homogeneous->setToolTip(HOMOGENTOOLTIP);
    requiredButton = new QPushButton(tr("Required\nValues"), this);
    requiredButton->setToolTip(REQUIREDTOOLTIP);
    incompatsButton = new QPushButton(tr("Incompatible\nValues"), this);
    incompatsButton->setToolTip(INCOMPATTOOLTIP);

    QWidget *widgets[] = {weight, homogeneous, requiredButton, incompatsButton};
    for(auto *const widget : widgets)
    {
        widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        widget->setFixedSize(widget->sizeHint());
        theGrid->addWidget(widget, row, column++);
    }
}


void AttributeWidget::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    if(attribute >= dataOptions->numAttributes)
    {
        attributeText->setHtml(tr("<html>N/A</html>"));
        weight->setEnabled(false);
        homogeneous->setEnabled(false);
        requiredButton->setEnabled(false);
        incompatsButton->setEnabled(false);
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
        requiredButton->setEnabled(false);
        requiredButton->setToolTip(ONLYONETOOLTIP);
        incompatsButton->setEnabled(false);
        incompatsButton->setToolTip(ONLYONETOOLTIP);
    }
    else
    {
        weight->setEnabled(true);
        weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
        homogeneous->setEnabled(true);
        homogeneous->setToolTip(HOMOGENTOOLTIP);
        requiredButton->setEnabled(true);
        requiredButton->setToolTip(REQUIREDTOOLTIP);
        incompatsButton->setEnabled(true);
        incompatsButton->setToolTip(INCOMPATTOOLTIP);
    }
    weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    homogeneous->setChecked(teamingOptions->desireHomogeneous[attribute]);
}

void AttributeWidget::updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts)
{
    const auto type = dataOptions->attributeType[attribute];

    QString questionWithResponses = "<html>" + dataOptions->attributeQuestionText.at(attribute);
    if((type == DataOptions::AttributeType::multicategorical) || (type == DataOptions::AttributeType::multiordered))
    {
        questionWithResponses += "<br><i>Multiple responses allowed.</i>";
    }
    questionWithResponses += "<hr><div style=\"margin-left:5%;\">";

    QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    int responseNum = 0;
    for(const auto &response : qAsConst(dataOptions->attributeQuestionResponses[attribute]))
    {
        if((type == DataOptions::AttributeType::ordered) || (type == DataOptions::AttributeType::multiordered))
        {
            // show response with starting number in bold
            QRegularExpressionMatch match = startsWithInteger.match(response);
            questionWithResponses += "<br><b>" + match.captured(1) + "</b>" + response.mid(match.capturedLength(1));
        }
        else if((type == DataOptions::AttributeType::categorical) || (type == DataOptions::AttributeType::multicategorical))
        {
            // show response with a preceding letter in bold (letter repeated for responses after 26)
            questionWithResponses += "<br><b>";
            questionWithResponses += (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
            questionWithResponses += "</b>. " + response;
        }
        else
        {
            // timezone, show response with GMT in bold
            QString timezoneName;
            float hours=0, minutes=0, offsetFromGMT=0;
            if(DataOptions::parseTimezoneInfoFromText(response, timezoneName, hours, minutes, offsetFromGMT))
            {
                QString GMTtext = QString("%1%2:%3").arg(hours >= 0 ? "+" : "").arg(static_cast<int>(hours)).arg(static_cast<int>(minutes), 2, 10, QChar('0'));
                questionWithResponses += "<br><b>" + GMTtext + "</b> " + timezoneName;
            }
            else
            {
                questionWithResponses += "<br><b>";
                questionWithResponses += (responseNum < 26 ?
                                              QString(char(responseNum + 'A')) :
                                              QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
                questionWithResponses += "</b>. " + response;
            }
        }
        questionWithResponses += "  (" +
                                 QString::number(responseCounts.empty() ?
                                                     (dataOptions->attributeQuestionResponseCounts[attribute].at(response)) :
                                                     (responseCounts.at(response)))
                                 + " " + tr("students") + ")";
        responseNum++;
    }
    questionWithResponses += "</div>";
    questionWithResponses += "</html>";
    attributeText->setHtml(questionWithResponses);

    // make the edit box as tall as needed to show all text
    const auto *const doc = attributeText->document();
    const QFontMetrics font(doc->defaultFont());
    const auto margins = attributeText->contentsMargins();
    const int height = (font.lineSpacing() * std::max(1.0, doc->size().height())) +
                       (doc->documentMargin() + attributeText->frameWidth()) * 2 + margins.top() + margins.bottom();
    attributeText->setMinimumHeight(height);

}
