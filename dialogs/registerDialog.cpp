#include "registerDialog.h"
#include "gruepr_globals.h"
#include <QGridLayout>
#include <QLabel>
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
    auto *theBox = new QVBoxLayout(this);
    setStyleSheet("background-color: white");

    auto *explanation = new QLabel(this);
    explanation->setStyleSheet(LABEL10PTSTYLE);
    explanation->setText(tr("Thank you for registering your copy of gruepr. "
                            "Doing so enables me to best support the community of educators that uses it. "
                            "This information will never be sold or shared.\n"
                            "\t-Josh\n"
                            "\t " GRUEPRHELPEMAIL "\n"));
    explanation->setWordWrap(true);
    theBox->addWidget(explanation);

    nameLineEdit = new QLineEdit(this);
    nameLineEdit->setStyleSheet(LINEEDITSTYLE);
    nameLineEdit->setPlaceholderText(tr("full name [required]"));
    theBox->addWidget(nameLineEdit);

    institutionLineEdit = new QLineEdit(this);
    institutionLineEdit->setStyleSheet(LINEEDITSTYLE);
    institutionLineEdit->setPlaceholderText(tr("institution [required]"));
    theBox->addWidget(institutionLineEdit);

    emailLineEdit = new QLineEdit(this);
    emailLineEdit->setStyleSheet(LINEEDITSTYLE);
    emailLineEdit->setPlaceholderText(tr("email address [required]"));
    theBox->addWidget(emailLineEdit);
    //force an email address-like input
    //(one or more letters, digits, or special symbols, then '@', then one or more letters, digits, or special symbols, then '.', then 2, 3 or 4 letters)
    static const QRegularExpression emailAddressFormat(EMAILADDRESSREGEX, QRegularExpression::CaseInsensitiveOption);
    emailLineEdit->setValidator(new QRegularExpressionValidator(emailAddressFormat, this));
    connect(emailLineEdit, &QLineEdit::textChanged, this, [this]()
                                                    {emailLineEdit->setStyleSheet(emailLineEdit->hasAcceptableInput()? LINEEDITSTYLE : LINEEDITERRORSTYLE);});

    //a spacer then ok/cancel buttons
    theBox->addSpacing(DIALOG_SPACER_ROWHEIGHT);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    theBox->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(nameLineEdit, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   emailLineEdit->hasAcceptableInput() && !(nameLineEdit->text().isEmpty()) && !(institutionLineEdit->text().isEmpty()));
                                                   name = nameLineEdit->text();});
    connect(institutionLineEdit, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   emailLineEdit->hasAcceptableInput() && !(nameLineEdit->text().isEmpty()) && !(institutionLineEdit->text().isEmpty()));
                                                   institution = institutionLineEdit->text();});
    connect(emailLineEdit, &QLineEdit::textChanged, this, [this]() {buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                                                   emailLineEdit->hasAcceptableInput() && !(nameLineEdit->text().isEmpty()) && !(institutionLineEdit->text().isEmpty()));
                                                   email = emailLineEdit->text();});

    adjustSize();
}

