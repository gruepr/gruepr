#include "gatherAttributeValuesDialog.h"
#include <QButtonGroup>
#include <QRegularExpression>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which attribute values should be required on each team or which pairings should be disallowed on the same team
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherAttributeValuesDialog::gatherAttributeValuesDialog(const int attribute, const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions,
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

    QString attributeDescription = "<html><br><b>" + tr("Attribute") + " " + QString::number(attribute + 1) + ":</b><br>";
    attributeDescription += dataOptions->attributeQuestionText.at(attribute) +"<hr>";
    for(int response = 0; response < numPossibleValues; response++)
    {
        if(response == numPossibleValues - 1)
        {
            attributeDescription += tr("-. value not set/unknown");
        }
        else if(dataOptions->attributeIsOrdered[attribute])
        {
            // show reponse with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            attributeDescription += match.captured(1) + match.captured(2);
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            attributeDescription += (response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))) +
                                         ". " + dataOptions->attributeQuestionResponses[attribute].at(response);
        }
        attributeDescription += "<br>";
    }

    attributeQuestion = new QLabel(this);
    attributeQuestion->setText(attributeDescription + "</html>");
    attributeQuestion->setWordWrap(true);
    theGrid->addWidget(attributeQuestion, 0, 0, 1, -1);

    auto *hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    theGrid->addWidget(hline, 1, 0, 1, -1);

    if(gatherType == incompatible)
    {
        // a radio button and a label for each response value
        selectOneExplanation = new QLabel(this);
        selectOneExplanation->setText("<html>" + tr("Prevent placing students with this response:") + "</html>");
        selectOneExplanation->setWordWrap(true);
        theGrid->addWidget(selectOneExplanation, 2, 0, 1, 2);

        selectOneValues = new QRadioButton[numPossibleValues];
        selectOneResponses = new QPushButton[numPossibleValues];
        selectOneValuesGroup = new QButtonGroup(this);
        for(int response = 0; response < numPossibleValues; response++)
        {
            theGrid->addWidget(&selectOneValues[response], response + 3, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
            selectOneValuesGroup->addButton(&selectOneValues[response]);

            if(response == numPossibleValues - 1)
            {
                selectOneResponses[response].setText(tr("-"));
            }
            else if(dataOptions->attributeIsOrdered[attribute])
            {
                // show reponse with starting number
                QRegularExpression startsWithNumber("^(\\d+)(.+)");
                QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
                selectOneResponses[response].setText(match.captured(1));
            }
            else
            {
                // show response with a preceding letter (letter repeated for responses after 26)
                selectOneResponses[response].setText((response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))));
            }
            selectOneResponses[response].setFlat(true);
            selectOneResponses[response].setStyleSheet("Text-align:left");
            connect(&selectOneResponses[response], &QPushButton::clicked, &selectOneValues[response], &QRadioButton::toggle);
            theGrid->addWidget(&selectOneResponses[response], response + 3, 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
        }

        auto *vline = new QFrame(this);
        vline->setFrameShape(QFrame::VLine);
        vline->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(vline, 2, 2, numPossibleValues + 1, 1);
    }

    // a checkbox and a label for each response value to set as required or as incompatible with the primary
    selectMultipleExplanation = new QLabel(this);
    int column;
    if(gatherType == incompatible)
    {
        selectMultipleExplanation->setText("<html>" + tr("on the same team as students with any of these responses:") + "</html>");
        column = 3;
    }
    else
    {
        selectMultipleExplanation->setText("<html>" + tr("Ensure each team has at least one student with each of these responses:") + "</html>");
        column = 0;
    }
    selectMultipleExplanation->setWordWrap(true);
    theGrid->addWidget(selectMultipleExplanation, 2, column, 1, -1);

    selectMultipleOnesValues = new QCheckBox[numPossibleValues];
    selectMultipleOnesResponses = new QPushButton[numPossibleValues];
    for(int response = 0; response < numPossibleValues; response++)
    {
        theGrid->addWidget(&selectMultipleOnesValues[response], response + 3, column, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

        if(response == numPossibleValues - 1)
        {
            selectMultipleOnesResponses[response].setText(tr("-"));
        }
        else if(dataOptions->attributeIsOrdered[attribute])
        {
            // show response with starting number
            QRegularExpression startsWithNumber("^(\\d+)(.+)");
            QRegularExpressionMatch match = startsWithNumber.match(dataOptions->attributeQuestionResponses[attribute].at(response));
            selectMultipleOnesResponses[response].setText(match.captured(1));
        }
        else
        {
            // show response with a preceding letter (letter repeated for responses after 26)
            selectMultipleOnesResponses[response].setText((response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26))));
        }
        selectMultipleOnesResponses[response].setFlat(true);
        selectMultipleOnesResponses[response].setStyleSheet("Text-align:left");
        connect(&selectMultipleOnesResponses[response], &QPushButton::clicked, &selectMultipleOnesValues[response], &QCheckBox::toggle);
        theGrid->addWidget(&selectMultipleOnesResponses[response], response + 3, column + 1, 1, 1,  Qt::AlignLeft | Qt::AlignVCenter);
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
    theGrid->addWidget(addValuesButton, numPossibleValues + 4, 0, 1, -1, Qt::AlignCenter);

    //explanatory text of which response pairs will be considered incompatible
    explanation = new QLabel(this);
    explanation->clear();
    theGrid->addWidget(explanation, numPossibleValues + 5, 0, 1, -1);
    theGrid->setRowStretch(numPossibleValues + 6, 1);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numPossibleValues + 7, DIALOG_SPACER_ROWHEIGHT);
    resetValuesButton = new QPushButton(this);
    resetValuesButton->setText(tr("&Clear all values"));
    resetValuesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    theGrid->addWidget(resetValuesButton, numPossibleValues + 8, 0, 1, 2);
    connect(resetValuesButton, &QPushButton::clicked, this, &gatherAttributeValuesDialog::clearAllValues);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numPossibleValues + 8, 3, -1, -1);
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
    delete [] selectMultipleOnesValues;
    delete [] selectMultipleOnesResponses;
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
                explanationText += "&nbsp;&nbsp;&nbsp;&nbsp;" + selectOneResponses[(pair.first)-1].text().split('.').at(0) +
                        " " + QChar(0x27f7) + " " + selectOneResponses[(pair.second)-1].text().split('.').at(0) + "<br>";
            }
            // remove all html tags, replace "-" with "not set/unknown"
            explanationText.remove("<html>").replace("-", tr("not set/unknown"));
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
                explanationText += "&nbsp;&nbsp;&nbsp;&nbsp;" + selectMultipleOnesResponses[(val)-1].text().split('.').at(0) + "<br>";
            }
            // remove all html tags, replace "-" with "not set/unknown"
            explanationText.remove("<html>").replace("-", tr("not set/unknown"));
            explanation->setText("<html><hr><br><b>" + explanationText + "</b></html>");
        }
    }
}


