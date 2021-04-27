#include "csvfile.h"
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QStandardItemModel>
#include <QString>

CsvFile::CsvFile(QObject *parent) : QObject(parent)
{
}

CsvFile::~CsvFile()
{
    if(file != nullptr)
    {
        close();
    }

    delete window;
}

//////////////////
// Open CSV file from Dialog Box, returning QFile (blank if unsuccessful)
//////////////////
bool CsvFile::open(QWidget *parent, const QString &caption, const QString &filepath, const QString &filetypeDescriptor)
{
    file = nullptr;
    stream = nullptr;

    QString actualCaption = caption;
    if(actualCaption == "")
    {
        actualCaption = "Open csv File";
    }

    QString fileName = QFileDialog::getOpenFileName(parent, actualCaption, filepath, filetypeDescriptor + " File (*.csv *.txt);;All Files (*)");

    if (!fileName.isEmpty())
    {
        file = new QFile(fileName);
        file->open(QIODevice::ReadOnly);
        stream = new QTextStream(file);
    }

    return (stream != nullptr);
}

//////////////////
// Retrieve the fileInfo
//////////////////
QFileInfo CsvFile::fileInfo()
{
    return QFileInfo(*file);
}


//////////////////
// Close CSV file
//////////////////
void CsvFile::close()
{
    delete stream;
    stream = nullptr;
    file->close();
    delete file;
    file = nullptr;
}


//////////////////
// Read the first line of the file, splitting & saving field texts into headerValues, making room in fieldMeanings for each
//////////////////
bool CsvFile::readHeader()
{
    stream->seek(0);
    headerValues = getLine();
    numFields = headerValues.size();
    fieldMeanings.reserve(numFields);
    for(int i = 0; i < numFields; i++)
    {
        fieldMeanings << "";
    }
    return !headerValues.isEmpty();
}


//////////////////
// Read the a line from the file, splitting & saving field texts into fieldValues
//////////////////
bool CsvFile::readDataRow()
{
    fieldValues = getLine(numFields);
    return !fieldValues.isEmpty();
}


//////////////////
// Open dialog box to let user choose which columns correspond to which fields
//////////////////
QDialog* CsvFile::chooseFieldMeaningsDialog(const QVector<possFieldMeaning> &possibleFieldMeanings, QWidget *parent)
{
    // if any of the fieldMeanings are empty, preload with possibleFieldMeaning based on matches to the patterns, if given
    for(int i = 0; i < numFields; i++)
    {
        if(fieldMeanings.at(i).isEmpty())
        {
            const QString &headerVal = headerValues.at(i);
            int matchPattern = 0;
            QString match = std::get<1>(possibleFieldMeanings.at(matchPattern));
            while((matchPattern < possibleFieldMeanings.size()) &&
                  !headerVal.contains(QRegularExpression(match, QRegularExpression::CaseInsensitiveOption)))
            {
                matchPattern++;
                match = std::get<1>(possibleFieldMeanings.at(matchPattern));
            }

            if(matchPattern != possibleFieldMeanings.size())
            {
                fieldMeanings[i] = std::get<0>(possibleFieldMeanings.at(matchPattern));
            }
            else
            {
                fieldMeanings[i] = "Unused";
            }
        }
    }

    window = new listTableDialog(tr("Select column definitions"), false, false, parent);
    //Set up window with a grid layout
    window->setMinimumSize(500, 300);

    auto *explanation = new QLabel(window);
    explanation->setText("<html>The following column headers were found in the file. "
                         "Please verify the category of information contained in each column. Select \"Unused\" for any field(s) that should be ignored.<hr></html>");
    explanation->setWordWrap(true);
    window->theGrid->addWidget(explanation, 0, 0, 1, -1);
    window->addSpacerRow(1);

    // a label and combobox for each column
    window->theTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    window->theTable->horizontalHeader()->setStyleSheet("QHeaderView{font: bold large}");
    window->theTable->setHorizontalHeaderLabels(QStringList({"Header Text","Category"}));
    window->theTable->setRowCount(numFields);
    for(int row = 0; row < numFields; row++)
    {
        auto *label = new QLabel();
        label->setText("\n" + headerValues.at(row) + "\n");
        label->setWordWrap(true);
        label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        window->theTable->setCellWidget(row, 0, label);

        auto *selector = new QComboBox();
        for(auto &meaning : possibleFieldMeanings)
        {
            selector->addItem(std::get<0>(meaning), std::get<2>(meaning));
        }
        selector->insertItem(0, "Unused");
        auto *model = qobject_cast<QStandardItemModel *>(selector->model());
        model->item(0)->setForeground(Qt::darkRed);
        selector->insertSeparator(1);
        selector->setCurrentText(fieldMeanings.at(row));
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        window->theTable->setCellWidget(row, 1, selector);

        connect(selector, &QComboBox::currentTextChanged, this, [this, row]{validateFieldSelectorBoxes(row);});
    }
    validateFieldSelectorBoxes();
    window->theTable->resizeColumnsToContents();
    window->theTable->resizeRowsToContents();
    window->theTable->adjustSize();

    window->adjustSize();
    return window;
}


