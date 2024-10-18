#include "whichFilesDialog.h"
#include "gruepr_globals.h"
#include <QPushButton>
#include <QToolTip>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to choose which item(s) to save or print
/////////////////////////////////////////////////////////////////////////////////////////////////////////

whichFilesDialog::whichFilesDialog(const Action saveOrPrint, const QStringList &previews, QWidget *parent)
    :QDialog (parent)
{
    const bool saveDialog = (saveOrPrint == whichFilesDialog::Action::save);
    const QString saveOrPrintString = (saveDialog? tr("save") : tr("print"));

    //Set up window with a grid layout
    setWindowTitle(tr("Choose files to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);
    auto *theGrid = new QGridLayout(this);
    const int totNumOptionAreaRows = 6; // 1 each for the text/pdf labels, the student, the instructor & the spreadsheet options, and horiz. lines between
    int row = 0;
    previousToolTipFont = QToolTip::font();
    QToolTip::setFont(QFont("Oxygen Mono", previousToolTipFont.pointSize()));

    auto *explanation = new QLabel(this);
    explanation->setStyleSheet(LABEL10PTSTYLE);
    explanation->setTextFormat(Qt::RichText);
    explanation->setText(tr("<br>You can ") + saveOrPrintString + tr(" the following files:<br>Preview a file by hovering over the title.<br>"));
    theGrid->addWidget(explanation, row++, 0, 1, -1);

    if(saveDialog) {
        auto *textfile = new QLabel(this);
        textfile->setStyleSheet(LABEL10PTSTYLE);
        textfile->setText(tr("text"));
        theGrid->addWidget(textfile, 1, 0);
        auto *pdftxtline = new QFrame(this);
        pdftxtline->setStyleSheet("border-color: " AQUAHEX);
        pdftxtline->setFrameShape(QFrame::VLine);
        pdftxtline->setFrameShadow(QFrame::Plain);
        pdftxtline->setLineWidth(1);
        pdftxtline->setFixedHeight(1);
        theGrid->addWidget(pdftxtline, row, 1, totNumOptionAreaRows, 1);
        auto *pdffile = new QLabel(this);
        pdffile->setStyleSheet(LABEL10PTSTYLE);
        pdffile->setText("pdf");
        theGrid->addWidget(pdffile, row++, 2);
    }

    studentFiletxtCheckBox = new QCheckBox(this);
    studentFiletxtCheckBox->setStyleSheet(CHECKBOXSTYLE);
    studentFiletxtCheckBox->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(studentFiletxtCheckBox, row, 0);
    connect(studentFiletxtCheckBox, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    connect(studentFiletxtCheckBox, &QCheckBox::toggled, this, [this](bool checked){studentFiletxt = checked;});

    auto *studentFileLabel = new QPushButton(this);
    studentFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(studentFileLabel, &QPushButton::clicked, studentFiletxtCheckBox, &QCheckBox::animateClick);
    studentFileLabel->setText(tr("Student's file:\nnames, email addresses, and team availability schedules."));
    theGrid->addWidget(studentFileLabel, row, 3);
    if(saveDialog) {
        studentFilepdfCheckBox = new QCheckBox(this);
        studentFilepdfCheckBox->setStyleSheet(CHECKBOXSTYLE);
        studentFilepdfCheckBox->resize(CHECKBOXSIZE,CHECKBOXSIZE);
        theGrid->addWidget(studentFilepdfCheckBox, row, 2);
        connect(studentFilepdfCheckBox, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(studentFilepdfCheckBox, &QCheckBox::toggled, this, [this](bool checked){studentFilepdf = checked;});
        connect(studentFileLabel, &QPushButton::clicked, studentFilepdfCheckBox, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty())) {
        studentFiletxtCheckBox->setToolTip(previews.at(0));
        if(saveDialog) {
            studentFilepdfCheckBox->setToolTip(previews.at(0));
        }
        studentFileLabel->setToolTip(previews.at(0));
    }

    row++;
    auto *belowStudentLine = new QFrame(this);
    belowStudentLine->setStyleSheet("border-color: " AQUAHEX);
    belowStudentLine->setFrameShape(QFrame::HLine);
    belowStudentLine->setFrameShadow(QFrame::Plain);
    belowStudentLine->setLineWidth(1);
    belowStudentLine->setFixedHeight(1);

    theGrid->addWidget(belowStudentLine, row++, 0, 1, -1);

    instructorFiletxtCheckBox = new QCheckBox(this);
    instructorFiletxtCheckBox->setStyleSheet(CHECKBOXSTYLE);
    instructorFiletxtCheckBox->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(instructorFiletxtCheckBox, row, 0);
    connect(instructorFiletxtCheckBox, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    connect(instructorFiletxtCheckBox, &QCheckBox::toggled, this, [this](bool checked){instructorFiletxt = checked;});
    auto *instructorFileLabel = new QPushButton(this);
    instructorFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(instructorFileLabel, &QPushButton::clicked, instructorFiletxtCheckBox, &QCheckBox::animateClick);
    instructorFileLabel->setText(tr("Instructor's file:\nFile data, teaming options, optimization data,\n"
                                    "names, email addresses, demographic and attribute data, and team availability schedule."));
    theGrid->addWidget(instructorFileLabel, row, 3);
    if(saveDialog) {
        instructorFilepdfCheckBox = new QCheckBox(this);
        instructorFilepdfCheckBox->setStyleSheet(CHECKBOXSTYLE);
        instructorFilepdfCheckBox->resize(CHECKBOXSIZE,CHECKBOXSIZE);
        theGrid->addWidget(instructorFilepdfCheckBox, row, 2);
        connect(instructorFilepdfCheckBox, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(instructorFilepdfCheckBox, &QCheckBox::toggled, this, [this](bool checked){instructorFilepdf = checked;});
        connect(instructorFileLabel, &QPushButton::clicked, instructorFilepdfCheckBox, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty())) {
        instructorFiletxtCheckBox->setToolTip(previews.at(1));
        if(saveDialog) {
            instructorFilepdfCheckBox->setToolTip(previews.at(1));
        }
        instructorFileLabel->setToolTip(previews.at(1));
    }

    row++;
    auto *belowInstructorLine = new QFrame(this);
    belowInstructorLine->setStyleSheet("border-color: " AQUAHEX);
    belowInstructorLine->setFrameShape(QFrame::HLine);
    belowInstructorLine->setFrameShadow(QFrame::Plain);
    belowInstructorLine->setLineWidth(1);
    belowInstructorLine->setFixedHeight(1);
    theGrid->addWidget(belowInstructorLine, row++, 0, 1, -1);

    spreadsheetFiletxtCheckBox = new QCheckBox(this);
    spreadsheetFiletxtCheckBox->setStyleSheet(CHECKBOXSTYLE);
    spreadsheetFiletxtCheckBox->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(spreadsheetFiletxtCheckBox, row, 0);
    connect(spreadsheetFiletxtCheckBox, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    connect(spreadsheetFiletxtCheckBox, &QCheckBox::toggled, this, [this](bool checked){spreadsheetFiletxt = checked;});
    auto *spreadsheetFileLabel = new QPushButton(this);
    spreadsheetFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(spreadsheetFileLabel, &QPushButton::clicked, spreadsheetFiletxtCheckBox, &QCheckBox::animateClick);
    spreadsheetFileLabel->setText(tr("Spreadsheet file:\nsections, teams, names, and email addresses in a tabular format."));
    theGrid->addWidget(spreadsheetFileLabel, row++, 3);
    if(!(previews.isEmpty())) {
        spreadsheetFiletxtCheckBox->setToolTip(previews.at(2));
        spreadsheetFileLabel->setToolTip(previews.at(2));
    }

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    if(saveDialog) {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Save"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Print"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    //a spacer then ok/cancel buttons
    theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
    theGrid->addWidget(buttonBox, row, 0, -1, -1);

    adjustSize();
}


whichFilesDialog::~whichFilesDialog()
{
    QToolTip::setFont(previousToolTipFont);
}


void whichFilesDialog::boxToggled()
{
    bool somethingClicked = studentFiletxtCheckBox->isChecked() || instructorFiletxtCheckBox->isChecked() || spreadsheetFiletxtCheckBox->isChecked();
    if(studentFilepdfCheckBox != nullptr) {
        somethingClicked = somethingClicked || studentFilepdfCheckBox->isChecked();
    }
    if(instructorFilepdfCheckBox != nullptr) {
        somethingClicked = somethingClicked || instructorFilepdfCheckBox->isChecked();
    }
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(somethingClicked);
}
