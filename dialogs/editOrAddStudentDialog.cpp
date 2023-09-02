#include "editOrAddStudentDialog.h"
#include <QCheckBox>
#include <QCollator>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to show/edit student data
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editOrAddStudentDialog::editOrAddStudentDialog(StudentRecord &student, const DataOptions *const dataOptions, QWidget *parent, bool newStudent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    if(newStudent) {
        setWindowTitle(tr("Add new student record"));
    }
    else {
        setWindowTitle(tr("Edit student record"));
    }
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
    setSizeGripEnabled(true);
    theGrid = new QGridLayout(this);

    auto *fieldAreaWidg = new QWidget;
    auto *fieldAreaLayout = new QVBoxLayout(fieldAreaWidg);
    fieldAreaWidg->setLayout(fieldAreaLayout);
    auto *fieldArea = new QScrollArea(this);
    fieldArea->setWidget(fieldAreaWidg);
    fieldArea->setStyleSheet(QString() + "QScrollArea{border: none;}" + SCROLLBARSTYLE);
    fieldArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    fieldArea->setWidgetResizable(true);

    if(dataOptions->timestampField != -1) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Survey timestamp"));
        datatext << new QLineEdit(this);
        datatext.last()->setStyleSheet(LINEEDITSTYLE);
        datatext.last()->setText(student.surveyTimestamp.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)));
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datatext.last(), 0);
        //Flag invalid timestamps
        connect(datatext.last(), &QLineEdit::textChanged, this, [thisLineEdit = datatext.last()]()
                        {if(QDateTime::fromString(thisLineEdit->text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat)).isValid())
                            {thisLineEdit->setStyleSheet(LINEEDITSTYLE);}
                         else
                            {thisLineEdit->setStyleSheet(QString(LINEEDITSTYLE).replace("color: " DEEPWATERHEX ";", "color: red;"));}});
    }

    if(dataOptions->firstNameField != -1) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("First name"));
        datatext << new QLineEdit(this);
        datatext.last()->setStyleSheet(LINEEDITSTYLE);
        datatext.last()->setText(student.firstname);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datatext.last(), 0);
    }

    if(dataOptions->lastNameField != -1) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Last name"));
        datatext << new QLineEdit(this);
        datatext.last()->setStyleSheet(LINEEDITSTYLE);
        datatext.last()->setText(student.lastname);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datatext.last(), 0);
    }

    if(dataOptions->emailField != -1) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Email address"));
        datatext << new QLineEdit(this);
        datatext.last()->setStyleSheet(LINEEDITSTYLE);
        datatext.last()->setText(student.email);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datatext.last(), 0);
    }

    if(dataOptions->genderIncluded) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        QStringList genderOptions;
        if(dataOptions->genderType == GenderType::biol) {
            explanation.last()->setText(tr("Gender"));
            genderOptions = QString(BIOLGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::adult) {
            explanation.last()->setText(tr("Gender identity"));
            genderOptions = QString(ADULTGENDERS).split('/');
        }
        else if(dataOptions->genderType == GenderType::child) {
            explanation.last()->setText(tr("Gender identity"));
            genderOptions = QString(CHILDGENDERS).split('/');
        }
        else { //if(dataOptions->genderResponses == GenderType::pronoun)
            explanation.last()->setText(tr("Pronouns"));
            genderOptions = QString(PRONOUNS).split('/');
        }
        databox << new ComboBoxThatPassesScrollwheel(this);
        databox.last()->setStyleSheet(COMBOBOXSTYLE);
        databox.last()->addItems(genderOptions);
        databox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        databox.last()->setCurrentIndex(static_cast<int>(student.gender));
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(databox.last(), 0, Qt::AlignLeft);
    }

    if(dataOptions->URMIncluded) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Racial/ethnic/cultural identity"));
        databox << new ComboBoxThatPassesScrollwheel(this);
        databox.last()->setStyleSheet(COMBOBOXSTYLE);
        databox.last()->addItems(dataOptions->URMResponses);
        databox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        databox.last()->setEditable(true);
        databox.last()->setCurrentText(student.URMResponse);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(databox.last(), 0, Qt::AlignLeft);
    }

    if(dataOptions->sectionIncluded) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Section"));
        databox << new ComboBoxThatPassesScrollwheel(this);
        databox.last()->setStyleSheet(COMBOBOXSTYLE);
        databox.last()->addItems(dataOptions->sectionNames);
        databox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        databox.last()->setEditable(true);
        databox.last()->setCurrentText(student.section);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(databox.last(), 0, Qt::AlignLeft);
    }

    if(dataOptions->numAttributes != 0) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Multiple choice questions"));

        auto *attributeFrame = new QFrame(this);
        attributeFrame->setStyleSheet("QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; border: none;}");
        auto *attributeFrameLayout = new QVBoxLayout;
        attributeFrame->setLayout(attributeFrameLayout);
        QGridLayout *attributeSelectorGrid = nullptr;
        if(dataOptions->numAttributes > 1) {
            attributeSelectorGrid = new QGridLayout;
            attributeSelectorGrid->setSpacing(0);
            attributeFrameLayout->addLayout(attributeSelectorGrid);
            attributeFrameLayout->addStrut(DIALOG_SPACER_ROWHEIGHT);
        }
        attributeStack = new QStackedWidget(this);
        attributeFrameLayout->addWidget(attributeStack);

        QList<int> orderedAttributes, categoricalAttributes, multicategoricalAttributes, multiorderedAttributes, timezoneAttribute;
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) {
                orderedAttributes << attribute;
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical) {
                categoricalAttributes << attribute;
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                multicategoricalAttributes << attribute;
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                multiorderedAttributes << attribute;
            }
            else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::timezone) {
                timezoneAttribute << attribute;
            }
        }

        // create the UI for each attribute - either a combobox with single possible values loaded or checkboxes for multiple possible values
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            auto *w = new QWidget;
            auto *layout = new QVBoxLayout;
            w->setLayout(layout);

            //add a selector button if there's multiple attributes
            if(dataOptions->numAttributes > 1) {
                const int rowSize = 5;  // number of buttons in each row
                attributeSelectorButtons << new QPushButton(tr("Q") + QString::number(attribute + 1));
                attributeSelectorButtons.last()->setFlat(true);
                QString stylesheet = attribute == 0? ATTRIBBUTTONONSTYLE : ATTRIBBUTTONOFFSTYLE;
                if(attribute == 0) {
                    stylesheet.replace("border-top-left-radius: 0px;", "border-top-left-radius: 5px;");
                }
                if( ((dataOptions->numAttributes < rowSize) && (attribute == (dataOptions->numAttributes - 1))) ||
                    ((dataOptions->numAttributes >= rowSize) && ((attribute / rowSize) == 0) && ((attribute % rowSize) == (rowSize - 1))) ) {
                    stylesheet.replace("border-top-right-radius: 0px;", "border-top-right-radius: 5px;");
                }
                if(attribute == (dataOptions->numAttributes-1)) {
                    stylesheet.replace("border-bottom-right-radius: 0px;", "border-bottom-right-radius: 5px;");
                }
                if( ((attribute / rowSize) == ((dataOptions->numAttributes - 1) / rowSize)) && ((attribute % rowSize) == 0) ) {
                    stylesheet.replace("border-bottom-left-radius: 0px;", "border-bottom-left-radius: 5px;");
                }
                attributeSelectorButtons.last()->setStyleSheet(stylesheet);
                attributeSelectorGrid->addWidget(attributeSelectorButtons.last(), attribute/rowSize, attribute%rowSize);
                connect(attributeSelectorButtons.last(), &QPushButton::clicked, this, [this, attribute, dataOptions]
                        {attributeStack->setCurrentIndex(attribute);
                            for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++) {
                                if( (attribute == attrib) ||
                                    (attributeSelectorButtons[attrib]->styleSheet()
                                         .contains("background-color: " OPENWATERHEX ";")) ) {
                                    attributeSelectorButtons[attrib]->setStyleSheet(
                                        attributeSelectorButtons[attrib]->styleSheet()
                                            .replace("white", "black")
                                            .replace(OPENWATERHEX, "white")
                                            .replace("black", OPENWATERHEX));
                                }
                            }
                        });
                attributeSelectorGrid->setColumnStretch(rowSize, 1);
            }

            // create a question text label
            auto *attributeQuestionText = new QLabel;
            attributeQuestionText->setStyleSheet(LABELSTYLE);
            attributeQuestionText->setWordWrap(true);
            layout->addWidget(attributeQuestionText, Qt::AlignLeft);

            // create a responsses label and load up the prefixes to use in the combobox or checkboxes (unless it's timezone question)
            QStringList prefixes;
            if(!timezoneAttribute.contains(attribute)) {
                auto *responsesLabel = new QLabel(this);
                responsesLabel->setStyleSheet(LABELSTYLE);
                responsesLabel->setTextFormat(Qt::RichText);
                responsesLabel->setWordWrap(true);
                responsesLabel->setIndent(10);
                auto *responsesFrame = new QFrame(this);
                responsesFrame->setStyleSheet("QFrame {background-color: " TRANSPARENT "; border: none; padding: 2 px;}");
                responsesFrame->setLineWidth(1);
                responsesFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
                auto *hlay = new QHBoxLayout(responsesFrame);
                hlay->addWidget(responsesLabel);
                static const QRegularExpression startsWithInteger(R"(^(\d++)([\.\,]?$|[\.\,]\D|[^\.\,]))");
                QRegularExpressionMatch match;
                int responseNum = 0;
                bool first = true;
                QString responsesText = "<html>";    //hanging indent
                for(const auto &response : qAsConst(dataOptions->attributeQuestionResponses[attribute]))
                {
                    if(!first) {
                        responsesText += "<br>";
                    }
                    first = false;
                    responsesText += "<b>";
                    if(orderedAttributes.contains(attribute) || multiorderedAttributes.contains(attribute))
                    {
                        // show response with starting number
                        match = startsWithInteger.match(response);
                        responsesText += match.captured(1) + "</b>" + response.mid(match.capturedLength(1));
                        prefixes << match.captured(1);
                    }
                    else if(categoricalAttributes.contains(attribute) || multicategoricalAttributes.contains(attribute))
                    {
                        // show response with a preceding letter (letter repeated for responses after 26)
                        QString prefix =  (responseNum < 26 ? QString(char(responseNum + 'A')) : QString(char(responseNum%26 + 'A')).repeated(1 + (responseNum/26)));
                        responsesText += prefix;
                        responsesText += "</b>. " + response;
                        prefixes << prefix;
                    }
                    responseNum++;
                }
                responsesText += "</html>";
                responsesLabel->setText(responsesText);
                layout->addWidget(responsesFrame, Qt::AlignLeft);
            }

            // create the combobox or group of checkboxes as relevant
            if(orderedAttributes.contains(attribute) || categoricalAttributes.contains(attribute)) {
                attributeQuestionText->setText((dataOptions->attributeQuestionText.at(attribute)));
                attributeCombobox << new ComboBoxThatPassesScrollwheel(this);
                attributeCombobox.last()->setStyleSheet(COMBOBOXSTYLE);
                attributeCombobox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                attributeCombobox.last()->setEditable(false);
                attributeCombobox.last()->insertItem(0, tr("no response/unknown"), -1);
                attributeCombobox.last()->insertSeparator(1);
                auto attributeValIter = dataOptions->attributeVals[attribute].cbegin();
                for(const auto &prefix : prefixes) {
                    attributeCombobox.last()->addItem(prefix, *attributeValIter);
                    attributeValIter++;
                }

                if(student.attributeResponse[attribute].isEmpty()) {
                    attributeCombobox.last()->setCurrentIndex(0);
                }
                else {
                    attributeCombobox.last()->setCurrentText(student.attributeResponse[attribute]);
                }
                layout->addWidget(attributeCombobox.last());
            }
            else if(timezoneAttribute.contains(attribute)) {
                attributeQuestionText->setText(tr("Timezone"));
                attributeCombobox << new ComboBoxThatPassesScrollwheel(this);
                attributeCombobox.last()->setStyleSheet(COMBOBOXSTYLE);
                attributeCombobox.last()->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                attributeCombobox.last()->setEditable(false);
                attributeCombobox.last()->insertItem(0,tr("no response/unknown"));
                attributeCombobox.last()->insertSeparator(1);
                QStringList timezones = QString(TIMEZONENAMES).split(";");
                for(auto &zonenameText : timezones) {
                    zonenameText.remove('"');
                    QString zonename;
                    float GMTOffset = 0;
                    DataOptions::parseTimezoneInfoFromText(zonenameText, zonename, GMTOffset);
                    attributeCombobox.last()->addItem(zonenameText, GMTOffset);
                }
                if(student.attributeResponse[attribute].isEmpty()) {
                    attributeCombobox.last()->setCurrentIndex(0);
                }
                else if(timezones.contains(student.attributeResponse[attribute])) {  // exact match to response found in timezones
                    attributeCombobox.last()->setCurrentText(student.attributeResponse[attribute]);
                }
                else {  // no exact match for some reason, so match to the numerical value of timezone if possible (and revert to "unknown" if still can't find)
                    int index = attributeCombobox.last()->findData(student.timezone);
                    attributeCombobox.last()->setCurrentIndex(index != -1? index : 0);
                }
                layout->addWidget(attributeCombobox.last());
            }
            else if(multicategoricalAttributes.contains(attribute) || multiorderedAttributes.contains(attribute)) {
                attributeQuestionText->setText((dataOptions->attributeQuestionText.at(attribute)));
                attributeMultibox << new QGroupBox(this);
                attributeMultibox.last()->setFlat(true);
                auto *internalLayout = new QVBoxLayout;
                for(int option = 0, totNumOptions = int(dataOptions->attributeQuestionResponses[attribute].size()); option < totNumOptions; option++) {
                    auto *responseCheckBox = new QCheckBox(prefixes.at(option));
                    responseCheckBox->setStyleSheet(CHECKBOXSTYLE);
                    responseCheckBox->setChecked(student.attributeVals[attribute].contains(option + 1));
                    internalLayout->addWidget(responseCheckBox);
                }
                attributeMultibox.last()->setLayout(internalLayout);
                layout->addWidget(attributeMultibox.last());
            }
            layout->addStretch(1);
            attributeStack->addWidget(w);
        }
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(attributeFrame, 0);
    }

    if(!dataOptions->dayNames.isEmpty()) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Schedule"));
        auto *adjustScheduleButton = new QPushButton(this);
        adjustScheduleButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        adjustScheduleButton->setText(tr("Adjust schedule"));
        connect(adjustScheduleButton, &QPushButton::clicked, this, [this, &student, dataOptions](){adjustSchedule(student, dataOptions);});
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(adjustScheduleButton, 0);
        for(int day = 0; day < MAX_DAYS; day++) {
            for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
                tempUnavailability[day][time] = student.unavailable[day][time];
            }
        }
    }

    if(dataOptions->prefTeammatesIncluded) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setTextFormat(Qt::RichText);
        explanation.last()->setText(tr("Preferred Teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline << new QPlainTextEdit(this);
        datamultiline.last()->setStyleSheet(PLAINTEXTEDITSTYLE);
        datamultiline.last()->setPlainText(student.prefTeammates);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datamultiline.last(), 0);
    }

    if(dataOptions->prefNonTeammatesIncluded) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setTextFormat(Qt::RichText);
        explanation.last()->setText(tr("Preferred Non-teammates") + "<br><i>&nbsp;&nbsp;" + tr("Firstname Lastname") +
                                      "<br>&nbsp;&nbsp;" + tr("Enter each name on a separate line") + "</i>");
        datamultiline << new QPlainTextEdit(this);
        datamultiline.last()->setStyleSheet(PLAINTEXTEDITSTYLE);
        datamultiline.last()->setPlainText(student.prefNonTeammates);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datamultiline.last(), 0);
    }

    if(dataOptions->numNotes > 0) {
        explanation << new QLabel(this);
        explanation.last()->setStyleSheet(LABELSTYLE);
        explanation.last()->setText(tr("Notes"));
        datamultiline << new QPlainTextEdit(this);
        datamultiline.last()->setStyleSheet(PLAINTEXTEDITSTYLE);
        datamultiline.last()->setPlainText(student.notes);
        fieldAreaLayout->addWidget(explanation.last(), 0, Qt::AlignLeft);
        fieldAreaLayout->addWidget(datamultiline.last(), 0);
    }

    if((dataOptions->prefTeammatesIncluded) || (dataOptions->prefNonTeammatesIncluded) || (dataOptions->numNotes > 0)) {
        // calculate the height of 1 row of text in the multilines
        QFontMetrics fm(datamultiline.last()->document()->defaultFont());
        QMargins margin = datamultiline.last()->contentsMargins();
        const int rowOfTextHeight = fm.lineSpacing() + qRound(datamultiline.last()->document()->documentMargin()) +
                                    datamultiline.last()->frameWidth() * 2 + margin.top() + margin.bottom();
        for(auto &textbox : datamultiline) {
            textbox->setFixedHeight(rowOfTextHeight + 3);
        }
    }

    fieldAreaLayout->addStretch(1);
    theGrid->addWidget(fieldArea, 0, 0, 1, -1);


    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(1, DIALOG_SPACER_ROWHEIGHT);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    theGrid->addWidget(buttonBox, 2, 0, -1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this, &student, dataOptions]{updateRecord(student, dataOptions); accept();});
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setMinimumSize(QSize(SM_DLG_SIZE, LG_DLG_SIZE));
    adjustSize();
}


