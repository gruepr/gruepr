#include "csvfile.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QString>
#include <QTableWidget>

CsvFile::CsvFile() = default;

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
    fieldMeanings.reserve(headerValues.size());
    return !headerValues.isEmpty();
}


//////////////////
// Read the a line from the file, splitting & saving field texts into fieldValues
//////////////////
bool CsvFile::readDataRow(const int minFields)
{
    fieldValues = getLine(minFields);
    return !fieldValues.isEmpty();
}


//////////////////
// Open dialog box to let user choose which columns correspond to which fields
//////////////////
QDialog* CsvFile::chooseFieldMeaningsDialog(const QStringList &possibleFieldMeanings, const QStringList &possibleMeaningMatchPatterns, QWidget *parent)
{
    // if not already set, preload fieldMeanings with meanings, based on matches to the patterns, if given
    if(fieldMeanings.isEmpty())
    {
        for(auto &headerVal : headerValues)
        {
            int matchPattern = 0;
            while((matchPattern < possibleMeaningMatchPatterns.size()) &&
                  !headerVal.contains(QRegularExpression(possibleMeaningMatchPatterns.at(matchPattern), QRegularExpression::CaseInsensitiveOption)))
            {
                matchPattern++;
            }

            if(matchPattern != possibleMeaningMatchPatterns.size())
            {
                fieldMeanings << possibleFieldMeanings.at(matchPattern);
            }
            else
            {
                fieldMeanings << "Unused";
            }
        }
    }

    window = new QDialog(parent);
    //Set up window with a grid layout
    window->setWindowTitle("Select column definitions");
    window->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    window->setSizeGripEnabled(true);
    window->setMinimumSize(300, 300);

    auto *theGrid = new QGridLayout(window);

    auto *explanation = new QLabel(window);
    explanation->setText("<html>The following column headers were found. "
                         "Select the the information contained in each column.<hr></html>");
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);

    auto *table = new QTableWidget(window);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->hide();
    table->horizontalHeader()->hide();
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setStyleSheet("QTableView::item{border-bottom: 1px solid black;}");
    table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    theGrid->addWidget(table, 1, 0, 1, -1);

    // a label and combobox for each column
    const int numFields = headerValues.size();
    table->setRowCount(numFields);
    table->setColumnCount(2);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

    for(int row = 0; row < numFields; row++)
    {
        auto *label = new QLabel();
        label->setText(headerValues.at(row));
        label->setWordWrap(true);
        table->setCellWidget(row, 0, label);

        auto *selector = new QComboBox();
        selector->addItems(possibleFieldMeanings);
        selector->insertItem(0, "Unused");
        selector->insertSeparator(1);
        selector->setCurrentText(fieldMeanings.at(row));
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        table->setCellWidget(row, 1, selector);

        connect(selector, &QComboBox::currentTextChanged, this, [this, row] (const QString &text){fieldMeanings[row] = text;});
    }
    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    table->adjustSize();

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(2, 20);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, window);
    theGrid->addWidget(buttonBox, 3, 1, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, window, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, window, &QDialog::reject);

    window->adjustSize();
    return window;
}


//////////////////
// Read one line from the file
//////////////////
QStringList CsvFile::getLine(const int minFields)
{
    return getLine(*stream, minFields);
}


//////////////////
// Read one line from a textStream, smartly handling commas within fields that are enclosed by quotation marks; returns fields as list of strings
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
