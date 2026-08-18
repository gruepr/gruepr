#include "whichFilesDialog.h"
#include "ui_whichFilesDialog.h"
#include "gruepr_globals.h"
#include <QGridLayout>
#include <QMenu>
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
    // Keep the radio-button column pinned to its natural (small) width regardless of how much the
    // content column's checkbox panel shrinks when switching between Spreadsheet/Custom/Student/Instructor.
    if(auto *grid = qobject_cast<QGridLayout*>(layout())) {
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
    }

    // enable correct set of custom options checkboxes and connect each to output struct
    connect(ui->fileDatacheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeFileData = checked;});
    connect(ui->teamingDatacheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeTeamingData = checked;});
    connect(ui->teamScorecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeTeamScore = checked;});
    ui->assignmentcheckBox->setVisible(!dataOptions->assignmentPreferenceFields.empty());
    connect(ui->assignmentcheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeTeamAssignment = checked;});
    ui->assignmentPreferencescheckBox->setVisible(!dataOptions->assignmentPreferenceFields.empty());
    connect(ui->assignmentPreferencescheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeAssignmentPreferences = checked;});
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
    const bool urmIdentity = dataOptions->URMIdentityIncluded;
    ui->URMIdentitycheckBox->setVisible(urmIdentity);
    connect(ui->URMIdentitycheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeURMIdentity = checked;});
    // Section is meaningful in a Spreadsheet/Custom export whenever a single teamset/tab can mix
    // students from more than one section -- true both when all sections were teamed together, and
    // when they were teamed separately (that still lands in one combined tab; see TeamsTabItem).
    const bool sect = dataOptions->sectionIncluded &&
                       (sectionType == TeamingOptions::SectionType::allTogether ||
                        sectionType == TeamingOptions::SectionType::allSeparately);
    ui->sectioncheckBox->setVisible(sect);
    connect(ui->sectioncheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeSect = checked;});
    const bool sectionSeparate = (sectionType == TeamingOptions::SectionType::allSeparately);
    const int numAttributes = dataOptions->numAttributes;
    const bool anyAttribute = (numAttributes > 0);
    const int NUM_COLUMNS = 3;
    const int firstAttributeRow = 4; // rows 0-1: name/gender/etc, row 2: assignment preferences, row 3: attributeLabel
    for(int attrib = 0; attrib < numAttributes; attrib++) {
        auto *checkBox = new QCheckBox(tr("Attribute Q") + QString::number(attrib + 1), this);
        checkBox->setToolTip(dataOptions->attributeQuestionText.at(attrib));
        ui->demographicGridLayout->addWidget(checkBox, firstAttributeRow + (attrib / NUM_COLUMNS), attrib % NUM_COLUMNS);
        customFileOptions.includeAttribute << false;
        connect(checkBox, &QCheckBox::toggled, this, [this, attrib](bool checked) {
            customFileOptions.includeAttribute[attrib] = checked;
        });
    }
    const bool sched = !dataOptions->dayNames.isEmpty();
    ui->schedulecheckBox->setVisible(sched);
    connect(ui->schedulecheckBox, &QCheckBox::toggled, this, [this](bool checked){customFileOptions.includeSchedule = checked;});

    const bool assignmentAvailable = !dataOptions->assignmentPreferenceFields.empty();
    ui->attributeLabel->setVisible(anyAttribute);
    ui->teammateGroupBox->setVisible(first || last || email || gender || urmIdentity || sect || anyAttribute || assignmentAvailable);
    ui->CustomFileContentsBox->hide();

    // The checkbox panel is shared by Custom (full set) and Spreadsheet (reduced set): Spreadsheet
    // drops file metadata, teaming options, and the whole "Each Team" group (team score, team outcome,
    // and schedule don't fit a flat one-row-per-student table). "Each Teammate" -- including per-student
    // assignment preferences -- stays available in both, gated only by whether that data exists
    // (already handled above/below, not by which radio is selected).
    auto showCustomFileContentsBox = [this](bool isSpreadsheet) {
        ui->CustomFileContentsBox->setVisible(true);
        ui->fileDatacheckBox->setVisible(!isSpreadsheet);
        ui->teamingDatacheckBox->setVisible(!isSpreadsheet);
        ui->teamGroupBox->setVisible(!isSpreadsheet);
        // Force the layout to recompute its size hint synchronously (rather than waiting for the next
        // posted LayoutRequest event) -- CustomFileContentsBox is becoming visible for the first time
        // in the same call where some of its children are being hidden, and without this, adjustSize()
        // (called right after, by the caller) can read a stale/oversized hint from before the hides land.
        ui->CustomFileContentsBox->layout()->activate();
        layout()->activate();
    };

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
        ui->CustomFileContentsBox->hide();
        rebuildSaveFormatMenu();
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
        ui->CustomFileContentsBox->hide();
        rebuildSaveFormatMenu();
        adjustSize();
    });
    if(previews.size() > 1) {
        ui->instructorFileRadioButton->setToolTip(previews.at(1));
        ui->instructorFilePushButton->setToolTip(previews.at(1));
    }

    ui->spreadsheetFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->spreadsheetFilePushButton, &QPushButton::clicked, ui->spreadsheetFileRadioButton, &QRadioButton::animateClick);
    connect(ui->spreadsheetFileRadioButton, &QRadioButton::toggled, this,
            [this, showCustomFileContentsBox, first, last, email, sect, sectionSeparate](bool checked){
        fileType = FileType::spreadsheet;
        if(checked) {
            showCustomFileContentsBox(true);
            // The first time Spreadsheet is selected, default-check the fields that make a plain
            // one-row-per-student table useful on its own; leave it alone after that so later
            // switches back to Spreadsheet don't fight the user's own unchecks.
            if(!appliedSpreadsheetDefaults) {
                appliedSpreadsheetDefaults = true;
                if(first) { ui->firstnamecheckBox->setChecked(true); }
                if(last) { ui->lastnamecheckBox->setChecked(true); }
                if(email) { ui->emailcheckBox->setChecked(true); }
                if(sect && sectionSeparate) { ui->sectioncheckBox->setChecked(true); }
            }
        }
        else {
            ui->CustomFileContentsBox->hide();
        }
        rebuildSaveFormatMenu();
        adjustSize();
    });
    if(previews.size() > 2) {
        ui->spreadsheetFileRadioButton->setToolTip(previews.at(2));
        ui->spreadsheetFilePushButton->setToolTip(previews.at(2));
    }

    ui->customFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->customFilePushButton, &QPushButton::clicked, ui->customFileRadioButton, &QRadioButton::animateClick);
    connect(ui->customFileRadioButton, &QRadioButton::toggled, this, [this, showCustomFileContentsBox](bool checked){
        fileType = FileType::custom;
        if(checked) {
            showCustomFileContentsBox(false);
        }
        else {
            ui->CustomFileContentsBox->hide();
        }
        rebuildSaveFormatMenu();
        adjustSize();
    });
    if(previews.size() > 3) {
        ui->customFileRadioButton->setToolTip(previews.at(3));
        ui->customFilePushButton->setToolTip(previews.at(3));
    }

    action = saveOrPrint;
    if(saveDialog) {
        ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
        rebuildSaveFormatMenu();
    }
    else {
        ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Print"));
    }
    ui->buttonBox->button(QDialogButtonBox::Save)->setStyleSheet(SMALLBUTTONSTYLE);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
}


WhichFilesDialog::~WhichFilesDialog()
{
    delete ui;
}


void WhichFilesDialog::rebuildSaveFormatMenu()
{
    if(action != Action::save) {
        // Print mode has no format choice -- just the plain "Print" button, no menu.
        return;
    }

    saveFormatMenu = new QMenu(this);
    auto addFormatAction = [this](const QString &label, SaveFormat format) {
        QAction *formatAction = saveFormatMenu->addAction(label);
        connect(formatAction, &QAction::triggered, this, [this, format](){
            saveFormat = format;
            accept();
        });
    };
    addFormatAction(tr("Save as PDF"), SaveFormat::pdf);
    addFormatAction(tr("Save as Text"), SaveFormat::text);
    if(fileType == FileType::spreadsheet) {
        addFormatAction(tr("Save as CSV"), SaveFormat::csv);
        addFormatAction(tr("Save as Excel"), SaveFormat::xlsx);
    }

    auto *saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    QMenu *oldMenu = saveButton->menu();
    saveButton->setMenu(saveFormatMenu);
    delete oldMenu;
}
