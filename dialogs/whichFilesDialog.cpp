#include "whichFilesDialog.h"
#include "ui_whichFilesDialog.h"
#include "gruepr_globals.h"
#include <QPushButton>
#include <QToolTip>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose which item(s) to save or print
/////////////////////////////////////////////////////////////////////////////////////////////////////////

WhichFilesDialog::WhichFilesDialog(const Action saveOrPrint, const DataOptions *const dataOptions, const QStringList &previews, QWidget *parent)
    :QDialog (parent),
    ui(new Ui::WhichFilesDialog)
{
    ui->setupUi(this);
    const bool saveDialog = (saveOrPrint == WhichFilesDialog::Action::save);
    const QString saveOrPrintString = (saveDialog? tr("save") : tr("print"));

    //Set up window with a grid layout
    setWindowTitle(tr("Choose files to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);
    setStyleSheet(QString(RADIOBUTTONSTYLE) + CHECKBOXSTYLE + LABEL10PTSTYLE + GROUPSTYLE);
    previousToolTipFont = QToolTip::font();
    QToolTip::setFont(QFont("Oxygen Mono", previousToolTipFont.pointSize()));

    // enable correct set of custom options
    const bool first = dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT;
    ui->firstnamecheckBox->setVisible(first);
    const bool last = dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT;
    ui->lastnamecheckBox->setVisible(last);
    const bool email = dataOptions->emailField != DataOptions::FIELDNOTPRESENT;
    ui->emailcheckBox->setVisible(email);
    const bool gender = dataOptions->genderIncluded;
    ui->gendercheckBox->setVisible(gender);
    const bool urm = dataOptions->URMIncluded;
    ui->URMcheckBox->setVisible(urm);
    const bool sect = dataOptions->sectionIncluded;
    ui->sectioncheckBox->setVisible(sect);
    QList<QCheckBox*> multichoiceCheckboxes = {ui->Q1checkBox, ui->Q2checkBox, ui->Q3checkBox, ui->Q4checkBox, ui->Q5checkBox, ui->Q6checkBox, ui->Q7checkBox,
                                               ui->Q8checkBox, ui->Q9checkBox, ui->Q10checkBox, ui->Q11checkBox, ui->Q12checkBox, ui->Q13checkBox, ui->Q14checkBox};
    bool anyMultiChoice = false;
    for(int attrib = 0; attrib < multichoiceCheckboxes.size(); attrib++) {
        const bool thisMultiChoice = dataOptions->attributeField[attrib] != DataOptions::FIELDNOTPRESENT;
        multichoiceCheckboxes[attrib]->setVisible(thisMultiChoice);
        anyMultiChoice = anyMultiChoice || thisMultiChoice;
    }
    const bool timezone = dataOptions->timezoneIncluded;
    ui->timezonecheckBox->setVisible(timezone);
    ui->multipleChoiceLabel->setVisible(anyMultiChoice || timezone);
    ui->teammateGroupBox->setVisible(first || last || email || gender || urm || sect || anyMultiChoice || timezone);
    const bool sched = !dataOptions->dayNames.isEmpty();
    ui->schedulecheckBox->setVisible(sched);
    ui->CustomFileContentsBox->hide();

    ui->studentFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->studentFilePushButton, &QPushButton::clicked, ui->studentFileRadioButton, &QRadioButton::animateClick);
    connect(ui->studentFileRadioButton, &QRadioButton::clicked, [this](){
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(!(previews.isEmpty())) {
        ui->studentFileRadioButton->setToolTip(previews.at(0));
        ui->studentFilePushButton->setToolTip(previews.at(0));
    }

    ui->instructorFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->instructorFilePushButton, &QPushButton::clicked, ui->instructorFileRadioButton, &QRadioButton::animateClick);
    connect(ui->instructorFileRadioButton, &QRadioButton::clicked, [this](){
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(previews.size() > 1) {
        ui->instructorFileRadioButton->setToolTip(previews.at(1));
        ui->instructorFilePushButton->setToolTip(previews.at(1));
    }

    ui->spreadsheetFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->spreadsheetFilePushButton, &QPushButton::clicked, ui->spreadsheetFileRadioButton, &QRadioButton::animateClick);
    connect(ui->spreadsheetFileRadioButton, &QRadioButton::clicked, [this](){
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
        adjustSize();
    });
    if(previews.size() > 2) {
        ui->spreadsheetFileRadioButton->setToolTip(previews.at(2));
        ui->spreadsheetFilePushButton->setToolTip(previews.at(2));
    }

    ui->customFilePushButton->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(ui->customFilePushButton, &QPushButton::clicked, ui->customFileRadioButton, &QRadioButton::animateClick);
    connect(ui->customFileRadioButton, &QRadioButton::clicked, [this](){
        ui->CustomFileContentsBox->setVisible(ui->customFileRadioButton->isChecked());
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
    QToolTip::setFont(previousToolTipFont);
    delete ui;
}
