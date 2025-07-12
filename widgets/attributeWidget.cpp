#include "attributeWidget.h"
#include "qbuttongroup.h"
#include "qradiobutton.h"
#include <QApplication>
#include <QFrame>
#include <QGridLayout>

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    // auto *theGrid = new QGridLayout(this);
    // theGrid->setContentsMargins(0,0,0,0);
    // theGrid->setHorizontalSpacing(4);
    // theGrid->setVerticalSpacing(2);
    // setLayout(theGrid);
    // int row = 0, column = 0;

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(4);

    auto *attributeSettingsLayout = new QVBoxLayout(this);
    attributeSettingsLayout->setSpacing(2);


    setLayout(mainLayout);


    // questionLabel = new QLabel(this);
    // questionLabel->setStyleSheet(LABEL12PTSTYLE);
    // questionLabel->setTextFormat(Qt::RichText);
    // questionLabel->setWordWrap(true);
    // theGrid->addWidget(questionLabel, row++, column, 1, -1);

    responsesLabel = new QLabel(this);
    responsesLabel->setTextFormat(Qt::RichText);
    responsesLabel->setWordWrap(true);
    responsesLabel->setIndent(10);
    auto *responsesFrame = new QFrame(this);
    responsesFrame->setLineWidth(1);
    responsesFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
    auto *hlay = new QHBoxLayout(responsesFrame);
    hlay->addWidget(responsesLabel);
    mainLayout->addWidget(responsesFrame, 7);

    // auto *weightPreLabel = new QLabel(tr("Weight:"), this);
    //theGrid->addWidget(weightPreLabel, row, column++, Qt::AlignLeft);
    // weight = new QDoubleSpinBox(this);
    // weight->setFocusPolicy(Qt::StrongFocus);
    // weight->installEventFilter(new MouseWheelBlocker(weight));
    // weight->setDecimals(1);
    // weight->setMinimum(0);
    // weight->setMaximum(TeamingOptions::MAXWEIGHT);
    // weight->setSuffix("  /  " + QString::number(TeamingOptions::MAXWEIGHT) + "   ");
    // weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
    // weight->setValue(1);
    // theGrid->addWidget(weight, row, column++);
    // theGrid->setColumnStretch(column++, 1);

    // auto *attributeDiversityLabel = new QLabel(tr("Attribute Diversity"), this);
    // attributeDiversityLabel->setToolTip(HOMOGENTOOLTIP);
    // attributeDiversityLabel->setAlignment(Qt::AlignCenter);
    // attributeSettingsLayout->addWidget(attributeDiversityLabel, 1, Qt::AlignCenter);

    // attribute_diversity_slider = new AttributeDiversitySlider(this);
    // attribute_diversity_slider->setToolTip(HOMOGENTOOLTIP);
    // QLabel *sliderLabel1 = new QLabel("Diverse", this);
    // QLabel *sliderLabel2 = new QLabel("Ignore", this);
    // QLabel *sliderLabel3 = new QLabel("Similar", this);
    // attributeSettingsLayout->addWidget(attribute_diversity_slider);
    // // Align labels under the slider evenly
    // QHBoxLayout *sliderLabelsLayout = new QHBoxLayout(this);
    // sliderLabelsLayout->addWidget(sliderLabel1);
    // sliderLabelsLayout->addWidget(sliderLabel2);
    // sliderLabelsLayout->addWidget(sliderLabel3);

    // Create a horizontal layout
    auto *attributeDiversityLayout = new QHBoxLayout(this);


    // Create the "Diverse" card
    diverseButton = new QRadioButton(this);
    diverseButton->setToolTip("All of the students in the team will have a wide range of responses to this question.");
    diverseCard = new QFrame(this);
    diverseCard->setObjectName("diverseCard");
    //diverseCard->setStyleSheet(RADIOBUTTONCARDSSTYLE);
    diverseButton->setIcon(QIcon(":/icons_new/heterogeneous.png"));
    diverseButton->setIconSize(QSize(40, 40)); // Bigger icon
    diverseButton->setStyleSheet("font-size: 15px;");
    auto *diverseLayout = new QVBoxLayout(diverseCard);
    QLabel *diverseLabel = new QLabel("Diverse", this);
    diverseButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    diverseLayout->addWidget(diverseButton, 0, Qt::AlignTop);
    diverseLayout->addWidget(diverseLabel,0,Qt::AlignBottom);
    diverseButton->setChecked(false);

    // Create the "Similar" card
    similarButton = new QRadioButton(this);
    similarButton->setToolTip("All of the students in the team will have similar responses to this question.");
    similarCard = new QFrame(this);
    similarCard->setObjectName("similarCard");
    //similarCard->setStyleSheet(RADIOBUTTONCARDSSTYLE);
    similarButton->setIcon(QIcon(":/icons_new/homogeneous.png"));
    similarButton->setIconSize(QSize(40, 40));
    similarButton->setStyleSheet("font-size: 15px;");
    auto *similarLayout = new QVBoxLayout(similarCard);
    QLabel *similarLabel = new QLabel("Similar", this);
    similarButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    similarLayout->addWidget(similarButton, 0, Qt::AlignTop);
    similarLayout->addWidget(similarLabel,0,Qt::AlignBottom);
    similarButton->setChecked(false);

    // Set square size for the "Diverse" button
    diverseCard->setFixedSize(100, 100); // You can change 100 to any size you prefer

    // Set square size for the "Similar" button
    similarCard->setFixedSize(100, 100); // Same here, adjust as needed

    // Optionally, keep the expanding size policy to ensure proper layout behavior
    diverseCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    similarCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    diverseCard->setStyleSheet("QFrame#diverseCard {border-color: " DEEPWATERHEX "; color: " DEEPWATERHEX "; background-color: white;}");
    similarCard->setStyleSheet("QFrame#similarCard {border-color: " DEEPWATERHEX "; color: " DEEPWATERHEX "; background-color: white;}");
    diverseLabel->setStyleSheet("background-color: white;");
    similarLabel->setStyleSheet("background-color: white;");

    QButtonGroup *radioButtonGroup = new QButtonGroup(this);
    radioButtonGroup->addButton(similarButton);
    radioButtonGroup->addButton(diverseButton);

    // Add the cards to the horizontal layout
    attributeDiversityLayout->addWidget(diverseCard);
    attributeDiversityLayout->addWidget(similarCard);

    //Signal to update style on selection
    connect(diverseButton, &QRadioButton::toggled, [=](bool checked) {
        diverseCard->setStyleSheet(checked ?
                                       "" :
                                       "QFrame#diverseCard {border-color: " DEEPWATERHEX "; color: " DEEPWATERHEX "; background-color: white;}"
                                   );
        // Update QLabel background color based on selection
        diverseLabel->setStyleSheet(checked ? "" : "background-color: white;");
    });
    connect(similarButton, &QRadioButton::toggled, [=](bool checked) {
        similarCard->setStyleSheet(checked ?
                                       "" :
                                       "QFrame#similarCard {border-color: " DEEPWATERHEX "; color: " DEEPWATERHEX "; background-color: white;}"
                                   );
        // Update QLabel background color based on selection
        similarLabel->setStyleSheet(checked ? "" : "background-color: white;");
    });


    attributeSettingsLayout->addLayout(attributeDiversityLayout, 2);

    setRequiredStudentsButton = new QPushButton(tr("Enforce Students Together"), this);
    setRequiredStudentsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    attributeSettingsLayout->addWidget(setRequiredStudentsButton, 1);
    QSpacerItem *buttonSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    attributeSettingsLayout->addItem(buttonSpacer);

    setIncompatibleStudentsButton = new QPushButton(tr("Enforce Students Separation"), this);
    setIncompatibleStudentsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    attributeSettingsLayout->addWidget(setIncompatibleStudentsButton, 1);
    mainLayout->addLayout(attributeSettingsLayout, 3);
}

