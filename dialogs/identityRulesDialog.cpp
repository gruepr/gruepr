#include "identityRulesDialog.h"
#include <QCombobox>
#include <QHeaderView>
#include <QListView>
#include <QMessageBox>
#include <QScrollArea>
#include <QSpinBox>

IdentityRulesDialog::IdentityRulesDialog(QWidget *parent, QMap<QString, Criterion::IdentityRule> *identityRules,
                                         const QStringList &identityOptions, const QString &title, const QString &hint)
    : QDialog(parent), mainLayout(new QVBoxLayout(this)), identityRules(identityRules), options(identityOptions)

{
    setWindowTitle(title);
    setMinimumSize(LG_DLG_SIZE, SM_DLG_SIZE);

    auto *instructions = new QLabel(hint, this);
    instructions->setStyleSheet(QString(INSTRUCTIONSLABELSTYLE));
    instructions->setAlignment(Qt::AlignCenter);
    instructions->setWordWrap(true);
    mainLayout->addWidget(instructions);

    // Table
    rulesTable = new QTableWidget(this);
    rulesTable->verticalHeader()->setVisible(false);
    rulesTable->setColumnCount(4);
    rulesTable->setHorizontalHeaderLabels({tr("Identity"), tr("Operator"), tr("Count"), ""});
    rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rulesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    rulesTable->setColumnWidth(3, 40);
    rulesTable->setSelectionMode(QAbstractItemView::NoSelection);
    rulesTable->setFocusPolicy(Qt::NoFocus);
    rulesTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    rulesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHeaderView *header = rulesTable->horizontalHeader();
    header->setStyleSheet("QHeaderView::section {background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; font-size: 12pt; "
                          "padding: 5px; border: 1px solid #ccc;}");
    rulesTable->setAlternatingRowColors(true);
    rulesTable->setStyleSheet("QTableWidget {alternate-background-color: #f0f0f0; background-color: white; "
                              "font-family: 'DM Sans'; font-size: 12pt; border: none;}");

    populateTable();

    scrollArea = new QScrollArea(this);
    scrollArea->setWidget(rulesTable);
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(scrollArea);

    // Buttons
    auto *addButton = new QPushButton(tr("Add New Rule"), this);
    addButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(addButton, &QPushButton::clicked, this, &IdentityRulesDialog::addNewIdentityRule);

    auto *okButton = new QPushButton(tr("Save and Close"), this);
    okButton->setStyleSheet(SMALLBUTTONSTYLE);
    connect(okButton, &QPushButton::clicked, this, [this]() {
        saveRules();
    });

    auto *cancelButton = new QPushButton(tr("Cancel"), this);
    cancelButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(okButton);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);

}

void IdentityRulesDialog::addNewIdentityRule()
{
    const QString defaultIdentity = options.isEmpty() ? "" : options.first();
    addRow(defaultIdentity, 0);
}

void IdentityRulesDialog::populateTable() {
    rulesTable->setRowCount(0);
    for (const auto [identityKey, valMap] : identityRules->asKeyValueRange()) {
        const QList<int> &noInRule = valMap.value("!=");
        for (const int val : noInRule) {
            addRow(identityKey, val);
        }
    }
}

QString IdentityRulesDialog::identityKeyFromRow(int row) const
{
    const auto *combo = qobject_cast<QComboBox *>(rulesTable->cellWidget(row, 0));
    if (!combo) {
        return {};
    }
    // If the combo text contains " or ", convert back to | delimited key
    QString key = combo->currentText();
    key.replace(" or ", "|");
    // Sort the parts so the key is canonical
    QStringList parts = key.split('|');
    parts.sort();
    return parts.join('|');
}

void IdentityRulesDialog::saveRules()
{
    // Validate: check for placeholders and duplicates
    QSet<QString> seen;
    for (int row = 0; row < rulesTable->rowCount(); ++row) {
        const QSpinBox *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));
        if (!spinBox) {
            continue;
        }
        const QString identityKey = identityKeyFromRow(row);
        const QString uniqueKey = identityKey + "|" + QString::number(spinBox->value());
        if (seen.contains(uniqueKey)) {
            QMessageBox::warning(this, tr("Duplicate Rule"),
                                 tr("There are duplicate rules. Each identity + count combination must be unique."));
            return;
        }
        seen.insert(uniqueKey);
    }

    // Rebuild the rules map from the table
    identityRules->clear();
    for (int row = 0; row < rulesTable->rowCount(); ++row) {
        const QString identityKey = identityKeyFromRow(row);
        const QSpinBox *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));
        if (identityKey.isEmpty() || !spinBox) {
            continue;
        }
        (*identityRules)[identityKey]["!="].append(spinBox->value());
    }

    accept();
}

void IdentityRulesDialog::addRow(const QString &identityKey, int value)
{
    const int row = rulesTable->rowCount();
    rulesTable->insertRow(row);

    // Column 0: identity combo box
    // For now single-select; will become multi-select later.
    // If identityKey contains '|', display it joined with " or ".
    auto *identityCombo = new QComboBox(this);
    identityCombo->addItems(options);
    if (identityKey.contains('|')) {
        // Grouped key — add it as a custom entry so it displays correctly
        const QString displayText = QString(identityKey).replace('|', " or ");
        identityCombo->addItem(displayText);
        identityCombo->setCurrentText(displayText);
    } else {
        identityCombo->setCurrentText(identityKey);
    }
    identityCombo->setStyleSheet(COMBOBOXSTYLE);
    auto *popupView = new QListView(identityCombo);
    popupView->setStyleSheet(
        "QListView {background-color: white; color: " DEEPWATERHEX "; outline: none; font-family: 'DM Sans'; font-size: 12pt;}"
        "QListView::item {padding: 4px;}"
        "QListView::item:selected {background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; font-size: 12pt;}"
        "QListView::item:hover {background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; font-size: 12pt;}");
    identityCombo->setView(popupView);
    rulesTable->setCellWidget(row, 0, identityCombo);

    // Column 1: operator (fixed to "!=")
    auto *operatorItem = new QTableWidgetItem("!=");
    operatorItem->setFlags(operatorItem->flags() & ~Qt::ItemIsEditable);
    operatorItem->setForeground(QBrush(QColor(DEEPWATERHEX)));
    operatorItem->setTextAlignment(Qt::AlignCenter);
    rulesTable->setItem(row, 1, operatorItem);

    // Column 2: count spinbox
    auto *spinBox = new QSpinBox(this);
    spinBox->setMinimum(0);
    spinBox->setValue(value);
    spinBox->setStyleSheet(SPINBOXSTYLE);
    rulesTable->setCellWidget(row, 2, spinBox);

    // Column 3: remove button
    auto *removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/icons_new/trashButton.png"));
    removeButton->setStyleSheet("QPushButton {background-color: " TRANSPARENT "; border: none;}"
                                "QPushButton:hover {background-color: #f0f0f0;}");
    removeButton->setCursor(Qt::PointingHandCursor);
    rulesTable->setCellWidget(row, 3, removeButton);
    connect(removeButton, &QPushButton::clicked, this, [this, removeButton]() {
        for (int r = 0; r < rulesTable->rowCount(); ++r) {
            if (rulesTable->cellWidget(r, 3) == removeButton) {
                rulesTable->removeRow(r);
                return;
            }
        }
    });
}
