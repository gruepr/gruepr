#include "customResponseOptionsDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose custom response options for an attribute question
/////////////////////////////////////////////////////////////////////////////////////////////////////////

customResponseOptionsDialog::customResponseOptionsDialog(const QStringList &currentCustomOptions, QWidget *parent)
    :listTableDialog(tr("Enter custom response options"), true, true, parent)
{
    optionLineEdit = new QLineEdit[MAXRESPONSEOPTIONS];

    setMinimumSize(XS_DLG_SIZE, XS_DLG_SIZE);

    noInvalidPunctuation = new QRegularExpressionValidator(QRegularExpression("[^,&<>/]*"), this);

    //Rows 1&2 - the number of teams selector and a spacer
    numOptionsLabel.setText(tr("Number of response options: "));
    theGrid->addWidget(&numOptionsLabel, 0, 0, 1, 1, Qt::AlignRight);
    numOptionsBox.setRange(2, MAXRESPONSEOPTIONS);
    numOptions = (currentCustomOptions.isEmpty() ? 4 : currentCustomOptions.size());
    numOptionsBox.setValue(numOptions);
    connect(&numOptionsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &customResponseOptionsDialog::refreshDisplay);
    theGrid->addWidget(&numOptionsBox, 0, 1, 1, -1, Qt::AlignLeft);
    addSpacerRow(1);

    //Row 3 - table of the option choices
    theTable->setRowCount(MAXRESPONSEOPTIONS);
    int widthCol0 = 0;
    options.reserve(MAXRESPONSEOPTIONS);
    for(int i = 0; i < MAXRESPONSEOPTIONS; i++)
    {
        auto *label = new QLabel(tr("Option ") + QString::number(i+1) + " ");
        theTable->setCellWidget(i, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        theTable->setCellWidget(i, 1, &optionLineEdit[i]);
        connect(&optionLineEdit[i], &QLineEdit::textEdited, this, &customResponseOptionsDialog::optionChanged);
        optionLineEdit[i].setText(i < currentCustomOptions.size() ? currentCustomOptions.at(i) : "");
        options << optionLineEdit[i].text();
    }
    theTable->horizontalHeader()->resizeSection(0, int(widthCol0 * TABLECOLUMN0OVERWIDTH));
    theTable->adjustSize();

    //Rows 4&5 - a spacer and a reminder to include numbering if the options are in a natural order
    addSpacerRow(3);
    numberingReminderLabel.setText(tr("If the options have a natural order, as with a Likert-scale response,\n"
                                      "include a corresponding number at the beginning of each reponse\n"
                                      "(e.g., \"1. Yes  /  2. Maybe  /  3. No\")"));
    theGrid->addWidget(&numberingReminderLabel, 4, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);

    //Add Clear All to the buttons on bottom
    auto *clearAllButton = new QPushButton(tr("Clear All"));
    connect(clearAllButton, &QPushButton::clicked, this, &customResponseOptionsDialog::clearAll);
    theGrid->addWidget(clearAllButton, BUTTONBOXROWINGRID, 0, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);

    refreshDisplay(numOptionsBox.value());
    adjustSize();
}


customResponseOptionsDialog::~customResponseOptionsDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] optionLineEdit;
    delete noInvalidPunctuation;
}


void customResponseOptionsDialog::refreshDisplay(int numOptionsBoxValue)
{
    numOptions = numOptionsBoxValue;

    options.clear();
    options.reserve(numOptions);

    //show a label and a combobox for as many options as chosen in the numTeams selection
    for(int i = 0; i < MAXRESPONSEOPTIONS; i++)
    {
        if(i < numOptions)
        {
            options << optionLineEdit[i].text().trimmed();
            theTable->showRow(i);
        }
        else
        {
            theTable->hideRow(i);
        }
    }

    // auto-adjust the height to accomodate change in number of rows in table
    int currWindowWidth = size().width();
    adjustSize();
    int newWindowHeight = size().height();
    resize(currWindowWidth, newWindowHeight);

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allFilled());
}


void customResponseOptionsDialog::optionChanged()
{
    options.clear();
    options.reserve(numOptions);
    for(int i = 0; i < numOptions; i++)
    {
        QString currText = optionLineEdit[i].text();
        int currPos = 0;
        if(noInvalidPunctuation->validate(currText, currPos) != QValidator::Acceptable)
        {
            QMessageBox::warning(this, tr("Format error"), tr("Sorry, the following punctuation is not allowed:\n"
                                                              "    ,  &  <  > / \n"
                                                              "Other punctuation is allowed."));

            optionLineEdit[i].setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
        }

        options << currText.trimmed();
    }

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allFilled());
}


void customResponseOptionsDialog::clearAll()
{
    options.clear();
    options.reserve(numOptions);
    for(int i = 0; i < numOptions; i++)
    {
        optionLineEdit[i].clear();
        options << optionLineEdit[i].text();
    }

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}


bool customResponseOptionsDialog::allFilled()
{
    return std::all_of(optionLineEdit, optionLineEdit + numOptions, [](const QLineEdit &lineEdit){return !lineEdit.text().isEmpty();});
}
