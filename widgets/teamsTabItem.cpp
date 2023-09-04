#include "teamsTabItem.h"
#include "canvashandler.h"
#include "dialogs/customTeamnamesDialog.h"
#include "dialogs/whichFilesDialog.h"
#include "gruepr.h"
#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFuture>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QTextStream>
#include <QtConcurrent>

TeamsTabItem::TeamsTabItem(TeamingOptions &incomingTeamingOptions, const DataOptions &incomingDataOptions, CanvasHandler *const incomingCanvas,
                           const QList<TeamRecord> &incomingTeams, const QList<StudentRecord> &incomingStudents, const QString &incomingTabName, QWidget *parent) : QWidget(parent)
{
    teamingOptions = new TeamingOptions(incomingTeamingOptions);   // teamingOptions might change, so need to hold on to values when teams were made
    addedPreventedTeammates = &incomingTeamingOptions.haveAnyPreventedTeammates;    // need ability to modify this setting for when prevented teammates are added using button on this tab
    dataOptions = new DataOptions(incomingDataOptions);
    canvas = incomingCanvas;
    teams = incomingTeams;
    students = incomingStudents;
    numStudents = dataOptions->numStudentsInSystem;
    tabName = incomingTabName;

    setContentsMargins(0, 0, 0, 0);
    teamDataLayout = new QVBoxLayout;
    teamDataLayout->setContentsMargins(0, 0, 0, 0);
    teamDataLayout->setSpacing(6);
    teamDataLayout->setStretch(0, 1);
    setLayout(teamDataLayout);

    teamDataTree = new TeamTreeWidget(this);
    teamDataLayout->addWidget(teamDataTree);

    rowsLayout = new QHBoxLayout;
    rowsLayout->setSpacing(6);
    teamDataLayout->addLayout(rowsLayout);

    auto helpIcon = new LabelWithInstantTooltip("", this);
    helpIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    rowsLayout->addWidget(helpIcon);
    dragDropExplanation = new LabelWithInstantTooltip(tr("You can adjust the teams"), this);
    dragDropExplanation->setStyleSheet(LABELSTYLE);
    rowsLayout->addWidget(dragDropExplanation);
    QString helpText = tr("<html><span style=\"color: black;\">Use drag-and-drop to move students and teams:<br>"
                          "&nbsp;&nbsp;»&nbsp;<u>Drag a student onto a team name</u> to move the student onto that team.<br>"
                          "&nbsp;&nbsp;»&nbsp;<u>Drag a student onto a student</u> to swap the locations of the two students.<br>"
                          "&nbsp;&nbsp;»&nbsp;<u>Drag a team onto a team</u> to reorder the teams.<br>"
                          "</span></html>");
    helpIcon->setToolTipText(helpText);
    dragDropExplanation->setToolTipText(helpText);

    undoButton = new QPushButton(QIcon(":/icons_new/undo.png"), "", this);
    undoButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    undoButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    undoButton->setEnabled(false);
    undoButton->setToolTip("");
    connect(undoButton, &QPushButton::clicked, this, &TeamsTabItem::undoRedoDragDrop);
    rowsLayout->addWidget(undoButton);

    redoButton = new QPushButton(QIcon(":/icons_new/redo.png"), "", this);
    redoButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    redoButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    redoButton->setEnabled(false);
    redoButton->setToolTip("");
    connect(redoButton, &QPushButton::clicked, this, &TeamsTabItem::undoRedoDragDrop);
    rowsLayout->addWidget(redoButton);

    rowsLayout->addStretch(0);

    expandAllButton = new QPushButton(tr("Expand All Rows"), this);
    expandAllButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    expandAllButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(expandAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::expandAll);
    rowsLayout->addWidget(expandAllButton);

    collapseAllButton = new QPushButton(tr("Collapse All Rows"), this);
    collapseAllButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    collapseAllButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(collapseAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::collapseAll);
    rowsLayout->addWidget(collapseAllButton);

    horLine = new QFrame(this);
    horLine->setStyleSheet("border-color: " DEEPWATERHEX);
    horLine->setLineWidth(1);
    horLine->setMidLineWidth(1);
    horLine->setFrameShape(QFrame::HLine);
    horLine->setFrameShadow(QFrame::Plain);
    teamDataLayout->addWidget(horLine);

    teamOptionsLayout = new QHBoxLayout;
    teamOptionsLayout->setSpacing(6);
    teamDataLayout->addLayout(teamOptionsLayout);

    teamnameCategories = QString(TEAMNAMECATEGORIES).split(",");
    teamnameLists = QString(TEAMNAMELISTS).split(';');
    teamnameTypes.clear();
    for(auto &teamnameCategory : teamnameCategories)
    {
        // use the last character as the type signifier, then remove the character from the actual name
        switch((*(teamnameCategory.crbegin())).unicode())
        {
        case '.':
            teamnameTypes << numeric;
            break;
        case '*':
            teamnameTypes << repeated;
            break;
        case '~':
            teamnameTypes << repeated_spaced;
            break;
        case '#':
            teamnameTypes << sequeled;
            break;
        case '@':
            teamnameTypes << random_sequeled;
            break;
        }

        teamnameCategory.chop(1);
    }
    teamnamesComboBox = new QComboBox(this);
    teamnamesComboBox->setStyleSheet(COMBOBOXSTYLE);
    teamnamesComboBox->setPlaceholderText(tr("Set team names"));
    teamnamesComboBox->addItems(teamnameCategories);
    teamnamesComboBox->addItem(tr("Custom names..."));
    teamnamesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(teamnamesComboBox, QOverload<int>::of(&QComboBox::activated), this, &TeamsTabItem::teamNamesChanged);
    teamOptionsLayout->addWidget(teamnamesComboBox);

    randTeamnamesCheckBox = new QCheckBox(tr("Randomize"), this);
    randTeamnamesCheckBox->setStyleSheet(CHECKBOXSTYLE);
    randTeamnamesCheckBox->setEnabled(false);
    randTeamnamesCheckBox->setChecked(false);
    randTeamnamesCheckBox->setToolTip(tr("Select to randomize the order of the team names"));
    connect(randTeamnamesCheckBox, &QCheckBox::clicked, this, &TeamsTabItem::randomizeTeamnames);
    teamOptionsLayout->addWidget(randTeamnamesCheckBox);

    sendToPreventedTeammates = new QPushButton(tr("Create teams with all new teammates"), this);
    sendToPreventedTeammates->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    sendToPreventedTeammates->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sendToPreventedTeammates->setFlat(true);
    sendToPreventedTeammates->setToolTip(tr("<html>Create another set of teams, with all current teammates set as \"Prevented Teammates\"</html>"));
    connect(sendToPreventedTeammates, &QPushButton::clicked, this, &TeamsTabItem::makeNewSetWithAllNewTeammates);
    teamOptionsLayout->addWidget(sendToPreventedTeammates);

    savePrintLayout = new QHBoxLayout;
    savePrintLayout->setSpacing(2);
    teamDataLayout->addLayout(savePrintLayout);

    QPixmap whiteSaveFile = QPixmap(":/icons_new/edit_document.png").scaled(SAVEPRINTICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&whiteSaveFile);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteSaveFile.rect(), QColor("white"));
    painter.end();
    saveTeamsButton = new QPushButton(QIcon(whiteSaveFile), tr("Save"), this);
    saveTeamsButton->setStyleSheet(SAVEPRINTBUTTONSTYLE);
    saveTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    saveTeamsButton->setToolTip(tr("Save this set of teams as text or pdf"));
    connect(saveTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::saveTeams);
    savePrintLayout->addWidget(saveTeamsButton);

    printTeamsButton = new QPushButton(QIcon(":/icons_new/print.png"), tr("Print"), this);
    printTeamsButton->setStyleSheet(SAVEPRINTBUTTONSTYLE);
    printTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    printTeamsButton->setToolTip(tr("Send this set of teams to the printer"));
    connect(printTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::printTeams);
    savePrintLayout->addWidget(printTeamsButton);

    teamDataTree->collapseAll();
    teamDataTree->resetDisplay(dataOptions, teamingOptions);
    refreshTeamDisplay();
    teamDataTree->sortByColumn(0, Qt::AscendingOrder);
    teamDataTree->headerItem()->setIcon(0, QIcon(":/icons_new/blank_arrow.png"));
    refreshDisplayOrder();
    connect(teamDataTree, &TeamTreeWidget::swapChildren, this, &TeamsTabItem::swapStudents);
    connect(teamDataTree, &TeamTreeWidget::reorderParents, this, &TeamsTabItem::moveATeam);
    connect(teamDataTree, &TeamTreeWidget::moveChild, this, &TeamsTabItem::moveAStudent);
    connect(teamDataTree, &TeamTreeWidget::updateTeamOrder, this, &TeamsTabItem::refreshDisplayOrder);
}


