#include "attributeWidget.h"
#include <QButtonGroup>
#include <QGridLayout>

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(4);
    auto *attributeSettingsLayout = new QVBoxLayout(this);
    attributeSettingsLayout->setSpacing(2);
    setLayout(mainLayout);

    responsesLabel = new QLabel(this);
    responsesLabel->setTextFormat(Qt::RichText);
    responsesLabel->setWordWrap(true);
    responsesLabel->setIndent(10);
    responsesLabel->setStyleSheet(LABEL10PTWHITEBGSTYLE);
    auto *responsesFrame = new QFrame(this);
    responsesFrame->setLineWidth(1);
    responsesFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
    auto *hlay = new QHBoxLayout(responsesFrame);
    hlay->addWidget(responsesLabel);
    mainLayout->addWidget(responsesFrame, 7);

    // Create a horizontal layout
    auto *attributeDiversityLayout = new QHBoxLayout(this);

    // Create the "Diverse" card
    diverseCard = new FrameThatForwardsMouseClicks(this);
    diverseCard->setFixedSize(100, 100);
    diverseCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    diverseCard->setToolTip("Teammates will have a range of responses to this question.");
    diverseButton = new QRadioButton(this);
    diverseButton->setIcon(QIcon(":/icons_new/diverse.png"));
    diverseButton->setIconSize(QSize(50, 50));
    diverseButton->setStyleSheet("font-size: 15px;");
    auto *diverseLayout = new QVBoxLayout(diverseCard);
    QLabel *diverseLabel = new QLabel("Diverse\nTeammates", this);
    diverseButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    diverseLayout->addWidget(diverseButton, 0, Qt::AlignTop);
    diverseLayout->addWidget(diverseLabel,0,Qt::AlignBottom);
    diverseCard->setStyleSheet(RADIOBUTTONCARDSELECTEDSSTYLE);
    diverseButton->setChecked(true);
    connect(diverseCard, &FrameThatForwardsMouseClicks::clicked, diverseButton, &QRadioButton::click);

    // Create the "Similar" card
    similarCard = new FrameThatForwardsMouseClicks(this);
    similarCard->setFixedSize(100, 100);
    similarCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    similarCard->setToolTip("Teammates will have similar responses to this question.");
    similarButton = new QRadioButton(this);
    similarButton->setIcon(QIcon(":/icons_new/similar.png"));
    similarButton->setIconSize(QSize(50, 50));
    similarButton->setStyleSheet("font-size: 15px;");
    auto *similarLayout = new QVBoxLayout(similarCard);
    QLabel *similarLabel = new QLabel("Similar\nTeammates", this);
    similarButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    similarLayout->addWidget(similarButton, 0, Qt::AlignTop);
    similarLayout->addWidget(similarLabel,0,Qt::AlignBottom);
    similarCard->setStyleSheet(RADIOBUTTONCARDUNSELECTEDSSTYLE);
    similarButton->setChecked(false);
    connect(similarCard, &FrameThatForwardsMouseClicks::clicked, similarButton, &QRadioButton::click);

    // Add the cards to the horizontal layout and update styles when toggled
    attributeDiversityLayout->addWidget(diverseCard);
    attributeDiversityLayout->addSpacing(4);
    attributeDiversityLayout->addWidget(similarCard);
    QButtonGroup *radioButtonGroup = new QButtonGroup(this);
    radioButtonGroup->addButton(similarButton);
    radioButtonGroup->addButton(diverseButton);
    connect(diverseButton, &QRadioButton::toggled, diverseCard, [this](bool checked) {
        diverseCard->setStyleSheet(checked ?
                                       RADIOBUTTONCARDSELECTEDSSTYLE :
                                       RADIOBUTTONCARDUNSELECTEDSSTYLE
                                   );
    });
    connect(similarButton, &QRadioButton::toggled, similarCard, [this](bool checked) {
        similarCard->setStyleSheet(checked ?
                                       RADIOBUTTONCARDSELECTEDSSTYLE :
                                       RADIOBUTTONCARDUNSELECTEDSSTYLE
                                   );
    });

    attributeSettingsLayout->addLayout(attributeDiversityLayout, 2);

    setRequiredValuesButton = new QPushButton(tr("Required Student"), this);
    setRequiredValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setRequiredValuesButton->setToolTip(REQUIREDTOOLTIP);
    setIncompatibleValuesButton = new QPushButton(tr("Separated Students"), this);
    setIncompatibleValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setIncompatibleValuesButton->setToolTip(INCOMPATTOOLTIP);

    attributeSettingsLayout->addWidget(setRequiredValuesButton, 1);
    QSpacerItem *buttonSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    attributeSettingsLayout->addItem(buttonSpacer);
    attributeSettingsLayout->addWidget(setIncompatibleValuesButton, 1);
    mainLayout->addLayout(attributeSettingsLayout, 3);
}