void editOrAddStudentDialog::updateRecord(StudentRecord &student, const DataOptions *const dataOptions)
{
    int textfield = 0, boxfield = 0, multilinefield = 0;
    if(dataOptions->timestampField != -1) {
        student.surveyTimestamp = QDateTime::fromString(datatext[textfield++]->text(), QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    }
    if(dataOptions->firstNameField != -1) {
        student.firstname = datatext[textfield++]->text();
    }
    if(dataOptions->lastNameField != -1) {
        student.lastname = datatext[textfield++]->text();
    }
    if(dataOptions->emailField != -1) {
        student.email = datatext[textfield++]->text();
    }
    if(dataOptions->genderIncluded) {
        student.gender = static_cast<Gender>(databox[boxfield++]->currentIndex());
    }
    if(dataOptions->URMIncluded) {
        student.URMResponse = databox[boxfield++]->currentText();
    }
    if(dataOptions->sectionIncluded) {
        student.section = databox[boxfield++]->currentText();
    }
/*
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
                    student.attributeVals[attribute] << int(dataOptions->attributeQuestionResponses[attribute].indexOf(optionCheckBox->text()) + 1);
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
*/
    if(!dataOptions->dayNames.isEmpty()){
        for(int day = 0; day < MAX_DAYS; day++) {
            for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
                student.unavailable[day][time] = tempUnavailability[day][time];
            }
        }
        student.availabilityChart = QObject::tr("Availability:");
        student.availabilityChart += "<table style='padding: 0px 3px 0px 3px;'><tr><th></th>";
        for(const auto &dayName : dataOptions->dayNames) {
            student.availabilityChart += "<th>" + dayName.toUtf8().left(3) + "</th>";   // using first 3 characters in day name as abbreviation
        }
        student.availabilityChart += "</tr>";
        for(int time = 0; time < dataOptions->timeNames.size(); time++) {
            student.availabilityChart += "<tr><th>" + dataOptions->timeNames.at(time).toUtf8() + "</th>";
            for(int day = 0; day < dataOptions->dayNames.size(); day++) {
                student.availabilityChart += QString(student.unavailable[day][time]?
                                                         "<td align = center> </td>" : "<td align = center bgcolor='PaleGreen'><b>√</b></td>");
            }
            student.availabilityChart += "</tr>";
        }
        student.availabilityChart += "</table>";

        student.ambiguousSchedule = (student.availabilityChart.count("√") == 0 || student.availabilityChart.count("√") == (dataOptions->dayNames.size() * dataOptions->timeNames.size()));
    }
    if(dataOptions->prefTeammatesIncluded) {
        student.prefTeammates = datamultiline[multilinefield++]->toPlainText();
    }
    if(dataOptions->prefNonTeammatesIncluded) {
        student.prefNonTeammates = datamultiline[multilinefield++]->toPlainText();
    }
    if(dataOptions->numNotes > 0) {
        student.notes = datamultiline[multilinefield++]->toPlainText();
    }
}


