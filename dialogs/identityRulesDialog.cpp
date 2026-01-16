#include "identityRulesDialog.h"
#include "qcombobox.h"
#include "qheaderview.h"
#include "qmessagebox.h"
#include "qscrollarea.h"
#include "qspinbox.h"

IdentityRulesDialog::IdentityRulesDialog(QWidget *parent, const Gender identity,
                                         TeamingOptions *teamingOptions, const DataOptions *dataOptions)
    : QDialog(parent), mainLayout(new QVBoxLayout(this))
{
    setMinimumSize(LG_DLG_SIZE, SM_DLG_SIZE);
    this->teamingOptions = teamingOptions;
    this->dataOptions = dataOptions;
    m_identity = identity;
    //number of each identity calculated where?
    setWindowTitle("Edit Identity Rules");
    //initialize labels
    //QHBoxLayout* labelsLayout = new QHBoxLayout();
    // QLabel* identityTitleLabel = new QLabel("Identity Rules for: " + identity, this);
    // identityTitleLabel->setStyleSheet(QString(LABEL12PTSTYLE));
    // identityTitleLabel->setAlignment(Qt::AlignCenter);
    // mainLayout->addWidget(identityTitleLabel);

    QLabel* instructions = new QLabel("<b><u>Identity Rules for: " + grueprGlobal::genderToString(m_identity) +
                                          "</u></b> <i>Click \"Add New Identity Rule\" to begin. Set " +
                                          grueprGlobal::genderToString(m_identity) +" != 1 to prevent the identity from being isolated in a team.</i>", this);
    instructions->setStyleSheet(QString(INSTRUCTIONSLABELSTYLE));
    instructions->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(instructions);

    // Table Widget
    rulesTable = new QTableWidget(this);
    rulesTable->verticalHeader()->setVisible(false);
    rulesTable->setColumnCount(4);
    rulesTable->setHorizontalHeaderLabels({"Identity Type", "Operator", "No of Identity", "Actions"});
    rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rulesTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    rulesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Set up the table's header style
    QHeaderView* header = rulesTable->horizontalHeader();
    header->setStyleSheet("QHeaderView::section {"
                          "background-color: " OPENWATERHEX";"
                          "color: white;"
                          "font-family: 'DM Sans'; font-size: 12pt;"
                          "padding: 5px;"
                          "border: 1px solid #ccc;"
                          "}");

    // Enable alternating row colors
    rulesTable->setAlternatingRowColors(true);
    rulesTable->setStyleSheet("QTableWidget {"
                              "alternate-background-color: #f0f0f0;"
                              "background-color: white;"
                              "font-family: 'DM Sans'; font-size: 12pt;"
                              "border: none;"
                              "}");

    populateTable();

    scrollArea = new QScrollArea(this);
    scrollContentWidget = new QWidget(this);
    rulesLayout = new QVBoxLayout(this);

    scrollArea->setWidget(rulesTable);
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(scrollArea);

    QPushButton* addNewIdentityRuleButton = new QPushButton(this);
    addNewIdentityRuleButton->setText("Add New Identity Rule");
    connect(addNewIdentityRuleButton, &QPushButton::clicked, this, &IdentityRulesDialog::addNewIdentityRule);

    QPushButton* okayButton = new QPushButton(this);
    okayButton->setText("Save and Close");
    connect(okayButton, &QPushButton::clicked, this, [teamingOptions, identity, this](){
        QList<int> rowValues;
        bool hasDuplicates = false;
        bool hasPlaceholder = false;
        QSet<int> uniqueNumbers;
        QList<int> uniqueNumbersList;
        // Get the 3rd row (index 2) of the rulesTable and iterate over the spinbox values
        for (int row = 0; row < rulesTable->rowCount(); ++row) {
            QSpinBox *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));  // 3rd row, index 2
            if (spinBox) {
                int value = spinBox->value();
                //qDebug() << "Checking each spinBox, row:" << row << "value:" << value;
                rowValues.append(value);

                // Check for duplicates and placeholders
                if (uniqueNumbers.contains(value)) {
                    hasDuplicates = true;
                    break;  // No need to continue once a duplicate is found
                }
                if (value == -1) {
                    hasPlaceholder = true;
                    break;  // No need to continue once a placeholder is found
                }
                uniqueNumbers.insert(value);
                uniqueNumbersList.append(value);
            }
        }
        if (hasDuplicates) {
            QMessageBox msgBox;
            msgBox.setText("Duplicate Error: There are duplicate entries in the identity rules.");
            msgBox.exec();
        }
        if (hasPlaceholder) {
            QMessageBox msgBox;
            msgBox.setText(" Placeholder Error: Please input a valid number in the no of Identity boxes.");
            msgBox.exec();
        }
        if (!hasDuplicates && !hasPlaceholder) {
            teamingOptions->genderIdentityRules[m_identity]["!="] = {};
            for (int num : uniqueNumbersList) {
                teamingOptions->genderIdentityRules[m_identity]["!="].append(num);
            }
            this->accept();  // Close the dialog
        }
    });
    QHBoxLayout* buttonsLayout = new QHBoxLayout(this);
    buttonsLayout->addWidget(addNewIdentityRuleButton);
    buttonsLayout->addWidget(okayButton);
    //to do: cancel button
    mainLayout->addLayout(buttonsLayout);
    addNewIdentityRuleButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    okayButton->setStyleSheet(SMALLBUTTONSTYLE);

    //okay button does validation check
    setLayout(mainLayout);
}

