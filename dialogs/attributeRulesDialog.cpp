#include "attributeRulesDialog.h"
#include "ui_attributeRulesDialog.h"
#include <QPushButton>
#include <QTabBar>

AttributeRulesDialog::AttributeRulesDialog(const int attribute, const DataOptions &dataOptions, const TeamingOptions &teamingOptions, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AttributeRulesDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Response rules - Q") + QString::number(attribute + 1));
    ui->tabWidget->tabBar()->setExpanding(true);
    ui->tabWidget->setStyleSheet(QString() + TABWIDGETSTYLE + LABELSTYLE);
    ui->reqScrollArea->setStyleSheet(QString("QScrollArea{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; "
                                                          "border: 1px solid; border-color: " AQUAHEX ";}") + SCROLLBARSTYLE);
    ui->reqScrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    ui->incompScrollArea->setStyleSheet(QString("QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX ";}")
                                                + SCROLLBARSTYLE);
    ui->incompScrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    auto *resetValuesButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    resetValuesButton->setText(tr("Clear all rules"));
    resetValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(resetValuesButton, &QPushButton::clicked, this, &AttributeRulesDialog::clearAllValues);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AttributeRulesDialog::Ok);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    requiredValues = teamingOptions.requiredAttributeValues[attribute];
    incompatibleValues = teamingOptions.incompatibleAttributeValues[attribute];
    DataOptions::AttributeType attributeType = dataOptions.attributeType[attribute];
    attributeValues.clear();
    attributeValues.reserve(dataOptions.attributeVals[attribute].size());
    auto valueIter = dataOptions.attributeVals[attribute].cbegin();
    for(const auto &response : dataOptions.attributeQuestionResponses[attribute]) {
        attributeValues.append({*valueIter, response});
        valueIter++;
    }
    attributeValues.append({-1, tr("Unknown / no response")});
    numPossibleValues = int(attributeValues.size());

    ui->questionNumLabel->setText(tr("Multiple choice question ") + QString::number(attribute + 1));
    ui->questionNumLabel_2->setText(tr("Multiple choice question ") + QString::number(attribute + 1));
    ui->questionTextLabel->setText(dataOptions.attributeQuestionText.at(attribute));
    ui->questionTextLabel->setStyleSheet(QString(LABELSTYLE).replace("10pt", "12pt"));
    ui->questionTextLabel_2->setText(dataOptions.attributeQuestionText.at(attribute));
    ui->questionTextLabel_2->setStyleSheet(QString(LABELSTYLE).replace("10pt", "12pt"));
    QString responses;
    bool firstTime = true;
    for(const auto &attributeValue : qAsConst(attributeValues)) {
        // create numbered list of responses (prefixing a number for the token value of -1 and
        // for unordered responses, since they don't already start with number)
        if(!firstTime) {
            responses += "<br>";
        }
        firstTime = false;
        if((attributeValue.value == -1) ||
           ((attributeType != DataOptions::AttributeType::ordered) && (attributeType != DataOptions::AttributeType::multiordered))) {
            responses += valuePrefix(attributeValue.value, attributeType) + ". ";
        }
        responses += attributeValue.response;
    }
    ui->responseValuesLabel->setText(responses);
    ui->responseValuesLabel_2->setText(responses);

    ui->reqExplanationLabel->setText(tr("Select the response(s) for which each team must have at least one student."));
    ui->incompExplanationLabel->setText(tr("For each response, select the other response(s) that should "
                                           "prevent two students from being placed on the same team."));
    QList<QFrame *> incompFrames;
    incompFrames.reserve(numPossibleValues);
    QList<QVBoxLayout *> incompFrameLayouts;
    incompFrameLayouts.reserve(numPossibleValues);
    QList<QLabel *> incompResponseLabels;
    incompResponseLabels.reserve(numPossibleValues);
    QList<QCheckBox *> incompCheckboxes;
    incompCheckboxes.reserve(numPossibleValues);
    QList<QFrame *> incompSepLines;
    incompSepLines.reserve(numPossibleValues);
    incompCheckboxList.reserve(numPossibleValues);
    reqCheckboxes.reserve(numPossibleValues);
    for(const auto &attributeValue : qAsConst(attributeValues)) {
        reqCheckboxes << new QCheckBox(this);
        reqCheckboxes.last()->setStyleSheet(QString(CHECKBOXSTYLE).replace("10pt;", "12pt; color: " DEEPWATERHEX ";"));
        reqCheckboxes.last()->setText(valuePrefix(attributeValue.value, attributeType));
        reqCheckboxes.last()->setChecked(requiredValues.contains(attributeValue.value));
        ui->reqScrollAreaLayout->addWidget(reqCheckboxes.last(), 0, Qt::AlignTop);

        incompFrames << new QFrame(this);
        incompFrames.last()->setStyleSheet("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; "
                                                   "border: 1px solid; border-color: " AQUAHEX ";}");
        incompFrameLayouts << new QVBoxLayout;
        incompFrames.last()->setLayout(incompFrameLayouts.last());
        QString responseLabelText;
        if(attributeValue.value == -1) {
            responseLabelText = attributeValue.response;
        }
        else if((attributeType == DataOptions::AttributeType::ordered) ||
                (attributeType == DataOptions::AttributeType::multiordered)) {
            responseLabelText = tr("Response ") + attributeValue.response;
        }
        else {
            responseLabelText = tr("Response ") + valuePrefix(attributeValue.value, attributeType) + ": " + attributeValue.response;
        }
        incompResponseLabels << new QLabel(responseLabelText, this);
        incompResponseLabels.last()->setStyleSheet(QString(LABELSTYLE).replace("10pt", "12pt"));
        incompResponseLabels.last()->setWordWrap(true);
        incompFrameLayouts.last()->addWidget(incompResponseLabels.last());
        incompSepLines << new QFrame(this);
        incompSepLines.last()->setStyleSheet("border-color: " AQUAHEX);
        incompSepLines.last()->setLineWidth(1);
        incompSepLines.last()->setFrameShape(QFrame::HLine);
        incompSepLines.last()->setFrameShadow(QFrame::Plain);
        incompSepLines.last()->setFixedHeight(1);
        incompFrameLayouts.last()->addWidget(incompSepLines.last(), 0, Qt::AlignVCenter);
        incompCheckboxes.clear();
        for(const auto &attributeValue2 : qAsConst(attributeValues)) {
            incompCheckboxes << new QCheckBox(this);
            incompCheckboxes.last()->setStyleSheet(QString(CHECKBOXSTYLE).replace("10pt", "12pt"));
            incompCheckboxes.last()->setText(valuePrefix(attributeValue2.value, attributeType));
            incompCheckboxes.last()->setChecked(incompatibleValues.contains({attributeValue.value, attributeValue2.value}) ||
                                                incompatibleValues.contains({attributeValue2.value, attributeValue.value}));
            incompFrameLayouts.last()->addWidget(incompCheckboxes.last(), Qt::AlignTop);
        }
        incompCheckboxList << incompCheckboxes;
        ui->incompScrollAreaLayout->addWidget(incompFrames.last(), 0, Qt::AlignTop);
    }
    ui->reqVerticalLayout->addStretch(0);
}

