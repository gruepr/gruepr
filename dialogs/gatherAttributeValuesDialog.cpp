#include "gatherAttributeValuesDialog.h"
#include <QButtonGroup>
#include <QRegularExpression>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which attribute values should be required on each team or which pairings should be disallowed on the same team
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherAttributeValuesDialog::gatherAttributeValuesDialog(const int attribute, const DataOptions *const dataOptions,
                                                         const TeamingOptions *const teamingOptions,
                                                         const GatherType gathertype, QWidget *parent)
    :QDialog (parent)
{
    numPossibleValues = dataOptions->attributeQuestionResponses[attribute].size() + 1;
    incompatibleValues = teamingOptions->incompatibleAttributeValues[attribute];
    requiredValues = teamingOptions->requiredAttributeValues[attribute];
    gatherType = gathertype;

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
    for(int response = 0; response < numPossibleValues; response++)
    {
        if(response == numPossibleValues-1)
        {
            attributeDescription += "--. " + tr("value not set/unknown");
        }
        else
        {
            QString responseText = dataOptions->attributeQuestionResponses[attribute].at(response);
            if(dataOptions->attributeType[attribute] == DataOptions::ordered)
            {
                // show reponse with starting number
                QRegularExpression startsWithNumber("^(\\d+)(.+)");
                QRegularExpressionMatch match = startsWithNumber.match(responseText);
                attributeDescription += match.captured(1) + match.captured(2);
            }
            else
            {
                // show response with a preceding letter (letter repeated for responses after 26)
                attributeDescription += (response<26 ? QString(char(response + 'A')) :
                                                       QString(char(response%26 + 'A')).repeated(1 + (response/26))) + ". " + responseText;
            }
        }
        attributeDescription += "<br>";
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
        for(int response = 0; response < numPossibleValues; response++)
        {
            theGrid->addWidget(&selectOneValues[response], row, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
            selectOneValuesGroup->addButton(&selectOneValues[response]);

            if(response == numPossibleValues-1)
            {
                selectOneResponses[response].setText("--");
            }
            else
            {
                QString responseText = dataOptions->attributeQuestionResponses[attribute].at(response);
                if(dataOptions->attributeType[attribute] == DataOptions::ordered)
                {
                    // show reponse's starting number
                    QRegularExpression startsWithNumber("^(\\d+)(.+)");
                    QRegularExpressionMatch match = startsWithNumber.match(responseText);
                    selectOneResponses[response].setText(match.captured(1));
                }
                else
                {
                    // show response's preceding letter (letter repeated for responses after 26)
                    selectOneResponses[response].setText((response<26 ? QString(char(response + 'A')) :
                                                                        QString(char(response%26 + 'A')).repeated(1 + (response/26))));
                }
            }
            selectOneResponses[response].setFlat(true);
            selectOneResponses[response].setStyleSheet("Text-align:left");
            connect(&selectOneResponses[response], &QPushButton::clicked, &selectOneValues[response], &QRadioButton::toggle);
            theGrid->addWidget(&selectOneResponses[response], row++, 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    // a checkbox and a label for each response value to set as required or as incompatible with the primary
    selectMultipleExplanation = new QLabel(this);
    int column = 0;
    row = 2;
    if(gatherType == incompatible)
    {
        selectMultipleExplanation->setText("<html>" +
                                           tr("on the same team as students with any of these responses:") +
                                           "</html>");
        column = 3;
    }
    else
    {
        selectMultipleExplanation->setText("<html>" +
                                           tr("Ensure each team has at least one student with each of these responses:") +
                                           "</html>");
    }
    selectMultipleExplanation->setWordWrap(true);
    theGrid->addWidget(selectMultipleExplanation, row++, column, 1, -1);

    selectMultipleValues = new QCheckBox[numPossibleValues];
    selectMultipleResponses = new QPushButton[numPossibleValues];
    for(int response = 0; response < numPossibleValues; response++)
    {
        theGrid->addWidget(&selectMultipleValues[response], row, column, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

        if(response == numPossibleValues-1)
        {
            selectMultipleResponses[response].setText(tr("--"));
        }
        else
        {
            QString responseText = dataOptions->attributeQuestionResponses[attribute].at(response);
            if(dataOptions->attributeType[attribute] == DataOptions::ordered)
            {
                // show response's starting number
                QRegularExpression startsWithNumber("^(\\d+)(.+)");
                QRegularExpressionMatch match = startsWithNumber.match(responseText);
                selectMultipleResponses[response].setText(match.captured(1));
            }
            else
            {
                // show response's preceding letter (letter repeated for responses after 26)
                selectMultipleResponses[response].setText((response<26 ? QString(char(response + 'A')) :
                                                                         QString(char(response%26 + 'A')).repeated(1+(response/26))));
            }
        }
        selectMultipleResponses[response].setFlat(true);
        selectMultipleResponses[response].setStyleSheet("Text-align:left");
        connect(&selectMultipleResponses[response], &QPushButton::clicked, &selectMultipleValues[response], &QCheckBox::toggle);
        theGrid->addWidget(&selectMultipleResponses[response], row++, column + 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
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
        explanation->setText("<html><hr><br><b>" +
                             tr("Students with these responses will not be placed on the same team:") +
                             "<br><br><br></b></html>");
    }
    else
    {
        explanation->setText("<html><hr><br><b>" +
                             tr("Each team will have at least one student with each of these responses:") +
                             "<br><br><br></b></html>");
    }
    adjustSize();
    updateExplanation();
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


void gatherAttributeValuesDialog::updateExplanation()
{
    if(gatherType == incompatible)
    {
        if(incompatibleValues.isEmpty())
        {
            explanation->setText("<html><hr><br><b>" + tr("Currently all responses are compatible.") + "<br></b></html>");
        }
        else
        {
            QString explanationText = tr("Students with these responses will not be placed on the same team:") + "<br>";
            for(const auto &pair : qAsConst(incompatibleValues))
            {
                QString firstResponse = (pair.first != -1) ? selectOneResponses[(pair.first)-1].text().split('.').at(0) :
                                                             tr("(value not set/unknown)");
                QString secondResponse = (pair.second != -1) ? selectOneResponses[(pair.second)-1].text().split('.').at(0) :
                                                             tr("(value not set/unknown)");
                explanationText += QString("&nbsp;&nbsp;&nbsp;&nbsp;") + QChar(0x2022) + " " +
                                   firstResponse + " " + QChar(0x27f7) + " " + secondResponse + "<br>";
            }
            // remove all html tags
            explanationText.remove("<html>");
            explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
        }
    }
    else
    {
        if(requiredValues.isEmpty())
        {
            explanation->setText("<html><hr><br><b>" + tr("Currently no responses are required.") + "<br></b></html>");
        }
        else
        {
            QString explanationText = tr("Each team will have at least one student with each of these responses:") + "<br>";
            for(const auto val : qAsConst(requiredValues))
            {
                QString responseText = (val != -1) ? selectMultipleResponses[(val)-1].text().split('.').at(0) : tr("(value not set/unknown)");
                explanationText += QString("&nbsp;&nbsp;&nbsp;&nbsp;") + QChar(0x2022) + " " + responseText + "<br>";
            }
            // remove all html tags
            explanationText.remove("<html>");
            explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
        }
    }
}


void gatherAttributeValuesDialog::addValues()
{
    if(gatherType == incompatible)
    {
        // create pairs for the primary value and each checked incompatible value
        for(int responseIndex1 = 0; responseIndex1 < numPossibleValues; responseIndex1++)
        {
            int responseValue1 = (responseIndex1 != (numPossibleValues-1)) ? responseIndex1+1 : -1;      // replace value for "unknown/not set" with -1
            for(int responseIndex2 = 0; responseIndex2 < numPossibleValues; responseIndex2++)
            {
                int responseValue2 = (responseIndex2 != (numPossibleValues-1)) ? responseIndex2+1 : -1;
                if(selectOneValues[responseIndex1].isChecked() && selectMultipleValues[responseIndex2].isChecked() &&
                     !incompatibleValues.contains(QPair<int,int>(responseValue1, responseValue2)) &&
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
            int responseValue = (responseIndex != (numPossibleValues-1)) ? responseIndex+1 : -1;      // replace value for "unknown/not set" with -1
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
