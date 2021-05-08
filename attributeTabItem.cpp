#include "attributeTabItem.h"

attributeTabItem::attributeTabItem(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    theGrid = new QGridLayout(this);
    theGrid->setHorizontalSpacing(4);
    theGrid->setVerticalSpacing(2);
    theGrid->setRowStretch(0,100);
    theGrid->setColumnStretch(3,100);
    setLayout(theGrid);

    attributeText = new QTextEdit(this);
    attributeText->setContextMenuPolicy(Qt::NoContextMenu);
    attributeText->setAcceptDrops(false);
    attributeText->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    attributeText->setReadOnly(true);
    attributeText->setUndoRedoEnabled(false);
    attributeText->setEnabled(true);
    attributeText->setPlaceholderText("Attribute question text");
    attributeText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    theGrid->addWidget(attributeText, 0, 0, 1, -1);

    weightLabel = new QLabel("Weight:", this);
    theGrid->addWidget(weightLabel, 1, 0, Qt::AlignCenter);

    weight = new QDoubleSpinBox(this);
    weight->setToolTip("<html>The relative importance of this attribute in forming the teams.</html>");
    weight->setButtonSymbols(QAbstractSpinBox::NoButtons);
    weight->setDecimals(1);
    weight->setMinimum(0);
    weight->setMaximum(100);
    weight->setValue(1);
    theGrid->addWidget(weight, 1, 1, Qt::AlignCenter);

    homogeneous = new QCheckBox("Prefer Homogeneous", this);
    homogeneous->setToolTip("If selected, all of the students on a team will have a similar response to this question.\n"
                            "If unselected, the students on a team will have a wide range of responses to this question.");
    theGrid->addWidget(homogeneous, 1, 2, Qt::AlignCenter);

    incompatsButton = new QPushButton("Incompatible\nResponses", this);
    incompatsButton->setToolTip("<html>Indicate response values that should prevent students from being on the same team.</html>");
    theGrid->addWidget(incompatsButton, 1, 3, Qt::AlignLeft | Qt::AlignVCenter);
}


void attributeTabItem::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    QString questionWithResponses = "<html>" + dataOptions->attributeQuestionText.at(attribute) + "<hr>" + tr("Responses:") + "<div style=\"margin-left:5%;\">";
    QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    for(int response = 0; response < dataOptions->attributeQuestionResponses[attribute].size(); response++)
    {
        if(dataOptions->attributeIsOrdered[attribute])
        {
            // show response with starting number in bold
            QRegularExpressionMatch match = startsWithInteger.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            questionWithResponses += "<br><b>" + match.captured(1) + "</b>" + dataOptions->attributeQuestionResponses[attribute].at(response).mid(match.capturedLength(1));
        }
        else
        {
            // show response with a preceding letter in bold (letter repeated for responses after 26)
            questionWithResponses += "<br><b>";
            questionWithResponses += (response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26)));
            questionWithResponses += "</b>. " + dataOptions->attributeQuestionResponses[attribute].at(response);
        }
    }
    questionWithResponses += "</div></html>";
    attributeText->setHtml(questionWithResponses);
    if(dataOptions->attributeMin[attribute] == dataOptions->attributeMax[attribute])
    {
        teamingOptions->attributeWeights[attribute] = 0;
        weight->setEnabled(false);
        weight->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
        homogeneous->setEnabled(false);
        homogeneous->setToolTip(tr("With only one response value, this attribute cannot be used for teaming"));
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
        incompatsButton->setEnabled(true);
        incompatsButton->setToolTip(tr("<html>Indicate response values that should prevent students from being on the same team.</html>"));
    }
    weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    homogeneous->setChecked(teamingOptions->desireHomogeneous[attribute]);
}
