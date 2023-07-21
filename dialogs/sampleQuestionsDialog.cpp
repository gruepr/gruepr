#include "sampleQuestionsDialog.h"
#include "ui_sampleQuestionsDialog.h"
#include "gruepr_globals.h"
#include <QPushButton>
#include <QTabBar>

SampleQuestionsDialog::SampleQuestionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SampleQuestionsDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    ui->tabWidget->tabBar()->setDocumentMode(true);
    ui->tabWidget->tabBar()->setExpanding(true);
    ui->tabWidget->setStyleSheet(QString() + TABWIDGETSTYLE + CHECKBOXSTYLE + RADIOBUTTONSTYLE + LABELSTYLE);
    ui->buttonBox->button(QDialogButtonBox::Close)->setStyleSheet(SMALLBUTTONSTYLE);
}

SampleQuestionsDialog::~SampleQuestionsDialog()
{
    delete ui;
}