void gatherAttributeValuesDialog::addValues()
{
    if(gatherType == incompatible)
    {
        // create pairs for the primary value and each checked incompatible value
        for(int response1 = 0; response1 < numPossibleValues; response1++)
        {
            for(int response2 = 0; response2 < numPossibleValues; response2++)
            {
                if(selectOneValues[response1].isChecked() && selectMultipleOnesValues[response2].isChecked() &&
                     !incompatibleValues.contains(QPair<int,int>(response1 + 1, response2 + 1)) &&
                     !incompatibleValues.contains(QPair<int,int>(response2 + 1, response1 + 1)) )
                {
                    int smaller = std::min(response1+1, response2+1), larger = std::max(response1+1, response2+1);
                    incompatibleValues << QPair<int,int>(smaller, larger);
                }
            }
        }
        std::sort(incompatibleValues.begin(), incompatibleValues.end(), [](const QPair<int,int> a, const QPair<int,int> b)
                                                                                {return (a.first != b.first? a.first < b.first : a.second < b.second);});
    }
    else
    {
        // set each checked value as required
        for(int response1 = 0; response1 < numPossibleValues; response1++)
        {
            if(selectMultipleOnesValues[response1].isChecked() && !requiredValues.contains(response1 + 1))
            {
                requiredValues << response1 + 1;
            }
        }
        std::sort(requiredValues.begin(), requiredValues.end());
    }

    updateExplanation();

    // reset checkboxes
    for(int response = 0; response < numPossibleValues; response++)
    {
        selectMultipleOnesValues[response].setChecked(false);
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
