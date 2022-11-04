#include "editOrAddStudentDialog.h"
#include <QCheckBox>
#include <QCollator>
#include <QScrollArea>
#include <QScrollBar>

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
    databox = new ComboBoxThatPassesScrollwheel[NUMCOMBOBOXES];
    attributeTabs = new QTabWidget(this);
    QVector<int> orderedAttributes, categoricalAttributes, multicategoricalAttributes, multiorderedAttributes, timezoneAttribute;
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered)
        {
            orderedAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical)
        {
            categoricalAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical)
        {
            multicategoricalAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)
        {
            multiorderedAttributes << attribute;
        }
        else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone)
        {
            timezoneAttribute << attribute;
        }
    }
    attributeCombobox = new ComboBoxThatPassesScrollwheel[orderedAttributes.size() + categoricalAttributes.size() + timezoneAttribute.size()];
    attributeMultibox = new QGroupBox[multicategoricalAttributes.size() + multiorderedAttributes.size()];

    // calculate the height of 1 row of text in the multilines
    QFontMetrics fm(datamultiline[0].document()->defaultFont());
    QMargins margin = datamultiline[0].contentsMargins();
    const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline[0].document()->documentMargin()) +
                                datamultiline[0].frameWidth() * 2 + margin.top() + margin.bottom();

    auto *fieldAreaWidg = new QWidget;
    auto *fieldAreaGrid = new QGridLayout(fieldAreaWidg);
    fieldAreaWidg->setLayout(fieldAreaGrid);
    auto *fieldArea = new QScrollArea(this);
    fieldArea->setWidget(fieldAreaWidg);

    int field = 0;
    if(dataOptions->timestampField != -1)
    {
        explanation[field].setText(tr("Survey timestamp"));
        datatext[timestamp].setText(student.surveyTimestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)));
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datatext[timestamp], field, 1);
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
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datatext[firstname], field, 1);
        field++;
    }

    if(dataOptions->lastNameField != -1)
    {
        explanation[field].setText(tr("Last name"));
        datatext[lastname].setText(student.lastname);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datatext[lastname], field, 1);
        field++;
    }

    if(dataOptions->emailField != -1)
    {
        explanation[field].setText(tr("Email address"));
        datatext[email].setText(student.email);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datatext[email], field, 1);
        field++;
    }

    if(dataOptions->genderIncluded)
    {
        QStringList genderOptions;
        if(dataOptions->genderType == GenderType::biol)
        {
            explanation[field].setText(tr("Gender"));
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::adult)
        {
            explanation[field].setText(tr("Gender identity"));
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::child)
        {
            explanation[field].setText(tr("Gender identity"));
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else //if(dataOptions->genderResponses == GenderType::pronoun)
        {
            explanation[field].setText(tr("Pronouns"));
            genderOptions = QString(PRONOUNS).split('/');
        }
        databox[gender].addItems(genderOptions);
        databox[gender].setCurrentIndex(static_cast<int>(student.gender));
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&databox[gender], field, 1);
        field++;
    }

    if(dataOptions->URMIncluded)
    {
        explanation[field].setText(tr("Racial/ethnic/cultural identity"));
        databox[ethnicity].addItems(dataOptions->URMResponses);
        databox[ethnicity].setEditable(true);
        databox[ethnicity].setCurrentText(student.URMResponse);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&databox[ethnicity], field, 1);
        field++;
    }

    if(dataOptions->sectionIncluded)
    {
        explanation[field].setText(tr("Section"));
        databox[section].addItems(dataOptions->sectionNames);
        databox[section].setEditable(true);
        databox[section].setCurrentText(student.section);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&databox[section], field, 1);
        field++;
    }

    if(dataOptions->numAttributes != 0)
    {
        // create the UI for each attribute - either a combobox with single possible values loaded or checkboxes for multiple possible values
        int comboboxNum = 0, multiboxNum = 0;
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
                for(auto &zonenameText : timezones)
                {
                    zonenameText.remove('"');
                    QString zonename;
                    float GMTOffset = 0;
                    DataOptions::parseTimezoneInfoFromText(zonenameText, zonename, GMTOffset);
                    attributeCombobox[comboboxNum].addItem(zonenameText, GMTOffset);
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
            else if(multicategoricalAttributes.contains(attribute) || multiorderedAttributes.contains(attribute))
            {
                attributeMultibox[multiboxNum].setFlat(true);
                auto *layout = new QVBoxLayout;
                for(int option = 0, totNumOptions = dataOptions->attributeQuestionResponses[attribute].size(); option < totNumOptions; option++)
                {
                    auto *responseCheckBox = new QCheckBox(dataOptions->attributeQuestionResponses[attribute].at(option));
                    responseCheckBox->setChecked(student.attributeVals[attribute].contains(option + 1));
                    layout->addWidget(responseCheckBox);
                }
                layout->addStretch(1);
                attributeMultibox[multiboxNum].setLayout(layout);
                multiboxNum++;
            }
        }

        // add to window the single attribute or, in tabs, the multiple attributes
        comboboxNum = 0;
        multiboxNum = 0;
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
                fieldAreaGrid->addWidget(&attributeCombobox[comboboxNum], field, 1);
            }
            else
            {
                fieldAreaGrid->addWidget(&attributeMultibox[multiboxNum], field, 1);
            }
        }
        else    // more than one attribute
        {
            explanation[field].setText(tr("Attributes"));
            for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
            {
                auto *w = new QWidget;
                auto *layout = new QVBoxLayout;
                auto *attributeQuestionText = new QLabel;
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
                else if(multicategoricalAttributes.contains(attribute) || multiorderedAttributes.contains(attribute))
                {
                    attributeQuestionText->setText((dataOptions->attributeQuestionText.at(attribute)));
                    layout->addWidget(&attributeMultibox[multiboxNum], 1, Qt::AlignVCenter);
                    multiboxNum++;
                }
                layout->addStretch(1);
                w->setLayout(layout);
                attributeTabs->addTab(w, QString::number(attribute+1));
            }
            fieldAreaGrid->addWidget(attributeTabs, field, 1);
            fieldAreaGrid->setRowMinimumHeight(field, rowOfTextHeight * 3);
        }
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        field++;
    }

    if(dataOptions->prefTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline[prefTeammates].setPlainText(student.prefTeammates);
        datamultiline[prefTeammates].setFixedHeight(rowOfTextHeight * 3);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datamultiline[prefTeammates], field, 1);
        field++;
    }

    if(dataOptions->prefNonTeammatesIncluded)
    {
        explanation[field].setTextFormat(Qt::RichText);
        explanation[field].setText(tr("Preferred Non-teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline[prefNonTeammates].setPlainText(student.prefNonTeammates);
        datamultiline[prefNonTeammates].setFixedHeight(rowOfTextHeight * 3);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datamultiline[prefNonTeammates], field, 1);
        field++;
    }

    if(dataOptions->numNotes > 0)
    {
        explanation[field].setText(tr("Notes"));
        datamultiline[notes].setPlainText(student.notes);
        datamultiline[notes].setFixedHeight(rowOfTextHeight * 3);
        fieldAreaGrid->addWidget(&explanation[field], field, 0);
        fieldAreaGrid->addWidget(&datamultiline[notes], field, 1);
        field++;
    }

    fieldArea->setWidgetResizable(true);
    theGrid->addWidget(fieldArea, 0, 0, 1, -1);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(1, rowOfTextHeight);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    theGrid->addWidget(buttonBox, 2, 0, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this, &student, dataOptions]{updateRecord(student, dataOptions); accept();});
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    fieldArea->setMinimumWidth(fieldAreaWidg->width() + fieldArea->verticalScrollBar()->sizeHint().width() + (2 * frameSize().width()));
    setMaximumHeight(parent->height());
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
    delete [] attributeMultibox;
}


