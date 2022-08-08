#include "gatherAttributeValuesDialog.h"
#include <QButtonGroup>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which attribute values should be required on each team or which pairings should be disallowed on the same team
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherAttributeValuesDialog::gatherAttributeValuesDialog(const int attribute, const DataOptions *const dataOptions,
                                                         const TeamingOptions *const teamingOptions,
                                                         const GatherType gathertype, QWidget *parent)
    :QDialog (parent)
{
    incompatibleValues = teamingOptions->incompatibleAttributeValues[attribute];
    requiredValues = teamingOptions->requiredAttributeValues[attribute];
    gatherType = gathertype;
    attributeType = dataOptions->attributeType[attribute];
    attributeValues.clear();
    attributeValues.append({-1, tr("value not set/unknown")});
    auto valueIter = dataOptions->attributeVals[attribute].cbegin();
    for(const auto &response : dataOptions->attributeQuestionResponses[attribute])
    {
        attributeValues.append({*valueIter, response});
        valueIter++;
    }
    numPossibleValues = attributeValues.size();

    //Set up window with a grid layout
    if(gatherType == incompatible)
    {
        setWindowTitle(tr("Incompatible responses for attribute ") + QString::number(attribute + 1));
    }
    else
    {
        setWindowTitle(tr("Required responses for attribute ") + QString::number(attribute + 1));
    }

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);
    int row = 0;

    QString attributeDescription = "<html><br><b>" + tr("Attribute") + " " + QString::number(attribute + 1) + ":</b><br>";
    attributeDescription += dataOptions->attributeQuestionText.at(attribute) +"<hr>";
    for(const auto &attributeValue : qAsConst(attributeValues))
    {
        // create numbered list of responses (prefixing a number for the token value of -1 and
        // for unordered responses, since they already start with number)
        if((attributeValue.value == -1) ||
           ((attributeType != DataOptions::AttributeType::ordered) && (attributeType != DataOptions::AttributeType::multiordered)))
        {
            attributeDescription += valuePrefix(attributeValue.value) + ". ";
        }
        attributeDescription += attributeValue.response + "<br>";
    }

    attributeQuestion = new QLabel(this);
    attributeQuestion->setText(attributeDescription + "</html>");
    attributeQuestion->setWordWrap(true);
    theGrid->addWidget(attributeQuestion, row++, 0, 1, -1);

    auto *hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(hline, row++, 0, 1, -1);

    if(gatherType == incompatible)
    {
        // for each response value, a radio button and a label with vline between

        auto *vline = new QFrame(this);
        vline->setFrameShape(QFrame::VLine);
        vline->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(vline, row, 2, numPossibleValues + 1, 1);

        selectOneExplanation = new QLabel(this);
        selectOneExplanation->setText("<html>" + tr("Prevent placing students with this response:") + "</html>");
        selectOneExplanation->setWordWrap(true);
        theGrid->addWidget(selectOneExplanation, row++, 0, 1, 2);

        selectOneValues = new QRadioButton[numPossibleValues];
        selectOneResponses = new QPushButton[numPossibleValues];
        selectOneValuesGroup = new QButtonGroup(this);
        int response = 0;
        for(const auto &attributeValue : qAsConst(attributeValues))
        {
            theGrid->addWidget(&selectOneValues[response], row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
            selectOneValuesGroup->addButton(&selectOneValues[response]);
            selectOneResponses[response].setText(valuePrefix(attributeValue.value));
            selectOneResponses[response].setFlat(true);
            selectOneResponses[response].setStyleSheet("Text-align:left");
            connect(&selectOneResponses[response], &QPushButton::clicked, &selectOneValues[response], &QRadioButton::toggle);
            theGrid->addWidget(&selectOneResponses[response], row++, 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
            response++;
        }
    }

    // a checkbox and a label for each response value to set as required or as incompatible with the primary
    selectMultipleExplanation = new QLabel(this);
    int column = 0;
    row = 2;
    if(gatherType == incompatible)
    {
        selectMultipleExplanation->setText("<html>" + tr("on the same team as students with any of these responses:") + "</html>");
        column = 3;
    }
    else
    {
        selectMultipleExplanation->setText("<html>" + tr("Ensure each team has at least one student with each of these responses:") + "</html>");
    }
    selectMultipleExplanation->setWordWrap(true);
    theGrid->addWidget(selectMultipleExplanation, row++, column, 1, -1);

    selectMultipleValues = new QCheckBox[numPossibleValues];
    selectMultipleResponses = new QPushButton[numPossibleValues];
    int response = 0;
    for(const auto &attributeValue : qAsConst(attributeValues))
    {
        theGrid->addWidget(&selectMultipleValues[response], row, column, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
        selectMultipleResponses[response].setText(valuePrefix(attributeValue.value));
        selectMultipleResponses[response].setFlat(true);
        selectMultipleResponses[response].setStyleSheet("Text-align:left");
        connect(&selectMultipleResponses[response], &QPushButton::clicked, &selectMultipleValues[response], &QCheckBox::toggle);
        theGrid->addWidget(&selectMultipleResponses[response], row++, column + 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
        response++;
    }

    if(gatherType == incompatible)
    {
        // set second and fifth columns as the ones to grow
        theGrid->setColumnStretch(1, 1);
        theGrid->setColumnStretch(4, 1);
    }
    else
    {
        // set second column as the one to grow
        theGrid->setColumnStretch(1, 1);
    }

    //button to add the currently checked values as required or incompatible
    addValuesButton = new QPushButton(this);
    if(gatherType == incompatible)
    {
        addValuesButton->setText(tr("&Add these incompatible responses"));
    }
    else
    {
        addValuesButton->setText(tr("&Add these required responses"));
    }
    addValuesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(addValuesButton, &QPushButton::clicked, this, &gatherAttributeValuesDialog::addValues);
    theGrid->addWidget(addValuesButton, row++, 0, 1, -1, Qt::AlignCenter);

    //explanatory text of which response pairs will be considered incompatible
    explanation = new QLabel(this);
    explanation->clear();
    theGrid->addWidget(explanation, row++, 0, 1, -1);
    theGrid->setRowStretch(row++, 1);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
    resetValuesButton = new QPushButton(this);
    resetValuesButton->setText(tr("&Clear all values"));
    resetValuesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    theGrid->addWidget(resetValuesButton, row, 0, 1, 2);
    connect(resetValuesButton, &QPushButton::clicked, this, &gatherAttributeValuesDialog::clearAllValues);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, row, 3, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if(gatherType == incompatible)
    {
        explanation->setText("<html><hr><br><b>" + tr("Students with these responses will not be placed on the same team:") + "<br><br><br></b></html>");
    }
    else
    {
        explanation->setText("<html><hr><br><b>" + tr("Each team will have at least one student with each of these responses:") + "<br><br><br></b></html>");
    }
    updateExplanation();

    adjustSize();
}


gatherAttributeValuesDialog::~gatherAttributeValuesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    if(gatherType == incompatible)
    {
        delete [] selectOneValues;
        delete [] selectOneResponses;
    }
    delete [] selectMultipleValues;
    delete [] selectMultipleResponses;
}

QString gatherAttributeValuesDialog::valuePrefix(int value)
{
    if(value == -1)
    {
        return tr("--");
    }

    if((attributeType == DataOptions::AttributeType::ordered) || (attributeType == DataOptions::AttributeType::multiordered))
    {
        // response's starting number
        return QString::number(value);
    }

    // response's preceding letter (letter repeated for responses after 26)
    int valueIndex = value - 1;
    return ((valueIndex < 26) ? QString(char(valueIndex + 'A')) : QString(char(valueIndex%26 + 'A')).repeated(1 + (valueIndex/26)));
}

void gatherAttributeValuesDialog::updateExplanation()
{
    QString explanationText;
    if(gatherType == incompatible)
    {
        if(incompatibleValues.isEmpty())
        {
            explanationText = tr("Currently all responses are compatible.") + "<br>";
        }
        else
        {
            explanationText = tr("Students with these responses will not be placed on the same team:") + "<br>";
            for(const auto &pair : qAsConst(incompatibleValues))
            {
                explanationText += QString("&nbsp;&nbsp;&nbsp;&nbsp;") + BULLET + " " +
                                   valuePrefix(pair.first) + " " + DOUBLEARROW + " " + valuePrefix(pair.second) + "<br>";
            }
        }
    }
    else
    {
        if(requiredValues.isEmpty())
        {
            explanationText = tr("Currently no responses are required.") + "<br>";
        }
        else
        {
            explanationText = tr("Each team will have at least one student with each of these responses:") + "<br>";
            for(const auto val : qAsConst(requiredValues))
            {
                explanationText += QString("&nbsp;&nbsp;&nbsp;&nbsp;") + BULLET + " " + valuePrefix(val) + "<br>";
            }
        }
    }
    // remove all html tags, replace the prefix for unknown values with the description, and then place all of this in the explanation
    explanationText.remove("<html>").replace("--", "(" + attributeValues.at(0).response + ")");
    explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
}


void gatherAttributeValuesDialog::addValues()
{
    if(gatherType == incompatible)
    {
        // create pairs for the primary value and each checked incompatible value
        for(int responseIndex1 = 0; responseIndex1 < numPossibleValues; responseIndex1++)
        {
            if(selectOneValues[responseIndex1].isChecked())
            {
                // find the response value for this radio button
                int responseValue1 = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
                while(attributeValues.at(responseIndex1).value != responseValue1)
                {
                    responseValue1++;
                }
                for(int responseIndex2 = 0; responseIndex2 < numPossibleValues; responseIndex2++)
                {
                    if(selectMultipleValues[responseIndex2].isChecked())
                    {
                        // find the response value for this checkbox
                        int responseValue2 = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
                        while(attributeValues.at(responseIndex2).value != responseValue2)
                        {
                            responseValue2++;
                        }
                        if(!incompatibleValues.contains(QPair<int,int>(responseValue1, responseValue2)) &&
                                !incompatibleValues.contains(QPair<int,int>(responseValue2, responseValue1)) )
                        {
                            int smaller = std::min(responseValue1, responseValue2), larger = std::max(responseValue1, responseValue2);
                            if(smaller == -1)
                            {
                                std::swap(smaller, larger);
                            }
                            incompatibleValues << QPair<int,int>(smaller, larger);
                        }
                    }
                }
            }
        }

        // sort, but put -1 after the largest positive
        std::sort(incompatibleValues.begin(), incompatibleValues.end(),
                        [](const QPair<int,int> a, const QPair<int,int> b)
                          {int largerThanMax = std::max(std::max(a.first, b.first), std::max(a.second, b.second)) + 1;
                           int a1 = (a.first != -1) ? a.first : largerThanMax, a2 = (a.second != -1) ? a.second : largerThanMax;
                           int b1 = (b.first != -1) ? b.first : largerThanMax, b2 = (b.second != -1) ? b.second : largerThanMax;
                           return (a1 != b1 ? a1 < b1 : a2 < b2);});
    }
    else
    {
        // set each checked value as required
        for(int responseIndex = 0; responseIndex < numPossibleValues; responseIndex++)
        {
            // find the response value for this checkbox
            int responseValue = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
            while(attributeValues.at(responseIndex).value != responseValue)
            {
                responseValue++;
            }
            if(selectMultipleValues[responseIndex].isChecked() && !requiredValues.contains(responseValue))
            {
                requiredValues << responseValue;
            }
        }

        // sort, but put -1 after the largest positive
        std::sort(requiredValues.begin(), requiredValues.end(), [](const int a, const int b)
                                                                  {return(((a != -1) && (b != -1)) ? (a < b) : (a != -1));});
    }

    updateExplanation();

    // reset checkboxes
    for(int response = 0; response < numPossibleValues; response++)
    {
        selectMultipleValues[response].setChecked(false);
    }
}


void gatherAttributeValuesDialog::clearAllValues()
{
    if(gatherType == incompatible)
    {
        incompatibleValues.clear();
    }
    else
    {
        requiredValues.clear();
    }
    updateExplanation();
}
