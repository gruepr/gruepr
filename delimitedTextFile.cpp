#include "delimitedTextFile.h"

DelimitedTextFile::DelimitedTextFile(Delimiter dlmtr)
{
    if(dlmtr == Delimiter::tab) {
        delimiter = '\t';
    }
}

DelimitedTextFile::~DelimitedTextFile()
{
    close();
}


bool DelimitedTextFile::openExistingFile(const QString &filepath)
{
    close();
    if(filepath.isEmpty()) {
        return false;
    }

    file = std::make_unique<QFile>(filepath);
    if(!file->open(QIODevice::ReadOnly)) {
        file.reset();
        return false;
    }

    stream = std::make_unique<QTextStream>(file.get());
    estimatedNumberRows = stream->readAll().count('\n');
    stream->seek(0);
    openFilePath = filepath;
    return true;
}


QString DelimitedTextFile::fileDialogFilter(const QString &filetypeDescriptor) const
{
    return filetypeDescriptor + " File (*.csv *.txt);;All Files (*)";
}


void DelimitedTextFile::releaseResource()
{
    stream.reset();
    file.reset();
}


bool DelimitedTextFile::atEnd()
{
    if(stream == nullptr) {
        return true;
    }
    return stream->atEnd();
}


void DelimitedTextFile::resetToStart()
{
    if(stream != nullptr) {
        stream->seek(0);
    }
}


//////////////////
// Read one raw row from the stream. An empty (but present) QStringList means "blank line, keep reading";
// std::nullopt means there are no more lines.
//////////////////
std::optional<QStringList> DelimitedTextFile::readNextRawRow()
{
    if(stream == nullptr || stream->atEnd()) {
        return std::nullopt;
    }
    return getLine(-1);
}


//////////////////
// Read one line from the file
//////////////////
QStringList DelimitedTextFile::getLine(const int minFields)
{
    return getLine(*stream, minFields, delimiter);
}


//////////////////
// Static function: Read one line from a textStream, smartly handling commas and newlines within fields that are enclosed by quotation marks; returns fields as list of strings
//////////////////
QStringList DelimitedTextFile::getLine(QTextStream &externalStream, const int minFields, const char delimiter)
{
    // read up to a newline
    QString line = externalStream.readLine();
    static const int MAX_LINES_TO_APPEND = 100;
    int linesAppended = 0;
    while(line.count('"')%2 == 1 && !externalStream.atEnd() && linesAppended < MAX_LINES_TO_APPEND) {
        line.append('\n' + externalStream.readLine());
        linesAppended++;
    }
    // if we hit the limit, the quote is unmatched — strip it so the parser doesn't stay in Quote state
    if(line.count('"')%2 == 1) {
        line.remove(line.lastIndexOf('"'), 1);
    }

    enum {Normal, Quote} state = Normal;
    QStringList fields;
    fields.reserve(std::max(minFields, int(line.count(delimiter))));
    QString value;

    for(int i = 0; i < line.size(); i++) {
        const QChar current=line.at(i);

        // Normal state
        if (state == Normal) {
            // Comma
            if (current == delimiter) {
                // Save field
                fields.append(value.trimmed());
                value.clear();
            }

            // Double-quote
            else if (current == '"') {
                state = Quote;
                value += current;
            }

            // Other character
            else {
                value += current;
            }
        }

        // In-quote state
        else if (state == Quote) {
            if (current == '"') {
                // Another quotation mark
                if (i < line.size()) {
                    if (i+1 < line.size() && line.at(i+1) == '"') {
                        // Skip a second quotation mark in a row as it is the escape sequence to represent a single quotation mark
                        value += '"';
                        i++;
                    }
                    else {
                        state = Normal;
                        value += '"';
                    }
                }
            }
            else {
                // Other character
                value += current;
            }
        }
    }
    if (!value.isEmpty()) {
        fields.append(value.trimmed());
    }

    // Quotes are left in until here; so when fields are trimmed, only whitespace outside of
    // quotes is removed.  The quotes are removed here.
    for (auto &field : fields) {
        if(field.length() >= 1) {
            if(field.at(0) == '"') {
                field = field.mid(1);
                if(field.length() >= 1) {
                    if(field.right(1) == '"') {
                        field = field.left(field.length() - 1);
                    }
                }
            }
        }
    }

    if(minFields == -1) {     // default value of -1 means just return however many fields are found
        return fields;
    }

    // no data found--just return empty QStringList
    if(fields.isEmpty()) {
        return fields;
    }

    // Append empty final field(s) to get up to minFields
    while(fields.size() < minFields) {
        fields.append("");
    }
    return fields;
}
