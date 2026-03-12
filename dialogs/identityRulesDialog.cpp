#include "identityRulesDialog.h"
#include "qcombobox.h"
#include "qheaderview.h"
#include "qmessagebox.h"
#include "qscrollarea.h"
#include "qspinbox.h"

IdentityRulesDialog::IdentityRulesDialog(QWidget *parent, Mode mode, TeamingOptions *teamingOptions, const DataOptions *dataOptions)
    : QDialog(parent), mainLayout(new QVBoxLayout(this)), teamingOptions(teamingOptions), dataOptions(dataOptions), mode(mode)
{
    const QString title = (mode == Mode::gender) ? tr("Gender Identity Rules")
                                                 : tr("Race/Ethnicity Identity Rules");
    setWindowTitle(title);
    setMinimumSize(LG_DLG_SIZE, SM_DLG_SIZE);

    const QString hint = (mode == Mode::gender)?
        tr("Set rules for gender identities. For example, \"Woman != 1\" would prevent a team from having exactly 1 woman.")
        : tr("Set rules for race/ethnicity responses. For example, \"Latino != 1\" would  prevent a team from having exactly 1 Latino student.");
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
    const QStringList options = identityOptions();
    addRow(options.isEmpty() ? "" : options.first());
}

void IdentityRulesDialog::populateTable()
{
    rulesTable->setRowCount(0);

    if (mode == Mode::gender) {
        for (const auto [gender, valMap] : teamingOptions->genderIdentityRules.asKeyValueRange()) {
            if (gender == Gender::unknown) {
                continue;
            }
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                for (const int val : std::as_const(values)) {
                    addRow(grueprGlobal::genderToString(gender), "!=", val);
                }
            }
        }
    } else {
        for (const auto [response, valMap] : teamingOptions->urmIdentityRules.asKeyValueRange()) {
            for (const auto [operation, values] : valMap.asKeyValueRange()) {
                for (const int val : std::as_const(values)) {
                    addRow(response, "!=", val);
                }
            }
        }
    }
}

void IdentityRulesDialog::saveRules()
{
    // Validate: check for placeholders and duplicates
    QSet<QString> seen;
    for (int row = 0; row < rulesTable->rowCount(); ++row) {
        const auto *combo = qobject_cast<QComboBox *>(rulesTable->cellWidget(row, 0));
        const auto *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));
        if (!combo || !spinBox) {
            continue;
        }

        const QString key = combo->currentText() + "|" + QString::number(spinBox->value());
        if (seen.contains(key)) {
            QMessageBox::warning(this, tr("Duplicate Rule"),
                                 tr("There are duplicate rules. Each identity + count combination must be unique."));
            return;
        }
        seen.insert(key);
    }

    // Rebuild the rules map from the table
    if (mode == Mode::gender) {
        teamingOptions->genderIdentityRules.clear();
        for (int row = 0; row < rulesTable->rowCount(); ++row) {
            const auto *combo = qobject_cast<QComboBox *>(rulesTable->cellWidget(row, 0));
            const auto *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));
            if (!combo || !spinBox) {
                continue;
            }
            const Gender g = grueprGlobal::stringToGender(combo->currentText());
            teamingOptions->genderIdentityRules[g]["!="].append(spinBox->value());
        }
    } else {
        teamingOptions->urmIdentityRules.clear();
        for (int row = 0; row < rulesTable->rowCount(); ++row) {
            const auto *combo = qobject_cast<QComboBox *>(rulesTable->cellWidget(row, 0));
            const auto *spinBox = qobject_cast<QSpinBox *>(rulesTable->cellWidget(row, 2));
            if (!combo || !spinBox) {
                continue;
            }
            const QString response = combo->currentText();
            teamingOptions->urmIdentityRules[response]["!="].append(spinBox->value());
        }
    }

    accept();
}

void IdentityRulesDialog::addRow(const QString &identityText, const QString &operation, int value)
{
    const int row = rulesTable->rowCount();
    rulesTable->insertRow(row);

    int column = 0;

    // Column 0: identity combo box
    auto *identityCombo = new QComboBox(this);
    identityCombo->addItems(identityOptions());
    identityCombo->setCurrentText(identityText);
    identityCombo->setStyleSheet(COMBOBOXSTYLE);
    rulesTable->setCellWidget(row, column++, identityCombo);

    // Column 1: operation (fixed for now to "!=")
    auto *operatorItem = new QTableWidgetItem(operation);
    operatorItem->setFlags(operatorItem->flags() & ~Qt::ItemIsEditable);
    operatorItem->setForeground(QBrush(QColor(DEEPWATERHEX)));
    operatorItem->setTextAlignment(Qt::AlignCenter);
    rulesTable->setItem(row, column++, operatorItem);

    // Column 2: count spinbox
    auto *spinBox = new QSpinBox(this);
    spinBox->setMinimum(0);
    spinBox->setValue(value);
    spinBox->setStyleSheet(SPINBOXSTYLE);
    rulesTable->setCellWidget(row, column++, spinBox);

    // Column 3: remove button
    auto *removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/icons_new/trashButton.png"));
    removeButton->setStyleSheet("QPushButton {background-color: " TRANSPARENT "; border: none;}"
                                "QPushButton:hover {background-color: " BUBBLYHEX ";}");
    removeButton->setCursor(Qt::PointingHandCursor);
    connect(removeButton, &QPushButton::clicked, this, [this, removeButton, column]() {
        // Find the current row of this button (handles shifting after deletions)
        for (int r = 0; r < rulesTable->rowCount(); ++r) {
            if (rulesTable->cellWidget(r, column) == removeButton) {
                rulesTable->removeRow(r);
                return;
            }
        }
    });
    rulesTable->setCellWidget(row, column++, removeButton);
}


QStringList IdentityRulesDialog::identityOptions() const
{
    QStringList options;
    if (mode == Mode::gender) {
        // Offer the gender identities that exist in the data (skip unknown)
        for (const Gender g : std::as_const(dataOptions->genderValues)) {
            if (g != Gender::unknown) {
                options << grueprGlobal::genderToString(g);
            }
        }
        // Always include the big three even if not yet seen in data
        const QStringList defaults = {grueprGlobal::genderToString(Gender::woman),
                                      grueprGlobal::genderToString(Gender::man),
                                      grueprGlobal::genderToString(Gender::nonbinary)};
        for (const QString &d : defaults) {
            if (!options.contains(d)) {
                options << d;
            }
        }
    } else {
        for (const QString &resp : std::as_const(dataOptions->URMResponses)) {
            if (resp != "--") {
                options << resp;
            }
        }
    }
    return options;
}
