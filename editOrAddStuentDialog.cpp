#include "editOrAddStuentDialog.h"
#include <QCollator>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show/edit student data
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editOrAddStudentDialog::editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QStringList sectionNames, QWidget *parent)
    :QDialog (parent)
{
    student = studentToBeEdited;
    internalDataOptions = *dataOptions;

    //Set up window with a grid layout
    if(studentToBeEdited.surveyTimestamp.secsTo(QDateTime::currentDateTime()) < 10)     // if timestamp is within the past 10 seconds, it is a new student
    {
        setWindowTitle(tr("Add new student record"));
    }
    else
    {
        setWindowTitle(tr("Edit student record"));
    }
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    setSizeGripEnabled(true);
    theGrid = new QGridLayout(this);

    int numFields = 4 + (internalDataOptions.genderIncluded?1:0) + (internalDataOptions.URMIncluded?1:0) + (internalDataOptions.sectionIncluded?1:0) +
                         internalDataOptions.numAttributes + (internalDataOptions.prefTeammatesIncluded?1:0) +
                        (internalDataOptions.prefNonTeammatesIncluded?1:0) + ((internalDataOptions.numNotes > 0)?1:0);
    explanation = new QLabel[numFields];
    datatext = new QLineEdit[numFields];
    datamultiline = new QPlainTextEdit[numFields];
    databox = new QComboBox[numFields];
    datacategorical = new CategoricalSpinBox[numFields];
    int field = 0;

    // calculate the height of 1 row of text in the multilines
    QFontMetrics fm(datamultiline[0].document()->defaultFont());
    QMargins margin = datamultiline[0].contentsMargins();
    const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline[0].document()->documentMargin()) + datamultiline[0].frameWidth() * 2 + margin.top() + margin.bottom();

    //Row 1 through 4--the required data
    QStringList fieldNames = {tr("Survey timestamp"), tr("First name"), tr("Last name"), tr("Email address")};
    QStringList fieldValues = {student.surveyTimestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)),
                               student.firstname, student.lastname, student.email};
    for(int i = 0; i < 4; i++)
    {
        explanation[field].setText(fieldNames.at(field));
        datatext[field].setText(fieldValues.at(field));
        connect(&datatext[field], &QLineEdit::editingFinished, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[field], field, 1);
        field++;
    }

    //Flag invalid timestamps
    connect(&(datatext[0]), &QLineEdit::textChanged, this, [this]()
                                                     {bool validTimeStamp = QDateTime::fromString(datatext[0].text(),
                                                            QLocale::system().dateTimeFormat(QLocale::ShortFormat)).isValid();
                                                      QString stylecolor = (validTimeStamp? "black" : "red");
                                                      datatext[0].setStyleSheet("QLineEdit {color: " + stylecolor + ";}");});

    if(internalDataOptions.genderIncluded)
    {
        explanation[field].setText(tr("Gender identity"));
        databox[field].addItems(QStringList() << tr("woman") << tr("man") << tr("nonbinary") << tr("unknown"));
        if(student.gender == StudentRecord::woman)
        {
            databox[field].setCurrentText(tr("woman"));
        }
        else if(student.gender == StudentRecord::man)
        {
            databox[field].setCurrentText(tr("man"));
        }
        else if(student.gender == StudentRecord::nonbinary)
        {
            databox[field].setCurrentText(tr("nonbinary"));
        }
        else
        {
           databox[field].setCurrentText(tr("unknown"));
        }
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(internalDataOptions.URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[field].addItems(internalDataOptions.URMResponses);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.URMResponse);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    if(internalDataOptions.sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        QCollator sortAlphanumerically;
        sortAlphanumerically.setNumericMode(true);
        sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(sectionNames.begin(), sectionNames.end(), sortAlphanumerically);
        databox[field].addItems(sectionNames);
        databox[field].setEditable(true);
        databox[field].setCurrentText(student.section);
        connect(&databox[field], &QComboBox::currentTextChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[field], field, 1);
        field++;
    }

    for(int attrib = 0; attrib < internalDataOptions.numAttributes; attrib++)
    {
        explanation[field].setText(tr("Attribute ") + QString::number(attrib + 1));
        QSpinBox *spinbox = &datacategorical[field];
        datacategorical[field].setWhatTypeOfValue(internalDataOptions.attributeIsOrdered[attrib] ? CategoricalSpinBox::numerical : CategoricalSpinBox::letter);
        datacategorical[field].setCategoricalValues(internalDataOptions.attributeQuestionResponses[attrib]);
        spinbox->setValue(student.attributeVal[attrib]);
        spinbox->setRange(0, internalDataOptions.attributeMax[attrib]);
        if(spinbox->value() == 0)
        {
            spinbox->setStyleSheet("QSpinBox {background-color: #DCDCDC;}");
        }
        connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int /*unused new value*/){ recordEdited(); });
        spinbox->setSpecialValueText(tr("not set/unknown"));
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(spinbox, field, 1);
        field++;
    }

    if(internalDataOptions.prefTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Teammates") + tr("<br><i>&nbsp;&nbsp;Firstname Lastname<br>&nbsp;&nbsp;Enter each name on a separate line</i>"));
        datamultiline[field].setPlainText(student.prefTeammates);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Non-teammates") + tr("<br><i>&nbsp;&nbsp;Firstname Lastname<br>&nbsp;&nbsp;Enter each name on a separate line</i>"));
        datamultiline[field].setPlainText(student.prefNonTeammates);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    if(internalDataOptions.numNotes > 0)
    {
        explanation[field].setText(tr("Notes"));
        datamultiline[field].setPlainText(student.notes);
        datamultiline[field].setFixedHeight(rowOfTextHeight * 3);
        connect(&datamultiline[field], &QPlainTextEdit::textChanged, this, &editOrAddStudentDialog::recordEdited);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[field], field, 1);
        field++;
    }

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numFields+1, DIALOG_SPACER_ROWHEIGHT);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numFields+2, 0, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


