#include "editOrAddStudentDialog.h"
#include <QCheckBox>
#include <QCollator>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show/edit student data
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editOrAddStudentDialog::editOrAddStudentDialog(StudentRecord &student, const DataOptions *const dataOptions, QWidget *parent, bool newStudent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    if(newStudent)
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

    int numFields = (dataOptions->timestampField != -1? 1 : 0) + (dataOptions->firstNameField != -1? 1 : 0) +
                    (dataOptions->lastNameField != -1? 1 : 0) + (dataOptions->emailField != -1? 1 : 0) +
                    (dataOptions->genderIncluded? 1 : 0) + (dataOptions->URMIncluded? 1 : 0) + (dataOptions->sectionIncluded? 1 : 0) +
                    ((dataOptions->numAttributes > 0)? 1 : 0) + (dataOptions->prefTeammatesIncluded? 1 : 0) +
                    (dataOptions->prefNonTeammatesIncluded? 1 : 0) + ((dataOptions->numNotes > 0)? 1 : 0);
    explanation = new QLabel[numFields];
    datatext = new QLineEdit[NUMSINGLELINES];
    datamultiline = new QPlainTextEdit[NUMMULTILINES];
    databox = new QComboBox[NUMCOMBOBOXES];
    attributeTabs = new QTabWidget(this);
    QVector<int> orderedAttributes, categoricalAttributes, multicategoricalAttributes, timezoneAttribute;
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        if(dataOptions->attributeType[attribute] == DataOptions::ordered)
        {
            orderedAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::categorical)
        {
            categoricalAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::multicategorical)
        {
            multicategoricalAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::timezone)
        {
            timezoneAttribute << attribute;
        }
    }
    attributeCombobox = new QComboBox[orderedAttributes.size() + categoricalAttributes.size() + timezoneAttribute.size()];
    attributemulticategoricalbox = new QGroupBox[multicategoricalAttributes.size()];

    // calculate the height of 1 row of text in the multilines
    QFontMetrics fm(datamultiline[0].document()->defaultFont());
    QMargins margin = datamultiline[0].contentsMargins();
    const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline[0].document()->documentMargin()) +
                                datamultiline[0].frameWidth() * 2 + margin.top() + margin.bottom();

    int field = 0;
    if(dataOptions->timestampField != -1)
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

    if(dataOptions->firstNameField != -1)
    {
        explanation[field].setText(tr("First name"));
        datatext[firstname].setText(student.firstname);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[firstname], field, 1);
        field++;
    }

    if(dataOptions->lastNameField != -1)
    {
        explanation[field].setText(tr("Last name"));
        datatext[lastname].setText(student.lastname);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[lastname], field, 1);
        field++;
    }

    if(dataOptions->emailField != -1)
    {
        explanation[field].setText(tr("Email address"));
        datatext[email].setText(student.email);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datatext[email], field, 1);
        field++;
    }

    if(dataOptions->genderIncluded)
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

    if(dataOptions->URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[ethnicity].addItems(dataOptions->URMResponses);
        databox[ethnicity].setEditable(true);
        databox[ethnicity].setCurrentText(student.URMResponse);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[ethnicity], field, 1);
        field++;
    }

    if(dataOptions->sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        databox[section].addItems(dataOptions->sectionNames);
        databox[section].setEditable(true);
        databox[section].setCurrentText(student.section);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&databox[section], field, 1);
        field++;
    }

    if(dataOptions->numAttributes != 0)
    {
        // create the UI for each attribute - either a combobox with single possible values loaded or checkboxes for multiple possible values
        int comboboxNum = 0, multicategoricalbox = 0;
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
        {
            if(orderedAttributes.contains(attribute) || categoricalAttributes.contains(attribute))
            {
                attributeCombobox[comboboxNum].setEditable(false);
                attributeCombobox[comboboxNum].insertItem(0, tr("not set/unknown"), -1);
                attributeCombobox[comboboxNum].insertSeparator(1);
                auto attributeValIter = dataOptions->attributeVals[attribute].cbegin();
                for(const auto &response : dataOptions->attributeQuestionResponses[attribute])
                {
                    attributeCombobox[comboboxNum].addItem(response, *attributeValIter);
                    attributeValIter++;
                }

                if(student.attributeResponse[attribute].isEmpty())
                {
                    attributeCombobox[comboboxNum].setCurrentIndex(0);
                }
                else
                {
                    attributeCombobox[comboboxNum].setCurrentText(student.attributeResponse[attribute]);
                }
                comboboxNum++;
            }
            else if(timezoneAttribute.contains(attribute))
            {
                attributeCombobox[comboboxNum].setEditable(false);
                attributeCombobox[comboboxNum].insertItem(0,tr("not set/unknown"));
                attributeCombobox[comboboxNum].insertSeparator(1);
                QStringList timezones = QString(TIMEZONENAMES).split(";");
                QRegularExpression offsetFinder(".*\\[GMT(.*):(.*)\\].*");  // characters after "[GMT" are +hh:mm "]"
                for(auto &timezone : timezones)
                {
                    timezone.remove(QChar('"'));
                    float timezoneOffset = 0;
                    QRegularExpressionMatch offset = offsetFinder.match(timezone);
                    if(offset.hasMatch())
                    {
                        float hours = offset.captured(1).toFloat();
                        float minutes = offset.captured(2).toFloat();
                        timezoneOffset = hours + ((hours < 0)? (-minutes/60) : (minutes/60));
                    }
                    attributeCombobox[comboboxNum].addItem(timezone, timezoneOffset);
                }
                if(student.attributeResponse[attribute].isEmpty())  // no response, so "unknown"
                {
                    attributeCombobox[comboboxNum].setCurrentIndex(0);
                }
                else if(timezones.contains(student.attributeResponse[attribute]))  // exact match to response found in timezones
                {
                    attributeCombobox[comboboxNum].setCurrentText(student.attributeResponse[attribute]);
                }
                else  // no exact match for some reason, so match to the numerical value of timezone if possible (and revert to "unknown" if still can't find)
                {
                    int index = attributeCombobox[comboboxNum].findData(student.timezone);
                    attributeCombobox[comboboxNum].setCurrentIndex(index != -1? index : 0);
                }
                comboboxNum++;
            }
            else if(multicategoricalAttributes.contains(attribute))
            {
                attributemulticategoricalbox[multicategoricalbox].setFlat(true);
                auto layout = new QVBoxLayout;
                for(int option = 0, totNumOptions = dataOptions->attributeQuestionResponses[attribute].size(); option < totNumOptions; option++)
                {
                    auto responseCheckBox = new QCheckBox(dataOptions->attributeQuestionResponses[attribute].at(option));
                    responseCheckBox->setChecked(student.attributeVals[attribute].contains(option + 1));
                    layout->addWidget(responseCheckBox);
                }
                layout->addStretch(1);
                attributemulticategoricalbox->setLayout(layout);
                multicategoricalbox++;
            }
        }

        // add to window the single attribute or, in tabs, the multiple attributes
        comboboxNum = 0;
        multicategoricalbox = 0;
        if(dataOptions->numAttributes == 1)
        {
            if(timezoneAttribute.contains(0))
            {
                explanation[field].setText(tr("Timezone"));
            }
            else
            {
                explanation[field].setText(dataOptions->attributeQuestionText.at(0));
            }

            if(orderedAttributes.contains(0) || categoricalAttributes.contains(0) || timezoneAttribute.contains(0))
            {
                theGrid->addWidget(&attributeCombobox[comboboxNum], field, 1);
            }
            else
            {
                theGrid->addWidget(&attributemulticategoricalbox[multicategoricalbox], field, 1);
            }
        }
        else    // more than one attribute
        {
            explanation[field].setText(tr("Attributes"));
            for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
            {
                auto w = new QWidget;
                auto layout = new QVBoxLayout;
                auto attributeQuestionText = new QLabel;
                layout->addWidget(attributeQuestionText, 0, Qt::AlignLeft);
                if(orderedAttributes.contains(attribute) || categoricalAttributes.contains(attribute))
                {
                    attributeQuestionText->setText((dataOptions->attributeQuestionText.at(attribute)));
                    layout->addWidget(&attributeCombobox[comboboxNum], 1, Qt::AlignVCenter);
                    comboboxNum++;
                }
                else if(timezoneAttribute.contains(attribute))
                {
                    attributeQuestionText->setText(tr("Timezone"));
                    layout->addWidget(&attributeCombobox[comboboxNum], 1, Qt::AlignVCenter);
                    comboboxNum++;
                }
                else if(multicategoricalAttributes.contains(attribute))
                {
                    attributeQuestionText->setText((dataOptions->attributeQuestionText.at(attribute)));
                    layout->addWidget(&attributemulticategoricalbox[multicategoricalbox], 1, Qt::AlignVCenter);
                    multicategoricalbox++;
                }
                layout->addStretch(1);
                w->setLayout(layout);
                attributeTabs->addTab(w, QString::number(attribute+1));
            }
            theGrid->addWidget(attributeTabs, field, 1);
            theGrid->setRowMinimumHeight(field, rowOfTextHeight * 3);
        }
        theGrid->addWidget(&explanation[field], field, 0);
        field++;
    }

    if(dataOptions->prefTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline[prefTeammates].setPlainText(student.prefTeammates);
        datamultiline[prefTeammates].setFixedHeight(rowOfTextHeight * 3);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[prefTeammates], field, 1);
        field++;
    }

    if(dataOptions->prefNonTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Non-teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline[prefNonTeammates].setPlainText(student.prefNonTeammates);
        datamultiline[prefNonTeammates].setFixedHeight(rowOfTextHeight * 3);
        theGrid->addWidget(&explanation[field], field, 0);
        theGrid->addWidget(&datamultiline[prefNonTeammates], field, 1);
        field++;
    }

    if(dataOptions->numNotes > 0)
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
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this, &student, dataOptions]{updateRecord(student, dataOptions); accept();});
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
    delete [] attributeCombobox;
    delete [] attributemulticategoricalbox;
}


