#include "findMatchingNameDialog.h"
#include "Levenshtein.h"
#include <QButtonGroup>
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to select a name from a list when a perfect match is not found
/////////////////////////////////////////////////////////////////////////////////////////////////////////

findMatchingNameDialog::findMatchingNameDialog(int numStudents, const StudentRecord *const student, const QString &searchName, QWidget *parent,
                                               const bool addStudentOption, const QString &searchEmail)
    :QDialog(parent)
{
    // create list of names (map is <Key = Levenshtein distance, Value = name & index in student array>)
    QMultiMap<int, QString> possibleStudents;
    for(int knownStudent = 0; knownStudent < numStudents; knownStudent++)
    {
        int rank = levenshtein::distance(searchName, student[knownStudent].firstname + " " + student[knownStudent].lastname);
        if(!searchEmail.isEmpty() && searchEmail.compare(student[knownStudent].email, Qt::CaseInsensitive) == 0)
        {
            rank = 0;
        }
        possibleStudents.insert(rank, student[knownStudent].firstname + " " + student[knownStudent].lastname + "&index=" + QString::number(knownStudent));
    }

    // Create student selection window
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    setWindowTitle("Choose student");
    theGrid = new QGridLayout(this);

    int row = 0;
    explanation = new QLabel(this);
    QString explanationText;
    if(!addStudentOption)
    {
        explanationText = tr("An exact match for") +
                          "<br><b>" + searchName + "</b><br>" +
                          tr("could not be found. The most closely matching names are:");
    }
    else
    {
        explanationText += tr("This student on the roster:") +
                           "<br><b>" + searchName + "</b><br>" +
                           tr("could not be found in the survey.") + "<br><br>" +
                           tr("Select one of the following options:");
    }
    explanation->setText(explanationText);
    theGrid->addWidget(explanation, row++, 0, 1, -1);

    namesList = new QComboBox(this);
    QMultiMap<int, QString>::const_iterator i = possibleStudents.constBegin();
    while (i != possibleStudents.constEnd())
    {
        QStringList nameAndNum = i.value().split("&index=");    // split off the index to use as the UserData role
        namesList->addItem(nameAndNum.at(0), nameAndNum.at(1).toInt());
        i++;
    }
    currSurveyName = namesList->currentText();
    currSurveyEmail = student[namesList->currentData().toInt()].email;
    currSurveyID = student[namesList->currentData().toInt()].ID;

    if(!addStudentOption)
    {
        theGrid->addWidget(namesList, row++, 0, 1, -1, Qt::AlignLeft);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Select match"));
        buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Ignore this student"));
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
        theGrid->addWidget(buttonBox, row, 0, 1, -1);

        connect(namesList, &QComboBox::currentTextChanged, this, [this, student](const QString &currText){currSurveyName = currText;
                                                                                                          currSurveyEmail = student[namesList->currentData().toInt()].email;
                                                                                                          currSurveyID = student[namesList->currentData().toInt()].ID;});
    }
    else
    {
        // buttons
        auto *addButton = new QPushButton("\n" + tr("Add ") + searchName + "\n" + tr("as a new student") + "\n", this);
        addButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(addButton, &QPushButton::clicked, this, [this]{addStudent = true; this->accept();});
        auto *mergeButton = new QPushButton("\n" + tr("Merge with\nthis record") + "\n", this);
        mergeButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(mergeButton, &QPushButton::clicked, this, &QDialog::accept);
        auto *ignoreButton = new QPushButton("\n" + tr("Ignore\nthis student") + "\n", this);
        ignoreButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        connect(ignoreButton, &QPushButton::clicked, this, &QDialog::reject);

        // UI for add student
        auto *addNameLabel = new QLabel(tr("Name") + ":  " + searchName, this);
        auto *addEmailLabel = new QLabel(tr("Email address") + ":  " + (searchEmail.isEmpty()? tr("--") : searchEmail), this);

        // UI for merge student
        auto *comboboxLabel = new QLabel(tr("The most closely matching names in the survey are:"), this);

        auto *nameGroup = new QButtonGroup(this);
        useRosterNameCheckbox = new QRadioButton(tr("Use roster name") + ":  " + searchName);
        useRosterNameCheckbox->setChecked(false);
        useRosterNameCheckbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        nameGroup->addButton(useRosterNameCheckbox);
        connect(useRosterNameCheckbox, &QRadioButton::clicked, this, [this](const bool checked){useRosterName = checked;});
        useSurveyNameCheckbox = new QRadioButton(tr("Use survey name") + ":  " + currSurveyName);
        useSurveyNameCheckbox->setChecked(true);
        useSurveyNameCheckbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        nameGroup->addButton(useSurveyNameCheckbox);

        auto *emailGroup = new QButtonGroup(this);
        useRosterEmailCheckbox = new QRadioButton(tr("Use roster email address") + ":  " + (searchEmail.isEmpty()? tr("--") : searchEmail));
        useRosterEmailCheckbox->setChecked(false);
        useRosterEmailCheckbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        emailGroup->addButton(useRosterEmailCheckbox);
        connect(useRosterEmailCheckbox, &QRadioButton::clicked, this, [this](const bool checked){useRosterEmail = checked;});
        useSurveyEmailCheckbox = new QRadioButton(tr("Use survey email address") + ":  " + (currSurveyEmail.isEmpty()? tr("--") : currSurveyEmail));
        useSurveyEmailCheckbox->setChecked(true);
        useSurveyEmailCheckbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        emailGroup->addButton(useSurveyEmailCheckbox);
        if(searchEmail == currSurveyEmail)
        {
            useRosterEmailCheckbox->setEnabled(false);
            useSurveyEmailCheckbox->setEnabled(false);
            useSurveyEmailCheckbox->setText(tr("The survey email address matches the roster"));
        }

        // setting up grid
        theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
        theGrid->addWidget(addButton, row, 0, 2, 1);
        theGrid->addWidget(addNameLabel, row++, 1, 1, -1, Qt::AlignBottom);
        theGrid->addWidget(addEmailLabel, row++, 1, 1, -1, Qt::AlignTop);
        auto *line1 = new QFrame(this);
        line1->setFrameShape(QFrame::HLine);
        line1->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(line1, row++, 0, 1, -1);
        theGrid->addWidget(mergeButton, row, 0, 7, 1);
        theGrid->addWidget(comboboxLabel, row++, 1, 1, -1);
        theGrid->addWidget(namesList, row++, 1, 1, -1, Qt::AlignLeft);
        theGrid->addWidget(useRosterNameCheckbox, row++, 1, 1, -1);
        theGrid->addWidget(useSurveyNameCheckbox, row++, 1, 1, -1);
        auto *line2 = new QFrame(this);
        line2->setFrameShape(QFrame::HLine);
        line2->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(line2, row++, 1, 1, -1);
        theGrid->addWidget(useRosterEmailCheckbox, row++, 1, 1, -1);
        theGrid->addWidget(useSurveyEmailCheckbox, row++, 1, 1, -1);
        auto *line3 = new QFrame(this);
        line3->setFrameShape(QFrame::HLine);
        line3->setFrameShadow(QFrame::Sunken);
        theGrid->addWidget(line3, row++, 0, 1, -1);
        theGrid->addWidget(ignoreButton, row, 0, 1, 1);

        connect(namesList, &QComboBox::currentTextChanged, this, [this, searchEmail, student](const QString &currText)
                                                                 {currSurveyName = currText;
                                                                  currSurveyEmail = student[namesList->currentData().toInt()].email;
                                                                  currSurveyID = student[namesList->currentData().toInt()].ID;
                                                                  useSurveyNameCheckbox->setText(tr("Use survey name") + ":  " + currSurveyName);
                                                                  useSurveyEmailCheckbox->setText(tr("Use survey email address")+ ":  " +
                                                                                           (currSurveyEmail.isEmpty()? tr("--") : currSurveyEmail));
                                                                  useRosterEmailCheckbox->setEnabled(searchEmail != currSurveyEmail);
                                                                  useSurveyEmailCheckbox->setEnabled(searchEmail != currSurveyEmail);
                                                                  if(searchEmail == currSurveyEmail)
                                                                  {useSurveyEmailCheckbox->setText(tr("The survey email address matches the roster"));}
                                                                  });
    }

    adjustSize();
}

findMatchingNameDialog::~findMatchingNameDialog()
{
    delete useSurveyNameCheckbox;
    delete useRosterNameCheckbox;
    delete useSurveyEmailCheckbox;
    delete useRosterEmailCheckbox;
}
