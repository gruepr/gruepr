#include "attributeWidget.h"
#include "dialogs/attributeRulesDialog.h"
#include <QButtonGroup>
#include <QGridLayout>
#include <QPainter>

AttributeWidget::AttributeWidget(int attribute, const DataOptions *const incomingDataOptions,
                                 TeamingOptions *const incomingTeamingOptions, QWidget *parent)
    : QWidget(parent), attribute(attribute), dataOptions(incomingDataOptions), teamingOptions(incomingTeamingOptions)
{
    setContentsMargins(0,0,0,0);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(4);
    auto *attributeSettingsLayout = new QVBoxLayout();
    attributeSettingsLayout->setSpacing(2);
    auto *attributeResponsesLayout = new QVBoxLayout();
    attributeResponsesLayout->setSpacing(2);
    setLayout(mainLayout);

    auto *responsesFrame = new QFrame(this);
    responsesFrame->setStyleSheet(BLUEFRAME);
    responsesLayout = new QVBoxLayout(responsesFrame);
    responsesLayout->setContentsMargins(2, 2, 2, 2);
    responsesLayout->setSpacing(4);
    auto *responsesText = new QLabel(tr(" Responses:"));
    responsesText->setStyleSheet(LABEL12PTSTYLE);
    responsesLayout->addWidget(responsesText);
    attributeResponsesLayout->addWidget(responsesFrame);
    attributeResponsesLayout->addStretch();
    mainLayout->addLayout(attributeResponsesLayout, 7);

    // Create a horizontal layout
    auto *attributeDiversityLayout = new QHBoxLayout();

    // Create the "Diverse" card
    diverseCard = new FrameThatForwardsMouseClicks(this);
    diverseCard->setFixedSize(100, 100);
    diverseCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    diverseCard->setToolTip(DIVERSETOOLTIP);
    diverseButton = new QRadioButton(this);
    diverseButton->setIcon(QIcon(":/icons_new/diverse.png"));
    diverseButton->setIconSize(QSize(50, 50));
    diverseButton->setStyleSheet("font-size: 15px;");
    auto *diverseLayout = new QVBoxLayout(diverseCard);
    auto *diverseLabel = new QLabel("Diverse\nTeammates", this);
    diverseButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    diverseLayout->addWidget(diverseButton, 0, Qt::AlignTop);
    diverseLayout->addWidget(diverseLabel,0,Qt::AlignBottom);
    diverseCard->setStyleSheet(QString() + RADIOBUTTONCARDSELECTEDSSTYLE + RADIOBUTTONSTYLE);
    diverseButton->setChecked(true);
    connect(diverseCard, &FrameThatForwardsMouseClicks::clicked, diverseButton, &QRadioButton::click);

    // Create the "Similar" card
    similarCard = new FrameThatForwardsMouseClicks(this);
    similarCard->setFixedSize(100, 100);
    similarCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    similarCard->setToolTip(SIMILARTOOLTIP);
    similarButton = new QRadioButton(this);
    similarButton->setIcon(QIcon(":/icons_new/similar.png"));
    similarButton->setIconSize(QSize(50, 50));
    similarButton->setStyleSheet("font-size: 15px;");
    auto *similarLayout = new QVBoxLayout(similarCard);
    auto *similarLabel = new QLabel("Similar\nTeammates", this);
    similarButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
    similarLayout->addWidget(similarButton, 0, Qt::AlignTop);
    similarLayout->addWidget(similarLabel,0,Qt::AlignBottom);
    similarCard->setStyleSheet(QString() + RADIOBUTTONCARDUNSELECTEDSSTYLE + RADIOBUTTONSTYLE);
    similarButton->setChecked(false);
    connect(similarCard, &FrameThatForwardsMouseClicks::clicked, similarButton, &QRadioButton::click);

    // Add the cards to the horizontal layout and update styles when toggled
    attributeDiversityLayout->addWidget(diverseCard);
    attributeDiversityLayout->addSpacing(4);
    attributeDiversityLayout->addWidget(similarCard);
    auto *radioButtonGroup = new QButtonGroup(this);
    radioButtonGroup->addButton(similarButton);
    radioButtonGroup->addButton(diverseButton);
    connect(diverseButton, &QRadioButton::toggled, diverseCard, [this](bool checked) {
        diverseCard->setStyleSheet(checked ?
                                       QString() + RADIOBUTTONCARDSELECTEDSSTYLE + RADIOBUTTONSTYLE :
                                       QString() + RADIOBUTTONCARDUNSELECTEDSSTYLE + RADIOBUTTONSTYLE
                                   );
    });
    connect(similarButton, &QRadioButton::toggled, similarCard, [this](bool checked) {
        similarCard->setStyleSheet(checked ?
                                       QString() + RADIOBUTTONCARDSELECTEDSSTYLE + RADIOBUTTONSTYLE :
                                       QString() + RADIOBUTTONCARDUNSELECTEDSSTYLE + RADIOBUTTONSTYLE
                                   );
    });
    connect(diverseButton, &QRadioButton::clicked, this, [this, attribute]() {
        teamingOptions->attributeDiversity[attribute] = Criterion::AttributeDiversity::diverse;
    });
    connect(similarButton, &QRadioButton::clicked, this, [this, attribute]() {
        teamingOptions->attributeDiversity[attribute] = Criterion::AttributeDiversity::similar;
    });

    attributeSettingsLayout->addLayout(attributeDiversityLayout, 2);

    setRequiredValuesButton = new QPushButton(tr("Required Student"), this);
    setRequiredValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setRequiredValuesButton->setToolTip(REQUIREDTOOLTIP);
    setIncompatibleValuesButton = new QPushButton(tr("Separated Students"), this);
    setIncompatibleValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    setIncompatibleValuesButton->setToolTip(INCOMPATTOOLTIP);
    connect(setRequiredValuesButton, &QPushButton::clicked, this, [this] {
        const int currAttribute = this->attribute;

        //Open specialized dialog box to collect attribute values that are required on each team
        auto *win = new AttributeRulesDialog(currAttribute, *dataOptions, *teamingOptions, AttributeRulesDialog::TypeOfRules::required, this);

        //If user clicks OK, replace with new sets of values
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            teamingOptions->haveAnyRequiredAttributes[currAttribute] = !(win->requiredValues.isEmpty());
            teamingOptions->requiredAttributeValues[currAttribute] = win->requiredValues;

            teamingOptions->haveAnyIncompatibleAttributes[currAttribute] = !(win->incompatibleValues.isEmpty());
            teamingOptions->incompatibleAttributeValues[currAttribute] = win->incompatibleValues;
        }

        delete win;
    });
    connect(setIncompatibleValuesButton, &QPushButton::clicked, this, [this] {
        const int currAttribute = this->attribute;

        //Open specialized dialog box to collect attribute values that are required on each team
        auto *win = new AttributeRulesDialog(currAttribute, *dataOptions, *teamingOptions, AttributeRulesDialog::TypeOfRules::incompatible, this);

        //If user clicks OK, replace with new sets of values
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            teamingOptions->haveAnyRequiredAttributes[currAttribute] = !(win->requiredValues.isEmpty());
            teamingOptions->requiredAttributeValues[currAttribute] = win->requiredValues;

            teamingOptions->haveAnyIncompatibleAttributes[currAttribute] = !(win->incompatibleValues.isEmpty());
            teamingOptions->incompatibleAttributeValues[currAttribute] = win->incompatibleValues;
        }

        delete win;
    });

    attributeSettingsLayout->addWidget(setRequiredValuesButton, 1);
    auto *buttonSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    attributeSettingsLayout->addItem(buttonSpacer);
    attributeSettingsLayout->addWidget(setIncompatibleValuesButton, 1);
    mainLayout->addLayout(attributeSettingsLayout, 3);
}