//////////////////
// Validate the selector boxes in the choose field meaning dialog--one per field unless there's an asterisk in the name, in which case there are as many as the number after
//////////////////
void CsvFile::validateFieldSelectorBoxes(int callingRow)
{
    // get list of rows in top-to-bottom order, but if this function is getting called by a selector box, then put its row at the front of the line
    QVector<int> rows(numFields);
    std::iota(rows.begin(), rows.end(), 0);
    if(callingRow != -1)
    {
        rows.remove(callingRow);
        rows.prepend(callingRow);
    }

    // start by counting all the values to count how many times each are used, and which are fully used
    std::map<QString, int> takenValues;     // mapping fieldMeaning -> number of fields selected with this meaning
    std::map<QString, int> fullyUsedValues; // mapping the same, but saving how many extra fields with this meaning
    for(auto row : rows)
    {
        // get the selected fieldMeaning
        const auto box = qobject_cast<QComboBox *>(window->theTable->cellWidget(row, 1));
        QString selection = box->currentText();

        // set it in the CsvFile's data
        fieldMeanings[row] = selection;

        // add this occurence in the takenValues mapping
        if(takenValues.count(selection) == 0)
        {
            // first ocurrence of this field; create the key/value
            takenValues[selection] = 1;
        }
        else
        {
            // key already exists
            takenValues[selection]++;
        }

        // if we are at or above the allowed number of ocurrences, note it
        if(takenValues[selection] >= box->currentData().toInt())
        {
            // add this occurence in the takenValues mapping
            if(fullyUsedValues.count(selection) == 0)
            {
                // field has just reached capacity; create the key/value
                fullyUsedValues[selection] = 1;
            }
            else
            {
                // key already exists; we have MORE than are allowed
                fullyUsedValues[selection]++;
            }
        }
        else
        {
            fullyUsedValues[selection] = 0;
        }
    }

    // Now go back through in reverse order and:
    //  1) replacing overused values with "Unused",
    //  2) setting fully used values in other boxes to red with a tooltip,
    //  3) clearing formatting of all non-overused values (except "Unused") and the fully used values that are currently chosen.
    // Then:
    //  4) clearing formatting of all items unchosen in any box (except "Unused").
    for(auto row = rows.rbegin(); row != rows.rend(); ++row)
    {
        auto box = qobject_cast<QComboBox *>(window->theTable->cellWidget(*row, 1));
        box->blockSignals(true);
        auto *model = qobject_cast<QStandardItemModel *>(box->model());
        for(auto &takenValue : takenValues)
        {
            QString fieldval = takenValue.first;
            int numAllowed = box->itemData(box->findText(fieldval)).toInt();
            QStandardItem *item = model->item(box->findText(fieldval));
            if((fullyUsedValues[fieldval] > 1) && (box->currentText() == fieldval))
            {
                // number exceeds max. allowed somehow, so set to unused
                box->setCurrentText("Unused");
                fieldMeanings[*row] = "Unused";
                fullyUsedValues[fieldval]--;
                if(numAllowed == 1)
                {
                    item->setToolTip("The \"" + fieldval + "\" field has already been assigned."
                                     "\nSelecting this will de-select it elsewhere.");
                }
                else
                {
                    item->setToolTip("All " + QString::number(numAllowed) + " \"" + fieldval + "\" fields have already been assigned."
                                     "\nSelecting this will de-select it elsewhere.");
                }
            }
            else if((fullyUsedValues[fieldval] == 1) && (box->currentText() != fieldval))
            {
                // at capacity, and not selected in this box
                item->setForeground(Qt::darkRed);
                if(numAllowed == 1)
                {
                    item->setToolTip("The \"" + fieldval + "\" field has already been assigned."
                                     "\nSelecting this will de-select it elsewhere.");
                }
                else
                {
                    item->setToolTip("All " + QString::number(numAllowed) + " \"" + fieldval + "\" fields have already been assigned."
                                     "\nSelecting this will de-select it elsewhere.");
                }
            }
            else if(fieldval != "Unused")
            {
                // below capacity or at capacity including this one
                item->setForeground(Qt::black);
                item->setToolTip("");
            }
        }

        // clearing formatting of all unchosen items except "Unused"
        for(int itemNum = 0; itemNum < box->count(); itemNum++)
        {
            if((takenValues.count(box->itemText(itemNum)) == 0) && (box->itemText(itemNum) != "Unused"))
            {
                model->item(itemNum)->setForeground(Qt::black);
                model->item(itemNum)->setToolTip("");
            }
        }
        box->blockSignals(false);
    }
}