void IdentityRulesDialog::addNewIdentityRule() {
    int row = rulesTable->rowCount();
    rulesTable->insertRow(row);
    // QLabel* identityLabel = new QLabel("No. " + identity);
    // QLabel* operatorLabel = new QLabel("!=", this);

    QSpinBox* noOfIdentitySpinBox = new QSpinBox(this);
    noOfIdentitySpinBox->setMinimum(-1);
    noOfIdentitySpinBox->setValue(-1);
    noOfIdentitySpinBox->setSpecialValueText("...");
    noOfIdentitySpinBox->setStyleSheet(SPINBOXSTYLE);

    // QHBoxLayout* newIdentityRuleLayout = new QHBoxLayout();
    // newIdentityRuleLayout->addWidget(identityLabel);
    // newIdentityRuleLayout->addWidget(operatorLabel);
    // newIdentityRuleLayout->addWidget(noOfIdentitySpinBox);

    QPushButton* removeRuleButton = new QPushButton(this);
    removeRuleButton->setIcon(QIcon(":/icons_new/trashButton.png"));
    connect(removeRuleButton, &QPushButton::clicked, this, [this, row, noOfIdentitySpinBox]() mutable {
        removeIdentityRule("!=", noOfIdentitySpinBox->value());
        rulesTable->removeRow(row);
    });

    QTableWidgetItem* identityItem = new QTableWidgetItem(grueprGlobal::genderToString(m_identity));
    identityItem->setFlags(identityItem->flags() & ~Qt::ItemIsEditable);

    QTableWidgetItem* operatorItem = new QTableWidgetItem("!=");
    operatorItem->setFlags(operatorItem->flags() & ~Qt::ItemIsEditable);

    rulesTable->setItem(row, 0, identityItem);
    rulesTable->setItem(row, 1, operatorItem);

    rulesTable->setCellWidget(row, 2, noOfIdentitySpinBox);  // Default value
    rulesTable->setCellWidget(row, 3, removeRuleButton);
}

void IdentityRulesDialog::removeIdentityRule(const QString &operatorString, int noOfIdentity) {
    teamingOptions->genderIdentityRules[m_identity]["!="].removeOne(noOfIdentity);
    if (teamingOptions->genderIdentityRules[m_identity]["!="].isEmpty()){
        teamingOptions->genderIdentityRules[m_identity].remove("!=");
    }
}

void IdentityRulesDialog::populateTable() {
    rulesTable->setRowCount(0);  // Clear table first
    for (const QString &operatorString : teamingOptions->genderIdentityRules[m_identity].keys()) {
        const QList<int> &noInRule = teamingOptions->genderIdentityRules[m_identity].value(operatorString);
        for (int noOfIdentity : noInRule) {
            int row = rulesTable->rowCount();
            rulesTable->insertRow(row);

            QSpinBox* noOfIdentitySpinBox = new QSpinBox(this);
            noOfIdentitySpinBox->setMinimum(0);
            noOfIdentitySpinBox->setValue(noOfIdentity);
            noOfIdentitySpinBox->setStyleSheet(SPINBOXSTYLE);

            QPushButton* removeRuleButton = new QPushButton(this);
            removeRuleButton->setIcon(QIcon(":/icons_new/trashButton.png"));
            connect(removeRuleButton, &QPushButton::clicked, this, [this, row, noOfIdentitySpinBox]() mutable {
                removeIdentityRule("!=", noOfIdentitySpinBox->value());
                rulesTable->removeRow(row);
            });

            QTableWidgetItem* identityItem = new QTableWidgetItem(grueprGlobal::genderToString(m_identity));
            identityItem->setFlags(identityItem->flags() & ~Qt::ItemIsEditable);

            QTableWidgetItem* operatorItem = new QTableWidgetItem("!=");
            operatorItem->setFlags(operatorItem->flags() & ~Qt::ItemIsEditable);

            rulesTable->setItem(row, 0, identityItem);
            rulesTable->setItem(row, 1, operatorItem);

            rulesTable->setCellWidget(row, 2, noOfIdentitySpinBox);  // Default value
            rulesTable->setCellWidget(row, 3, removeRuleButton);
        }
    }
}