AttributeRulesDialog::~AttributeRulesDialog()
{
    delete ui;
}

QString AttributeRulesDialog::valuePrefix(int value, DataOptions::AttributeType attributeType)
{
    if(value == -1) {
        return "--";
    }

    if((attributeType == DataOptions::AttributeType::ordered) || (attributeType == DataOptions::AttributeType::multiordered)) {
        // response's starting number
        return QString::number(value);
    }

    // response's preceding letter (letter repeated for responses after 26)
    int valueIndex = value - 1;
    return ((valueIndex < 26) ? QString(char(valueIndex + 'A')) : QString(char(valueIndex%26 + 'A')).repeated(1 + (valueIndex/26)));
}

void AttributeRulesDialog::Ok()
{
    incompatibleValues.clear();

    for(int response1Index = 0; response1Index < numPossibleValues; response1Index++) {
        // find the response value for this set
        int response1Value = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
        while(attributeValues.at(response1Index).value != response1Value) {
            response1Value++;
        }
        // get the checkboxes in this set
        const auto &checkBoxList = incompCheckboxList.at(response1Index);

        // look at each checkbox in the set
        for(int response2Index = 0; response2Index < numPossibleValues; response2Index++) {
            // find the response value for this checkbox
            int response2Value = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
            while(attributeValues.at(response2Index).value != response2Value) {
                response2Value++;
            }
            if(checkBoxList[response2Index]->isChecked() && !incompatibleValues.contains({response1Value, response2Value})) {
                incompatibleValues.append({response1Value, response2Value});
            }
        }
    }

    requiredValues.clear();
    for(int responseIndex = 0; responseIndex < numPossibleValues; responseIndex++) {
        // find the response value for this checkbox
        int responseValue = -1;    // start with -1 for token "unknown" value, then increase to real values of positive integers
        while(attributeValues.at(responseIndex).value != responseValue) {
            responseValue++;
        }
        if(reqCheckboxes.at(responseIndex)->isChecked() && !requiredValues.contains(responseValue)) {
            requiredValues << responseValue;
        }
    }

    accept();
}

void AttributeRulesDialog::clearAllValues()
{
    incompatibleValues.clear();
    for(auto &checkBoxList : incompCheckboxList) {
        for(auto &checkBox : checkBoxList) {
            checkBox->setChecked(false);
        }
    }

    requiredValues.clear();
    for(auto &checkBox : reqCheckboxes) {
        checkBox->setChecked(false);
    }
}
