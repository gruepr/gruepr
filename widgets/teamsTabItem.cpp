#include "teamsTabItem.h"
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

TeamsTabItem::TeamsTabItem(TeamingOptions *const incomingTeamingOptions, const DataOptions *const incomingDataOptions,
                           TeamRecord incomingTeams[], int incomingNumTeams, StudentRecord incomingStudents[], QWidget *parent) : QWidget(parent)
{
    teamingOptions = new TeamingOptions(*incomingTeamingOptions);   // teamingOptions might change, so need to hold on to values when teams were made
    addedPreventedTeammates = &incomingTeamingOptions->haveAnyPreventedTeammates;    // need ability to modify this setting, to mark when prevented teammates are added
    dataOptions = new DataOptions(*incomingDataOptions);
    teams = incomingTeams;
    numTeams = incomingNumTeams;
    students = incomingStudents;
    numStudents = dataOptions->numStudentsInSystem;

    setContentsMargins(0,0,0,0);
    teamDataLayout = new QVBoxLayout;
    teamDataLayout->setSpacing(6);
    teamDataLayout->setStretch(0,100);
    setLayout(teamDataLayout);

    QString fileAndSectionName = "<html><b>" + tr("File") + ":</b> " + dataOptions->dataFile.fileName();
    if(dataOptions->sectionNames.size() > 1)
    {
        fileAndSectionName += ",  <b>" + tr("Section") + ":</b> " + teamingOptions->sectionName;
    }
    fileAndSectionName += "</html>";
    fileAndSectionLabel = new QLabel(fileAndSectionName, this);
    teamDataLayout->addWidget(fileAndSectionLabel);

    teamDataTree = new TeamTreeWidget(this);
    teamDataTree->setStyleSheet("QHeaderView::section{"
                                "	border-top:0px solid #D8D8D8;"
                                "	border-left:0px solid #D8D8D8;"
                                "	border-right:1px solid black;"
                                "	border-bottom: 1px solid black;"
                                "	background-color:Gainsboro;"
                                "	padding:4px;"
                                "	font-weight:bold;"
                                "	color:black;"
                                "	text-align:center;}"
                                "QHeaderView::down-arrow{"
                                "	image: url(:/icons/down_arrow.png);"
                                "	width:18px;"
                                "	subcontrol-origin:padding;"
                                "	subcontrol-position:bottom left;}"
                                "QHeaderView::up-arrow{"
                                "	image: url(:/icons/up_arrow.png);"
                                "	width:18px;"
                                "	subcontrol-origin:padding;"
                                "	subcontrol-position:top left;}"
                                "QTreeWidget::item:selected{"
                                "	color: black;"
                                "	background-color: #cccccc;}"
                                "QTreeWidget::item:hover{"
                                "	color: black;"
                                "	background-color: #cccccc;}"
                                "QTreeWidget::branch{"
                                "	background-color: white;}"
                                "QTreeView::branch:has-siblings:adjoins-item {"
                                "	border-image: url(:/icons/branch-more.png);}"
                                "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
                                "	border-image: url(:/icons/branch-end.png);}"
                                "QTreeView::branch:has-children:!has-siblings:closed,QTreeView::branch:closed:has-children:has-siblings {"
                                "	border-image: none; "
                                "	image: url(:/icons/branch-closed.png);}"
                                "QTreeView::branch:open:has-children:!has-siblings,QTreeView::branch:open:has-children:has-siblings {"
                                "	border-image: none;"
                                "	image: url(:/icons/branch-open.png);}");
    teamDataTree->setMouseTracking(true);
    teamDataTree->setHeaderHidden(false);
    teamDataTree->header()->setStretchLastSection(false);
    teamDataTree->setDragDropMode(QAbstractItemView::InternalMove);
    teamDataTree->setDropIndicatorShown(false);
    teamDataTree->setAlternatingRowColors(true);
    teamDataTree->setAutoScroll(false);
    teamDataTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    teamDataTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    teamDataLayout->addWidget(teamDataTree);

    dragDropExplanation = new QLabel(tr("Use drag-and-drop to adjust the placement of teams and students."), this);
    teamDataLayout->addWidget(dragDropExplanation);

    teamOptionsLayout = new QHBoxLayout;
    teamOptionsLayout->setSpacing(6);
    teamDataLayout->addLayout(teamOptionsLayout);

    expandAllButton = new QPushButton(tr("Expand All"), this);
    expandAllButton->setToolTip(tr("Show the students in all of the teams"));
    teamOptionsLayout->addWidget(expandAllButton);

    collapseAllButton = new QPushButton(tr("Collapse All"), this);
    collapseAllButton->setToolTip(tr("Show only the summarized data for all of the teams"));
    teamOptionsLayout->addWidget(collapseAllButton);

    teamOptionsLayout->addStretch();
    vertLine = new QFrame(this);
    vertLine->setFrameShape(QFrame::VLine);
    vertLine->setFrameShadow(QFrame::Sunken);
    teamOptionsLayout->addWidget(vertLine);
    teamOptionsLayout->addStretch();

    setNamesLabel = new QLabel(tr("Set team names:"), this);
    teamOptionsLayout->addWidget(setNamesLabel);

    teamnameCategories = QString(TEAMNAMECATEGORIES).split(",");
    teamnameLists = QString(TEAMNAMELISTS).split(';');
    teamnameTypes.clear();
    for(auto &teamnameCategory : teamnameCategories)
    {
        // use the last character as the type signifier, then remove the character from the actual name
        switch((*(teamnameCategory.crbegin())).toLatin1())
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
    teamnamesComboBox->addItems(teamnameCategories);
    teamnamesComboBox->addItem(tr("Custom names"));
    teamnamesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    teamnamesComboBox->setCurrentIndex(0);
    teamnamesComboBox->setToolTip(tr("Change the team names"));
    connect(teamnamesComboBox, QOverload<int>::of(&QComboBox::activated), this, &TeamsTabItem::teamNamesChanged);
    teamOptionsLayout->addWidget(teamnamesComboBox);

    randTeamnamesCheckBox = new QCheckBox(tr("Randomize"), this);
    randTeamnamesCheckBox->setEnabled(false);
    randTeamnamesCheckBox->setChecked(false);
    randTeamnamesCheckBox->setToolTip(tr("Select to randomize the order of the team names"));
    connect(randTeamnamesCheckBox, &QCheckBox::clicked, this, &TeamsTabItem::randomizeTeamnames);
    teamOptionsLayout->addWidget(randTeamnamesCheckBox);

    teamOptionsLayout->addStretch();
    vertLine = new QFrame(this);
    vertLine->setFrameShape(QFrame::VLine);
    vertLine->setFrameShadow(QFrame::Sunken);
    teamOptionsLayout->addWidget(vertLine);
    teamOptionsLayout->addStretch();

    sendToPreventedTeammates = new QPushButton(QIcon(":/icons/notfriends.png"), tr("Load as\nprevented\nteammates"), this);
    sendToPreventedTeammates->setIconSize(SAVEPRINTICONSIZE);
    sendToPreventedTeammates->setToolTip(tr("<html>Send all of these teams to the \"Prevented Teammates\" teaming option so that "
                                            "another set of teams can be created with everyone getting all new teammates</html>"));
    connect(sendToPreventedTeammates, &QPushButton::clicked, this, &TeamsTabItem::makePrevented);
    teamOptionsLayout->addWidget(sendToPreventedTeammates);


    savePrintLayout = new QHBoxLayout;
    savePrintLayout->setSpacing(2);
    teamDataLayout->addLayout(savePrintLayout);
    QFont biggerFont = this->font();
    biggerFont.setPointSize(BIGGERFONTSIZE);

    saveTeamsButton = new QPushButton(QIcon(":/icons/save.png"), tr("Save"), this);
    saveTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    saveTeamsButton->setFont(biggerFont);
    saveTeamsButton->setToolTip(tr("Save this set of teams as text or pdf"));
    connect(saveTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::saveTeams);
    savePrintLayout->addWidget(saveTeamsButton);

    printTeamsButton = new QPushButton(QIcon(":/icons/print.png"), tr("Print"), this);
    printTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    printTeamsButton->setFont(biggerFont);
    printTeamsButton->setToolTip(tr("Send this set of teams to the printer"));
    connect(printTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::printTeams);
    savePrintLayout->addWidget(printTeamsButton);

    teamDataTree->collapseAll();
    teamDataTree->resetDisplay(dataOptions);
    refreshTeamDisplay();
    teamDataTree->sortByColumn(0, Qt::AscendingOrder);
    teamDataTree->headerItem()->setIcon(0, QIcon(":/icons/blank_arrow.png"));
    refreshDisplayOrder();
    connect(teamDataTree, &TeamTreeWidget::swapChildren, this, &TeamsTabItem::swapStudents);
    connect(teamDataTree, &TeamTreeWidget::reorderParents, this, &TeamsTabItem::moveATeam);
    connect(teamDataTree, &TeamTreeWidget::moveChild, this, &TeamsTabItem::moveAStudent);
    connect(teamDataTree, &TeamTreeWidget::updateTeamOrder, this, &TeamsTabItem::refreshDisplayOrder);
    connect(expandAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::expandAll);
    connect(collapseAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::collapseAll);
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
        randTeamnamesCheckBox->setEnabled(index > 7 && index < teamnameLists.size());
    }

    // Get team numbers in the order that they are currently displayed/sorted
    QVector<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    // Set team names to:
    if(index == 0)
    {
        // arabic numbers
        for(int team = 0; team < numTeams; team++)
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
        for(int team = 0; team < numTeams; team++)
        {
            teams[teamDisplayNums.at(team)].name = M[(team+1)/1000]+C[((team+1)%1000)/100]+X[((team+1)%100)/10]+I[((team+1)%10)];
        }
        prevIndex = 1;
    }
    else if(index == 2)
    {
        // hexadecimal numbers
        for(int team = 0; team < numTeams; team++)
        {
            teams[teamDisplayNums.at(team)].name = QString::number(team, 16).toUpper();
        }
        prevIndex = 2;
    }
    else if(index == 3)
    {
        // binary numbers
        const int numDigitsInLargestTeam = QString::number(numTeams-1, 2).size();       // the '-1' is to make the first team 0
        for(int team = 0; team < numTeams; team++)
        {
            teams[teamDisplayNums.at(team)].name = QString::number(team, 2).rightJustified(numDigitsInLargestTeam, '0'); // pad w/ 0 to use same number of digits
        }
        prevIndex = 2;
    }
    else if(index < teamnameLists.size())
    {
        // one of the listed team names from gruepr_structs_and_consts.h
        const TeamNameType teamNameType = randTeamnamesCheckBox->isChecked() ? random_sequeled : teamnameTypes.at(index);
        const QStringList teamNames = teamnameLists.at(index).split(',');

        QVector<int> random_order(teamNames.size());
        if(teamNameType == random_sequeled)
        {
            std::iota(random_order.begin(), random_order.end(), 0);
#ifdef Q_OS_MACOS
            std::random_device randDev;
            std::mt19937 pRNG(randDev());
#endif
#ifdef Q_OS_WIN32
            std::mt19937 pRNG{static_cast<long unsigned int>(time(nullptr))};     //minGW does not play well with std::random_device
#endif
            std::shuffle(random_order.begin(), random_order.end(), pRNG);
        }

        for(int team = 0; team < numTeams; team++)
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
        teamNames.reserve(numTeams);
        for(int team = 0; team < numTeams; team++)
        {
            teamNames << teams[teamDisplayNums.at(team)].name;
        }
        auto *window = new customTeamnamesDialog(numTeams, teamNames, this);

        // If user clicks OK, use these team names, otherwise revert to previous option
        int reply = window->exec();
        if(reply == QDialog::Accepted)
        {
            for(int team = 0; team < numTeams; team++)
            {
                teams[teamDisplayNums.at(team)].name = (window->teamName[team].text().isEmpty()? QString::number(team+1) : window->teamName[team].text());
            }
            prevIndex = teamnameLists.size();
            bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            teamnamesComboBox->setItemText(teamnameLists.size(), tr("Current names"));
            teamnamesComboBox->removeItem(teamnameLists.size()+1);
            teamnamesComboBox->addItem(tr("Custom names"));
            teamnamesComboBox->blockSignals(currentValue);
        }
        else
        {
            bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            teamnamesComboBox->blockSignals(currentValue);
        }

        delete window;
    }

    // Put list of options back to just built-ins plus "Custom names"
    if(teamnamesComboBox->currentIndex() < teamnameLists.size())
    {
        teamnamesComboBox->removeItem(teamnameLists.size()+1);
        teamnamesComboBox->removeItem(teamnameLists.size());
        teamnamesComboBox->addItem(tr("Custom names"));
    }

    // Update team names in table and tooltips
    for(int team = 0; team < numTeams; team++)
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


void TeamsTabItem::makePrevented()
{
    for(int teamNum = 0; teamNum < numTeams; teamNum++)
    {
        for(const auto index1 : qAsConst(teams[teamNum].studentIndexes))
        {
            for(const auto index2 : qAsConst(teams[teamNum].studentIndexes))
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
    for(int row = 0; row < numTeams; row++)
    {
        auto *const teamItem = teamDataTree->topLevelItem(row);
        teamItem->setData(lastCol, TEAMINFO_SORT_ROLE, row);
        teamItem->setData(lastCol, TEAMINFO_DISPLAY_ROLE, QString::number(row));
        teamItem->setText(lastCol, QString::number(row));
    }
}


QVector<int> TeamsTabItem::getTeamNumbersInDisplayOrder()
{
    QVector<int> teamDisplayNums;
    teamDisplayNums.reserve(numTeams);
    const int lastCol = teamDataTree->columnCount()-1;
    for(int order = 0; order < numTeams; order++)
    {
        int row = 0;
        while((teamDataTree->topLevelItem(row)->data(lastCol, TEAMINFO_SORT_ROLE).toInt() != order) && (row < numTeams))
        {
            row++;
        }
        teamDisplayNums << teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt();
    }
    return teamDisplayNums;
}


void TeamsTabItem::swapStudents(int studentAteam, int studentAID, int studentBteam, int studentBID)
{
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

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons/updown_arrow.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    if(studentAteam == studentBteam)
    {
        std::swap(teams[studentAteam].studentIndexes[teams[studentAteam].studentIndexes.indexOf(studentAIndex)],
                  teams[studentBteam].studentIndexes[teams[studentBteam].studentIndexes.indexOf(studentBIndex)]);

        // Re-score the teams and refresh all the info
        gruepr::getTeamScores(students, numStudents, teams, numTeams, teamingOptions, dataOptions);
        teams[studentAteam].refreshTeamInfo(students);
        teams[studentAteam].createTooltip();

        //get the team item in the tree
        QTreeWidgetItem *teamItem = nullptr;
        int row = 0;
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentAteam) && (row < numTeams))
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
        QVector<TeamTreeWidgetItem*> childItems;
        childItems.reserve(numStudentsOnTeam);
        for(int studentNum = 0; studentNum < numStudentsOnTeam; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentAteam].studentIndexes[studentNum]], dataOptions);
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
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentAteam) && (row < numTeams))
        {
            row++;
        }
        teamAItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));
        row = 0;
        while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != studentBteam) && (row < numTeams))
        {
            row++;
        }
        teamBItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));

        //refresh the info for both teams
        gruepr::getTeamScores(students, numStudents, teams, numTeams, teamingOptions, dataOptions);
        teams[studentAteam].refreshTeamInfo(students);
        teams[studentAteam].createTooltip();
        teams[studentBteam].refreshTeamInfo(students);
        teams[studentBteam].createTooltip();

        QString firstStudentName = students[teams[studentAteam].studentIndexes[0]].lastname+students[teams[studentAteam].studentIndexes[0]].firstname;
        teamDataTree->refreshTeam(teamAItem, teams[studentAteam], studentAteam, firstStudentName, dataOptions);
        firstStudentName = students[teams[studentBteam].studentIndexes[0]].lastname+students[teams[studentBteam].studentIndexes[0]].firstname;
        teamDataTree->refreshTeam(teamBItem, teams[studentBteam], studentBteam, firstStudentName, dataOptions);
        teamAItem->setBackgroundColor(teams[studentAteam].score);
        teamBItem->setBackgroundColor(teams[studentBteam].score);

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
        QVector<TeamTreeWidgetItem*> childItems;
        childItems.reserve(std::max(numStudentsOnTeamA, numStudentsOnTeamB));
        for(int studentNum = 0; studentNum < numStudentsOnTeamA; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentAteam].studentIndexes[studentNum]], dataOptions);
            teamAItem->addChild(childItems[studentNum]);
        }
        childItems.clear();
        for(int studentNum = 0; studentNum < numStudentsOnTeamB; studentNum++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[teams[studentBteam].studentIndexes[studentNum]], dataOptions);
            teamBItem->addChild(childItems[studentNum]);
        }
    }

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::moveAStudent(int oldTeam, int studentID, int newTeam)
{
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

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons/updown_arrow.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    //remove student from old team and add to new team
    teams[oldTeam].studentIndexes.removeOne(studentIndex);
    teams[oldTeam].size--;
    teams[newTeam].studentIndexes << studentIndex;
    teams[newTeam].size++;

    //get the team items in the tree
    TeamTreeWidgetItem *oldTeamItem = nullptr, *newTeamItem = nullptr;
    int row = 0;
    while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != oldTeam) && (row < numTeams))
    {
        row++;
    }
    oldTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));
    row = 0;
    while((teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt() != newTeam) && (row < numTeams))
    {
        row++;
    }
    newTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(row));

    //refresh the info for both teams
    gruepr::getTeamScores(students, numStudents, teams, numTeams, teamingOptions, dataOptions);
    teams[oldTeam].refreshTeamInfo(students);
    teams[oldTeam].createTooltip();
    teams[newTeam].refreshTeamInfo(students);
    teams[newTeam].createTooltip();

    QString firstStudentName = students[teams[oldTeam].studentIndexes[0]].lastname+students[teams[oldTeam].studentIndexes[0]].firstname;
    teamDataTree->refreshTeam(oldTeamItem, teams[oldTeam], oldTeam, firstStudentName, dataOptions);
    firstStudentName = students[teams[newTeam].studentIndexes[0]].lastname+students[teams[newTeam].studentIndexes[0]].firstname;
    teamDataTree->refreshTeam(newTeamItem, teams[newTeam], newTeam, firstStudentName, dataOptions);
    oldTeamItem->setBackgroundColor(teams[oldTeam].score);
    newTeamItem->setBackgroundColor(teams[newTeam].score);

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
    QVector<TeamTreeWidgetItem*> childItems;
    childItems.reserve(std::max(numStudentsOnStudentTeam, numStudentsOnNewTeam));
    for(int studentNum = 0; studentNum < numStudentsOnStudentTeam; studentNum++)
    {
        childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
        teamDataTree->refreshStudent(childItems[studentNum], students[teams[oldTeam].studentIndexes[studentNum]], dataOptions);
        oldTeamItem->addChild(childItems[studentNum]);
    }
    childItems.clear();
    for(int studentNum = 0; studentNum < numStudentsOnNewTeam; studentNum++)
    {
        childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
        teamDataTree->refreshStudent(childItems[studentNum], students[teams[newTeam].studentIndexes[studentNum]], dataOptions);
        newTeamItem->addChild(childItems[studentNum]);
    }

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::moveATeam(int teamA, int teamB)
{
    if(teamA == teamB)
    {
        return;
    }

    teamDataTree->setUpdatesEnabled(false);

    //maintain current sort order
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons/updown_arrow.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    // find the teamA and teamB top level items in teamDataTree
    int teamARow=0, teamBRow=0;
    for(int row = 0; row < numTeams; row++)
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
    else if(teamARow - teamBRow == 1)       // dragging just one row above ==> just swap the two
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
    else if(teamARow > teamBRow)            // dragging team onto a team listed earlier in the table
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
    else                                    // dragging team onto a team listed later in the table
    {
        // remember where team B is
        int teamBSortOrder = teamDataTree->topLevelItem(teamBRow)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();

        // backwards from teamB up to teamA, decrement sort column data
        for(int row = teamBRow; row < teamARow; row++)
        {
            int teamAboveRow = teamDataTree->topLevelItem(row)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() - 1;
            teamDataTree->topLevelItem(row)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamAboveRow);
            teamDataTree->topLevelItem(row)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamAboveRow));
            teamDataTree->topLevelItem(row)->setText(teamDataTree->columnCount()-1, QString::number(teamAboveRow));
        }

        // set sort column data for teamA to teamB
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
        teamDataTree->topLevelItem(teamARow)->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
        teamDataTree->topLevelItem(teamARow)->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
    }

    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    // rewrite all of the sort column data, just to be sure (can remove this line?)
    refreshDisplayOrder();

    teamDataTree->setUpdatesEnabled(true);
}