TeamsTabItem::~TeamsTabItem()
{
    // delete dynamically allocated objects
    delete teamingOptions;
    delete dataOptions;
}


void TeamsTabItem::teamNamesChanged(int index)
{
    static int prevIndex = 0;   // hold on to previous index, so we can go back to it if cancelling custom team name dialog box

    if(index != prevIndex)      // reset the randomize teamnames checkbox if we just moved to a new index
    {
        randTeamnamesCheckBox->setChecked(false);
        randTeamnamesCheckBox->setEnabled(index > 3 && index < teamnameLists.size());
    }

    // Get team numbers in the order that they are currently displayed/sorted
    QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    // Set team names to:
    if(index == 0)
    {
        // arabic numbers
        for(int team = 0; team < teams.size(); team++)
        {
            teams[teamDisplayNums.at(team)].name = QString::number(team+1);
        }
        prevIndex = 0;
    }
    else if(index == 1)
    {
        // roman numerals
        QString M[] = {"","M","MM","MMM"};
        QString C[] = {"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
        QString X[] = {"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
        QString I[] = {"","I","II","III","IV","V","VI","VII","VIII","IX"};
        for(int team = 0; team < teams.size(); team++)
        {
            teams[teamDisplayNums.at(team)].name = M[(team+1)/1000]+C[((team+1)%1000)/100]+X[((team+1)%100)/10]+I[((team+1)%10)];
        }
        prevIndex = 1;
    }
    else if(index == 2)
    {
        // hexadecimal numbers
        for(int team = 0; team < teams.size(); team++)
        {
            teams[teamDisplayNums.at(team)].name = QString::number(team, 16).toUpper();
        }
        prevIndex = 2;
    }
    else if(index == 3)
    {
        // binary numbers
        const int numDigitsInLargestTeam = QString::number(int(teams.size())-1, 2).size();       // the '-1' is to make the first team 0
        for(int team = 0; team < teams.size(); team++)
        {
            teams[teamDisplayNums.at(team)].name = QString::number(team, 2).rightJustified(numDigitsInLargestTeam, '0'); // pad w/ 0 to use same number of digits
        }
        prevIndex = 3;
    }
    else if(index < teamnameLists.size())
    {
        // one of the listed team names from gruepr_structs_and_consts.h
        const TeamNameType teamNameType = randTeamnamesCheckBox->isChecked() ? random_sequeled : teamnameTypes.at(index);
        const QStringList teamNames = teamnameLists.at(index).split(',');

        QList<int> random_order(teamNames.size());
        if(teamNameType == random_sequeled)
        {
            std::iota(random_order.begin(), random_order.end(), 0);
            std::random_device randDev;
            std::mt19937 pRNG(randDev());
            //std::mt19937 pRNG{static_cast<long unsigned int>(time(nullptr))};     //only for minGW, which does not play well with std::random_device
            std::shuffle(random_order.begin(), random_order.end(), pRNG);
        }

        for(int team = 0; team < teams.size(); team++)
        {
            switch(teamNameType)
            {
                case numeric:
                    break;
                case repeated:
                    teams[teamDisplayNums.at(team)].name = (teamNames[team%(teamNames.size())]).repeated((team/teamNames.size())+1);
                    break;
                case repeated_spaced:
                    teams[teamDisplayNums.at(team)].name = (teamNames[team%(teamNames.size())]+" ").repeated((team/teamNames.size())+1).trimmed();
                    break;
                case sequeled:
                    teams[teamDisplayNums.at(team)].name = (teamNames[team%(teamNames.size())]) +
                                                           ((team/teamNames.size() == 0) ? "" : " " + QString::number(team/teamNames.size()+1));
                    break;
                case random_sequeled:
                    teams[teamDisplayNums.at(team)].name = (teamNames[random_order[team%(teamNames.size())]]) +
                                                           ((team/teamNames.size() == 0) ? "" : " " + QString::number(team/teamNames.size()+1));
                    break;
            }
        }
        prevIndex = index;
    }
    else if(teamnamesComboBox->currentText() == tr("Current names"))
    {
        // Keeping the current custom names
    }
    else
    {
        // Open custom dialog box to collect teamnames
        QStringList teamNames;
        teamNames.reserve(teams.size());
        for(int team = 0; team < teams.size(); team++)
        {
            teamNames << teams[teamDisplayNums.at(team)].name;
        }
        auto *window = new customTeamnamesDialog(int(teams.size()), teamNames, this);

        // If user clicks OK, use these team names, otherwise revert to previous option
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
            for(int team = 0; team < teams.size(); team++)
            {
                    teams[teamDisplayNums.at(team)].name = (window->teamNames[team]->text().isEmpty()? QString::number(team+1) : window->teamNames[team]->text());
            }
            prevIndex = int(teamnameLists.size());
            bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            teamnamesComboBox->setItemText(int(teamnameLists.size()), tr("Current names"));
            teamnamesComboBox->removeItem(int(teamnameLists.size())+1);
            teamnamesComboBox->addItem(tr("Custom names..."));
            teamnamesComboBox->blockSignals(currentValue);
        }
        else
        {
            bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            randTeamnamesCheckBox->setEnabled(prevIndex > 3 && prevIndex < teamnameLists.size());
            teamnamesComboBox->blockSignals(currentValue);
        }

        delete window;
    }

    // Put list of options back to just built-ins plus "Custom names..."
    if(teamnamesComboBox->currentIndex() < teamnameLists.size())
    {
        teamnamesComboBox->removeItem(int(teamnameLists.size())+1);
        teamnamesComboBox->removeItem(int(teamnameLists.size()));
        teamnamesComboBox->addItem(tr("Custom names..."));
    }

    // Update team names in table and tooltips
    for(int team = 0; team < teams.size(); team++)
    {
        teams[team].createTooltip();
        teamDataTree->topLevelItem(team)->setText(0, tr("Team ") + teams[teamDisplayNums.at(team)].name);
        teamDataTree->topLevelItem(team)->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        teamDataTree->topLevelItem(team)->setData(0, TEAMINFO_DISPLAY_ROLE, tr("Team ") + teams[teamDisplayNums.at(team)].name);
        for(int column = 0, numColsForToolTips = teamDataTree->columnCount()-1; column < numColsForToolTips; column++)
        {
            teamDataTree->topLevelItem(team)->setToolTip(column, teams[teamDisplayNums.at(team)].tooltip);
        }
    }
    teamDataTree->resizeColumnToContents(0);
}


void TeamsTabItem::randomizeTeamnames()
{
    teamNamesChanged(teamnamesComboBox->currentIndex());
}


void TeamsTabItem::makeNewSetWithAllNewTeammates()
{
    for(auto &team : teams)
    {
        for(const auto index1 : qAsConst(team.studentIndexes))
        {
            for(const auto index2 : qAsConst(team.studentIndexes))
            {
                if(index1 != index2)
                {
                    students[index1].preventedWith[students[index2].ID] = true;
                }
            }
        }
    }
    *addedPreventedTeammates = true;
    QMessageBox::information(this, tr("Successfully loaded"), tr("These teams have all been loaded into the \"Prevented Teammates\" setting. "
                                                                 "Pushing \"Create Teams\" will create a new set of teams with everyone getting all new teammates."));
}


void TeamsTabItem::refreshDisplayOrder()
{
    // Any time teams have been reordered, refresh the hidden display order column
    const int lastCol = teamDataTree->columnCount()-1;
    for(int row = 0; row < teams.size(); row++)
    {
        auto *const teamItem = teamDataTree->topLevelItem(row);
        teamItem->setData(lastCol, TEAMINFO_SORT_ROLE, row);
        teamItem->setData(lastCol, TEAMINFO_DISPLAY_ROLE, QString::number(row));
        teamItem->setText(lastCol, QString::number(row));
    }
}


QList<int> TeamsTabItem::getTeamNumbersInDisplayOrder()
{
    QList<int> teamDisplayNums;
    teamDisplayNums.reserve(teams.size());
    const int lastCol = teamDataTree->columnCount()-1;
    for(int order = 0; order < teams.size(); order++)
    {
        int row = 0;
        while((teamDataTree->topLevelItem(row)->data(lastCol, TEAMINFO_SORT_ROLE).toInt() != order) && (row < teams.size()))
        {
            row++;
        }
        teamDisplayNums << teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt();
    }
    return teamDisplayNums;
}


void TeamsTabItem::swapStudents(const QList<int> &arguments) // QList<int> arguments = int studentAteam, int studentAID, int studentBteam, int studentBID
{
    int studentAteam = arguments.at(0), studentAID = arguments.at(1), studentBteam = arguments.at(2), studentBID = arguments.at(3);
    if(studentAID == studentBID)
    {
        return;
    }

    //Get index for each ID
    int studentAIndex = 0, studentBIndex = 0;
    while((studentAIndex < numStudents) && students[studentAIndex].ID != studentAID)
    {
        studentAIndex++;
    }
    while((studentBIndex < numStudents) && students[studentBIndex].ID != studentBID)
    {
        studentBIndex++;
    }

    //Load undo onto stack anc clear redo stack
    QString UndoTooltip = tr("Undo swapping ") + students[studentAIndex].firstname + " " + students[studentAIndex].lastname +
                           tr(" with ") + students[studentBIndex].firstname + " " + students[studentBIndex].lastname;
    undoItems.prepend({&TeamsTabItem::swapStudents, {studentAteam, studentBID, studentBteam, studentAID}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    if(studentAteam == studentBteam)
    {
        std::swap(teams[studentAteam].studentIndexes[teams[studentAteam].studentIndexes.indexOf(studentAIndex)],
                  teams[studentBteam].studentIndexes[teams[studentBteam].studentIndexes.indexOf(studentBIndex)]);

        // Re-score the teams and refresh all the info
        gruepr::updateTeamScores(students.constData(), numStudents, teams.data(), int(teams.size()), teamingOptions, dataOptions);
        teams[studentAteam].refreshTeamInfo(students.constData());
        teams[studentAteam].createTooltip();

        //get the team item in the tree
        QTreeWidgetItem *teamItem = nullptr;
        int row = 0;
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentAteam) && (row < teams.size()))
        {
            row++;
        }
        teamItem = teamDataTree->topLevelItem(row);

        //clear and refresh student items in table
        for(auto &child : teamItem->takeChildren())
        {
            delete child;
        }
        int numStudentsOnTeam = teams[studentAteam].size;
        QList<TeamTreeWidgetItem*> childItems;
        childItems.reserve(numStudentsOnTeam);
        for(int studentNum = 0; studentNum < numStudentsOnTeam; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentAteam].studentIndexes[studentNum]], dataOptions, teamingOptions);
            teamItem->addChild(childItems[studentNum]);
        }
    }
    else
    {
        teams[studentAteam].studentIndexes.replace(teams[studentAteam].studentIndexes.indexOf(studentAIndex), studentBIndex);
        teams[studentBteam].studentIndexes.replace(teams[studentBteam].studentIndexes.indexOf(studentBIndex), studentAIndex);

        //get the team items in the tree
        TeamTreeWidgetItem *teamAItem = nullptr, *teamBItem = nullptr;
        int row = 0;
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentAteam) && (row < teams.size()))
        {
            row++;
        }
        teamAItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));
        row = 0;
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentBteam) && (row < teams.size()))
        {
            row++;
        }
        teamBItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));

        //refresh the info for both teams
        gruepr::updateTeamScores(students.constData(), numStudents, teams.data(), int(teams.size()), teamingOptions, dataOptions);
        teams[studentAteam].refreshTeamInfo(students.constData());
        teams[studentAteam].createTooltip();
        teams[studentBteam].refreshTeamInfo(students.constData());
        teams[studentBteam].createTooltip();

        QString firstStudentName = students[teams[studentAteam].studentIndexes[0]].lastname+students[teams[studentAteam].studentIndexes[0]].firstname;
        QString firstStudentSection = students[teams[studentAteam].studentIndexes[0]].section;
        teamDataTree->refreshTeam(teamAItem, teams[studentAteam], studentAteam, firstStudentName, firstStudentSection, dataOptions, teamingOptions);
        firstStudentName = students[teams[studentBteam].studentIndexes[0]].lastname+students[teams[studentBteam].studentIndexes[0]].firstname;
        firstStudentSection = students[teams[studentBteam].studentIndexes[0]].section;
        teamDataTree->refreshTeam(teamBItem, teams[studentBteam], studentBteam, firstStudentName, firstStudentSection, dataOptions, teamingOptions);
        teamAItem->setScoreColor(teams[studentAteam].score);
        teamBItem->setScoreColor(teams[studentBteam].score);

        //clear and refresh student items on both teams in table
        for(auto &child : teamAItem->takeChildren())
        {
            delete child;
        }
        for(auto &child : teamBItem->takeChildren())
        {
            delete child;
        }
        int numStudentsOnTeamA = teams[studentAteam].size;
        int numStudentsOnTeamB = teams[studentBteam].size;
        QList<TeamTreeWidgetItem*> childItems;
        childItems.reserve(std::max(numStudentsOnTeamA, numStudentsOnTeamB));
        for(int studentNum = 0; studentNum < numStudentsOnTeamA; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentAteam].studentIndexes[studentNum]], dataOptions, teamingOptions);
            teamAItem->addChild(childItems[studentNum]);
        }
        childItems.clear();
        for(int studentNum = 0; studentNum < numStudentsOnTeamB; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentBteam].studentIndexes[studentNum]], dataOptions, teamingOptions);
            teamBItem->addChild(childItems[studentNum]);
        }
    }

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::moveAStudent(const QList<int> &arguments) // QList<int> arguments = int oldTeam, int studentID, int newTeam
{
    int oldTeam = arguments.at(0), studentID = arguments.at(1), newTeam = arguments.at(2);

    if((oldTeam == newTeam) || (teams[oldTeam].size == 1))
    {
        return;
    }

    //Get index for the ID
    int studentIndex = 0;
    while((studentIndex < numStudents) && students[studentIndex].ID != studentID)
    {
        studentIndex++;
    }

    //Load undo onto stack and clear redo stack
    QString UndoTooltip = tr("Undo moving ") + students[studentIndex].firstname + " " + students[studentIndex].lastname +
                           tr(" from Team ") + teams[oldTeam].name + tr(" to Team ") + teams[newTeam].name;
    undoItems.prepend({&TeamsTabItem::moveAStudent, {newTeam, studentID, oldTeam}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    //remove student from old team and add to new team
    teams[oldTeam].studentIndexes.removeOne(studentIndex);
    teams[oldTeam].size--;
    teams[newTeam].studentIndexes << studentIndex;
    teams[newTeam].size++;

    //get the team items in the tree
    TeamTreeWidgetItem *oldTeamItem = nullptr, *newTeamItem = nullptr;
    int row = 0;
    while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != oldTeam) && (row < teams.size()))
    {
        row++;
    }
    oldTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));
    row = 0;
    while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != newTeam) && (row < teams.size()))
    {
        row++;
    }
    newTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));

    //refresh the info for both teams
    gruepr::updateTeamScores(students.constData(), numStudents, teams.data(), int(teams.size()), teamingOptions, dataOptions);
    teams[oldTeam].refreshTeamInfo(students.constData());
    teams[oldTeam].createTooltip();
    teams[newTeam].refreshTeamInfo(students.constData());
    teams[newTeam].createTooltip();

    QString firstStudentName = students[teams[oldTeam].studentIndexes[0]].lastname+students[teams[oldTeam].studentIndexes[0]].firstname;
    QString firstStudentSection = students[teams[oldTeam].studentIndexes[0]].section;
    teamDataTree->refreshTeam(oldTeamItem, teams[oldTeam], oldTeam, firstStudentName, firstStudentSection, dataOptions, teamingOptions);
    firstStudentName = students[teams[newTeam].studentIndexes[0]].lastname+students[teams[newTeam].studentIndexes[0]].firstname;
    firstStudentSection = students[teams[newTeam].studentIndexes[0]].section;
    teamDataTree->refreshTeam(newTeamItem, teams[newTeam], newTeam, firstStudentName, firstStudentSection, dataOptions, teamingOptions);
    oldTeamItem->setScoreColor(teams[oldTeam].score);
    newTeamItem->setScoreColor(teams[newTeam].score);

    //clear and refresh student items on both teams in table
    for(auto &child : oldTeamItem->takeChildren())
    {
        delete child;
    }
    for(auto &child : newTeamItem->takeChildren())
    {
        delete child;
    }
    //clear and refresh student items on both teams in table
    int numStudentsOnStudentTeam = teams[oldTeam].size;
    int numStudentsOnNewTeam = teams[newTeam].size;
    QList<TeamTreeWidgetItem*> childItems;
    childItems.reserve(std::max(numStudentsOnStudentTeam, numStudentsOnNewTeam));
    for(int studentNum = 0; studentNum < numStudentsOnStudentTeam; studentNum++)
    {
        childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
        teamDataTree->refreshStudent(childItems[studentNum], students[teams[oldTeam].studentIndexes[studentNum]], dataOptions, teamingOptions);
        oldTeamItem->addChild(childItems[studentNum]);
    }
    childItems.clear();
    for(int studentNum = 0; studentNum < numStudentsOnNewTeam; studentNum++)
    {
        childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
        teamDataTree->refreshStudent(childItems[studentNum], students[teams[newTeam].studentIndexes[studentNum]], dataOptions, teamingOptions);
        newTeamItem->addChild(childItems[studentNum]);
    }

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::moveATeam(const QList<int> &arguments)  // QList<int> arguments = int teamA, int teamB
{
    int teamA = arguments.at(0), teamB = arguments.at(1);   // teamB = -1 if moving to the last row

    if(teamA == teamB)
    {
        return;
    }

    // find the teamA and teamB top level items in teamDataTree
    int teamARow = 0, teamBRow = teamDataTree->topLevelItemCount();    // teamBRow == teamDataTree->topLevelItemCount() will correspond to teamB == -1
    for(int row = 0; row < teams.size(); row++)
    {
        if(teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() == teamA)
        {
            teamARow = row;
        }
        else if(teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() == teamB)
        {
            teamBRow = row;
        }
    }

    if(teamBRow - teamARow == 1)            // dragging just one row down ==> no change in order
    {
        return;
    }

    //Load undo onto stack and clear redo stack
    int teamBelowTeamA = ((teamARow < teams.size()-1) ? teamDataTree->topLevelItem(teamARow+1)->data(0, TEAM_NUMBER_ROLE).toInt() : -1);
    QString UndoTooltip = tr("Undo reordering Team ") + teams[teamA].name;
    undoItems.prepend({&TeamsTabItem::moveATeam, {teamA, teamBelowTeamA}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order, then adjust sort data for teamA and teamB, then resort
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);
    if(teamARow - teamBRow == 1)    // dragging just one row above ==> just swap the two
    {
        // swap sort column data
        int teamASortOrder = teamDataTree->topLevelItem(teamARow)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();
        int teamBSortOrder = teamDataTree->topLevelItem(teamBRow)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
        teamDataTree->topLevelItem(teamARow)->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
        teamDataTree->topLevelItem(teamBRow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamASortOrder);
        teamDataTree->topLevelItem(teamBRow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamASortOrder));
        teamDataTree->topLevelItem(teamBRow)->setText(teamDataTree->columnCount()-1, QString::number(teamASortOrder));
    }
    else if(teamARow > teamBRow)    // dragging team onto a team listed earlier in the table
    {
        // backwards from teamA-1 up to teamB, increment sort column data
        for(int row = teamARow-1; row > teamBRow; row--)
        {
            int teamBelowRow = teamDataTree->topLevelItem(row)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() + 1;
            teamDataTree->topLevelItem(row)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBelowRow);
            teamDataTree->topLevelItem(row)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBelowRow));
            teamDataTree->topLevelItem(row)->setText(teamDataTree->columnCount()-1, QString::number(teamBelowRow));
        }
        // set sort column data for teamA to teamB
        int teamBSortOrder = teamDataTree->topLevelItem(teamBRow)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
        teamDataTree->topLevelItem(teamARow)->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
    }
    else                            // dragging team onto a team listed later in the table (including to the bottom of the table, when teamBRow == teamDataTree->topLevelItemCount())
    {
        if(teamBRow == teamDataTree->topLevelItemCount())
        {
            // set sort column data for teamA to end
            teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBRow);
            teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBRow));
            teamDataTree->topLevelItem(teamARow)->setText(teamDataTree->columnCount()-1, QString::number(teamBRow));
        }
        else
        {
            int teamBSortOrder = teamDataTree->topLevelItem(teamBRow)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();
            // set sort column data for teamA to teamB
            teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
            teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
            teamDataTree->topLevelItem(teamARow)->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
        }
    }
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    // rewrite all of the sort column data
    refreshDisplayOrder();

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::undoRedoDragDrop()
{
    bool undoingSomething = (sender() == undoButton);

    auto redoItemsCopy = redoItems;

    auto *itemStack = (undoingSomething? &undoItems : &redoItems);
    if(itemStack->isEmpty())
    {
        return;
    }
    auto item = itemStack->takeFirst();

    (this->*(item.action))(item.arguments);

    redoItems = redoItemsCopy;      // performing the action (in the previous line) puts it back on the undo list and clears the redo list
    if(undoingSomething)
    {
        redoItems.prepend(undoItems.takeFirst());
        redoItems.first().ToolTip.replace(tr("Undo"), tr("Redo"));
    }
    else    // redoingSomething
    {
        redoItems.removeFirst();
    }

    redoButton->setEnabled(!redoItems.isEmpty());
    redoButton->setToolTip(redoItems.isEmpty()? "" : redoItems.first().ToolTip);
    undoButton->setEnabled(!undoItems.isEmpty());
    undoButton->setToolTip(undoItems.isEmpty()? "" : undoItems.first().ToolTip);
}


