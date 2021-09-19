#include "gatherURMResponsesDialog.h"
#include "gruepr_consts.h"
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
    explanation->setText(tr("<html>Students gave the following responses when asked about their racial/ethnic/cultural identity. "
                            "Which of these should be considered underrepresented?<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);
    addSpacerRow(1);

    // In table, a checkbox and a label for each response values
    const int numResponses = URMResponses.size();
    enableValue = new QCheckBox[numResponses];
    responses = new QPushButton[numResponses];
    theTable->setRowCount(numResponses);
    int widthCol0 = 0;
    for(int response = 0; response < numResponses; response++)
    {
        const QString &responseText = URMResponses.at(response);
        enableValue[response].setChecked(URMResponsesConsideredUR.contains(responseText));
        enableValue[response].setStyleSheet("Text-align:center; margin-left:10%; margin-right:10%;");
        theTable->setCellWidget(response, 0, &enableValue[response]);
        widthCol0 = std::max(widthCol0, enableValue[response].width());
        responses[response].setText(responseText);
        responses[response].setFlat(true);
        responses[response].setStyleSheet("Text-align:left");
        theTable->setCellWidget(response, 1, &responses[response]);
        connect(&responses[response], &QPushButton::clicked, &enableValue[response], &QCheckBox::toggle);
        connect(&enableValue[response], &QCheckBox::stateChanged, this, [&, response](int state){
                                                                                 if(state == Qt::Checked)
                                                                                   {URMResponsesConsideredUR << responses[response].text();
                                                                                    responses[response].setStyleSheet("Text-align:left;font-weight: bold;");}
                                                                                 else
                                                                                   {URMResponsesConsideredUR.removeAll(responses[response].text());
                                                                                    responses[response].setStyleSheet("Text-align:left;");}
                                                                                 });
    }
    theTable->horizontalHeader()->resizeSection(0, int(widthCol0 * TABLECOLUMN0OVERWIDTH));
    theTable->adjustSize();

    adjustSize();
}


gatherURMResponsesDialog::~gatherURMResponsesDialog()
{
    //delete dynamically allocated arrays created in class constructor
    delete [] enableValue;
    delete [] responses;
}
