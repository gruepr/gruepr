#include "gatherURMResponsesDialog.h"
#include "gruepr_globals.h"
#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which racial/ethnic/cultural identities should be considered underrepresented
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherURMResponsesDialog::gatherURMResponsesDialog(const QStringList &URMResponses, const QStringList &currURMResponsesConsideredUR, QWidget *parent)
    :listTableDialog (tr("Select underrepresented race/ethnicity responses"), true, true, parent)
{
    URMResponsesConsideredUR = currURMResponsesConsideredUR;

    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    // Rows 1&2 - explanation and spacer
    auto *explanation = new QLabel(this);
    explanation->setStyleSheet(LABEL10PTSTYLE);
    explanation->setText(tr("<html>Students gave the following responses when asked about their racial/ethnic/cultural identity. "
                            "Which of these should be considered underrepresented?<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);
    addSpacerRow(1);

    // In table, a checkbox and a label for each response values
    const int numResponses = int(URMResponses.size());
    theTable->setRowCount(numResponses);
    int widthCol0 = 0, rowHeight = 0;
    QList<QCheckBox*> enableValue;
    enableValue.reserve(numResponses);
    responses.reserve(numResponses);
    for(int response = 0; response < numResponses; response++) {
        const QString &responseText = URMResponses.at(response);
        enableValue << new QCheckBox(this);
        enableValue.last()->setChecked(URMResponsesConsideredUR.contains(responseText));
        enableValue.last()->setStyleSheet(CHECKBOXSTYLE);
        theTable->setCellWidget(response, 0, enableValue.last());
        widthCol0 = std::max(widthCol0, enableValue.last()->width());
        responses << new QPushButton(this);
        responses.last()->setText(responseText);
        responses.last()->setFlat(true);
        responses.last()->setStyleSheet(URMResponsesConsideredUR.contains(responseText)?
                                        QString(SMALLBUTTONSTYLETRANSPARENTFLAT).replace("12pt", "12pt; font-weight: bold") : SMALLBUTTONSTYLETRANSPARENTFLAT);
        theTable->setCellWidget(response, 1, responses.last());
        rowHeight = std::max(rowHeight, std::max(enableValue.last()->height(), responses.last()->height()));
        connect(responses.last(), &QPushButton::clicked, enableValue.last(), &QCheckBox::toggle);
        connect(enableValue.last(), &QCheckBox::checkStateChanged, this, [this, thisResponse=responses.last()](Qt::CheckState state){
                                                                                 if(state == Qt::CheckState::Checked) {
                                                                                    URMResponsesConsideredUR << thisResponse->text();
                                                                                     thisResponse->setStyleSheet(QString(SMALLBUTTONSTYLETRANSPARENTFLAT)
                                                                                                                    .replace("12pt", "12pt; font-weight: bold"));
                                                                                 }
                                                                                 else {
                                                                                    URMResponsesConsideredUR.removeAll(thisResponse->text());
                                                                                    thisResponse->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
                                                                                 }
                                                                                 });
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLEOVERSIZE));
    for(int response = 0; response < numResponses; response++) {
        theTable->verticalHeader()->resizeSection(response, int(float(rowHeight) * TABLEOVERSIZE));
    }
    theTable->adjustSize();

    adjustSize();
}