void editOrAddStudentDialog::updateRecord(StudentRecord &student, const DataOptions *const dataOptions)
{
    student.surveyTimestamp = QDateTime::fromString(datatext[timestamp].text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    student.firstname = datatext[firstname].text();
    student.lastname = datatext[lastname].text();
    student.email = datatext[email].text();
    if(dataOptions->genderIncluded)
    {
        student.gender = static_cast<Gender>(databox[gender].currentIndex());
    }
    if(dataOptions->URMIncluded)
    {
        student.URMResponse = databox[ethnicity].currentText();
    }
    if(dataOptions->sectionIncluded)
    {
        student.section = databox[section].currentText();
    }
    int comboboxNum = 0, multiboxNum = 0;
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
    {
        student.attributeVals[attribute].clear();
        if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
           (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered))
        {
            QStringList attributeResponse;
            for(int itemNum = 0; itemNum < attributeMultibox[multiboxNum].layout()->count(); itemNum++)
            {
                // loop through all items in the attributeMultibox: make sure it's a checkbox, then add the response if it's checked
                auto *optionCheckBox = qobject_cast<QCheckBox*>(attributeMultibox[multiboxNum].layout()->itemAt(itemNum)->widget());
                if((optionCheckBox != nullptr) && (optionCheckBox->isChecked()))
                {
                    student.attributeVals[attribute] << (dataOptions->attributeQuestionResponses[attribute].indexOf(optionCheckBox->text()) + 1);
                    attributeResponse << optionCheckBox->text();
                }
            }
            student.attributeResponse[attribute] = attributeResponse.join(',');

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
                if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone)
                {
                    student.timezone = 0;
                }
            }
            else
            {
                student.attributeResponse[attribute] = attributeCombobox[comboboxNum].currentText();
                if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone)
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