void TeamsTabItem::saveTeams()
{
    createFileContents();
    const int previewLength = 1000;
    QStringList previews = {studentsFileContents.left(previewLength) + "...",
                            instructorsFileContents.mid(instructorsFileContents.indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, previewLength) + "...",
                            spreadsheetFileContents.left(previewLength) + "..."};

    //Open specialized dialog box to choose which file(s) to save
    auto *window = new whichFilesDialog(whichFilesDialog::Action::save, previews, this);
    int result = window->exec();

    if(result == QDialogButtonBox::Ok && (window->instructorFiletxt->isChecked() || window->studentFiletxt->isChecked() || window->spreadsheetFiletxt->isChecked()))
    {
        //save to text files
        QString baseFileName = QFileDialog::getSaveFileName(this, tr("Choose a location and base filename for the text file(s)"), "",
                                                            tr("Text File (*.txt);;All Files (*)"));
        if ( !(baseFileName.isEmpty()) )
        {
            bool problemSaving = false;
            if(window->instructorFiletxt->isChecked())
            {
                QString fullFilename = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName();
                if(window->studentFiletxt->isChecked() || window->spreadsheetFiletxt->isChecked())
                {
                    fullFilename += tr("_instructor");
                }
                fullFilename += "." + QFileInfo(baseFileName).suffix();
                QFile instructorsFile(fullFilename);
                if(instructorsFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&instructorsFile);
                    out << instructorsFileContents;
                    instructorsFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(window->studentFiletxt->isChecked())
            {
                QString fullFilename = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName();
                if(window->instructorFiletxt->isChecked() || window->spreadsheetFiletxt->isChecked())
                {
                    fullFilename += tr("_student");
                }
                fullFilename += "." + QFileInfo(baseFileName).suffix();
                QFile studentsFile(fullFilename);
                if(studentsFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&studentsFile);
                    out << studentsFileContents;
                    studentsFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(window->spreadsheetFiletxt->isChecked())
            {
                QString fullFilename = QFileInfo(baseFileName).path() + "/" + QFileInfo(baseFileName).completeBaseName();
                if(window->studentFiletxt->isChecked() || window->spreadsheetFiletxt->isChecked())
                {
                    fullFilename += tr("_spreadsheet");
                }
                fullFilename += "." + QFileInfo(baseFileName).suffix();
                QFile spreadsheetFile(fullFilename);
                if(spreadsheetFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    QTextStream out(&spreadsheetFile);
                    out << spreadsheetFileContents;
                    spreadsheetFile.close();
                }
                else
                {
                    problemSaving = true;
                }
            }
            if(problemSaving)
            {
                QMessageBox::critical(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
            }
        }
    }
    if(result == QDialogButtonBox::Ok && (window->instructorFilepdf->isChecked() || window->studentFilepdf->isChecked()))
    {
        //save as formatted pdf files
        printFiles(window->instructorFilepdf->isChecked(), window->studentFilepdf->isChecked(), false, true);
    }
    delete window;
}


void TeamsTabItem::postTeamsToCanvas()
{
    if(!internetIsGood())
    {
        return;
    }

    //create canvasHandler and/or authenticate as needed
    if(canvas == nullptr)
    {
        canvas = new CanvasHandler();
    }
    if(!canvas->authenticated)
    {
        QSettings savedSettings;
        QString savedCanvasURL = savedSettings.value("canvasURL").toString();
        QString savedCanvasToken = savedSettings.value("canvasToken").toString();

        QStringList newURLAndToken = canvas->askUserForManualToken(savedCanvasURL, savedCanvasToken, this);
        if(newURLAndToken.isEmpty())
        {
            return;
        }

        savedCanvasURL = (newURLAndToken.at(0).isEmpty() ? savedCanvasURL : newURLAndToken.at(0));
        savedCanvasToken =  (newURLAndToken.at(1).isEmpty() ? savedCanvasToken : newURLAndToken.at(1));
        savedSettings.setValue("canvasURL", savedCanvasURL);
        savedSettings.setValue("canvasToken", savedCanvasToken);

        canvas->setBaseURL(savedCanvasURL);
        canvas->authenticate(savedCanvasToken);
    }

    //ask the user in which course we're creating the teams
    auto *busyBox = canvas->busy();
    QStringList courseNames = canvas->getCourses();
    canvas->notBusy(busyBox);
    auto *canvasCourses = new QDialog;
    canvasCourses->setWindowTitle(tr("Choose Canvas course"));
    canvasCourses->setWindowIcon(QIcon(":/icons_new/canvas.png"));
    canvasCourses->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    int i = 1;
    auto *label = new QLabel(tr("In which course should these teams be created?"), canvasCourses);
    label->setStyleSheet(LABELSTYLE);
    auto *coursesComboBox = new QComboBox(canvasCourses);
    coursesComboBox->setStyleSheet(COMBOBOXSTYLE);
    for(const auto &courseName : qAsConst(courseNames))
    {
        coursesComboBox->addItem(courseName);
        coursesComboBox->setItemData(i++, QString::number(canvas->getStudentCount(courseName)) + " students", Qt::ToolTipRole);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesComboBox);
    vLayout->addWidget(buttonBox);
    canvasCourses->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, canvasCourses, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, canvasCourses, &QDialog::reject);
    if((canvasCourses->exec() == QDialog::Rejected))
    {
        return;
    }

    // get team numbers in the order that they are currently displayed/sorted
    QList<int> teamDisplayNum;
    teamDisplayNum.reserve(teams.size());
    for(int row = 0; row < teams.size(); row++)
    {
        int team = 0;
        while(teamDataTree->topLevelItem(row)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() != team)
        {
            team++;
        }
        teamDisplayNum << teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt();
    }
    // assemble each team in display order
    QStringList teamNames;
    QList<StudentRecord> teamRoster;
    QList<QList<StudentRecord>> teamRosters;
    for(int teamNum = 0; teamNum < teams.size(); teamNum++)
    {
        int team = teamDisplayNum.at(teamNum);
        teamNames << teams[team].name;
        teamRoster.clear();
        //loop through each teammate in the team
        for(int teammate = 0; teammate < teams[team].size; teammate++)
        {
            teamRoster << students[teams[team].studentIndexes.at(teammate)];
        }
        teamRosters << teamRoster;
    }

    busyBox = canvas->busy();
    QSize iconSize = canvas->busyBoxIcon->pixmap().size();
    QPixmap icon;
    QEventLoop loop;
    if(canvas->createTeams(coursesComboBox->currentText(), tabName, teamNames, teamRosters))
    {
        canvas->busyBoxLabel->setText(tr("Success!"));
        icon.load(":/icons_new/ok.png");
        canvas->busyBoxIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->notBusy(busyBox);
    }
    else
    {
        canvas->busyBoxLabel->setText(tr("Error. Teams not created."));
        icon.load(":/icons_new/error.png");
        canvas->busyBoxIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->notBusy(busyBox);
        return;
    }
    delete canvasCourses;
}


void TeamsTabItem::printTeams()
{
    createFileContents();
    const int previewLength = 1000;
    QStringList previews = {studentsFileContents.left(previewLength) + "...",
                            instructorsFileContents.mid(instructorsFileContents.indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, previewLength) + "...",
                            spreadsheetFileContents.left(previewLength) + "..."};

    //Open specialized dialog box to choose which file(s) to print
    auto *window = new whichFilesDialog(whichFilesDialog::Action::print, previews, this);
    int result = window->exec();

    if(result == QDialogButtonBox::Ok && (window->instructorFiletxt->isChecked() || window->studentFiletxt->isChecked() || window->spreadsheetFiletxt->isChecked()))
    {
        printFiles(window->instructorFiletxt->isChecked(), window->studentFiletxt->isChecked(), window->spreadsheetFiletxt->isChecked(), false);
    }
    delete window;
}


void TeamsTabItem::refreshTeamDisplay()
{
    //Create TeamTreeWidgetItems for the teams and students
    QList<TeamTreeWidgetItem*> parentItems;
    parentItems.reserve(teams.size());
    QList<TeamTreeWidgetItem*> childItems;
    childItems.reserve(numStudents);

    //iterate through teams to update the tree of teams and students
    for(int teamNum = 0; teamNum < teams.size(); teamNum++)
    {
        const auto &currentTeam = teams[teamNum];
        parentItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::team, teamDataTree->columnCount(), currentTeam.score);
        QString firstStudentName = students[currentTeam.studentIndexes[0]].lastname+students[currentTeam.studentIndexes[0]].firstname;
        QString firstStudentSection = students[currentTeam.studentIndexes[0]].section;
        teamDataTree->refreshTeam(parentItems.last(), currentTeam, teamNum, firstStudentName, firstStudentSection, dataOptions, teamingOptions);

        //remove all student items in the team
        for(auto &studentItem : parentItems.last()->takeChildren())
        {
            parentItems.last()->removeChild(studentItem);
            delete studentItem;
        }

        //add new student items
        for(int studentOnTeam = 0, numStudentsOnTeam = currentTeam.size; studentOnTeam < numStudentsOnTeam; studentOnTeam++)
        {
            childItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems.last(), students[currentTeam.studentIndexes[studentOnTeam]], dataOptions, teamingOptions);
            parentItems.last()->addChild(childItems.last());
        }

        parentItems.last()->setExpanded(false);
    }

    // Finally, put each team in the table for display
    teamDataTree->setUpdatesEnabled(false);
    teamDataTree->clear();
    for(auto &parentItem : parentItems)
    {
        teamDataTree->addTopLevelItem(parentItem);
    }
    for(int column = 0; column < teamDataTree->columnCount(); column++)
    {
        teamDataTree->resizeColumnToContents(column);
    }
    teamDataTree->setUpdatesEnabled(true);

    teamDataTree->setSortingEnabled(true);
}


