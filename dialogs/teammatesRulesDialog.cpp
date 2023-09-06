#include "teammatesRulesDialog.h"
#include "ui_TeammatesRulesDialog.h"
#include "gruepr_globals.h"

TeammatesRulesDialog::TeammatesRulesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TeammatesRulesDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Teammate rules"));
    ui->tabWidget->tabBar()->setExpanding(true);
    ui->tabWidget->setStyleSheet(QString() + TABWIDGETSTYLE + LABELSTYLE);
    auto scrollAreas = {ui->requiredScrollArea, ui->preventedScrollArea, ui->requestedScrollArea};
    auto scrollAreaWidgets = {ui->requiredScrollAreaWidget, ui->preventedScrollAreaWidget, ui->requestedScrollAreaWidget};
    auto addFrames = {ui->required_addFrame, ui->prevented_addFrame, ui->requested_addFrame};
    auto loadButtons = {ui->required_loadButton, ui->prevented_loadButton, ui->requested_loadButton};
    for(auto &scrollArea : scrollAreas) {
        scrollArea->setStyleSheet(QString() + "QScrollArea{background-color: " TRANSPARENT "; color: " DEEPWATERHEX ";}" + SCROLLBARSTYLE);
    }
    for(auto &scrollAreaWidget : scrollAreaWidgets) {
        scrollAreaWidget->setStyleSheet("background-color: " TRANSPARENT "; color: " TRANSPARENT ";");
    }
    for(auto &addFrame : addFrames) {
        addFrame->setStyleSheet(QString() +
                                "QFrame{background-color: " BUBBLYHEX "; color: " DEEPWATERHEX "; border: 1px solid; border-color: " AQUAHEX ";}" +
                                COMBOBOXSTYLE);
    }
    for(auto &loadButton : loadButtons) {
        loadButton->setStyleSheet(SMALLTOOLBUTTONSTYLEINVERTED);
    }
    ui->line->setStyleSheet("border-color: " AQUAHEX);
    ui->line->setFixedHeight(1);

    auto *resetValuesButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    resetValuesButton->setText(tr("Clear all rules"));
    resetValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    auto *restoreValuesButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    restoreValuesButton->setText(tr("Save to csv file"));
    restoreValuesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    connect(resetValuesButton, &QPushButton::clicked, this, &TeammatesRulesDialog::clearAllValues);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TeammatesRulesDialog::Ok);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TeammatesRulesDialog::~TeammatesRulesDialog()
{
    delete ui;
}

void TeammatesRulesDialog::clearAllValues()
{

}

void TeammatesRulesDialog::Ok()
{

}
