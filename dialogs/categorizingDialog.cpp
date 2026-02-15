#include "categorizingDialog.h"
#include "csvfile.h"
#include "dialogs/dataTypesTableDialog.h"
#include "gruepr_globals.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTableWidget>

CategorizingDialog::CategorizingDialog(QWidget* parent, CsvFile* surveyFile, DataOptions::DataSource dataSource): QDialog (parent) {
    this->source = dataSource;
    auto *mainLayout = new QVBoxLayout();
    this->surveyFile = surveyFile;
    setMinimumSize(QSize(790, 450));
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);

    auto *titleLayout = new QHBoxLayout();
    auto *categorizingFields = new QLabel(this);
    categorizingFields->setText("Assign Each Field to a Gruepr Data Type");
    categorizingFields->setStyleSheet(LABEL14PTSTYLE);
    auto* successLabel = new QLabel(this);
    successLabel->setWordWrap(true);
    successLabel->setText("The data found in your file is shown below. Gruepr can usually auto-detect the category of data found in each column. "
                          "If necessary, you can use the dropdown boxes in the second row to change these auto-determined categories.");
    successLabel->setStyleSheet(INSTRUCTIONSLABELSTYLE);
    auto *categoryHelpButton = new QPushButton(this);
    categoryHelpButton->setIcon(QPixmap(":/icons_new/lightbulb.png").scaled(40,40,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    categoryHelpButton->setStyleSheet(STANDARDBUTTON);
    categoryHelpButton->setToolTip("What do these data categories mean?");
    titleLayout->addWidget(categorizingFields);
    titleLayout->addWidget(categoryHelpButton);
    auto *titleWidget = new QWidget();
    titleWidget->setLayout(titleLayout);
    titleLayout->setSpacing(5);

    connect(categoryHelpButton, &QPushButton::clicked, this, [this](){
        auto *dialog = new dataTypesTableDialog(this);
        dialog->show();
    });

    auto *dataSetTableHeaderWidget = new QWidget(this);
    datasetTableHeaderLayout = new QHBoxLayout();
    datasetTableWidget = new QTableWidget(this);

    auto *datasetTableScrollArea = new QScrollArea(this);
    datasetTableScrollArea->setStyleSheet(SCROLLBARSTYLE);

    auto *datasetTableScrollAreaWidget = new QWidget(this);
    auto *datasetTableScrollAreaLayout = new QVBoxLayout();
    datasetTableHeaderLayout->setSpacing(0);
    datasetTableHeaderLayout->setContentsMargins(0, 0, 0, 0);
    dataSetTableHeaderWidget->setLayout(datasetTableHeaderLayout);
    datasetTableScrollArea->setWidget(datasetTableScrollAreaWidget);
    datasetTableScrollAreaWidget->setLayout(datasetTableScrollAreaLayout);
    datasetTableScrollAreaLayout->addWidget(dataSetTableHeaderWidget);
    datasetTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    datasetTableWidget->horizontalHeader()->setVisible(false);
    datasetTableScrollAreaLayout->addWidget(datasetTableWidget);
    datasetTableScrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    datasetTableScrollAreaLayout->setSpacing(0);
    datasetTableScrollAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    datasetTableScrollArea->setWidgetResizable(true);
    datasetTableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    datasetTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    datasetTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    datasetTableWidget->setStyleSheet(QString("QTableView{gridline-color: lightGray; font-family: 'DM Sans'; font-size: 12pt;}"
                                                "QTableCornerButton::section{border-top: none; border-left: none; border-right: 1px solid gray; "
                                                "border-bottom: none; background-color: " DEEPWATERHEX ";}"
                                                "QTableWidget::item{border-right: 1px solid lightGray; color: black;}") +
                                                SCROLLBARSTYLE);
    datasetTableWidget->horizontalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                                                   "border-bottom: none; background-color:" DEEPWATERHEX "; "
                                                   "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}"
                                                   "QHeaderView::section{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                                                   "border-bottom: none; background-color:" DEEPWATERHEX "; "
                                                   "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:left;}");
    datasetTableWidget->verticalHeader()->setStyleSheet("QHeaderView{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                 "background-color:" DEEPWATERHEX "; "
                                                 "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}"
                                                 "QHeaderView::section{border-top: none; border-left: none; border-right: none; border-bottom: none;"
                                                 "background-color:" DEEPWATERHEX "; "
                                                 "font-family: 'DM Sans'; font-size: 12pt; color: white; text-align:center;}");
    //below is stupid way needed to get text in the top-left corner cell
    datasetTableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    datasetTableWidget->verticalHeader()->setVisible(false);

    confirmCancelButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setText(tr("Confirm"));
    confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    confirmCancelButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    confirmCancelButtonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
    connect(confirmCancelButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(confirmCancelButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(titleWidget, 1, Qt::AlignCenter);
    mainLayout->addWidget(successLabel);
    mainLayout->addWidget(datasetTableScrollArea, 8);
    mainLayout->addWidget(confirmCancelButtonBox);
    populateTable();

    setLayout(mainLayout);
}

void CategorizingDialog::populateTable(){
    datasetTableWidget->setColumnCount(surveyFile->numFields);
    surveyFile->readDataRow(CsvFile::ReadLocation::beginningOfFile);
    QStringList columnNames = surveyFile->fieldValues;
    for (int column = 0; column < columnNames.count(); column++){
        auto *columnWidget = new QWidget(this);
        auto *columnWidgetLayout = new QVBoxLayout();
        columnWidgetLayout->setSpacing(0);
        columnWidgetLayout->setContentsMargins(0, 0, 0, 0);
        columnWidget->setLayout(columnWidgetLayout);
        auto *columnName = new QLabel(columnNames[column], this);
        auto *columnComboBox = new QComboBox(this);
        columnComboBox->setStyleSheet(COMBOBOXSTYLE);
        columnComboBox->setFocusPolicy(Qt::StrongFocus);  // remove scrollwheel from affecting the value,
        columnComboBox->installEventFilter(new MouseWheelBlocker(columnComboBox));  // as it's too easy to mistake scrolling through the rows with changing the value
        columnWidgetLayout->addWidget(columnName);
        columnWidgetLayout->addWidget(columnComboBox);
        columnName->setStyleSheet(FAKETABLEHEADERWIDGETSTYLE);
        datasetTableHeaderLayout->addWidget(columnWidget);
        dataTypeComboBoxes.append(columnComboBox);
        columnWidgets.append(columnWidget);
    }

    initializeComboBoxes();

    for (int i = 0; i < columnWidgets.count(); i++){
        datasetTableWidget->setColumnWidth(i, columnWidgets[i]->sizeHint().width());
    }

    int row = 0;
    datasetTableWidget->setRowCount(surveyFile->estimatedNumberRows-1);
    while(surveyFile->readDataRow()){
        QStringList fieldValues = surveyFile->fieldValues;
        for (int fieldValueIndex = 0; fieldValueIndex < fieldValues.count(); fieldValueIndex++){

            auto *fieldValue = new QLabel(fieldValues[fieldValueIndex], this);
            fieldValue->setWordWrap(true);
            //fieldValue->setFixedWidth(200);
            fieldValue->setStyleSheet((row % 2 == 0)? LABEL10PTSTYLE : LABEL10PTBUBBLYBGSTYLE);
            datasetTableWidget->setCellWidget(row, fieldValueIndex, fieldValue);
        }
        row++;
    }
    surveyFile->readDataRow(CsvFile::ReadLocation::beginningOfFile);
}

bool CategorizingDialog::initializeComboBoxes()
{
    if(!surveyFile->readHeader()) {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    if(surveyFile->headerValues.size() < 2) {
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        surveyFile->close((source == DataOptions::DataSource::fromGoogle) || (source == DataOptions::DataSource::fromCanvas));
        return false;
    }

    // See if there are header fields after any of (preferred teammates / non-teammates, section, or schedule) since those are probably notes fields
    static const QRegularExpression lastKnownMeaningfulField("(.*(like to not have on your team).*)|(.*(want to avoid working with).*)|"
                                                             "(.*(like to have on your team).*)|(.*(want to work with).*)|"
                                                             ".*(which section are you enrolled).*|(.*(check).+(times).*)",
                                                             QRegularExpression::CaseInsensitiveOption);
    const int notesFieldsProbBeginAt = 1 + int(surveyFile->headerValues.lastIndexOf(lastKnownMeaningfulField));
    if((notesFieldsProbBeginAt != 0) && (notesFieldsProbBeginAt != surveyFile->headerValues.size())) {
        //if notesFieldsProbBeginAt == 0 then none of these questions exist, so assume no notes because list ends with attributes
        //and if notesFieldsProbBeginAt == headervalues size, also assume no notes because list ends with one of these questions
        for(int field = notesFieldsProbBeginAt; field < surveyFile->fieldMeanings.size(); field++) {
            surveyFile->fieldMeanings[field] = "Notes";
        }
    }

    // Ask user what the columns mean
    QList<possFieldMeaning> surveyFieldOptions = {{"Timestamp", "(timestamp)|(^submitted$)", 1},
                                                  {"First Name", "((first)|(given)|(preferred))(?!.*last).*(name)", 1},
                                                  {"Last Name", "^(?!.*first).*((last)|(sur)|(family)).*(name)", 1},
                                                  {"Email Address", "(e).*(mail)", 1},
                                                  {"Gender", "((gender)|(pronouns))", 1},
                                                  {"Racial/ethnic identity", "((minority)|(ethnic))", 1},
                                                  {"Schedule", "((check)|(select)).+(times)", MAX_DAYS},
                                                  {"Section", "which section are you enrolled", 1},
                                                  {"Timezone","(time zone)", 1},
                                                  {"Grade", "(grade)|(marks)", 1},
                                                  {"Preferred Teammates", "(like to have on your team)|(want to work with)", MAX_PREFTEAMMATES},
                                                  {"Preferred Non-teammates", "(like to not have on your team)|(want to avoid working with)", MAX_PREFTEAMMATES},
                                                  {"Multiple Choice", ".*", MAX_ATTRIBUTES},
                                                  {"Notes", "", MAX_NOTES}};
    // see if each field is a value to be ignored; if not and the fieldMeaning is empty, preload with possibleFieldMeaning based on matches to the patterns
    for(int i = 0; i < surveyFile->numFields; i++) {
        const QString &headerVal = surveyFile->headerValues.at(i);

        bool ignore = false;
        for(const auto &matchpattern : std::as_const(surveyFile->fieldsToBeIgnored)) {
            if(headerVal.contains(QRegularExpression(matchpattern, QRegularExpression::CaseInsensitiveOption))) {
                surveyFile->fieldMeanings[i] = "**IGNORE**";
                ignore = true;
            }
            // if this is coming from Canvas, see if it's the LMSID field and, if so, set the field
            if((source == DataOptions::DataSource::fromCanvas) && (headerVal.compare("id", Qt::CaseInsensitive) == 0)) {
                surveyFile->fieldMeanings[i] = "**LMSID**";
                ignore = true;
            }
        }

        if(!ignore && surveyFile->fieldMeanings.at(i).isEmpty()) {
            int matchPattern = 0;
            QString match;
            do {
                match = surveyFieldOptions.at(matchPattern).regExSearchString;
                matchPattern++;
            } while((matchPattern < surveyFieldOptions.size()) && !headerVal.contains(QRegularExpression(match, QRegularExpression::CaseInsensitiveOption)));

            if(matchPattern != surveyFieldOptions.size()) {
                surveyFile->fieldMeanings[i] = surveyFieldOptions.at(matchPattern - 1).nameShownToUser;
            }
            else {
                surveyFile->fieldMeanings[i] = UNUSEDTEXT;
            }
        }
    }

    // a label and combobox for each column
    for(int column = 0; column < surveyFile->numFields; column++) {
        auto *selector = dataTypeComboBoxes[column];
        for(const auto &meaning : std::as_const(surveyFieldOptions)) {
            selector->addItem(meaning.nameShownToUser, meaning.maxNumOfFields);
        }
        selector->insertItem(0, UNUSEDTEXT);
        auto *model = qobject_cast<QStandardItemModel *>(selector->model());
        model->item(0)->setForeground(Qt::darkRed);
        selector->insertSeparator(1);
        if((surveyFile->fieldMeanings.at(column) == "**IGNORE**") || (surveyFile->fieldMeanings.at(column) == "**LMSID**")) {
            selector->addItem(surveyFile->fieldMeanings.at(column));
            selector->setCurrentText(surveyFile->fieldMeanings.at(column));
            //ui->tableWidget->hideCol(column);
            //edge case not handled yet
        }
        else {
            selector->setCurrentText(surveyFile->fieldMeanings.at(column));
        }
        // selector->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        // const int width = selector->minimumSizeHint().width();
        // selector->setMinimumWidth(width);
        // selector->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        connect(selector, &QComboBox::currentTextChanged, this, [this, column]{validateFieldSelectorBoxes(column);});
    }
    validateFieldSelectorBoxes(-1);
    return true;
}

void CategorizingDialog::validateFieldSelectorBoxes(int callingRow)
{
    // get list of rows in top-to-bottom order, but if this function is getting called by a selector box, then put its row at the front of the line
    QList<int> rows(surveyFile->numFields);
    std::iota(rows.begin(), rows.end(), 0);
    if(callingRow != -1) {
        rows.remove(callingRow);
        rows.prepend(callingRow);
    }

    // start by counting all the values to count how many times each are used, and which are fully used
    std::map<QString, int> takenValues;     // mapping fieldMeaning -> number of fields selected with this meaning
    std::map<QString, int> fullyUsedValues; // mapping the same, but saving how many extra fields with this meaning
    for(auto row : rows) {
        // get the selected fieldMeaning
        const auto *box = dataTypeComboBoxes[row];
        if((box->currentText() == "**IGNORE**") || (box->currentText() == "**LMSID**")) {
            continue;
        }
        const QString selection = box->currentText();

        // set it in the CsvFile's data
        surveyFile->fieldMeanings[row] = selection;

        // add this occurence in the takenValues mapping
        if(takenValues.count(selection) == 0) {
            // first ocurrence of this field; create the key/value
            takenValues[selection] = 1;
        }
        else {
            // key already exists
            takenValues[selection]++;
        }

        // if we are at or above the allowed number of ocurrences, note it
        if(takenValues[selection] >= box->currentData().toInt()) { //combo box notes allowed number of ocurrences
            // add this occurence in the takenValues mapping
            if(fullyUsedValues.count(selection) == 0) {
                // field has just reached capacity; create the key/value
                fullyUsedValues[selection] = 1;
            }
            else {
                // key already exists; we have MORE than are allowed
                fullyUsedValues[selection]++;
            }
        }
        else {
            fullyUsedValues[selection] = 0;
        }
    }

    // Now go back through in reverse order and:
    //  1) replacing overused values with "Unused",
    //  2) setting fully used values in other boxes to red with a tooltip,
    //  3) clearing formatting of all non-overused values (except "Unused") and the fully used values that are currently chosen.
    // Then:
    //  4) clearing formatting of all items unchosen in any box (except "Unused").
    for(auto row = rows.rbegin(); row != rows.rend(); ++row) {
        auto *box = dataTypeComboBoxes[*row];
        if((box->currentText() == "**IGNORE**") || (box->currentText() == "**LMSID**")) {
            continue;
        }
        box->blockSignals(true);
        auto *model = qobject_cast<QStandardItemModel *>(box->model());
        for(auto &takenValue : takenValues) {
            const QString fieldval = takenValue.first;
            const int numAllowed = box->itemData(box->findText(fieldval)).toInt();
            QStandardItem *item = model->item(box->findText(fieldval));
            if((fullyUsedValues[fieldval] > 1) && (box->currentText() == fieldval)) {
                // number exceeds max. allowed somehow, so set to unused
                box->setCurrentText(UNUSEDTEXT);
                surveyFile->fieldMeanings[*row] = UNUSEDTEXT;
                fullyUsedValues[fieldval]--;
                if(numAllowed == 1) {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if((fullyUsedValues[fieldval] == 1) && (box->currentText() != fieldval)) {
                // at capacity, and not selected in this box
                item->setForeground(Qt::darkRed);
                if(numAllowed == 1) {
                    item->setToolTip(tr("The \"") + fieldval + tr("\" field has already been assigned."
                                                                  "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(tr("All ") + QString::number(numAllowed) + " \"" + fieldval + tr("\" fields have already been assigned."
                                                                                                      "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if(fieldval != UNUSEDTEXT) {
                // below capacity or at capacity including this one
                item->setForeground(Qt::black);
                item->setToolTip("");
            }
        }

        // clearing formatting of all unchosen items except "Unused"
        for(int itemNum = 0; itemNum < box->count(); itemNum++) {
            if((takenValues.count(box->itemText(itemNum)) == 0) && (box->itemText(itemNum) != UNUSEDTEXT)) {
                model->item(itemNum)->setForeground(Qt::black);
                model->item(itemNum)->setToolTip("");
            }
        }
        box->blockSignals(false);
    }
}