//////////////////
//Setup printer and then print paginated file(s) in boxes
//////////////////
void TeamsTabItem::createFileContents()
{
    spreadsheetFileContents = tr("Section") + "\t" + tr("Team") + "\t" + tr("Name") + "\t" + tr("Email") + "\n";

    instructorsFileContents = tr("File: ") + dataOptions->dataFile.filePath() + "\n" + tr("Section: ") + teamingOptions->sectionName + "\n\n";
    instructorsFileContents += tr("Teaming Options") + ":";
    if(dataOptions->genderIncluded)
    {
        instructorsFileContents += (teamingOptions->isolatedWomenPrevented? ("\n" + tr("Isolated women prevented")) : "");
        instructorsFileContents += (teamingOptions->isolatedMenPrevented? ("\n" + tr("Isolated men prevented")) : "");
        instructorsFileContents += (teamingOptions->isolatedNonbinaryPrevented? ("\n" + tr("Isolated nonbinary students prevented")) : "");
        instructorsFileContents += (teamingOptions->singleGenderPrevented? ("\n" + tr("Single gender teams prevented")) : "");
    }
    if(dataOptions->URMIncluded && teamingOptions->isolatedURMPrevented)
    {
        instructorsFileContents += "\n" + tr("Isolated URM students prevented");
    }
    if(teamingOptions->scheduleWeight > 0)
    {
        instructorsFileContents += "\n" + tr("Meeting block size is ") + QString::number(teamingOptions->meetingBlockSize) +
                                                                         tr(" hour") + ((teamingOptions->meetingBlockSize == 1) ? "" : tr("s"));
        instructorsFileContents += "\n" + tr("Minimum number of meeting times = ") + QString::number(teamingOptions->minTimeBlocksOverlap);
        instructorsFileContents += "\n" + tr("Desired number of meeting times = ") + QString::number(teamingOptions->desiredTimeBlocksOverlap);
        instructorsFileContents += "\n" + tr("Schedule weight = ") + QString::number(double(teamingOptions->scheduleWeight));
    }
    for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
    {
        instructorsFileContents += "\n" + tr("Attribute ") + QString::number(attrib+1) + ": "
                + tr("weight") + " = " + QString::number(double(teamingOptions->attributeWeights[attrib])) +
                + ", " + (teamingOptions->desireHomogeneous[attrib]? tr("homogeneous") : tr("heterogeneous"));
    }
    instructorsFileContents += "\n\n\n";
    for(int attrib = 0; attrib < dataOptions->numAttributes; attrib++)
    {
        QString questionWithResponses = tr("Attribute ") + QString::number(attrib+1) + "\n" +
                                        dataOptions->attributeQuestionText.at(attrib) + "\n" + tr("Responses:");
        for(int response = 0; response < dataOptions->attributeQuestionResponses[attrib].size(); response++)
        {
            if((dataOptions->attributeType[attrib] == DataOptions::AttributeType::ordered) ||
                (dataOptions->attributeType[attrib] == DataOptions::AttributeType::multiordered))
            {
                questionWithResponses += "\n\t" + dataOptions->attributeQuestionResponses[attrib].at(response);
            }
            else if((dataOptions->attributeType[attrib] == DataOptions::AttributeType::categorical) ||
                    (dataOptions->attributeType[attrib] == DataOptions::AttributeType::multicategorical))
            {
                questionWithResponses += "\n\t" + (response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26)));
                questionWithResponses += ". " + dataOptions->attributeQuestionResponses[attrib].at(response);
            }
        }
        questionWithResponses += "\n\n\n";
        instructorsFileContents += questionWithResponses;
    }

    studentsFileContents = "";

    // get team numbers in the order that they are currently displayed/sorted
    QList<int> teamDisplayNum;
    teamDisplayNum.reserve(teams.size());
    for(int row = 0; row < teams.size(); row++)
    {
        int team = 0;
        while(teamDataTree->topLevelItem(row)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() != team)
        {
            team++;
        }
        teamDisplayNum << teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt();
    }

    // get the relevant gender terminology
    QStringList genderOptions;
    if(dataOptions->genderType == GenderType::biol)
    {
        genderOptions = QString(BIOLGENDERS7CHAR).split('/');
    }
    else if(dataOptions->genderType == GenderType::adult)
    {
        genderOptions = QString(ADULTGENDERS7CHAR).split('/');
    }
    else if(dataOptions->genderType == GenderType::child)
    {
        genderOptions = QString(CHILDGENDERS7CHAR).split('/');
    }
    else //if(dataOptions->genderType == GenderType::pronoun)
    {
        genderOptions = QString(PRONOUNS7CHAR).split('/');
    }

    //loop through every team
    for(int teamNum = 0; teamNum < teams.size(); teamNum++)
    {
        int team = teamDisplayNum.at(teamNum);
        instructorsFileContents += tr("Team ") + teams[team].name + tr("  -  Score = ") + QString::number(double(teams[team].score), 'f', 2) + "\n\n";
        studentsFileContents += tr("Team ") + teams[team].name + "\n\n";

        //loop through each teammate in the team
        for(int teammate = 0; teammate < teams[team].size; teammate++)
        {
            const auto &thisStudent = students[teams[team].studentIndexes.at(teammate)];
            if(dataOptions->genderIncluded)
            {
                instructorsFileContents += " " + genderOptions.at(static_cast<int>(thisStudent.gender)) + " ";
            }
            if(dataOptions->URMIncluded)
            {
                if(thisStudent.URM)
                {
                    instructorsFileContents += " URM ";
                }
                else
                {
                    instructorsFileContents += "     ";
                }
            }
            for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++)
            {
                auto value = thisStudent.attributeVals[attribute].constBegin();
                if(*value != -1)
                {
                    if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered)
                    {
                        instructorsFileContents += (QString::number(*value)).leftJustified(3);
                    }
                    else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::categorical)
                    {
                        instructorsFileContents += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))).leftJustified(3) :
                                                                     (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)))).leftJustified(3);
                    }
                    else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical)
                    {
                        const auto lastValue = thisStudent.attributeVals[attribute].constEnd();
                        QString attributeList;
                        while(value != lastValue)
                        {
                            attributeList += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))) :
                                                               (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26))));
                            value++;
                            if(value != lastValue)
                            {
                                 attributeList += ",";
                            }
                        }
                        instructorsFileContents += attributeList.leftJustified(3);
                    }
                    else if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)
                    {
                        const auto lastValue = thisStudent.attributeVals[attribute].constEnd();
                        QString attributeList;
                        while(value != lastValue)
                        {
                            attributeList += QString::number(*value);
                            value++;
                            if(value != lastValue)
                            {
                                 attributeList += ",";
                            }
                        }
                        instructorsFileContents += attributeList.leftJustified(3);
                    }
                }
                else
                {
                    instructorsFileContents += (QString("?")).leftJustified(3);
                }
            }
            int nameSize = int((thisStudent.firstname + " " + thisStudent.lastname).size());
            instructorsFileContents += "\t" + thisStudent.firstname + " " + thisStudent.lastname +
                    QString(std::max(2,30-nameSize), ' ') + thisStudent.email + "\n";
            studentsFileContents += thisStudent.firstname + " " + thisStudent.lastname +
                    QString(std::max(2,30-nameSize), ' ') + thisStudent.email + "\n";
            spreadsheetFileContents += thisStudent.section + "\t" + teams[team].name + "\t" + thisStudent.firstname +
                    " " + thisStudent.lastname + "\t" + thisStudent.email + "\n";
        }
        if(!dataOptions->dayNames.isEmpty())
        {
            instructorsFileContents += "\n" + tr("Availability:") + "\n            ";
            studentsFileContents += "\n" + tr("Availability:") + "\n            ";

            for(const auto &dayName : dataOptions->dayNames)
            {
                // using first 3 characters in day name as abbreviation
                instructorsFileContents += "  " + dayName.left(3) + "  ";
                studentsFileContents += "  " + dayName.left(3) + "  ";
            }
            instructorsFileContents += "\n";
            studentsFileContents += "\n";

            for(int time = 0; time < dataOptions->timeNames.size(); time++)
            {
                instructorsFileContents += dataOptions->timeNames.at(time) + QString((11-dataOptions->timeNames.at(time).size()), ' ');
                studentsFileContents += dataOptions->timeNames.at(time) + QString((11-dataOptions->timeNames.at(time).size()), ' ');
                for(int day = 0; day < dataOptions->dayNames.size(); day++)
                {
                    QString percentage;
                    if(teams[team].size > teams[team].numStudentsWithAmbiguousSchedules)
                    {
                        percentage = QString::number((100*teams[team].numStudentsAvailable[day][time]) /
                                                     (teams[team].size-teams[team].numStudentsWithAmbiguousSchedules)) + "% ";
                    }
                    else
                    {
                        percentage = "?";
                    }
                    QStringView left3 = QStringView{dataOptions->dayNames.at(day).left(3)};
                    instructorsFileContents += QString((4+left3.size())-percentage.size(), ' ') + percentage;
                    studentsFileContents += QString((4+left3.size())-percentage.size(), ' ') + percentage;
                }
                instructorsFileContents += "\n";
                studentsFileContents += "\n";
            }
        }
        instructorsFileContents += "\n\n";
        studentsFileContents += "\n\n";
    }
}