void editOrAddStudentDialog::updateRecord(StudentRecord &student, const DataOptions *const dataOptions)
{
    student.surveyTimestamp = QDateTime::fromString(datatext[timestamp].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[firstname].text();
    student.lastname = datatext[lastname].text();
    student.email = datatext[email].text();
    if(dataOptions->genderIncluded)
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
    if(dataOptions->URMIncluded)
    {
        student.URMResponse = databox[ethnicity].currentText();
    }
    if(dataOptions->sectionIncluded)
    {
        student.section = databox[section].currentText();
    }
    int comboboxNum = 0, multicategoricalbox = 0;
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        student.attributeVals[attribute].clear();
        if(dataOptions->attributeType[attribute] == DataOptions::multicategorical)
        {
            for(int itemNum = 0; itemNum < attributemulticategoricalbox[multicategoricalbox].layout()->count(); itemNum++)
            {
                // loop through all items in the attributemulticategoricalbox: make sure it's a checkbox, then add the response if it's checked
                auto *optionCheckBox = qobject_cast<QCheckBox*>(attributemulticategoricalbox[multicategoricalbox].layout()->itemAt(itemNum)->widget());
                QStringList attributeResponse;
                if((optionCheckBox != nullptr) && (optionCheckBox->isChecked()))
                {
                    student.attributeVals[attribute] << (dataOptions->attributeQuestionResponses[attribute].indexOf(optionCheckBox->text()) + 1);
                    attributeResponse << optionCheckBox->text();
                }
                student.attributeResponse[attribute] = attributeResponse.join(',');
            }
            if(student.attributeVals[attribute].isEmpty())
            {
                student.attributeVals[attribute] << -1;
                student.attributeResponse[attribute] = "";
            }
        }
        else
        {
            if(attributeCombobox[comboboxNum].currentIndex() == 0)
            {
                student.attributeResponse[attribute] = "";
                student.attributeVals[attribute] << -1;
                if(dataOptions->attributeType[attribute] == DataOptions::timezone)
                {
                    student.timezone = 0;
                }
            }
            else
            {
                student.attributeResponse[attribute] = attributeCombobox[comboboxNum].currentText();
                if(dataOptions->attributeType[attribute] == DataOptions::timezone)
                {
                    student.timezone = attributeCombobox[comboboxNum].currentData().toFloat();
                    student.attributeVals[attribute] << int(student.timezone);
                }
                else
                {
                    student.attributeVals[attribute] << attributeCombobox[comboboxNum].currentData().toInt();
                }
            }
            comboboxNum++;
        }
    }
    if(dataOptions->prefTeammatesIncluded)
    {
        student.prefTeammates = datamultiline[prefTeammates].toPlainText();
    }
    if(dataOptions->prefNonTeammatesIncluded)
    {
        student.prefNonTeammates = datamultiline[prefNonTeammates].toPlainText();
    }
    if(dataOptions->numNotes > 0)
    {
        student.notes = datamultiline[notes].toPlainText();
    }
}
