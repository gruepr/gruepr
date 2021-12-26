#include "editOrAddStudentDialog.h"
#include <QCollator>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show/edit student data
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editOrAddStudentDialog::editOrAddStudentDialog(const StudentRecord &studentToBeEdited, const DataOptions *const dataOptions, QWidget *parent)
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

    int numFields = (internalDataOptions.timestampField != -1? 1 : 0) + (internalDataOptions.firstNameField != -1? 1 : 0) +
                    (internalDataOptions.lastNameField != -1? 1 : 0) + (internalDataOptions.emailField != -1? 1 : 0) +
                    (internalDataOptions.genderIncluded? 1 : 0) + (internalDataOptions.URMIncluded? 1 : 0) + (internalDataOptions.sectionIncluded? 1 : 0) +
                    ((internalDataOptions.numAttributes > 0)? 1 : 0) + (internalDataOptions.prefTeammatesIncluded? 1 : 0) +
                    (internalDataOptions.prefNonTeammatesIncluded? 1 : 0) + ((internalDataOptions.numNotes > 0)? 1 : 0);
    explanation = new QLabel[numFields];
    datatext = new QLineEdit[NUMSINGLELINES];
    datamultiline = new QPlainTextEdit[NUMMULTILINES];
    databox = new QComboBox[NUMCOMBOBOXES];
    attributeTabs = new QTabWidget;
    datacategorical = new CategoricalSpinBox[internalDataOptions.numAttributes];
    int field = 0;

    // calculate the height of 1 row of text in the multilines
    QFontMetrics fm(datamultiline[0].document()->defaultFont());
    QMargins margin = datamultiline[0].contentsMargins();
    const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline[0].document()->documentMargin()) +
                                datamultiline[0].frameWidth() * 2 + margin.top() + margin.bottom();

    if(internalDataOptions.timestampField != -1)
    {
        explanation[field].setText(tr("Survey timestamp"));
        datatext[timestamp].setText(student.surveyTimestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)));
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[timestamp], field, 1);
        //Flag invalid timestamps
        connect(&datatext[timestamp], &QLineEdit::textChanged, this, [this]()
                                                         {bool validTimeStamp = QDateTime::fromString(datatext[timestamp].text(),
                                                                QLocale::system().dateTimeFormat(QLocale::ShortFormat)).isValid();
                                                          QString stylecolor = (validTimeStamp? "black" : "red");
                                                          datatext[timestamp].setStyleSheet("QLineEdit {color: " + stylecolor + ";}");});
        field++;
    }

    if(internalDataOptions.firstNameField != -1)
    {
        explanation[field].setText(tr("First name"));
        datatext[firstname].setText(student.firstname);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[firstname], field, 1);
        field++;
    }

    if(internalDataOptions.lastNameField != -1)
    {
        explanation[field].setText(tr("Last name"));
        datatext[lastname].setText(student.lastname);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[lastname], field, 1);
        field++;
    }

    if(internalDataOptions.emailField != -1)
    {
        explanation[field].setText(tr("Email address"));
        datatext[email].setText(student.email);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[email], field, 1);
        field++;
    }

    if(internalDataOptions.genderIncluded)
    {
        explanation[field].setText(tr("Gender identity"));
        databox[gender].addItems(QStringList() << tr("woman") << tr("man") << tr("nonbinary") << tr("unknown"));
        if(student.gender == StudentRecord::woman)
        {
            databox[gender].setCurrentText(tr("woman"));
        }
        else if(student.gender == StudentRecord::man)
        {
            databox[gender].setCurrentText(tr("man"));
        }
        else if(student.gender == StudentRecord::nonbinary)
        {
            databox[gender].setCurrentText(tr("nonbinary"));
        }
        else
        {
           databox[gender].setCurrentText(tr("unknown"));
        }
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[gender], field, 1);
        field++;
    }

    if(internalDataOptions.URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[ethnicity].addItems(internalDataOptions.URMResponses);
        databox[ethnicity].setEditable(true);
        databox[ethnicity].setCurrentText(student.URMResponse);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[ethnicity], field, 1);
        field++;
    }

    if(internalDataOptions.sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        databox[section].addItems(internalDataOptions.sectionNames);
        databox[section].setEditable(true);
        databox[section].setCurrentText(student.section);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[section], field, 1);
        field++;
    }

    if(internalDataOptions.numAttributes != 0)
    {
        for(int attribute = 0; attribute < internalDataOptions.numAttributes; attribute++)
        {
            datacategorical[attribute].setWhatTypeOfValue((internalDataOptions.attributeType[attribute] == DataOptions::ordered) ?
                                                           CategoricalSpinBox::numerical : CategoricalSpinBox::letter);
            datacategorical[attribute].setCategoricalValues(internalDataOptions.attributeQuestionResponses[attribute]);
            datacategorical[attribute].setValue(student.attributeVals[attribute].first());
            datacategorical[attribute].setRange(0, internalDataOptions.attributeMax[attribute]);
            datacategorical[attribute].setSpecialValueText(tr("not set/unknown"));
        }
        if(internalDataOptions.numAttributes == 1)
        {
            explanation[field].setText(tr("Attribute"));
            theGrid->addWidget(&datacategorical[0], field, 1);
        }
        else
        {
            explanation[field].setText(tr("Attributes"));
            for(int attribute = 0; attribute < internalDataOptions.numAttributes; attribute++)
            {
                auto w = new QWidget;
                auto layout = new QVBoxLayout;
                layout->addWidget(&datacategorical[attribute], 0, Qt::AlignVCenter);
                w->setLayout(layout);
                attributeTabs->addTab(w, QString::number(attribute+1));
            }
            theGrid->addWidget(attributeTabs, field, 1);
            theGrid->setRowMinimumHeight(field, rowOfTextHeight * 3);
        }
        theGrid->addWidget(&explanation[field], field, 0);
        field++;
    }

    if(internalDataOptions.prefTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Teammates") + tr("<br><i>&nbsp;&nbsp;Firstname Lastname<br>"
                                                                  "&nbsp;&nbsp;Enter each name on a separate line</i>"));
        datamultiline[prefTeammates].setPlainText(student.prefTeammates);
        datamultiline[prefTeammates].setFixedHeight(rowOfTextHeight * 3);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[prefTeammates], field, 1);
        field++;
    }

    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Non-teammates") + tr("<br><i>&nbsp;&nbsp;Firstname Lastname<br>"
                                                                       "&nbsp;&nbsp;Enter each name on a separate line</i>"));
        datamultiline[prefNonTeammates].setPlainText(student.prefNonTeammates);
        datamultiline[prefNonTeammates].setFixedHeight(rowOfTextHeight * 3);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[prefNonTeammates], field, 1);
        field++;
    }

    if(internalDataOptions.numNotes > 0)
    {
        explanation[field].setText(tr("Notes"));
        datamultiline[notes].setPlainText(student.notes);
        datamultiline[notes].setFixedHeight(rowOfTextHeight * 3);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[notes], field, 1);
        field++;
    }

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(numFields+1, rowOfTextHeight);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, numFields+2, 0, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]{updateRecord(); accept();});
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