//////////////////
//Setup printer and then print paginated file(s) in boxes
//////////////////
void TeamsTabItem::printFiles(bool printInstructorsFile, bool printStudentsFile, bool printSpreadsheetFile, bool printToPDF)
{
    // connecting to the printer is spun off into a separate thread because sometimes it causes ~30 second hang
    // message box explains what's happening
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setText(printToPDF? tr("Setting up PDF writer...") : tr("Connecting to printer..."));
    msgBox->setStandardButtons(QMessageBox::NoButton);
    msgBox->setModal(false);
    msgBox->show();
    QEventLoop loop;
    connect(this, &TeamsTabItem::connectedToPrinter, &loop, &QEventLoop::quit);
    QPrinter *printer = nullptr;
    QFuture<QPrinter*> future = QtConcurrent::run(&TeamsTabItem::setupPrinter, this);
    loop.exec();
    printer = future.result();
    msgBox->close();
    msgBox->deleteLater();
    QApplication::restoreOverrideCursor();

    bool doIt;
    QString baseFileName;
    if(printToPDF)
    {
        printer->setOutputFormat(QPrinter::PdfFormat);
        baseFileName = QFileDialog::getSaveFileName(this, tr("Choose a location and base filename"), "", tr("PDF File (*.pdf);;All Files (*)"));
        doIt = !(baseFileName.isEmpty());
    }
    else
    {
        printer->setOutputFormat(QPrinter::NativeFormat);
        QPrintDialog printDialog(printer);
        printDialog.setWindowTitle(tr("Print"));
        doIt = (printDialog.exec() == QDialog::Accepted);
    }

    if(doIt)
    {
        QFont printFont = PRINTFONT;

        if(printInstructorsFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" +
                                   QFileInfo(baseFileName).completeBaseName() + "_instructor." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            printOneFile(instructorsFileContents, "\n\n\n", printFont, printer);
        }
        if(printStudentsFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" +
                                   QFileInfo(baseFileName).completeBaseName() + "_student." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            printOneFile(studentsFileContents, "\n\n\n", printFont, printer);

        }
        if(printSpreadsheetFile)
        {
            if(printToPDF)
            {
                QString fileName = QFileInfo(baseFileName).path() + "/" +
                                   QFileInfo(baseFileName).completeBaseName() + "_spreadsheet." + QFileInfo(baseFileName).suffix();
                printer->setOutputFileName(fileName);
            }
            QTextDocument textDocument(spreadsheetFileContents, this);
            printFont.setPointSize(PRINTOUT_FONTSIZE);
            textDocument.setDefaultFont(printFont);
            printer->setPageOrientation(QPageLayout::Landscape);
            textDocument.print(printer);
        }
    }
    delete printer;
}