editOrAddStudentDialog::~editOrAddStudentDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] explanation;
    delete [] datatext;
    delete [] datamultiline;
    delete [] databox;
    delete [] datacategorical;
}


void editOrAddStudentDialog::recordEdited()
{
    student.surveyTimestamp = QDateTime::fromString(datatext[0].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[1].text();
    student.lastname = datatext[2].text();
    student.email = datatext[3].text();
    int field = 4;
    if(internalDataOptions.genderIncluded)
    {
        if(databox[field].currentText() == tr("woman"))
        {
            student.gender = StudentRecord::woman;
        }
        else if(databox[field].currentText() == tr("man"))
        {
            student.gender = StudentRecord::man;
        }
        else if(databox[field].currentText() == tr("nonbinary"))
        {
            student.gender = StudentRecord::nonbinary;
        }
        else
        {
            student.gender = StudentRecord::unknown;
        }
        field++;
    }
    if(internalDataOptions.URMIncluded)
    {
        student.URMResponse = databox[field].currentText();
        field++;
    }
    if(internalDataOptions.sectionIncluded)
    {
        student.section = databox[field].currentText();
        field++;
    }
    for(int attribute = 0; attribute < internalDataOptions.numAttributes; attribute++)
    {
        if(datacategorical[field].value() == 0)
        {
            student.attributeVal[attribute] = -1;
            student.attributeResponse[attribute] = "";
            datacategorical[field].setStyleSheet("QSpinBox {background-color: #DCDCDC;}");
        }
        else
        {
            student.attributeVal[attribute] = datacategorical[field].value();
            student.attributeResponse[attribute] = internalDataOptions.attributeQuestionResponses[attribute].at(datacategorical[field].value() - 1);
            datacategorical[field].setStyleSheet("QSpinBox {}");
        }
        field++;
    }
    if(internalDataOptions.prefTeammatesIncluded)
    {
        student.prefTeammates = datamultiline[field].toPlainText();
        field++;
    }
    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        student.prefNonTeammates = datamultiline[field].toPlainText();
        field++;
    }
    if(internalDataOptions.numNotes > 0)
    {
        student.notes = datatext[field].text();
        field++;
    }
}