void IdentityRulesDialog::updateDialog(){

    //delete everything from rules layout
    while (rulesLayout->count() > 1) {
        rulesLayout->removeItem(rulesLayout->itemAt(1));
    }

    //recreate the rulesLayout
    for (const QString &operatorString : teamingOptions->genderIdentityRules[m_identity].keys()) {
        QList<int> noInRule = teamingOptions->genderIdentityRules[m_identity].value(operatorString);
        for (int noOfIdentity : std::as_const(noInRule)){

            QLabel* identityLabel = new QLabel("No. " + grueprGlobal::genderToString(m_identity));
            QLabel* operatorLabel = new QLabel(operatorString, this);

            QSpinBox* noOfIdentitySpinBox = new QSpinBox(this);
            noOfIdentitySpinBox->setMinimum(0);
            noOfIdentitySpinBox->setValue(noOfIdentity);

            QHBoxLayout* newIdentityRuleLayout = new QHBoxLayout();
            newIdentityRuleLayout->addWidget(identityLabel);
            newIdentityRuleLayout->addWidget(operatorLabel);
            newIdentityRuleLayout->addWidget(noOfIdentitySpinBox);

            QPushButton* removeRuleButton = new QPushButton(this);
            removeRuleButton->setIcon(QIcon(":/icons_new/trashButton.png"));
            connect(removeRuleButton, &QPushButton::clicked, this, [this, operatorString, noOfIdentity]() mutable {
                teamingOptions->genderIdentityRules[m_identity][operatorString].removeOne(noOfIdentity);
                if (teamingOptions->genderIdentityRules[m_identity][operatorString].isEmpty()){
                    teamingOptions->genderIdentityRules[m_identity].remove(operatorString);
                }
                updateDialog();
            });
            newIdentityRuleLayout->addWidget(removeRuleButton);
            rulesLayout->addLayout(newIdentityRuleLayout);
        }
    }
    // scrollContentWidget->setLayout(rulesLayout);
    // scrollArea->setWidget(scrollContentWidget);
    // mainLayout->addWidget(scrollArea);
}

QHBoxLayout* IdentityRulesDialog::createIdentityOperatorRule(Gender identity, QString operatorString, int noOfIdentity){
    QHBoxLayout* eachIdentityRuleLayout = new QHBoxLayout();
    QLabel* currentIdentityLabel = new QLabel(grueprGlobal::genderToString(identity));
    QComboBox* operatorComboBox = new QComboBox(this);
    //operatorComboBox->addItem(">");    // Greater than
    operatorComboBox->addItem("=");    // Equal to
    //operatorComboBox->addItem(">=");   // Greater than or equal to
    //operatorComboBox->addItem("<=");   // Less than or equal to
    //operatorComboBox->addItem("<");    // Less than
    operatorComboBox->addItem("!=");   // Not equal to

    int operatorIndex = operatorComboBox->findText(operatorString);
    if (operatorIndex != -1) {
        operatorComboBox->setCurrentIndex(operatorIndex);
    }

    QSpinBox* noOfIdentitySpinBox = new QSpinBox(this);
    noOfIdentitySpinBox->setMinimum(0);
    noOfIdentitySpinBox->setValue(noOfIdentity); //initialize to passed parameter
    // Connect combobox to update identityRules

    connect(operatorComboBox, &QComboBox::currentTextChanged, this, [this, operatorComboBox, identity, noOfIdentitySpinBox](const QString &newOperator) {
        if (teamingOptions->genderIdentityRules.contains(identity)) {
            int currentValue = noOfIdentitySpinBox->value(); //how is this initialized?
            //remove the old value
            teamingOptions->genderIdentityRules[identity][operatorComboBox->currentText()].remove(currentValue);
            //remove the old operator if it is now empty
            if (teamingOptions->genderIdentityRules[identity].isEmpty()){
                teamingOptions->genderIdentityRules[identity].remove(operatorComboBox->currentText());  // Remove old operator
            }
            //add the new operator and value
            teamingOptions->genderIdentityRules[identity][newOperator].append(currentValue);
        }
    });

    // Connect spinbox to update identityRules
    connect(noOfIdentitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, identity, operatorComboBox, noOfIdentitySpinBox](int newValue) {
        if (teamingOptions->genderIdentityRules.contains(identity)) {
            int currentValue = noOfIdentitySpinBox->value();
            QString currentOperator = operatorComboBox->currentText();
            teamingOptions->genderIdentityRules[identity][currentOperator].remove(currentValue);
            teamingOptions->genderIdentityRules[identity][currentOperator].append(newValue);
        }
    });
    eachIdentityRuleLayout->addWidget(currentIdentityLabel);
    eachIdentityRuleLayout->addWidget(operatorComboBox);
    eachIdentityRuleLayout->addWidget(noOfIdentitySpinBox);

    return eachIdentityRuleLayout;
}