void AttributeWidget::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    if(attribute >= dataOptions->numAttributes) {
        //questionLabel->setText(tr("N/A"));
        responsesLabel->setText(tr("N/A"));
        // weight->setEnabled(false);
        diverseCard->setEnabled(false);
        similarCard->setEnabled(false);
        setRequiredStudentsButton->setEnabled(false);
        setIncompatibleStudentsButton->setEnabled(false);
        return;
    }

    updateQuestionAndResponses(attribute, dataOptions);

    if(dataOptions->attributeVals[attribute].size() == 1) {
        teamingOptions->attributeWeights[attribute] = 0;
        // weight->setEnabled(false);
        // weight->setToolTip(ONLYONETOOLTIP);
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        //attribute_diversity_slider->setToolTip(ONLYONETOOLTIP);
        setRequiredStudentsButton->setEnabled(true);
        setIncompatibleStudentsButton->setEnabled(true);
        //requiredIncompatsButton->setToolTip(ONLYONETOOLTIP);
    }
    else {
        // weight->setEnabled(true);
        // weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        //attribute_diversity_slider->setToolTip(HOMOGENTOOLTIP);
        setRequiredStudentsButton->setEnabled(true);
        setIncompatibleStudentsButton->setEnabled(true);
        // requiredIncompatsButton->setToolTip(REQUIREDINCOMPATTOOLTIP);
    }

    // weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    //Convert AttributeDiversity to Slider Index then set slider value
    bool homogeneous = teamingOptions->attributeDiversity[attribute];
    diverseButton->setChecked(!homogeneous);
    similarButton->setChecked(homogeneous);
}

void AttributeWidget::updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts)
{
    const auto type = dataOptions->attributeType[attribute];

    QString questionText = "<html>" + dataOptions->attributeQuestionText.at(attribute);
    if((type == DataOptions::AttributeType::multicategorical) || (type == DataOptions::AttributeType::multiordered)) {
        questionText += "<br><i>Multiple responses allowed.</i>";
    }
    questionText += "</html>";
    //questionLabel->setText(questionText);

    static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    QRegularExpressionMatch match;
    int responseNum = 0;
    bool first = true;

    QString responsesText = "";
    //Create Table to store text
    responsesText += "<u>Responses</u>";
    responsesText += "<br>"; //add a new row
    for(const auto &response : std::as_const(dataOptions->attributeQuestionResponses[attribute])) {
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

        // Display response count
        // responsesText += QString::number(responseCounts.empty() ?
        //                                      (dataOptions->attributeQuestionResponseCounts[attribute].at(response)) :
        //                                      (responseCounts.at(response)));
        responsesText += "<br>"; //add a new row
        responseNum++;
    }
    responsesLabel->setText(responsesText);
}
