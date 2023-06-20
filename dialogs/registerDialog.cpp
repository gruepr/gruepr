#include "registerDialog.h"
#include "gruepr_globals.h"
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to register the software
/////////////////////////////////////////////////////////////////////////////////////////////////////////

registerDialog::registerDialog(QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Register your copy of gruepr"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theBox = new QVBoxLayout(this);
    setStyleSheet("background-color: white");

    explanation = new QLabel(this);
    explanation->setStyleSheet(LABELSTYLE);
    explanation->setText(tr("Thank you for registering your copy of gruepr. "
                            "Doing so enables me to best support the community of educators that uses it. "
                            "This information will never be sold or shared.\n"
                            "\t-Josh\n"
                            "\t info@gruepr.com\n"));
    explanation->setWordWrap(true);
    theBox->addWidget(explanation);

    name = new QLineEdit(this);
    name->setStyleSheet(LINEEDITSTYLE);
    name->setPlaceholderText(tr("full name [required]"));
    theBox->addWidget(name);

    institution = new QLineEdit(this);
    institution->setStyleSheet(LINEEDITSTYLE);
    institution->setPlaceholderText(tr("institution [required]"));
    theBox->addWidget(institution);

    email = new QLineEdit(this);
    email->setStyleSheet(LINEEDITSTYLE);
    email->setPlaceholderText(tr("email address [required]"));
    theBox->addWidget(email);
    //force an email address-like input
    //(one or more letters, digits, or special symbols, then '@', then one or more letters, digits, or special symbols, then '.', then 2, 3 or 4 letters)
    QRegularExpression emailAddressFormat(EMAILADDRESSREGEX, QRegularExpression::CaseInsensitiveOption);
    email->setValidator(new QRegularExpressionValidator(emailAddressFormat, this));
    connect(email, &QLineEdit::textChanged, this, [this]()
                                             {QString stylesheet = (email->hasAcceptableInput())? LINEEDITSTYLE : LINEEDITERRORSTYLE;
                                              email->setStyleSheet(stylesheet); });

    //a spacer then ok/cancel buttons
    theBox->addSpacing(DIALOG_SPACER_ROWHEIGHT);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->setStyleSheet("background-color: #" BUBBLYHEX ";");
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLE);
    theBox->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(name, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(institution, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});
    connect(email, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   email->hasAcceptableInput() && !(name->text().isEmpty()) && !(institution->text().isEmpty()));});

    adjustSize();
}

