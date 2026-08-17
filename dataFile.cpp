#include "dataFile.h"
#include "gruepr_globals.h"
#include "widgets/styledComboBox.h"
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QString>
#include <map>
#include <numeric>

DataFile::~DataFile()
{
    delete window;
}


//////////////////
// Open a file for reading via Dialog Box, delegating the actual open to the subclass
//////////////////
bool DataFile::open(QWidget *parent, const QString &caption, const QString &filepath, const QString &filetypeDescriptor)
{
    const QString fileName = QFileDialog::getOpenFileName(parent, caption, filepath, fileDialogFilter(filetypeDescriptor));
    if(fileName.isEmpty()) {
        return false;
    }
    return openExistingFile(fileName);
}


//////////////////
// Retrieve the fileInfo
//////////////////
QFileInfo DataFile::fileInfo()
{
    if(openFilePath.isEmpty()) {
        return {};
    }
    return QFileInfo(openFilePath);
}


bool DataFile::isOpen()
{
    return !openFilePath.isEmpty();
}


//////////////////
// Close the file, releasing the format-specific resource
//////////////////
void DataFile::close(bool deleteFile)
{
    releaseResource();
    if(deleteFile && !openFilePath.isEmpty()) {
        QFile::remove(openFilePath);
    }
    openFilePath.clear();
}


//////////////////
// Read the first row, splitting & saving field texts into headerValues, making room in fieldMeanings for each
//////////////////
bool DataFile::readHeader()
{
    resetToStart();
    headerValues = readNextRawRow().value_or(QStringList());
    if (!headerValues.isEmpty() && headerValues[0].startsWith(QChar(0xFEFF))) {
        // Strip BOM, in case the file was itself produced from a BOM-prefixed source
        headerValues[0] = headerValues[0].mid(1);
    }
    numFields = int(headerValues.size());
    fieldMeanings.clear();
    fieldMeanings.fill("", numFields);
    return !headerValues.isEmpty();
}


//////////////////
// Read a row, splitting & saving field texts into fieldValues. Blank rows are skipped.
//////////////////
bool DataFile::readDataRow(ReadLocation readLocation)
{
    if(readLocation == ReadLocation::beginningOfFile) {
        resetToStart();
    }

    std::optional<QStringList> row;
    do {
        row = readNextRawRow();
    } while(row.has_value() && row->isEmpty());

    if(!row.has_value()) {
        fieldValues.clear();
        return false;
    }

    fieldValues = *row;
    while(fieldValues.size() < numFields) {
        fieldValues << QString();
    }
    return true;
}