void editOrAddStudentDialog::adjustSchedule(const StudentRecord &student, const DataOptions *const dataOptions)
{
    auto *adjustScheduleWindow = new QDialog(this);
    //Set up window with a grid layout
    adjustScheduleWindow->setWindowTitle(tr("Adjust Schedule"));
    adjustScheduleWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
    auto *adjustScheduleWindowGrid = new QGridLayout(adjustScheduleWindow);
    int gridrow = 0, gridcolumn = 0;

    //explanation and a spacer row
    auto *adjustScheduleWindowExplanation = new QLabel(adjustScheduleWindow);
    adjustScheduleWindowExplanation->setText("<html>" + student.firstname + " " + student.lastname + tr(" is AVAILABLE to meet at these times: <hr></html>"));
    adjustScheduleWindowExplanation->setWordWrap(true);
    adjustScheduleWindowGrid->addWidget(adjustScheduleWindowExplanation, gridrow++, gridcolumn, 1, -1);
    adjustScheduleWindowGrid->setRowMinimumHeight(gridrow++, DIALOG_SPACER_ROWHEIGHT);

    //create a window copy of the temp schedule array, and use the values to pre-fill the grid of checkboxes
    //connect the clicking of any checkbox to updating the corresponding element in the window copy of the array
    bool windowUnavailability[MAX_DAYS][MAX_BLOCKS_PER_DAY];
    QCheckBox *checkBox[MAX_DAYS][MAX_BLOCKS_PER_DAY];
    for(int day = 0; day < MAX_DAYS; day++) {
        for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
            windowUnavailability[day][time] = tempUnavailability[day][time];
        }
    }
    gridcolumn = 1;
    for(const auto &dayName : dataOptions->dayNames) {
        auto *columnHeader = new QLabel(adjustScheduleWindow);
        columnHeader->setText(dayName.toUtf8().left(3));   // using first 3 characters in day name as abbreviation
        adjustScheduleWindowGrid->addWidget(columnHeader, gridrow, gridcolumn++, 1, 1);
    }
    gridrow++;
    for(int time = 0; time < dataOptions->timeNames.size(); time++) {
        gridcolumn = 0;
        auto *rowHeader = new QLabel(adjustScheduleWindow);
        rowHeader->setText(dataOptions->timeNames.at(time).toUtf8());
        adjustScheduleWindowGrid->addWidget(rowHeader, gridrow, gridcolumn++, 1, 1);
        for(int day = 0; day < dataOptions->dayNames.size(); day++) {
            checkBox[day][time] = new QCheckBox(adjustScheduleWindow);
            checkBox[day][time]->setChecked(!windowUnavailability[day][time]);
            adjustScheduleWindowGrid->addWidget(checkBox[day][time], gridrow, gridcolumn++, 1, 1);
            connect(checkBox[day][time], &QCheckBox::clicked, adjustScheduleWindow, [&windowUnavailability, day, time](bool checked){windowUnavailability[day][time] = !checked;});
        }
        gridrow++;
    }

    //a spacer then an invertAll button and ok/cancel buttons
    //if ok button is pushed, copy the window copy of the schedule array back in to the temp array
    gridcolumn = 0;
    adjustScheduleWindowGrid->setRowMinimumHeight(gridrow++, DIALOG_SPACER_ROWHEIGHT);
    auto *invertAllButton = new QPushButton(QIcon(":/icons/swap.png"), tr("Invert Schedule"), adjustScheduleWindow);
    connect(invertAllButton, &QPushButton::clicked, adjustScheduleWindow, [&windowUnavailability, &dataOptions, checkBox]()
                                                                           {for(int day = 0; day < dataOptions->dayNames.size(); day++) {
                                                                                for(int time = 0; time < dataOptions->timeNames.size(); time++) {
                                                                                    windowUnavailability[day][time] = !windowUnavailability[day][time];
                                                                                    checkBox[day][time]->toggle();
                                                                                }
                                                                           }
                                                                           });
    adjustScheduleWindowGrid->addWidget(invertAllButton, gridrow, gridcolumn++, 1, 1);
    auto *adjustScheduleWindowButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, adjustScheduleWindow);
    connect(adjustScheduleWindowButtonBox, &QDialogButtonBox::accepted, adjustScheduleWindow, [this, &windowUnavailability, adjustScheduleWindow]()
                                                                                               {for(int day = 0; day < MAX_DAYS; day++) {
                                                                                                    for(int time = 0; time < MAX_BLOCKS_PER_DAY; time++) {
                                                                                                        tempUnavailability[day][time] = windowUnavailability[day][time];
                                                                                                    }
                                                                                                }
                                                                                                adjustScheduleWindow->accept();});
    connect(adjustScheduleWindowButtonBox, &QDialogButtonBox::rejected, adjustScheduleWindow, &QDialog::reject);
    adjustScheduleWindowGrid->addWidget(adjustScheduleWindowButtonBox, gridrow, gridcolumn, 1, -1);

    adjustScheduleWindow->adjustSize();
    adjustScheduleWindow->exec();

    delete adjustScheduleWindow;
}
