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
    saveDialog = (saveOrPrint == whichFilesDialog::Action::save);
    QString saveOrPrintString = (saveDialog? tr("save") : tr("print"));

    //Set up window with a grid layout
    setWindowTitle(tr("Choose files to ") + saveOrPrintString);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);
    const int totNumOptionAreaRows = 6; // 1 each for the text/pdf labels, the student, the instructor & the spreadsheet options, and horiz. lines between
    int row = 0;
    previousToolTipFont = QToolTip::font();
    QToolTip::setFont(QFont("Oxygen Mono", previousToolTipFont.pointSize()));

    explanation = new QLabel(this);
    explanation->setStyleSheet(LABELSTYLE);
    explanation->setTextFormat(Qt::RichText);
    explanation->setText(tr("<br>You can ") + saveOrPrintString + tr(" the following files:<br>Preview a file by hovering over the title.<br>"));
    theGrid->addWidget(explanation, row++, 0, 1, -1);

    if(saveDialog) {
        textfile = new QLabel(this);
        textfile->setStyleSheet(LABELSTYLE);
        textfile->setText(tr("text"));
        theGrid->addWidget(textfile, 1, 0);
        auto *pdftxtline = new QFrame(this);
        pdftxtline->setStyleSheet("border-color: " AQUAHEX);
        pdftxtline->setFrameShape(QFrame::VLine);
        pdftxtline->setFrameShadow(QFrame::Plain);
        pdftxtline->setLineWidth(1);
        pdftxtline->setFixedHeight(1);
        theGrid->addWidget(pdftxtline, row, 1, totNumOptionAreaRows, 1);
        pdffile = new QLabel(this);
        pdffile->setStyleSheet(LABELSTYLE);
        pdffile->setText("pdf");
        theGrid->addWidget(pdffile, row++, 2);
    }

    studentFiletxt = new QCheckBox(this);
    studentFiletxt->setStyleSheet(CHECKBOXSTYLE);
    studentFiletxt->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(studentFiletxt, row, 0);
    connect(studentFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);

    studentFileLabel = new QPushButton(this);
    studentFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(studentFileLabel, &QPushButton::clicked, studentFiletxt, &QCheckBox::animateClick);
    studentFileLabel->setText(tr("Student's file:\nnames, email addresses, and team availability schedules."));
    theGrid->addWidget(studentFileLabel, row, 3);
    if(saveDialog) {
        studentFilepdf = new QCheckBox(this);
        studentFilepdf->setStyleSheet(CHECKBOXSTYLE);
        studentFilepdf->resize(CHECKBOXSIZE,CHECKBOXSIZE);
        theGrid->addWidget(studentFilepdf, row, 2);
        connect(studentFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(studentFileLabel, &QPushButton::clicked, studentFilepdf, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty())) {
        studentFiletxt->setToolTip(previews.at(0));
        if(saveDialog) {
            studentFilepdf->setToolTip(previews.at(0));
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

    instructorFiletxt = new QCheckBox(this);
    instructorFiletxt->setStyleSheet(CHECKBOXSTYLE);
    instructorFiletxt->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(instructorFiletxt, row, 0);
    connect(instructorFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    instructorFileLabel = new QPushButton(this);
    instructorFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(instructorFileLabel, &QPushButton::clicked, instructorFiletxt, &QCheckBox::animateClick);
    instructorFileLabel->setText(tr("Instructor's file:\nFile data, teaming options, optimization data,\n"
                                    "names, email addresses, demographic and attribute data, and team availability schedule."));
    theGrid->addWidget(instructorFileLabel, row, 3);
    if(saveDialog) {
        instructorFilepdf = new QCheckBox(this);
        instructorFilepdf->setStyleSheet(CHECKBOXSTYLE);
        instructorFilepdf->resize(CHECKBOXSIZE,CHECKBOXSIZE);
        theGrid->addWidget(instructorFilepdf, row, 2);
        connect(instructorFilepdf, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
        connect(instructorFileLabel, &QPushButton::clicked, instructorFilepdf, &QCheckBox::animateClick);
    }
    if(!(previews.isEmpty())) {
        instructorFiletxt->setToolTip(previews.at(1));
        if(saveDialog) {
            instructorFilepdf->setToolTip(previews.at(1));
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

    spreadsheetFiletxt = new QCheckBox(this);
    spreadsheetFiletxt->setStyleSheet(CHECKBOXSTYLE);
    spreadsheetFiletxt->resize(CHECKBOXSIZE,CHECKBOXSIZE);
    theGrid->addWidget(spreadsheetFiletxt, row, 0);
    connect(spreadsheetFiletxt, &QCheckBox::clicked, this, &whichFilesDialog::boxToggled);
    spreadsheetFileLabel = new QPushButton(this);
    spreadsheetFileLabel->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
    connect(spreadsheetFileLabel, &QPushButton::clicked, spreadsheetFiletxt, &QCheckBox::animateClick);
    spreadsheetFileLabel->setText(tr("Spreadsheet file:\nsections, teams, names, and email addresses in a tabular format."));
    theGrid->addWidget(spreadsheetFileLabel, row++, 3);
    if(!(previews.isEmpty())) {
        spreadsheetFiletxt->setToolTip(previews.at(2));
        spreadsheetFileLabel->setToolTip(previews.at(2));
    }

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    if(saveDialog) {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Save"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
        buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Print"));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    connect(buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button){QDialog::done(buttonBox->standardButton(button));});

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
    bool somethingClicked = studentFiletxt->isChecked() || instructorFiletxt->isChecked() || spreadsheetFiletxt->isChecked();
    if(saveDialog) {
        somethingClicked = somethingClicked || studentFilepdf->isChecked() || instructorFilepdf->isChecked();
    }
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(somethingClicked);
}
