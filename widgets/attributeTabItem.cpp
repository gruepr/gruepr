#include "attributeTabItem.h"

attributeTabItem::attributeTabItem(TabType tabType, QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    theGrid = new QGridLayout(this);
    theGrid->setHorizontalSpacing(4);
    theGrid->setVerticalSpacing(2);
    theGrid->setRowStretch(0,100);
    theGrid->setColumnStretch(3,100);
    setLayout(theGrid);
    int row = 0, column = 0;

    attributeText = new QTextEdit(this);
    if(tabType == gruepr)
    {
        attributeText->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
        attributeText->setContextMenuPolicy(Qt::NoContextMenu);
        attributeText->setAcceptDrops(false);
        attributeText->setReadOnly(true);
        attributeText->setUndoRedoEnabled(false);
    }
    else
    {
        attributeText->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
        attributeText->setAcceptDrops(true);
        attributeText->setReadOnly(false);
        attributeText->setUndoRedoEnabled(true);
    }
    attributeText->setEnabled(true);
    attributeText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    theGrid->addWidget(attributeText, row++, column, 1, -1);

    if(tabType == gruepr)
    {
        weightLabel = new QLabel(tr("Weight:"), this);
        theGrid->addWidget(weightLabel, row, column++, Qt::AlignCenter);

        weight = new QDoubleSpinBox(this);
        weight->setToolTip(tr("<html>The relative importance of this attribute in forming the teams.</html>"));
        weight->setButtonSymbols(QAbstractSpinBox::NoButtons);
        weight->setDecimals(1);
        weight->setMinimum(0);
        weight->setMaximum(TeamingOptions::MAXWEIGHT);
        weight->setValue(1);
        theGrid->addWidget(weight, row, column++, Qt::AlignCenter);

        homogeneous = new QCheckBox(tr("Prefer Homogeneous"), this);
        homogeneous->setToolTip(tr("If selected, all of the students on a team will have a similar response to this question.\n"
                                   "If unselected, the students on a team will have a wide range of responses to this question."));
        theGrid->addWidget(homogeneous, row, column++, Qt::AlignCenter);

        requiredButton = new QPushButton(tr("Required\nAttributes"), this);
        requiredButton->setToolTip(tr("<html>Indicate attribute value(s) where each team should have at least one student with that value.</html>"));
        theGrid->addWidget(requiredButton, row, column++, Qt::AlignRight | Qt::AlignVCenter);

        incompatsButton = new QPushButton(tr("Incompatible\nAttributes"), this);
        incompatsButton->setToolTip(tr("<html>Indicate attribute value(s) that should prevent students from being on the same team.</html>"));
        theGrid->addWidget(incompatsButton, row, column, Qt::AlignLeft | Qt::AlignVCenter);
    }
    else
    {
        attributeResponses = new ComboBoxWithElidedContents(tr("Very high / Above average / Average / Below average / Very low"), this);
        theGrid->addWidget(attributeResponses, row++, column, 1, -1);
        allowMultipleResponses = new QCheckBox(tr("Allow student to select multiple options"), this);
        theGrid->addWidget(allowMultipleResponses, row, column, 1, -1);
    }
}


void attributeTabItem::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
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
    QString questionWithResponses = "<html>" + dataOptions->attributeQuestionText.at(attribute) + "<hr>" + tr("Responses:") + "<div style=\"margin-left:5%;\">";
    QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    const auto &responses = dataOptions->attributeQuestionResponses[attribute];
    int responseNum = 0;
    for(const auto &response : qAsConst(responses))
    {
        if(dataOptions->attributeType[attribute] == DataOptions::ordered)
        {
            // show response with starting number in bold
            QRegularExpressionMatch match = startsWithInteger.match(response);
            questionWithResponses += "<br><b>" + match.captured(1) + "</b>" + response.mid(match.capturedLength(1));
        }
        else
        {
            // show response with a preceding letter in bold (letter repeated for responses after 26)
            questionWithResponses += "<br><b>";
            questionWithResponses += (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
            questionWithResponses += "</b>. " + response;
        }
        questionWithResponses += " (" + QString::number(dataOptions->attributeQuestionResponseCounts[attribute].at(response)) + " " + tr("students") + ")";
        responseNum++;
    }
    questionWithResponses += "</div>";
    if(dataOptions->attributeType[attribute] == DataOptions::multicategorical)
    {
        questionWithResponses += "<br><i>Multiple responses allowed.</i>";
    }
    questionWithResponses += "</html>";
    attributeText->setHtml(questionWithResponses);
    if(dataOptions->attributeVals[attribute].size() == 1)
    {
        teamingOptions->attributeWeights[attribute] = 0;
        weight->setEnabled(false);
        weight->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
        homogeneous->setEnabled(false);
        homogeneous->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
        requiredButton->setEnabled(false);
        requiredButton->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
        incompatsButton->setEnabled(false);
        incompatsButton->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
    }
    else
    {
        weight->setEnabled(true);
        weight->setToolTip(tr("The relative importance of this attribute in forming the teams"));
        homogeneous->setEnabled(true);
        homogeneous->setToolTip(tr("If selected, all of the students on a team will have a similar response to this question.\n"
                                   "If unselected, the students on a team will have a wide range of responses to this question."));
        requiredButton->setEnabled(true);
        requiredButton->setToolTip(tr("<html>Indicate attribute value(s) where each team should have at least one student with that value.</html>"));
        incompatsButton->setEnabled(true);
        incompatsButton->setToolTip(tr("<html>Indicate attribute value(s) that should prevent students from being on the same team.</html>"));
    }
    weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    homogeneous->setChecked(teamingOptions->desireHomogeneous[attribute]);
}
