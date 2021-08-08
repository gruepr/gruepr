#include "baseTimeZoneDialog.h"
#include "gruepr_consts.h"
#include <QDateTime>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to select base timezone for the class in the survey
/////////////////////////////////////////////////////////////////////////////////////////////////////////

baseTimezoneDialog::baseTimezoneDialog(QWidget *parent)
    :QDialog (parent)
{
    //Set up window with a grid layout
    setWindowTitle(tr("Class timezone"));
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    theGrid = new QGridLayout(this);

    //explanation and a spacer row
    explanation = new QLabel(this);
    explanation->setText(tr("<html>Students were asked to fill out their schedule using their home timezone. "
                            "Which of these should be used as the base timezone for the class? "
                            "The schedules will all be adjusted to correspond to this value.<hr></html>"));
    explanation->setWordWrap(true);
    theGrid->addWidget(explanation, 0, 0, 1, -1);
    theGrid->setRowMinimumHeight(1, DIALOG_SPACER_ROWHEIGHT);

    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.date(), local.time(), Qt::UTC);
    const float hoursToGMTFromHere = (local.secsTo(UTC)/3600.0F) - (local.isDaylightTime()? 1 : 0);

    QStringList timeZoneNames = QString(TIMEZONENAMES).split(";");
    timezones = new QComboBox(this);
    for(int zone = 0; zone < timeZoneNames.size(); zone++)
    {
        QString zonename = timeZoneNames.at(zone);
        zonename.remove('"');
        float GMTOffset = 0;
        QRegularExpression offsetFinder(".*\\[GMT(.*):(.*)\\].*");  // characters after "[GMT" are +hh:mm "]"
        QRegularExpressionMatch offset = offsetFinder.match(zonename);
        if(offset.hasMatch())
        {
            int hours = offset.captured(1).toInt();
            float minutes = offset.captured(2).toFloat();
            GMTOffset = hours + ((hours < 0)? (-minutes/60) : (minutes/60));
        }
        timezones->insertItem(zone, zonename);
        timezones->setItemData(zone, GMTOffset);
    }
    int index = timezones->findData(hoursToGMTFromHere);
    timezones->setCurrentIndex(index != -1? index : 0);
    baseTimezoneVal = timezones->currentData().toFloat();
    connect(timezones, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {baseTimezoneVal = timezones->currentData().toFloat();});
    theGrid->addWidget(timezones, 2, 0, 1, -1);

    //a spacer then ok button
    theGrid->setRowMinimumHeight(3, DIALOG_SPACER_ROWHEIGHT);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    theGrid->addWidget(buttonBox, 4, 0, 1, -1);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    adjustSize();
}
