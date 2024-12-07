#include "editSectionNamesDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to edit the Section names
/////////////////////////////////////////////////////////////////////////////////////////////////////////

editSectionNamesDialog::editSectionNamesDialog(const QStringList &incomingSectionNames, QWidget *parent)
    :listTableDialog (tr("Edit section names"), true, true, parent), sectionNames(incomingSectionNames), originalSectionNames(incomingSectionNames)
{
    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    //Table of team names
    const int numSections = incomingSectionNames.size();
    theTable->setRowCount(numSections);
    int widthCol0 = 0, rowHeight = 0;
    sectionNameLineEdits.reserve(numSections);
    int section = 0;
    for(const auto &sectionName : incomingSectionNames) {
        auto *label = new QLabel(QString::number(section + 1), this);
        label->setStyleSheet(LABEL10PTSTYLE);
        theTable->setCellWidget(section, 0, label);
        widthCol0 = std::max(widthCol0, label->width());
        sectionNameLineEdits << new QLineEdit(this);
        sectionNameLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
        sectionNameLineEdits.last()->setPlaceholderText(sectionName);
        connect(sectionNameLineEdits.last(), &QLineEdit::textChanged, this, [this, numSections](){
            sectionNames.clear();
            for(int i = 0; i < numSections; i++) {
                sectionNames << (sectionNameLineEdits[i]->text().isEmpty() ? originalSectionNames.at(i) : sectionNameLineEdits[i]->text());
            }
        });
        theTable->setCellWidget(section, 1, sectionNameLineEdits.last());
        rowHeight = std::max(rowHeight, std::max(label->height(), sectionNameLineEdits.last()->height()));
        section++;
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLEOVERSIZE));
    for(int i = 0; i < numSections; i++) {
        theTable->verticalHeader()->resizeSection(i, int(float(rowHeight) * TABLEOVERSIZE));
    }
    theTable->adjustSize();

    //A reset table button
    auto *resetNamesButton = new QPushButton(tr("Reset All"), this);
    resetNamesButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(resetNamesButton, &QPushButton::clicked, this, &editSectionNamesDialog::clearAllNames);
    addButton(resetNamesButton);

    adjustSize();
}


void editSectionNamesDialog::clearAllNames()
{
    for(auto &sectionNameLineEdit : sectionNameLineEdits) {
        sectionNameLineEdit->clear();
    }
}