QPrinter* TeamsTabItem::setupPrinter()
{
    auto *printer = new QPrinter(QPrinter::HighResolution);
    printer->setPageOrientation(QPageLayout::Portrait);
    emit connectedToPrinter();
    return printer;
}

void TeamsTabItem::printOneFile(const QString &file, const QString &delimiter, QFont &font, QPrinter *printer)
{
    QPainter painter(printer);
    painter.setFont(font);
    QFont titleFont = font;
    titleFont.setBold(true);
    int LargeGap = printer->logicalDpiY()/2, MediumGap = LargeGap/2, SmallGap = MediumGap/2;
    int pageHeight = painter.window().height() - 2*LargeGap;

    QStringList eachTeam = file.split(delimiter, Qt::SkipEmptyParts);

    //paginate the output
    QStringList currentPage;
    QList<QStringList> pages;
    int y = 0;
    QStringList::const_iterator it = eachTeam.cbegin();
    while (it != eachTeam.cend())
    {
        //calculate height on page of this team text
        int textWidth = painter.window().width() - 2*LargeGap - 2*SmallGap;
        int maxHeight = painter.window().height();
        QRect textRect = painter.boundingRect(0, 0, textWidth, maxHeight, Qt::TextWordWrap, *it);
        int height = textRect.height() + 2*SmallGap;
        if(y + height > pageHeight && !currentPage.isEmpty())
        {
            pages.push_back(currentPage);
            currentPage.clear();
            y = 0;
        }
        currentPage.push_back(*it);
        y += height + MediumGap;
        ++it;
    }
    if (!currentPage.isEmpty())
    {
        pages.push_back(currentPage);
    }

    //print each page, 1 at a time
    for (int pagenum = 0; pagenum < pages.size(); pagenum++)
    {
        if (pagenum > 0)
        {
            printer->newPage();
        }
        QTransform savedTransform = painter.worldTransform();
        painter.translate(0, LargeGap);
        QStringList::const_iterator it = pages[pagenum].cbegin();
        while (it != pages[pagenum].cend())
        {
            QString title = it->left(it->indexOf('\n')) + " ";
            QString body = it->right(it->size() - (it->indexOf('\n')+1));
            int boxWidth = painter.window().width() - 2*LargeGap;
            int textWidth = boxWidth - 2*SmallGap;
            int maxHeight = painter.window().height();
            QRect titleRect = painter.boundingRect(LargeGap+SmallGap, SmallGap, textWidth, maxHeight, Qt::TextWordWrap, title);
            QRect bodyRect = painter.boundingRect(LargeGap+SmallGap, SmallGap+titleRect.height(), textWidth, maxHeight, Qt::TextWordWrap, body);
            int boxHeight = titleRect.height() + bodyRect.height() + 2 * SmallGap;
            painter.setPen(QPen(Qt::black, 2, Qt::SolidLine));
            painter.setBrush(Qt::white);
            painter.drawRect(LargeGap, 0, boxWidth, boxHeight);
            painter.setFont(titleFont);
            painter.drawText(titleRect, Qt::TextWordWrap, title);
            painter.setFont(font);
            painter.drawText(bodyRect, Qt::TextWordWrap, body);
            painter.translate(0, boxHeight);
            painter.translate(0, MediumGap);
            ++it;
        }
        painter.setWorldTransform(savedTransform);
        painter.drawText(painter.window(), Qt::AlignHCenter | Qt::AlignBottom, QString::number(pagenum + 1));
    }
}