void TeamsTabItem::saveTeams()
{
    createFileContents();
    const int previewLength = 1000;
    QStringList previews = {studentsFileContents.left(previewLength) + "...",
                            instructorsFileContents.mid(instructorsFileContents.indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, previewLength) + "...",
                            spreadsheetFileContents.left(previewLength) + "..."};

    //Open specialized dialog box to choose which file(s) to save
    auto *window = new whichFilesDialog(whichFilesDialog::save, previews, this);
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
            else
            {
                this->window()->setWindowModified(false);
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


void TeamsTabItem::printTeams()
{
    createFileContents();
    const int previewLength = 1000;
    QStringList previews = {studentsFileContents.left(previewLength) + "...",
                            instructorsFileContents.mid(instructorsFileContents.indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, previewLength) + "...",
                            spreadsheetFileContents.left(previewLength) + "..."};

    //Open specialized dialog box to choose which file(s) to print
    auto *window = new whichFilesDialog(whichFilesDialog::print, previews, this);
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
    QVector<TeamTreeWidgetItem*> parentItems;
    parentItems.reserve(numTeams);
    QVector<TeamTreeWidgetItem*> childItems;
    childItems.reserve(numStudents);

    //iterate through teams to update the tree of teams and students
    int studentNum = 0;
    for(int teamNum = 0; teamNum < numTeams; teamNum++)
    {
        const auto &currentTeam = teams[teamNum];
        parentItems[teamNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::team, teamDataTree->columnCount(), currentTeam.score);
        QString firstStudentName = students[currentTeam.studentIndexes[0]].lastname+students[currentTeam.studentIndexes[0]].firstname;
        teamDataTree->refreshTeam(parentItems[teamNum], currentTeam, teamNum, firstStudentName, dataOptions);

        //remove all student items in the team
        for(auto &studentItem : parentItems[teamNum]->takeChildren())
        {
            parentItems[teamNum]->removeChild(studentItem);
            delete studentItem;
        }

        //add new student items
        for(int studentOnTeam = 0, numStudentsOnTeam = currentTeam.size; studentOnTeam < numStudentsOnTeam; studentOnTeam++)
        {
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::student);
            teamDataTree->refreshStudent(childItems[studentNum], students[currentTeam.studentIndexes[studentOnTeam]], dataOptions);
            parentItems[teamNum]->addChild(childItems[studentNum]);
            studentNum++;
        }

        parentItems[teamNum]->setExpanded(false);
    }

    // Finally, put each team in the table for display
    teamDataTree->setUpdatesEnabled(false);
    teamDataTree->clear();
    for(int teamNum = 0; teamNum < numTeams; teamNum++)
    {
        teamDataTree->addTopLevelItem(parentItems[teamNum]);
    }
    for(int column = 0; column < teamDataTree->columnCount(); column++)
    {
        teamDataTree->resizeColumnToContents(column);
    }
    teamDataTree->setUpdatesEnabled(true);

    teamDataTree->setSortingEnabled(true);

    this->window()->setWindowModified(true);
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
            if(dataOptions->attributeType[attrib] == DataOptions::ordered)
            {
                questionWithResponses += "\n\t" + dataOptions->attributeQuestionResponses[attrib].at(response);
            }
            else if(dataOptions->attributeType[attrib] == DataOptions::categorical)
            {
                // show response with a preceding letter (letter repeated for responses after 26)
                questionWithResponses += "\n\t" + (response < 26 ? QString(char(response + 'A')) : QString(char(response%26 + 'A')).repeated(1 + (response/26)));
                questionWithResponses += ". " + dataOptions->attributeQuestionResponses[attrib].at(response);
            }
            else
            {
                // multicategorical--collect all letters
            }
        }
        questionWithResponses += "\n\n\n";
        instructorsFileContents += questionWithResponses;
    }

    studentsFileContents = "";

    // get team numbers in the order that they are currently displayed/sorted
    QVector<int> teamDisplayNum;
    teamDisplayNum.reserve(numTeams);
    for(int row = 0; row < numTeams; row++)
    {
        int team = 0;
        while(teamDataTree->topLevelItem(row)->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() != team)
        {
            team++;
        }
        teamDisplayNum << teamDataTree->topLevelItem(row)->data(0, TEAM_NUMBER_ROLE).toInt();
    }

    //loop through every team
    for(int teamNum = 0; teamNum < numTeams; teamNum++)
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
                if(thisStudent.gender == StudentRecord::woman)
                {
                    instructorsFileContents += "  woman  ";
                }
                else if(thisStudent.gender == StudentRecord::man)
                {
                    instructorsFileContents += "   man   ";
                }
                else if(thisStudent.gender == StudentRecord::nonbinary)
                {
                    instructorsFileContents += " nonbin. ";
                }
                else
                {
                    instructorsFileContents += " unknown ";
                }
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
                const auto *value = thisStudent.attributeVals[attribute].constBegin();
                if(*value != -1)
                {
                    if(dataOptions->attributeType[attribute] == DataOptions::ordered)
                    {
                        instructorsFileContents += (QString::number(*value)).leftJustified(3);
                    }
                    else if(dataOptions->attributeType[attribute] == DataOptions::categorical)
                    {
                        instructorsFileContents += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))).leftJustified(3) :
                                                                     (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)))).leftJustified(3);
                    }
                    else
                    {
                        //multicategorical
                        const auto *const lastValue = thisStudent.attributeVals[attribute].constEnd();
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
                }
                else
                {
                    instructorsFileContents += (QString("?")).leftJustified(3);
                }
            }
            int nameSize = (thisStudent.firstname + " " + thisStudent.lastname).size();
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

            for(int day = 0; day < dataOptions->dayNames.size(); day++)
            {
                // using first 3 characters in day name as abbreviation
                instructorsFileContents += "  " + dataOptions->dayNames.at(day).left(3) + "  ";
                studentsFileContents += "  " + dataOptions->dayNames.at(day).left(3) + "  ";
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
                    instructorsFileContents += QString((4+dataOptions->dayNames.at(day).leftRef(3).size())-percentage.size(), ' ') + percentage;
                    studentsFileContents += QString((4+dataOptions->dayNames.at(day).leftRef(3).size())-percentage.size(), ' ') + percentage;
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
    QFuture<QPrinter*> future = QtConcurrent::run(this, &TeamsTabItem::setupPrinter);
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
        this->window()->setWindowModified(false);
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
    QVector<QStringList> pages;
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
