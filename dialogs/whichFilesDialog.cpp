#include "whichFilesDialog.h"
#include "ui_whichFilesDialog.h"
#include "gruepr_globals.h"
#include <QPushButton>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose which item(s) to save or print
/////////////////////////////////////////////////////////////////////////////////////////////////////////

WhichFilesDialog::WhichFilesDialog(const Action saveOrPrint, const DataOptions *const dataOptions, const TeamingOptions::SectionType sectionType,
                                   const QStringList &previews, QWidget *parent)
    :QDialog (parent),
    ui(new Ui::WhichFilesDialog)
{
    ui->setupUi(this);
    const bool saveDialog = (saveOrPrint == WhichFilesDialog::Action::save);
    const QString saveOrPrintString = (saveDialog? tr("save") : tr("print"));

    //Set up window with a grid layout
    setWindowTitle(tr("Choose file to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);
    setStyleSheet(QString(RADIOBUTTONSTYLE) + CHECKBOXSTYLE + LABEL10PTSTYLE + GROUPSTYLE + MONOTOOLTIPSTYLE);

    // enable correct set of custom options checkboxes and connect each to output struct
    connect(ui->fileDatacheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeFileData = checked;});
    connect(ui->teamingDatacheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeTeamingData = checked;});
    connect(ui->teamScorecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeTeamScore = checked;});
    const bool first = dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT;
    ui->firstnamecheckBox->setVisible(first);
    connect(ui->firstnamecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeFirstName = checked;});
    const bool last = dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT;
    ui->lastnamecheckBox->setVisible(last);
    connect(ui->lastnamecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeLastName = checked;});
    const bool email = dataOptions->emailField != DataOptions::FIELDNOTPRESENT;
    ui->emailcheckBox->setVisible(email);
    connect(ui->emailcheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeEmail = checked;});
    const bool gender = dataOptions->genderIncluded;
    ui->gendercheckBox->setVisible(gender);
    connect(ui->gendercheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeGender = checked;});
    const bool urm = dataOptions->URMIncluded;
    ui->URMcheckBox->setVisible(urm);
    connect(ui->URMcheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeURM = checked;});
    const bool sect = dataOptions->sectionIncluded && sectionType == TeamingOptions::SectionType::allTogether;
    ui->sectioncheckBox->setVisible(sect);
    connect(ui->sectioncheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeSect = checked;});
    QList<QCheckBox*> multichoiceCheckboxes = {ui->Q1checkBox, ui->Q2checkBox, ui->Q3checkBox, ui->Q4checkBox, ui->Q5checkBox, ui->Q6checkBox,
                                               ui->Q7checkBox, ui->Q8checkBox, ui->Q9checkBox, ui->Q10checkBox, ui->Q11checkBox, ui->Q12checkBox,
                                               ui->Q13checkBox, ui->Q14checkBox, ui->Q15checkBox};
    bool anyMultiChoice = false;
    for(int attrib = 0; attrib < multichoiceCheckboxes.size(); attrib++) {
        const bool thisMultiChoice = dataOptions->attributeField[attrib] != DataOptions::FIELDNOTPRESENT;
        anyMultiChoice = anyMultiChoice || thisMultiChoice;
        multichoiceCheckboxes[attrib]->setVisible(thisMultiChoice);
        customFileOptions.includeMultiChoice << false;
        connect(multichoiceCheckboxes[attrib], &QCheckBox::toggled, this, [this, attrib](bool checked){customFileOptions.includeMultiChoice[attrib] = checked;});
    }
    const bool sched = !dataOptions->dayNames.isEmpty();
    ui->schedulecheckBox->setVisible(sched);
    connect(ui->schedulecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeSchedule = checked;});

    ui->multipleChoiceLabel->setVisible(anyMultiChoice);
    ui->teammateGroupBox->setVisible(first || last || email || gender || urm || sect || anyMultiChoice);
    ui->CustomFileContentsBox->hide();

    ui->studentFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    if((dataOptions->firstNameField == DataOptions::FIELDNOTPRESENT) &&
        (dataOptions->lastNameField == DataOptions::FIELDNOTPRESENT) &&
        (dataOptions->emailField == DataOptions::FIELDNOTPRESENT) &&
        (dataOptions->dayNames.isEmpty())) {
        ui->studentFilePushButton->hide();
        ui->studentFileRadioButton->hide();
    }
    else if((dataOptions->firstNameField == DataOptions::FIELDNOTPRESENT) &&
        (dataOptions->lastNameField == DataOptions::FIELDNOTPRESENT) &&
        (dataOptions->emailField == DataOptions::FIELDNOTPRESENT)) {
        ui->studentFilePushButton->setText(ui->studentFilePushButton->text().remove(tr("the name and email address of each teammate and ")));
    }
    else if((dataOptions->firstNameField == DataOptions::FIELDNOTPRESENT) && (dataOptions->lastNameField == DataOptions::FIELDNOTPRESENT)) {
        ui->studentFilePushButton->setText(ui->studentFilePushButton->text().remove(tr("name and ")));
    }
    else if(dataOptions->emailField == DataOptions::FIELDNOTPRESENT) {
        ui->studentFilePushButton->setText(ui->studentFilePushButton->text().remove(tr("and email address ")));
    }
    if(dataOptions->dayNames.isEmpty()) {
        ui->studentFilePushButton->setText(ui->studentFilePushButton->text().remove(tr("and the team availability schedule")));
    }

    connect(ui->studentFilePushButton, &QPushButton::clicked, ui->studentFileRadioButton, &QRadioButton::animateClick);
    connect(ui->studentFileRadioButton, &QRadioButton::toggled, this, [this](){
        fileType = FileType::student;
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(!(previews.isEmpty())) {
        ui->studentFileRadioButton->setToolTip(previews.at(0));
        ui->studentFilePushButton->setToolTip(previews.at(0));
    }

    ui->instructorFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->instructorFilePushButton, &QPushButton::clicked, ui->instructorFileRadioButton, &QRadioButton::animateClick);
    connect(ui->instructorFileRadioButton, &QRadioButton::toggled, this, [this](){
        fileType = FileType::instructor;
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(previews.size() > 1) {
        ui->instructorFileRadioButton->setToolTip(previews.at(1));
        ui->instructorFilePushButton->setToolTip(previews.at(1));
    }

    ui->spreadsheetFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->spreadsheetFilePushButton, &QPushButton::clicked, ui->spreadsheetFileRadioButton, &QRadioButton::animateClick);
    connect(ui->spreadsheetFileRadioButton, &QRadioButton::toggled, this, [this](){
        fileType = FileType::spreadsheet;
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(previews.size() > 2) {
        ui->spreadsheetFileRadioButton->setToolTip(previews.at(2));
        ui->spreadsheetFilePushButton->setToolTip(previews.at(2));
    }

    ui->customFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->customFilePushButton, &QPushButton::clicked, ui->customFileRadioButton, &QRadioButton::animateClick);
    connect(ui->customFileRadioButton, &QRadioButton::toggled, this, [this](bool checked){
        fileType = FileType::custom;
        ui->CustomFileContentsBox->setVisible(checked);
        adjustSize();
    });
    if(previews.size() > 3) {
        ui->customFileRadioButton->setToolTip(previews.at(3));
        ui->customFilePushButton->setToolTip(previews.at(3));
    }

    if(saveDialog) {
        ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save as text"));
        ui->buttonBox->button(QDialogButtonBox::SaveAll)->setText(tr("Save as pdf"));
        connect(ui->buttonBox->button(QDialogButtonBox::SaveAll), &QPushButton::clicked, this, [this](){pdf = true;});
    }
    else {
        ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Print"));
        ui->buttonBox->button(QDialogButtonBox::SaveAll)->hide();
    }
    ui->buttonBox->button(QDialogButtonBox::Save)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->buttonBox->button(QDialogButtonBox::SaveAll)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


WhichFilesDialog::~WhichFilesDialog()
{
    delete ui;
}