//////////////////
// Open dialog box to let user choose which columns correspond to which fields
//////////////////
QDialog* DataFile::chooseFieldMeaningsDialog(const QList<possFieldMeaning> &possibleFieldMeanings, QWidget *parent)
{
    // see if each field is a value to be ignored; if not and the fieldMeaning is empty, preload with possibleFieldMeaning based on matches to the patterns
    for(int i = 0; i < numFields; i++) {
        const QString &headerVal = headerValues.at(i);

        bool ignore = false;
        for(const auto &matchpattern : std::as_const(fieldsToBeIgnored)) {
            if(headerVal.contains(QRegularExpression(matchpattern, QRegularExpression::CaseInsensitiveOption))) {
                fieldMeanings[i] = "**IGNORE**";
                ignore = true;
            }
        }

        if(!ignore && fieldMeanings.at(i).isEmpty()) {
            int matchPattern = 0;
            QString match;
            do {
                match = possibleFieldMeanings.at(matchPattern).regExSearchString;
                matchPattern++;
            }
            while((matchPattern < possibleFieldMeanings.size()) &&
                  !headerVal.contains(QRegularExpression(match, QRegularExpression::CaseInsensitiveOption)));
            if(matchPattern != possibleFieldMeanings.size()) {
                fieldMeanings[i] = possibleFieldMeanings.at(matchPattern - 1).nameShownToUser;
            }
            else {
                fieldMeanings[i] = UNUSEDTEXT;
            }
        }
    }

    window = new listTableDialog(QObject::tr("Select column definitions"), false, false, parent);
    window->setMinimumSize(DIALOGWIDTH, DIALOGHEIGHT);

    auto *explanation = new QLabel(window);
    explanation->setStyleSheet(LABEL10PTSTYLE);
    explanation->setText(QObject::tr("<html>The following fields were found in the first row of the file. "
                         "Please verify the category of information contained in each column. Select \"") + UNUSEDTEXT +
                         QObject::tr("\" for any field(s) that should be ignored.<hr></html>"));
    explanation->setWordWrap(true);
    window->theGrid->addWidget(explanation, 0, 0, 1, -1);

    auto *hasHeaderRowCheckbox = new QCheckBox(window);
    hasHeaderRowCheckbox->setStyleSheet(CHECKBOXSTYLE);
    hasHeaderRowCheckbox->setText(QObject::tr("This file has a header row"));
    hasHeaderRowCheckbox->setChecked(true);
    window->theGrid->addWidget(hasHeaderRowCheckbox, 1, 0, 1, -1);
    QObject::connect(hasHeaderRowCheckbox, &QCheckBox::clicked, window, [this, hasHeaderRowCheckbox]
                                                      {hasHeaderRow = hasHeaderRowCheckbox->isChecked();
                                                       if(hasHeaderRow)
                                                         {window->theTable->setHorizontalHeaderLabels(QStringList({HEADERTEXT, CATEGORYTEXT}));}
                                                       else
                                                         {window->theTable->setHorizontalHeaderLabels(QStringList({ROW1TEXT, CATEGORYTEXT}));}});

    // a label and combobox for each column
    window->theTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    window->theTable->horizontalHeader()->setStyleSheet("QHeaderView::section {background-color: " OPENWATERHEX "; color: white; padding: 5px; "
                                                                              "border-top: none; border-bottom: none; border-left: none; "
                                                                              "border-right: 1px solid white; "
                                                                              "font-family: 'DM Sans'; font-size: 12pt;}");
    window->theTable->setStyleSheet("QTableView{background-color: white; alternate-background-color: lightGray; border: none;}"
                                    "QTableView::item{border-top: none; border-bottom: none; border-left: none; border-right: 1px solid darkGray; padding: 3px;}" +
                                    QString(SCROLLBARSTYLE).replace(DEEPWATERHEX, OPENWATERHEX));
    window->theTable->setAlternatingRowColors(true);
    window->theTable->setHorizontalHeaderLabels(QStringList({HEADERTEXT, CATEGORYTEXT}));
    window->theTable->setRowCount(numFields);
    for(int row = 0; row < numFields; row++) {
        auto *label = new QLabel("\n" + headerValues.at(row) + "\n", window);
        label->setStyleSheet(LABEL10PTSTYLE);
        label->setWordWrap(true);
        window->theTable->setCellWidget(row, 0, label);

        auto *selector = new StyledComboBox(window);
        selector->setFocusPolicy(Qt::StrongFocus);  // remove scrollwheel from affecting the value,
        selector->installEventFilter(new MouseWheelBlocker(selector));  // as it's too easy to mistake scrolling through the rows with changing the value
        for(const auto &meaning : std::as_const(possibleFieldMeanings)) {
            selector->addItem(meaning.nameShownToUser, meaning.maxNumOfFields);
        }
        selector->insertItem(0, UNUSEDTEXT);
        auto *model = qobject_cast<QStandardItemModel *>(selector->model());
        model->item(0)->setForeground(Qt::darkRed);
        selector->insertSeparator(1);
        if(fieldMeanings.at(row) == "**IGNORE**") {
            selector->setCurrentText(UNUSEDTEXT);
            fieldMeanings[row] = UNUSEDTEXT;
            window->theTable->hideRow(row);
        }
        else {
            selector->setCurrentText(fieldMeanings.at(row));
        }
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        const int width = selector->minimumSizeHint().width();
        selector->setMinimumWidth(width);
        selector->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        window->theTable->setCellWidget(row, 1, selector);
        QObject::connect(selector, &QComboBox::currentTextChanged, window, [this, row]{validateFieldSelectorBoxes(row);});
    }
    validateFieldSelectorBoxes();
    window->theTable->resizeColumnsToContents();
    window->theTable->resizeRowsToContents();
    window->theTable->adjustSize();

    window->adjustSize();
    return window;
}


//////////////////
// Validate the selector boxes in the choose field meaning dialog:
// one per field unless there's an asterisk in the name, in which case there are as many as the number after
//////////////////
void DataFile::validateFieldSelectorBoxes(int callingRow)
{
    // get list of rows in top-to-bottom order, but if this function is getting called by a selector box, then put its row at the front of the line
    QList<int> rows(numFields);
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
        const auto *box = qobject_cast<StyledComboBox *>(window->theTable->cellWidget(row, 1));
        const QString selection = box->currentText();

        // set it in the DataFile's data
        fieldMeanings[row] = selection;

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
        if(takenValues[selection] >= box->currentData().toInt()) {
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
        auto *box = qobject_cast<StyledComboBox *>(window->theTable->cellWidget(*row, 1));
        box->blockSignals(true);
        auto *model = qobject_cast<QStandardItemModel *>(box->model());
        for(auto &takenValue : takenValues) {
            const QString fieldval = takenValue.first;
            const int numAllowed = box->itemData(box->findText(fieldval)).toInt();
            QStandardItem *item = model->item(box->findText(fieldval));
            if((fullyUsedValues[fieldval] > 1) && (box->currentText() == fieldval)) {
                // number exceeds max. allowed somehow, so set to unused
                box->setCurrentText(UNUSEDTEXT);
                fieldMeanings[*row] = UNUSEDTEXT;
                fullyUsedValues[fieldval]--;
                if(numAllowed == 1) {
                    item->setToolTip(QObject::tr("The \"") + fieldval + QObject::tr("\" field has already been assigned."
                                     "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(QObject::tr("All ") + QString::number(numAllowed) + " \"" + fieldval +
                                     QObject::tr("\" fields have already been assigned."
                                     "\nSelecting this will de-select it elsewhere."));
                }
            }
            else if((fullyUsedValues[fieldval] == 1) && (box->currentText() != fieldval)) {
                // at capacity, and not selected in this box
                item->setForeground(Qt::darkRed);
                if(numAllowed == 1) {
                    item->setToolTip(QObject::tr("The \"") + fieldval + QObject::tr("\" field has already been assigned."
                                     "\nSelecting this will de-select it elsewhere."));
                }
                else {
                    item->setToolTip(QObject::tr("All ") + QString::number(numAllowed) + " \"" + fieldval +
                                     QObject::tr("\" fields have already been assigned."
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
