#include "gatherURMResponsesDialog.h"
#include "gruepr_globals.h"
#include <QHeaderView>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to gather which racial/ethnic/cultural identities should be considered underrepresented
/////////////////////////////////////////////////////////////////////////////////////////////////////////

gatherURMResponsesDialog::gatherURMResponsesDialog(const QStringList &URMResponses, const QStringList &currURMResponsesConsideredUR, QWidget *parent)
    :listTableDialog (tr("Select underrepresented race/ethnicity responses"), true, true, parent)
{
    URMResponsesConsideredUR = currURMResponsesConsideredUR;

    setMinimumSize(SM_DLG_SIZE, SM_DLG_SIZE);

    // Rows 1&2 - explanation and spacer
    explanation = new QLabel(this);
    explanation->setStyleSheet(LABELSTYLE);
    explanation->setText(tr("<html>Students gave the following responses when asked about their racial/ethnic/cultural identity. "
                            "Which of these should be considered underrepresented?<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);
    addSpacerRow(1);

    // In table, a checkbox and a label for each response values
    const int numResponses = int(URMResponses.size());
    theTable->setRowCount(numResponses);
    int widthCol0 = 0;
    for(int response = 0; response < numResponses; response++)
    {
        const QString &responseText = URMResponses.at(response);
        enableValue << new QCheckBox(this);
        enableValue.last()->setChecked(URMResponsesConsideredUR.contains(responseText));
        enableValue.last()->setStyleSheet(QString(CHECKBOXSTYLE).replace("QCheckBox {", "QCheckBox {Text-align:center; margin-left:10%; margin-right:10%;"));
        theTable->setCellWidget(response, 0, enableValue.last());
        widthCol0 = std::max(widthCol0, enableValue.last()->width());
        responses << new QPushButton(this);
        responses.last()->setText(responseText);
        responses.last()->setFlat(true);
        responses.last()->setStyleSheet(SMALLBUTTONSTYLETRANSPARENTFLAT);
        theTable->setCellWidget(response, 1, responses.last());
        connect(responses.last(), &QPushButton::clicked, enableValue.last(), &QCheckBox::toggle);
        connect(enableValue.last(), &QCheckBox::stateChanged, this, [this, response](int state){
                                                                                 if(state == Qt::Checked) {
                                                                                    URMResponsesConsideredUR << responses[response]->text();
                                                                                    responses.last()->setStyleSheet("Text-align:left;font-weight: bold;");
                                                                                 }
                                                                                 else {
                                                                                    URMResponsesConsideredUR.removeAll(responses[response]->text());
                                                                                    responses.last()->setStyleSheet("Text-align:left;");
                                                                                 }
                                                                                 });
    }
    theTable->horizontalHeader()->resizeSection(0, int(float(widthCol0) * TABLECOLUMN0OVERWIDTH));
    theTable->resizeColumnsToContents();
    theTable->resizeRowsToContents();
    theTable->adjustSize();

    adjustSize();
}
