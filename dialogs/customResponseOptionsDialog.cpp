#include "customResponseOptionsDialog.h"
#include "gruepr_globals.h"
#include "surveyMakerWizard.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom response options for an attribute question
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customResponseOptionsDialog::customResponseOptionsDialog(const QStringList &currentCustomOptions, QWidget *parent)
    :listTableDialog(tr("Enter custom response options"), true, true, parent),
    options(currentCustomOptions)
{
    setMinimumSize(XS_DLG_SIZE, XS_DLG_SIZE);

    bool comingInOrdered = stripPrecedingOrderNumbers(options);

    //Rows 1&2 - the number of options selector and a checkbox for ordered responses
    numOptionsLayout = new QHBoxLayout;
    numOptionsLabel = new QLabel(tr("Number of response options: "), this);
    numOptionsLabel->setStyleSheet(QString(LABELSTYLE).replace("QLabel {", "QLabel {background-color: " TRANSPARENT ";"));
    numOptionsLayout->addWidget(numOptionsLabel, 0, Qt::AlignRight);
    numOptionsBox = new QSpinBox(this);
    numOptionsBox->setStyleSheet(SPINBOXSTYLE);
    numOptionsBox->setRange(2, MAXRESPONSEOPTIONS);
    numOptions = (currentCustomOptions.isEmpty() ? 4 : int(currentCustomOptions.size()));
    numOptionsBox->setValue(numOptions);
    connect(numOptionsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &customResponseOptionsDialog::refreshDisplay);
    numOptionsLayout->addWidget(numOptionsBox, 0, Qt::AlignLeft);
    theGrid->addLayout(numOptionsLayout, 0, 1, 1, 1);
    orderedResponsesCheckbox = new QCheckBox("Options have a natural order, as with a Likert-scale response.");
    orderedResponsesCheckbox->setStyleSheet(CHECKBOXSTYLE);
    orderedResponsesCheckbox->setChecked(comingInOrdered);
    connect(orderedResponsesCheckbox, &QCheckBox::clicked, this, &customResponseOptionsDialog::refreshDisplay);
    theGrid->addWidget(orderedResponsesCheckbox, 1, 1, 1, 1);

    //Row 3 - table of the option choices
    theTable->setRowCount(MAXRESPONSEOPTIONS);
    optionLabels.reserve(MAXRESPONSEOPTIONS);
    optionLineEdits.reserve(MAXRESPONSEOPTIONS);
    int widthCol0 = 0;
    for(int i = 0; i < MAXRESPONSEOPTIONS; i++)
    {
        optionLabels << new QLabel(tr("Option ") + QString::number(i+1) + " ");
        optionLabels.last()->setStyleSheet(QString(LABELSTYLE).replace("QLabel {", "QLabel {background-color: " TRANSPARENT ";"));
        theTable->setCellWidget(i, 0, optionLabels.last());
        optionLineEdits << new QLineEdit;
        optionLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
        widthCol0 = std::max(widthCol0, optionLabels.last()->width());
        theTable->setCellWidget(i, 1, optionLineEdits.last());
        connect(optionLineEdits.last(), &QLineEdit::textEdited, this, &customResponseOptionsDialog::refreshDisplay);
        optionLineEdits.last()->setText(i < options.size() ? options.at(i) : "");
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLECOLUMN0OVERWIDTH));
    theTable->adjustSize();

    //Add Clear All to the buttons on bottom
    clearAllButton = new QPushButton(tr("Clear All"), this);
    clearAllButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(clearAllButton, &QPushButton::clicked, this, &customResponseOptionsDialog::clearAll);
    addButton(clearAllButton);

    refreshDisplay();
    adjustSize();
}


void customResponseOptionsDialog::refreshDisplay()
{
    numOptions = numOptionsBox->value();

    options.clear();

    //show a label and a combobox for as many options as chosen in the numTeams selection
    int i = 0;
    for(const auto &optionLineEdit : optionLineEdits)
    {
        if(i < numOptions)
        {
            QString currText = optionLineEdits[i]->text();
            int currPos = 0;
            if(SurveyMakerWizard::noInvalidPunctuation.validate(currText, currPos) != QValidator::Acceptable)
            {
                SurveyMakerWizard::invalidExpression(optionLineEdits[i], currText, this);
            }

            options << (orderedResponsesCheckbox->isChecked() ? (QString::number(i+1) + ". ") : "") + optionLineEdit->text().trimmed();
            optionLabels[i]->setText(tr("Option ") + (orderedResponsesCheckbox->isChecked() ? QString::number(i+1) : QString(char((i%26)+65)).repeated((i/26)+1)));
            theTable->showRow(i);
        }
        else
        {
            theTable->hideRow(i);
        }
        i++;
    }

    // auto-adjust the height to accomodate change in number of rows in table
    theTable->resizeColumnsToContents();
    theTable->resizeRowsToContents();
    theTable->adjustSize();
    int currWindowWidth = size().width();
    adjustSize();
    int newWindowHeight = size().height();
    resize(currWindowWidth, newWindowHeight);

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allFilled());
}


void customResponseOptionsDialog::clearAll()
{
    options.clear();
    for(int i = 0; i < numOptions; i++)
    {
        optionLineEdits[i]->clear();
        options << optionLineEdits[i]->text();
    }

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}


bool customResponseOptionsDialog::allFilled()
{
    return std::all_of(optionLineEdits.constBegin(), optionLineEdits.constBegin() + numOptions, [](const QLineEdit* lineEdit){return !lineEdit->text().isEmpty();});
}

bool customResponseOptionsDialog::stripPrecedingOrderNumbers(QStringList &options)
{
    bool allStartedWithOrderNumbers = true;
    int i = 0;
    for(const auto &option : options) {
        allStartedWithOrderNumbers = allStartedWithOrderNumbers && option.startsWith(QString::number(i+1) + ". ");
        i++;
    }

    if(allStartedWithOrderNumbers) {
        for(auto &option : options) {
            option = option.sliced((QString::number(i+1) + ". ").size());
        }
    }

    return allStartedWithOrderNumbers;
}