//////////////////
// Read one line from the file
//////////////////
QStringList CsvFile::getLine(const int minFields)
{
    return getLine(*stream, minFields);
}


//////////////////
// Static function: Read one line from a textStream, smartly handling commas and newlines within fields that are enclosed by quotation marks; returns fields as list of strings
//////////////////
QStringList CsvFile::getLine(QTextStream &externalStream, const int minFields)
{
    // read up to a newline
    QString line = externalStream.readLine();
    // if there's a newline within a field, the number of " characters will be odd, so appeand to next newline and check again
    while(line.count('"')%2 == 1)
    {
        line.append(externalStream.readLine());
    }

    enum State {Normal, Quote} state = Normal;
    QStringList fields;
    QString value;

    for(int i = 0; i < line.size(); i++)
    {
        QChar current=line.at(i);

        // Normal state
        if (state == Normal)
        {
            // Comma
            if (current == ',')
            {
                // Save field
                fields.append(value.trimmed());
                value.clear();
            }

            // Double-quote
            else if (current == '"')
            {
                state = Quote;
                value += current;
            }

            // Other character
            else
            {
                value += current;
            }
        }

        // In-quote state
        else if (state == Quote)
        {
            // Another double-quote
            if (current == '"')
            {
                if (i < line.size())
                {
                    // A double double-quote?
                    if (i+1 < line.size() && line.at(i+1) == '"')
                    {
                        value += '"';

                        // Skip a second quote character in a row
                        i++;
                    }
                    else
                    {
                        state = Normal;
                        value += '"';
                    }
                }
            }

            // Other character
            else
            {
                value += current;
            }
        }
    }
    if (!value.isEmpty())
    {
        fields.append(value.trimmed());
    }

    // Quotes are left in until here; so when fields are trimmed, only whitespace outside of
    // quotes is removed.  The quotes are removed here.
    for (int i=0; i<fields.size(); ++i)
    {
        if(fields[i].length() >= 1)
        {
            if(fields[i].at(0) == '"')
            {
                fields[i] = fields[i].mid(1);
                if(fields[i].length() >= 1)
                {
                    if(fields[i].right(1) == '"')
                    {
                        fields[i] = fields[i].left(fields[i].length() - 1);
                    }
                }
            }
        }
    }

    if(minFields == -1)      // default value of -1 means just return however many fields are found
    {
        return fields;
    }

    // no data found--just return empty QStringList
    if(fields.isEmpty())
    {
        return fields;
    }

    // Append empty final field(s) to get up to minFields
    while(fields.size() < minFields)
    {
        fields.append("");
    }

    return fields;
}
