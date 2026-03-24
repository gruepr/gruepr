#include "identityRulesDialog.h"
#include "gruepr_globals.h"
#include "widgets/checkableComboBox.h"
#include "widgets/labelWithInstantTooltip.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

IdentityRulesDialog::IdentityRulesDialog(QWidget *parent, QMap<QString, Criterion::IdentityRule> *identityRules,
                                         const QStringList &identityOptions, const QString &title)
    : QDialog(parent), mainLayout(new QVBoxLayout(this)), identityRules(identityRules), options(identityOptions)

{
    setWindowTitle(title);
    setMinimumSize(LG_DLG_SIZE, SM_DLG_SIZE);

    auto *helpFrame = new QFrame(this);
    helpFrame->setStyleSheet("background-color: " TROPICALHEX "; color: " DEEPWATERHEX "; border: 1px solid; border-color: black;");
    auto *helpLayout = new QHBoxLayout(helpFrame);
    auto *helpIcon = new LabelWithInstantTooltip("", this);
    helpIcon->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    helpIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    helpLayout->addWidget(helpIcon);
    auto *explanation = new LabelWithInstantTooltip(tr("Help me understand these rules"), this);
    explanation->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    helpLayout->addWidget(explanation);
    const QString identityOne = options[0];
    const QString identityTwo = options.size() > 1? options[1] : options[0];
    const QString identityThree = options.size() > 2? options[2] : identityOne;
    const QString helpText = tr("<html><span style=\"color: black;\">Use this dialog to set the identity rules for forming teams. For example:"
                                "<ul>") +
                                "<li> <strong>" + identityOne + tr(" does not equal 0</strong><br>"
                                            "means every team must have 1 or more ") + identityOne + " students.</li>"
                                "<li> <strong>" + identityTwo + tr(" or ") + identityThree + tr(" does not equal 1</strong><br>"
                                            "considers ") + identityTwo + tr(" and ") + identityThree + tr(" as interchangable and prevents a "
                                            "team from having exactly 1 student from either identity.</li>") +
                                "</ul>";
    helpIcon->setToolTipText(helpText);
    explanation->setToolTipText(helpText);
    mainLayout->addWidget(helpFrame, 0, Qt::AlignLeft);


    // Table
    rulesTable = new QTableWidget(this);
    rulesTable->verticalHeader()->setVisible(false);
    rulesTable->setColumnCount(4);
    rulesTable->setHorizontalHeaderLabels({tr("Identity"), "", tr("Count"), ""});
    rulesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    rulesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    rulesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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
    const auto *combo = qobject_cast<CheckableComboBox *>(rulesTable->cellWidget(row, 0));
    if (!combo) {
        return {};
    }
    QStringList checked = combo->checkedItems();
    checked.sort();
    return checked.join('|');
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
        if (identityKey.isEmpty()) {
            grueprGlobal::errorMessage(this, tr("Invalid Rule"),
                                 tr("Each rule must have at least one identity selected."));
            return;
        }
        const QString uniqueKey = identityKey + "|" + QString::number(spinBox->value());
        if (seen.contains(uniqueKey)) {
            grueprGlobal::errorMessage(this, tr("Duplicate Rule"),
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
    auto *identityCombo = new CheckableComboBox(this);
    identityCombo->addItems(options);
    const QStringList checkedIdentities = identityKey.split('|');
    identityCombo->setCheckedItems(checkedIdentities);
    rulesTable->setCellWidget(row, 0, identityCombo);

    // Column 1: operator (fixed to "!=")
    auto *operatorItem = new QTableWidgetItem(" does not equal ");
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