void AttributeWidget::setValues(int attribute, const DataOptions *const dataOptions, TeamingOptions *teamingOptions)
{
    if(attribute >= dataOptions->numAttributes) {
        responsesLabel->setText(tr("N/A"));
        // weight->setEnabled(false);
        diverseCard->setEnabled(false);
        similarCard->setEnabled(false);
        setRequiredValuesButton->setEnabled(false);
        setIncompatibleValuesButton->setEnabled(false);
        return;
    }

    updateQuestionAndResponses(attribute, dataOptions);

    if(dataOptions->attributeVals[attribute].size() == 1) {
        teamingOptions->attributeWeights[attribute] = 0;
        // weight->setEnabled(false);
        // weight->setToolTip(ONLYONETOOLTIP);
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        setRequiredValuesButton->setEnabled(true);
        setIncompatibleValuesButton->setEnabled(true);
    }
    else {
        // weight->setEnabled(true);
        // weight->setToolTip(TeamingOptions::WEIGHTTOOLTIP);
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        setRequiredValuesButton->setEnabled(true);
        setIncompatibleValuesButton->setEnabled(true);
    }

    // weight->setValue(double(teamingOptions->attributeWeights[attribute]));
    bool similar = (teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::similar);
    diverseButton->setChecked(!similar);
    similarButton->setChecked(similar);
}

void AttributeWidget::updateQuestionAndResponses(int attribute, const DataOptions *const dataOptions, const std::map<QString, int> &responseCounts)
{
    const auto type = dataOptions->attributeType[attribute];

    static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    QRegularExpressionMatch match;
    int responseNum = 0;

    QString responsesText = "";
    //Create Table to store text
    responsesText += tr("<u>Responses</u>");
    for(const auto &response : std::as_const(dataOptions->attributeQuestionResponses[attribute])) {
        responsesText += "<br><b>";
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
        responsesText += " (" + QString::number(responseCounts.empty() ?
                                              (dataOptions->attributeQuestionResponseCounts[attribute].at(response)) :
                                              (responseCounts.at(response))) +
                         " " + tr("students") + ")";
        responseNum++;

        /*
            Label with Bar Graph
            // Create a widget to hold labels with bars
            QWidget *container = new QWidget;
            QVBoxLayout *layout = new QVBoxLayout(container);

            // Data
            QStringList labels = {"Item A", "Item B", "Item C", "Item D"};
            QList<int> values = {75, 45, 90, 60};
            int maxValue = 100;

            // Create labels with bars
            for (int i = 0; i < labels.size(); ++i) {
                QWidget *row = new QWidget;
                QHBoxLayout *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);

                // Label
                QLabel *label = new QLabel(labels[i]);
                label->setFixedWidth(100);
                rowLayout->addWidget(label);

                // Bar container with overlay capability
                QWidget *barContainer = new QWidget;
                barContainer->setFixedSize(150, 20);  // Fixed size for all bars

                // Create bar as child of container
                QFrame *bar = new QFrame(barContainer);
                bar->setFrameStyle(QFrame::NoFrame);
                bar->setStyleSheet("background-color: #4CAF50; border-radius: 2px;");
                int barWidth = (values[i] * 150) / maxValue;
                bar->setGeometry(0, 0, barWidth, 20);

                // Create value label on top of bar
                QLabel *valueLabel = new QLabel(QString::number(values[i]), barContainer);
                valueLabel->setAlignment(Qt::AlignCenter);
                valueLabel->setStyleSheet("color: white; font-weight: bold;");
                valueLabel->setGeometry(0, 0, 150, 20);  // Full width for centering

                rowLayout->addWidget(barContainer);
                rowLayout->addStretch();
                layout->addWidget(row);
            }

        */

    }
    responsesLabel->setText(responsesText);
}