void editOrAddStudentDialog::updateRecord()
{
    student.surveyTimestamp = QDateTime::fromString(datatext[timestamp].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[firstname].text();
    student.lastname = datatext[lastname].text();
    student.email = datatext[email].text();
    if(internalDataOptions.genderIncluded)
    {
        if(databox[gender].currentText() == tr("woman"))
        {
            student.gender = StudentRecord::woman;
        }
        else if(databox[gender].currentText() == tr("man"))
        {
            student.gender = StudentRecord::man;
        }
        else if(databox[gender].currentText() == tr("nonbinary"))
        {
            student.gender = StudentRecord::nonbinary;
        }
        else
        {
            student.gender = StudentRecord::unknown;
        }
    }
    if(internalDataOptions.URMIncluded)
    {
        student.URMResponse = databox[ethnicity].currentText();
    }
    if(internalDataOptions.sectionIncluded)
    {
        student.section = databox[section].currentText();
    }
    for(int attribute = 0; attribute < internalDataOptions.numAttributes; attribute++)
    {
        if(datacategorical[attribute].value() == 0)
        {
            student.attributeVals[attribute].first() = -1;
            student.attributeResponse[attribute] = "";
        }
        else
        {
            student.attributeVals[attribute].first() = datacategorical[attribute].value();
            student.attributeResponse[attribute] = internalDataOptions.attributeQuestionResponses[attribute].at(datacategorical[attribute].value() - 1);
        }
    }
    if(internalDataOptions.prefTeammatesIncluded)
    {
        student.prefTeammates = datamultiline[prefTeammates].toPlainText();
    }
    if(internalDataOptions.prefNonTeammatesIncluded)
    {
        student.prefNonTeammates = datamultiline[prefNonTeammates].toPlainText();
    }
    if(internalDataOptions.numNotes > 0)
    {
        student.notes = datamultiline[notes].toPlainText();
    }
}