void AttributeWidget::setValues()
{
    if(attribute >= dataOptions->numAttributes) {
        diverseCard->setEnabled(false);
        similarCard->setEnabled(false);
        setRequiredValuesButton->setEnabled(false);
        setIncompatibleValuesButton->setEnabled(false);
        return;
    }

    updateResponses();

    if(dataOptions->attributeVals[attribute].size() == 1) {
        teamingOptions->attributeWeights[attribute] = 0;
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        setRequiredValuesButton->setEnabled(true);
        setIncompatibleValuesButton->setEnabled(true);
    }
    else {
        diverseCard->setEnabled(true);
        similarCard->setEnabled(true);
        setRequiredValuesButton->setEnabled(true);
        setIncompatibleValuesButton->setEnabled(true);
    }

    const bool similar = (teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::similar);
    diverseButton->setChecked(!similar);
    similarButton->setChecked(similar);
}

void AttributeWidget::updateResponses(const std::map<QString, int> &responseCounts)
{
    setUpdatesEnabled(false);

    // clear the responsesLayout
    for(auto &responseRow : responseRows) {
        responsesLayout->removeWidget(responseRow);
        responseRow->deleteLater();
    }
    responseRows.clear();

    const auto type = dataOptions->attributeType[attribute];

    static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
    QRegularExpressionMatch match;

    auto &responseCountRef = responseCounts.empty() ? dataOptions->attributeQuestionResponseCounts[attribute] : responseCounts;
    int totNumResponses = 0;
    for (const auto& pair : responseCountRef) {
        totNumResponses += pair.second;
    }

    int responseNum = 0;
    for(const auto &response : std::as_const(dataOptions->attributeQuestionResponses[attribute])) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 2, 2);
        rowLayout->setSpacing(4);

        // Response count bar graph
        auto *responseBarGraph = new ResponseLabelBarGraph(responseCountRef.at(response), totNumResponses, BARGRAPHWIDTH, row);

        // Response value
        QString label, text;
        if((type == DataOptions::AttributeType::ordered) || (type == DataOptions::AttributeType::multiordered)) {
            // show response with starting number
            match = startsWithInteger.match(response);
            label = match.captured(1);
            text = " " + response.mid(match.capturedLength(1));
        }
        else if((type == DataOptions::AttributeType::categorical) || (type == DataOptions::AttributeType::multicategorical)) {
            // show response with a preceding letter (letter repeated for responses after 26)
            label = (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
            text = ". " + response;
        }
        else {
            // timezone, show response with GMT
            QString timezoneName;
            float hours=0, minutes=0, offsetFromGMT=0;
            if(DataOptions::parseTimezoneInfoFromText(response, timezoneName, hours, minutes, offsetFromGMT)) {
                const QString GMTtext = QString("[GMT %1%2:%3]").arg(hours >= 0 ? "+" : "").arg(int(hours)).arg(int(minutes), 2, 10, QChar('0'));
                label = GMTtext;
                text = " " + timezoneName;
            }
            else {
                label = (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
                text = ". " + response;
            }
        }
        auto *responseLabel = new QLabel(QString("<b>%1</b>%2").arg(label, text), row);
        responseLabel->setStyleSheet(LABEL10PTSTYLE);
        responseLabel->setWordWrap(true);

        rowLayout->addWidget(responseBarGraph);
        rowLayout->addWidget(responseLabel);
        row->setToolTip(QString::number(responseCountRef.at(response)) + tr(" students gave response ") + label + ".");

        responseRows << row;
        responsesLayout->addWidget(row);
        responseNum++;
    }
    setUpdatesEnabled(true);
}


ResponseLabelBarGraph::ResponseLabelBarGraph(int value, int maxValue, int barWidth, QWidget *parent)
    : QWidget(parent), m_value(value), m_barWidth(barWidth)
{
    setFixedSize(barWidth, 20);
    if(maxValue == 0) {
        m_maxValue = 1;
        return;
    }
    m_maxValue = maxValue;
}

void ResponseLabelBarGraph::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int barHeight = height();

    // Draw bar background
    painter.fillRect(0, 0, m_barWidth, barHeight, QColor(Qt::white));

    // Draw filled portion
    painter.fillRect(1, 1, 1 + (((m_barWidth * m_value) - 1) / m_maxValue), barHeight - 1, QColor(DEEPWATERHEX));   // using ceiling division

    // Draw border
    painter.setPen(OPENWATERHEX);
    painter.drawRect(0, 0, m_barWidth, barHeight);
}
