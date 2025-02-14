#include "teamsTabItem.h"
#include "dialogs/customTeamnamesDialog.h"
#include "gruepr.h"
#include "LMS/canvashandler.h"
#include "widgets/labelWithInstantTooltip.h"
#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QFuture>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QtConcurrentRun>
#include <QVBoxLayout>

const QStringList TeamsTabItem::teamnameCategories = QString(TEAMNAMECATEGORIES).split(",");
const QStringList TeamsTabItem::teamnameLists = QString(TEAMNAMELISTS).split(';');

TeamsTabItem::TeamsTabItem(TeamingOptions &incomingTeamingOptions, const TeamSet &incomingTeamSet, QList<StudentRecord> &incomingStudents,
                           const QStringList &incomingSectionNames, const QString &incomingTabName, QPushButton *letsDoItButton, QWidget *parent)
    : QWidget(parent)
{
    teamingOptions = new TeamingOptions(incomingTeamingOptions);   // teamingOptions might change, so need to hold on to values when teams were made to use in print/save
    sectionNames = incomingSectionNames;
    teams = incomingTeamSet;
    students = incomingStudents;
    numStudents = students.size();
    tabName = incomingTabName;

    init(incomingTeamingOptions, incomingStudents, letsDoItButton, TabType::newTab);
}

TeamsTabItem::TeamsTabItem(const QJsonObject &jsonTeamsTab, TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents,
                           const QStringList &incomingSectionNames, QPushButton *letsDoItButton, QWidget *parent)
    :QWidget(parent)
{
    teamingOptions = new TeamingOptions(jsonTeamsTab["teamingOptions"].toObject());
    sectionNames = incomingSectionNames;
    numStudents = jsonTeamsTab["numStudents"].toInt();
    const QJsonArray studentsArray = jsonTeamsTab["students"].toArray();
    students.reserve(studentsArray.size());
    for(const auto &student : studentsArray) {
        students.emplaceBack(student.toObject());
    }
    QJsonArray teamsArray = jsonTeamsTab["teams"].toArray();
    teams.dataOptions = DataOptions(teamsArray.begin()->toObject()["dataOptions"].toObject());
    teams.reserve(teamsArray.size());
    for(const auto &team : qAsConst(teamsArray)) {
        teams.emplaceBack(&teams.dataOptions, team.toObject(), students);
    }
    randomizedTeamNames = jsonTeamsTab["randomizedNames"].toBool();
    sectionsInTeamNames = jsonTeamsTab["sectionsInNames"].toBool();
    tabName = jsonTeamsTab["tabName"].toString();

    init(incomingTeamingOptions, incomingStudents, letsDoItButton, TabType::fromJSON);
}

void TeamsTabItem::init(TeamingOptions &incomingTeamingOptions, QList<StudentRecord> &incomingStudents, QPushButton *letsDoItButton, TabType tabType)
{
    //pointers to items back out in gruepr, so they can be used for "create new teams with all new teammates"
    externalTeamingOptions = &incomingTeamingOptions;
    externalStudents = &incomingStudents;
    externalDoItButton = letsDoItButton;

    setContentsMargins(0, 0, 0, 0);
    auto *teamDataLayout = new QVBoxLayout;
    teamDataLayout->setContentsMargins(0, 0, 0, 0);
    teamDataLayout->setSpacing(6);
    teamDataLayout->setStretch(0, 1);
    setLayout(teamDataLayout);

    teamDataTree = new TeamTreeWidget(this);
    teamDataLayout->addWidget(teamDataTree);

    auto *rowsLayout = new QHBoxLayout;
    rowsLayout->setSpacing(6);
    teamDataLayout->addLayout(rowsLayout);

    auto helpIcon = new LabelWithInstantTooltip("", this);
    helpIcon->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    helpIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    rowsLayout->addWidget(helpIcon);
    auto *dragDropExplanation = new LabelWithInstantTooltip(tr("Adjust the teams"), this);
    dragDropExplanation->setStyleSheet(QString(LABEL10PTSTYLE) + BIGTOOLTIPSTYLE);
    rowsLayout->addWidget(dragDropExplanation);
    const QString helpText = tr("<html><span style=\"color: black;\">Use drag-and-drop to move students and teams:"
                                 "<ul>"
                                 "<li><u>Drag a student onto a team name</u> to move the student onto that team.</li>"
                                 "<li><u>Drag a student onto a student</u> to swap the locations of the two students.</li>"
                                 "<li><u>Drag a team onto a team</u> to manually reorder the teams.</li>"
                                 "</ul>"
                                 "You can name this team set by double clicking the "
                                 "\"Team set ") + QString::number(teamingOptions->teamsetNumber) + tr("\" button at the top.</span></html>");
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

    auto *expandAllButton = new QPushButton(tr("Expand All Teams"), this);
    expandAllButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    expandAllButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(expandAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::expandAll);
    rowsLayout->addWidget(expandAllButton);

    auto *collapseAllButton = new QPushButton(tr("Collapse All Teams"), this);
    collapseAllButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    collapseAllButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(collapseAllButton, &QPushButton::clicked, teamDataTree, &TeamTreeWidget::collapseAll);
    rowsLayout->addWidget(collapseAllButton);

    auto *horLine = new QFrame(this);
    horLine->setStyleSheet("border-color: " DEEPWATERHEX);
    horLine->setLineWidth(1);
    horLine->setMidLineWidth(1);
    horLine->setFrameShape(QFrame::HLine);
    horLine->setFrameShadow(QFrame::Plain);
    teamDataLayout->addWidget(horLine);

    auto *teamOptionsLayout = new QHBoxLayout;
    teamOptionsLayout->setSpacing(6);
    teamDataLayout->addLayout(teamOptionsLayout);

    teamnamesComboBox = new QComboBox(this);
    teamnamesComboBox->setStyleSheet(COMBOBOXSTYLE);
    teamnamesComboBox->setPlaceholderText(tr("Set team names"));
    teamnamesComboBox->addItems(teamnameCategories);
    teamnamesComboBox->addItem(tr("Custom names..."));
    teamnamesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(teamnamesComboBox, QOverload<int>::of(&QComboBox::activated), this, &TeamsTabItem::changeTeamNames);
    teamOptionsLayout->addWidget(teamnamesComboBox);

    randTeamnamesCheckBox = new QCheckBox(tr("Randomize"), this);
    randTeamnamesCheckBox->setStyleSheet(CHECKBOXSTYLE);
    randTeamnamesCheckBox->setEnabled(false);
    randTeamnamesCheckBox->setChecked(randomizedTeamNames);
    randTeamnamesCheckBox->setToolTip(tr("Select to randomize the order of the team names"));
    connect(randTeamnamesCheckBox, &QCheckBox::clicked, this, &TeamsTabItem::randomizeTeamnames);
    teamOptionsLayout->addWidget(randTeamnamesCheckBox);

    if(teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) {
        addSectionToTeamnamesCheckBox = new QCheckBox(tr("Add Section"), this);
        addSectionToTeamnamesCheckBox->setStyleSheet(CHECKBOXSTYLE);
        addSectionToTeamnamesCheckBox->setChecked(sectionsInTeamNames);
        addSectionToTeamnamesCheckBox->setToolTip(tr("Add the section name to the team names"));
        connect(addSectionToTeamnamesCheckBox, &QCheckBox::toggled, this, &TeamsTabItem::toggleSectionsInTeamNames);
        teamOptionsLayout->addWidget(addSectionToTeamnamesCheckBox);
    }

    teamOptionsLayout->addStretch(1);

    auto *sendToPreventedTeammates = new QPushButton(tr("Create teams with all new teammates"), this);
    sendToPreventedTeammates->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    sendToPreventedTeammates->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sendToPreventedTeammates->setFlat(true);
    sendToPreventedTeammates->setToolTip(tr("<html>Create a new set of teams, adding all of these teammates as \"Prevented Teammates\"</html>"));
    connect(sendToPreventedTeammates, &QPushButton::clicked, this, &TeamsTabItem::makeNewSetWithAllNewTeammates);
    teamOptionsLayout->addWidget(sendToPreventedTeammates);

    auto *savePrintLayout = new QHBoxLayout;
    savePrintLayout->setSpacing(2);
    teamDataLayout->addLayout(savePrintLayout);

    QPixmap whiteSaveFile = QPixmap(":/icons_new/edit_document.png").scaled(SAVEPRINTICONSIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&whiteSaveFile);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(whiteSaveFile.rect(), QColor("white"));
    painter.end();
    auto *saveTeamsButton = new QPushButton(QIcon(whiteSaveFile), tr("Save"), this);
    saveTeamsButton->setStyleSheet(SAVEPRINTBUTTONSTYLE);
    saveTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    saveTeamsButton->setToolTip(tr("Save this set of teams as text or pdf"));
    connect(saveTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::saveTeams);
    savePrintLayout->addWidget(saveTeamsButton);

    auto *printTeamsButton = new QPushButton(QIcon(":/icons_new/print.png"), tr("Print"), this);
    printTeamsButton->setStyleSheet(SAVEPRINTBUTTONSTYLE);
    printTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    printTeamsButton->setToolTip(tr("Send this set of teams to the printer"));
    connect(printTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::printTeams);
    savePrintLayout->addWidget(printTeamsButton);

    auto *postTeamsButton = new QPushButton(QIcon(CanvasHandler::icon()), tr("Post"), this);
    postTeamsButton->setStyleSheet(SAVEPRINTBUTTONSTYLE);
    postTeamsButton->setIconSize(SAVEPRINTICONSIZE);
    postTeamsButton->setToolTip(tr("Post this set of teams to Canvas as a new group set"));
    connect(postTeamsButton, &QPushButton::clicked, this, &TeamsTabItem::postTeamsToCanvas);
    savePrintLayout->addWidget(postTeamsButton);

    teamDataTree->resetDisplay(&teams.dataOptions, teamingOptions);
    if(tabType == TabType::newTab) {
        teamDataTree->sortByColumn(0, Qt::AscendingOrder);
        teamDataTree->headerItem()->setIcon(0, QIcon(":/icons_new/blank_arrow.png"));
    }
    else {
        teamDataTree->sortByColumn(teamDataTree->columnCount() - 1, Qt::AscendingOrder);
        teamDataTree->headerItem()->setIcon(0, QIcon(":/icons_new/upDownButton_white.png"));
    }
    refreshTeamDisplay();
    refreshDisplayOrder();
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) {
        //expand the sections, then collapse the teams
        teamDataTree->expandAll();
        teamDataTree->collapseAll();
    }

    connect(teamDataTree, &TeamTreeWidget::swapStudents, this, &TeamsTabItem::swapStudents);
    connect(teamDataTree, &TeamTreeWidget::reorderTeams, this, &TeamsTabItem::moveATeam);
    connect(teamDataTree, &TeamTreeWidget::moveStudent, this, &TeamsTabItem::moveAStudent);
    connect(teamDataTree, &TeamTreeWidget::updateTeamOrder, this, &TeamsTabItem::refreshDisplayOrder);
}

TeamsTabItem::~TeamsTabItem()
{
    delete teamingOptions;
}


QJsonObject TeamsTabItem::toJson() const
{
    // Get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    QJsonArray teamsArray;
    for(const auto teamNum : teamDisplayNums) {
        auto &team = teams[teamNum];
        teamsArray.append(team.toJson());
    }
    QJsonArray studentsArray;
    for(const auto &student : students) {
        studentsArray.append(student.toJson());
    }

    QJsonObject content {
        {"teamingOptions", teamingOptions->toJson()},
        {"teams", teamsArray},
        {"students", studentsArray},
        {"numStudents", numStudents},
        {"randomizedNames", randomizedTeamNames},
        {"sectionsInNames", sectionsInTeamNames},
        {"tabName", tabName}
    };

    return content;
}


void TeamsTabItem::changeTeamNames(int index)
{
    static int prevIndex = 0;   // hold on to previous index, so we can go back to it if cancelling custom team name dialog box

    if(index != prevIndex) {     // reset the randomize teamnames checkbox if we just moved to a new index
        randTeamnamesCheckBox->setChecked(false);
        randTeamnamesCheckBox->setEnabled(index > 3 && index < teamnameLists.size());
    }

    // Get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    // Set team names to:
    if(index == 0) {
        // arabic numbers
        unsigned int team = 1;
        for(const auto teamNum : teamDisplayNums) {
            teams[teamNum].name = QString::number(team);
            team++;
        }
        prevIndex = 0;
    }
    else if(index == 1) {
        // roman numerals
        const QStringList M = {"","M","MM","MMM"};
        const QStringList C = {"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
        const QStringList X = {"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
        const QStringList I = {"","I","II","III","IV","V","VI","VII","VIII","IX"};
        unsigned int team = 1;
        for(const auto teamNum : teamDisplayNums) {
            teams[teamNum].name = M.at(team/1000) + C.at(((team)%1000)/100) + X.at(((team)%100)/10) + I.at(((team)%10));
            team++;
        }
        prevIndex = 1;
    }
    else if(index == 2) {
        // hexadecimal numbers
        unsigned int team = 0;
        for(const auto teamNum : teamDisplayNums) {
            teams[teamNum].name = QString::number(team, 16).toUpper();
            team++;
        }
        prevIndex = 2;
    }
    else if(index == 3) {
        // binary numbers
        const int numDigitsInLargestTeam = QString::number(int(teams.size())-1, 2).size();       // the '-1' is to make the first team 0
        unsigned int team = 0;
        for(const auto teamNum : teamDisplayNums) {
            teams[teamNum].name = QString::number(team, 2).rightJustified(numDigitsInLargestTeam, '0'); // pad w/ 0 to use same number of digits
            team++;
        }
        prevIndex = 3;
    }
    else if(index < teamnameLists.size()) {
        // one of the listed team names from gruepr_structs_and_consts.h
        const TeamNameType teamNameType = randTeamnamesCheckBox->isChecked() ? TeamNameType::random_sequeled : TEAMNAMETYPES[index];
        const QStringList teamNames = teamnameLists.at(index).split(',');

        QList<int> random_order(teamNames.size());
        if(teamNameType == TeamNameType::random_sequeled) {
            std::iota(random_order.begin(), random_order.end(), 0);
            std::random_device randDev;
            std::mt19937 pRNG(randDev());
            std::shuffle(random_order.begin(), random_order.end(), pRNG);
        }

        unsigned int team = 0;
        for(const auto teamNum : teamDisplayNums) {
            switch(teamNameType) {
                case TeamNameType::numeric:
                    break;
                case TeamNameType::repeated:
                    teams[teamNum].name = (teamNames[team%(teamNames.size())]).repeated((team/teamNames.size())+1);
                    break;
                case TeamNameType::repeated_spaced:
                    teams[teamNum].name = (teamNames[team%(teamNames.size())]+" ").repeated((team/teamNames.size())+1).trimmed();
                    break;
                case TeamNameType::sequeled:
                    teams[teamNum].name = (teamNames[team%(teamNames.size())]) +
                                            ((team/teamNames.size() == 0) ? "" : " " + QString::number(team/teamNames.size()+1));
                    break;
                case TeamNameType::random_sequeled:
                    teams[teamNum].name = (teamNames[random_order[team%(teamNames.size())]]) +
                                            ((team/teamNames.size() == 0) ? "" : " " + QString::number(team/teamNames.size()+1));
                    break;
            }
            team++;
        }
        prevIndex = index;
    }
    else if(teamnamesComboBox->currentText() == tr("Current names")) {
        // Keeping the current custom names
    }
    else {
        // Open custom dialog box to collect teamnames
        QStringList currentTeamNames;
        currentTeamNames.reserve(teams.size());
        for(const auto teamNum : teamDisplayNums) {
            currentTeamNames << teams[teamNum].name;
        }
        auto *window = new customTeamnamesDialog(int(teams.size()), currentTeamNames, this);

        // If user clicks OK, use these team names, otherwise revert to previous option
        const int reply = window->exec();
        if(reply == QDialog::Accepted) {
            unsigned int team = 0;
            for(const auto teamNum : teamDisplayNums) {
                teams[teamNum].name = (window->teamNames[team]);
                team++;
            }
            prevIndex = int(teamnameLists.size());
            const bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            teamnamesComboBox->setItemText(int(teamnameLists.size()), tr("Current names"));
            teamnamesComboBox->removeItem(int(teamnameLists.size())+1);
            teamnamesComboBox->addItem(tr("Custom names..."));
            teamnamesComboBox->blockSignals(currentValue);
        }
        else {
            const bool currentValue = teamnamesComboBox->blockSignals(true);
            teamnamesComboBox->setCurrentIndex(prevIndex);
            randTeamnamesCheckBox->setEnabled(prevIndex > 3 && prevIndex < teamnameLists.size());
            teamnamesComboBox->blockSignals(currentValue);
        }

        delete window;
    }

    // Put list of options back to just built-ins plus "Custom names..."
    if(teamnamesComboBox->currentIndex() < teamnameLists.size()) {
        teamnamesComboBox->removeItem(int(teamnameLists.size())+1);
        teamnamesComboBox->removeItem(int(teamnameLists.size()));
        teamnamesComboBox->addItem(tr("Custom names..."));
    }

    // prepend section names, if applicable
    if((addSectionToTeamnamesCheckBox != nullptr) && addSectionToTeamnamesCheckBox->isChecked()) {
        for(const auto teamNum : teamDisplayNums) {
            auto &team = teams[teamNum];
            const StudentRecord *const firstStudent = findStudentFromID(team.studentIDs.at(0));
            team.name.prepend(firstStudent->section + "-");
        }
    }

    updateTeamNamesInTableAndTooltips();

    // Update team names in table and tooltips
    emit saveState();
}


void TeamsTabItem::toggleSectionsInTeamNames(bool addSectionNames)
{
    sectionsInTeamNames = addSectionNames;

    // Get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    for(const auto teamNum : teamDisplayNums) {
        auto &team = teams[teamNum];
        const StudentRecord *const firstStudent = findStudentFromID(team.studentIDs.at(0));
        if(firstStudent != nullptr) {
            const QString sectionNotifier = firstStudent->section + "-";
            if(addSectionNames) {
                team.name.prepend(sectionNotifier);
            }
            else {
                team.name.replace(team.name.indexOf(sectionNotifier), sectionNotifier.size(), "");
            }
        }
    }

    updateTeamNamesInTableAndTooltips();
}


void TeamsTabItem::randomizeTeamnames(bool random)
{
    randomizedTeamNames = random;
    changeTeamNames(teamnamesComboBox->currentIndex());
}


void TeamsTabItem::updateTeamNamesInTableAndTooltips()
{
    auto item = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    while(item != nullptr) {
        if(item->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
            int teamNum = item->data(0, TEAM_NUMBER_ROLE).toInt();
            teams[teamNum].createTooltip();
            item->setText(0, tr("Team ") + teams[teamNum].name);
            item->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
            item->setData(0, TEAMINFO_DISPLAY_ROLE, tr("Team ") + teams[teamNum].name);
            for(int column = 0, numColsForToolTips = teamDataTree->columnCount()-1; column < numColsForToolTips; column++) {
                item->setToolTip(column, teams[teamNum].tooltip);
            }
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(item));
    }

    teamDataTree->resizeColumnToContents(0);
    return;
}


void TeamsTabItem::swapStudents(const QList<int> &arguments) // QList<int> arguments = studentAteamNum, studentAID, studentBteamNum, studentBID
{
    if(arguments.size() != 4) {
        return;
    }

    int studentATeamNum = arguments.at(0), studentAID = arguments.at(1), studentBTeamNum = arguments.at(2), studentBID = arguments.at(3);

    //Sanity and bounds checks
    if((studentAID == studentBID) ||
        (studentAID < 0) || (studentBID < 0) ||
        (studentATeamNum < 0) || (studentATeamNum > teams.size()) ||
        (studentBTeamNum < 0) || (studentBTeamNum > teams.size())) {
        return;
    }

    //Get references to each team and student
    auto &studentATeam = teams[studentATeamNum];
    auto &studentBTeam = teams[studentBTeamNum];
    const StudentRecord* studentA = findStudentFromID(studentAID);
    if(studentA == nullptr) {
        return;
    }
    const StudentRecord* studentB = findStudentFromID(studentBID);
    if(studentB == nullptr) {
        return;
    }

    //Load undo onto stack and clear redo stack
    const QString UndoTooltip = tr("Undo swapping ") + studentA->firstname + " " + studentA->lastname +
                                tr(" with ") + studentB->firstname + " " + studentB->lastname;
    undoItems.prepend({&TeamsTabItem::swapStudents, {studentATeamNum, studentBID, studentBTeamNum, studentAID}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    refreshDisplayOrder();
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton_white.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    if(studentATeamNum == studentBTeamNum) {
        // just switching placement of two students on the SAME team
        std::swap(studentATeam.studentIDs[studentATeam.studentIDs.indexOf(studentA->ID)],
                  studentBTeam.studentIDs[studentBTeam.studentIDs.indexOf(studentB->ID)]);      //(of course, studentATeam == studentBTeam)

        // Re-score the teams and refresh all the info
        gruepr::calcTeamScores(students, numStudents, teams, teamingOptions);
        teams[studentATeamNum].refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
        teams[studentATeamNum].createTooltip();

        //get the team item in the tree
        auto teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
        while((teamItem != nullptr) &&
               ((teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
                (teamItem->data(0, TEAM_NUMBER_ROLE).toInt() != studentATeamNum))) {
            teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamItem));
        }
        if((teamItem != nullptr) && (teamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team)) {
            //clear and refresh this team's student items in table
            for(auto &child : teamItem->takeChildren()) {
                delete child;
            }
            const int numStudentsOnTeam = studentATeam.size;
            QList<TeamTreeWidgetItem*> childItems;
            childItems.reserve(numStudentsOnTeam);
            for(int studentNum = 0; studentNum < numStudentsOnTeam; studentNum++) {
                //Find each teammate based on the ID and make them a leaf on the old team
                const StudentRecord* teammate = findStudentFromID(studentATeam.studentIDs.at(studentNum));
                if(teammate == nullptr) {
                    continue;
                }
                childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
                teamDataTree->refreshStudent(childItems[studentNum], *teammate, &teams.dataOptions, teamingOptions);
                teamItem->addChild(childItems[studentNum]);
            }
        }
    }
    else {
        //switching students on two different teams
        studentATeam.studentIDs.replace(studentATeam.studentIDs.indexOf(studentA->ID), studentB->ID);
        studentBTeam.studentIDs.replace(studentBTeam.studentIDs.indexOf(studentB->ID), studentA->ID);

        //get the team items in the tree
        auto studentATeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
        while((studentATeamItem != nullptr) &&
               ((studentATeamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
                (studentATeamItem->data(0, TEAM_NUMBER_ROLE).toInt() != studentATeamNum))) {
            studentATeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(studentATeamItem));
        }
        auto studentBTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
        while((studentBTeamItem != nullptr) &&
               ((studentBTeamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
                (studentBTeamItem->data(0, TEAM_NUMBER_ROLE).toInt() != studentBTeamNum))) {
            studentBTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(studentBTeamItem));
        }

        //refresh the info for both teams
        if((studentATeamItem != nullptr) && (studentATeamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team) &&
            (studentBTeamItem != nullptr) && (studentBTeamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team)) {
            gruepr::calcTeamScores(students, numStudents, teams, teamingOptions);
            studentATeam.refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
            studentATeam.createTooltip();
            studentBTeam.refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
            studentBTeam.createTooltip();
            for(const auto &stu : qAsConst(students)) {
                if(studentATeam.studentIDs.first() == stu.ID) {
                    const QString firstStudentName = stu.lastname + stu.firstname;
                    teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::existingTeam, studentATeamItem, studentATeam,
                                              studentATeamNum, firstStudentName, &teams.dataOptions, teamingOptions);
                }
                else if(studentBTeam.studentIDs.first() == stu.ID) {
                    const QString firstStudentName = stu.lastname + stu.firstname;
                    teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::existingTeam, studentBTeamItem, studentBTeam,
                                              studentBTeamNum, firstStudentName, &teams.dataOptions, teamingOptions);
                }
            }
            studentATeamItem->setScoreColor(studentATeam.score);
            studentBTeamItem->setScoreColor(studentBTeam.score);

            //clear and refresh student items on both teams in table
            for(auto &child : studentATeamItem->takeChildren()) {
                delete child;
            }
            for(auto &child : studentBTeamItem->takeChildren()) {
                delete child;
            }
            const int numStudentsOnTeamA = studentATeam.size;
            const int numStudentsOnTeamB = studentBTeam.size;
            QList<TeamTreeWidgetItem*> childItems;
            childItems.reserve(std::max(numStudentsOnTeamA, numStudentsOnTeamB));
            for(int studentNum = 0; studentNum < numStudentsOnTeamA; studentNum++) {
                //Find each teammate based on the ID and make them a leaf on the team
                const StudentRecord* teammate = findStudentFromID(studentATeam.studentIDs.at(studentNum));
                if(teammate == nullptr) {
                    continue;
                }
                childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
                teamDataTree->refreshStudent(childItems[studentNum], *teammate, &teams.dataOptions, teamingOptions);
                studentATeamItem->addChild(childItems[studentNum]);
            }
            childItems.clear();
            for(int studentNum = 0; studentNum < numStudentsOnTeamB; studentNum++) {
                //Find each teammate based on the ID and make them a leaf on the team
                const StudentRecord* teammate = findStudentFromID(studentBTeam.studentIDs.at(studentNum));
                if(teammate == nullptr) {
                    continue;
                }
                childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
                teamDataTree->refreshStudent(childItems[studentNum], *teammate, &teams.dataOptions, teamingOptions);
                studentBTeamItem->addChild(childItems[studentNum]);
            }
        }
    }

    teamDataTree->setUpdatesEnabled(true);
    emit saveState();
}


void TeamsTabItem::moveAStudent(const QList<int> &arguments) // QList<int> arguments = int oldTeam, int studentID, int newTeam
{
    if(arguments.size() != 3) {
        return;
    }

    int oldTeamNum = arguments.at(0), studentID = arguments.at(1), newTeamNum = arguments.at(2);

    //Sanity and bounds check on teamNums
    if((oldTeamNum == newTeamNum) ||
        (oldTeamNum < 0) || (oldTeamNum > teams.size()) ||
        (newTeamNum < 0) || (newTeamNum > teams.size())) {
        return;
    }

    auto &oldTeam = teams[oldTeamNum];
    auto &newTeam = teams[newTeamNum];

    //Don't leave an empty team
    if(oldTeam.size == 1) {
        return;
    }

    const StudentRecord* student = findStudentFromID(studentID);
    if(student == nullptr) {
        return;
    }

    //Load undo onto stack and clear redo stack
    const QString UndoTooltip = tr("Undo moving ") + student->firstname + " " + student->lastname +
                                tr(" from Team ") + teams.at(oldTeamNum).name + tr(" to Team ") + teams.at(newTeamNum).name;
    undoItems.prepend({&TeamsTabItem::moveAStudent, {newTeamNum, studentID, oldTeamNum}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order
    refreshDisplayOrder();
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton_white.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    //remove student from old team and add to new team
    oldTeam.studentIDs.removeOne(studentID);
    oldTeam.size--;
    newTeam.studentIDs << studentID;
    newTeam.size++;

    //get the team items in the tree
    auto oldTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    while((oldTeamItem != nullptr) &&
           ((oldTeamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
            (oldTeamItem->data(0, TEAM_NUMBER_ROLE).toInt() != oldTeamNum))) {
        oldTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(oldTeamItem));
    }
    auto newTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    while((newTeamItem != nullptr) &&
           ((newTeamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
            (newTeamItem->data(0, TEAM_NUMBER_ROLE).toInt() != newTeamNum))) {
        newTeamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(newTeamItem));
    }

    //refresh the info, tooltip, treeitem for both teams
    if((oldTeamItem != nullptr) && (oldTeamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team) &&
        (newTeamItem != nullptr) && (newTeamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team)) {
        gruepr::calcTeamScores(students, numStudents, teams, teamingOptions);
        oldTeam.refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
        oldTeam.createTooltip();
        newTeam.refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
        newTeam.createTooltip();
        for(const auto &stu : qAsConst(students)) {
            if(oldTeam.studentIDs.first() == stu.ID) {
                const QString firstStudentName = stu.lastname + stu.firstname;
                teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::existingTeam, oldTeamItem, oldTeam,
                                          oldTeamNum, firstStudentName, &teams.dataOptions, teamingOptions);
            }
            else if(newTeam.studentIDs.first() == stu.ID) {
                const QString firstStudentName = stu.lastname + stu.firstname;
                teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::existingTeam, newTeamItem, newTeam,
                                          newTeamNum, firstStudentName, &teams.dataOptions, teamingOptions);
            }
        }
        oldTeamItem->setScoreColor(oldTeam.score);
        newTeamItem->setScoreColor(newTeam.score);

        //clear and refresh student items on both teams in table
        for(auto &child : oldTeamItem->takeChildren()) {
            delete child;
        }
        for(auto &child : newTeamItem->takeChildren()) {
            delete child;
        }
        //clear and refresh student items on both teams in table
        const int numStudentsOnOldTeam = oldTeam.size;
        const int numStudentsOnNewTeam = newTeam.size;
        QList<TeamTreeWidgetItem*> childItems;
        childItems.reserve(std::max(numStudentsOnOldTeam, numStudentsOnNewTeam));
        for(int studentNum = 0; studentNum < numStudentsOnOldTeam; studentNum++) {
            //Find each teammate based on the ID and make them a leaf on the old team
            const StudentRecord* teammate = findStudentFromID(oldTeam.studentIDs.at(studentNum));
            if(teammate == nullptr) {
                continue;
            }
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
            teamDataTree->refreshStudent(childItems[studentNum], *teammate, &teams.dataOptions, teamingOptions);
            oldTeamItem->addChild(childItems[studentNum]);
        }
        childItems.clear();
        for(int studentNum = 0; studentNum < numStudentsOnNewTeam; studentNum++) {
            //Find each teammate based on the ID and make them a leaf on the new team
            const StudentRecord* teammate = findStudentFromID(newTeam.studentIDs.at(studentNum));
            if(teammate == nullptr) {
                continue;
            }
            childItems[studentNum] = new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
            teamDataTree->refreshStudent(childItems[studentNum], *teammate, &teams.dataOptions, teamingOptions);
            newTeamItem->addChild(childItems[studentNum]);
        }
    }

    teamDataTree->setUpdatesEnabled(true);
    emit saveState();
}


void TeamsTabItem::moveATeam(const QList<int> &arguments)  // QList<int> arguments = int teamA, int teamB
{
    if(arguments.size() != 2) {
        return;
    }

    int teamANum = arguments.at(0), teamBNum = arguments.at(1);   // teamB = SORT_TO_END if moving to the last row

    //Sanity and bounds check on teamNums
    if((teamANum == teamBNum) ||
        (teamANum < 0) || (teamANum > teams.size()) ||
        (teamBNum < 0) || ((teamBNum > teams.size()) && teamBNum != SORT_TO_END)) {
        return;
    }

    // find the teamA and teamB items in teamDataTree, saving a pointer to the item and their visual order# in the display
    auto teamAItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    while((teamAItem != nullptr) &&
           ((teamAItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
            (teamAItem->data(0, TEAM_NUMBER_ROLE).toInt() != teamANum))) {
        teamAItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamAItem));
    }
    const int teamASortOrder = ((teamAItem != nullptr) ? (teamAItem->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt()) : -1);
    auto teamBItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    int largestSortOrder = 0;
    while((teamBItem != nullptr) &&
           ((teamBItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
            (teamBItem->data(0, TEAM_NUMBER_ROLE).toInt() != teamBNum))) {
        if(teamBItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
            largestSortOrder = teamBItem->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt();
        }
        teamBItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamBItem));
    }
    const int teamBSortOrder = ((teamBItem != nullptr) ? (teamBItem->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt()) : (largestSortOrder + 1));

    if((teamASortOrder == -1) || (teamBSortOrder - teamASortOrder == 1) || (teamAItem == nullptr)) {
        // error or dragging just one row down ==> no change in order
        return;
    }

    //Load undo onto stack and clear redo stack
    auto itemBelowTeamA = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamAItem));
    while((itemBelowTeamA != nullptr) && (itemBelowTeamA->treeItemType != TeamTreeWidgetItem::TreeItemType::team)) {
        itemBelowTeamA = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(itemBelowTeamA));
    }
    const int teamBelowTeamA = ((itemBelowTeamA != nullptr) && itemBelowTeamA->treeItemType == TeamTreeWidgetItem::TreeItemType::team) ?
                                   itemBelowTeamA->data(0, TEAM_NUMBER_ROLE).toInt() : SORT_TO_END;
    const QString UndoTooltip = tr("Undo moving Team ") + teams[teamANum].name;
    undoItems.prepend({&TeamsTabItem::moveATeam, {teamANum, teamBelowTeamA}, UndoTooltip});
    undoButton->setEnabled(true);
    undoButton->setToolTip(UndoTooltip);
    redoItems.clear();
    redoButton->setEnabled(false);
    redoButton->setToolTip("");

    teamDataTree->setUpdatesEnabled(false);

    //hold current sort order, then adjust sort data for teamA and teamB, then resort
    teamDataTree->headerItem()->setIcon(teamDataTree->sortColumn(), QIcon(":/icons_new/upDownButton_white.png"));
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);
    if(teamASortOrder > teamBSortOrder) {
        // dragging team onto a team listed earlier in the table
        // backwards from item above teamA up to teamB, increment sort column data for every team item
        auto teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemAbove(teamAItem));
        while((teamItem != nullptr) &&
               ((teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
                (teamItem->data(0, TEAM_NUMBER_ROLE).toInt() != teamBNum))) {
            if(teamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
                const int teamItemNewSortOrder = teamItem->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() + 1;
                teamItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamItemNewSortOrder);
                teamItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamItemNewSortOrder));
                teamItem->setText(teamDataTree->columnCount()-1, QString::number(teamItemNewSortOrder));
            }
            teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemAbove(teamItem));
        }
        // increment sort column data for teamB
        teamBItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder + 1);
        teamBItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder + 1));
        teamBItem->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder + 1));
        // set sort column data for teamA to teamB
        teamAItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
        teamAItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
        teamAItem->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
    }
    else {
        // dragging team onto a team listed later in the table (possibly bottom of the table)
        // from item below teamA down to teamB, decrement sort column data for every team item
        auto teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamAItem));
        while((teamItem != nullptr) &&
               ((teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) ||
                (teamItem->data(0, TEAM_NUMBER_ROLE).toInt() != teamBNum))) {
            if(teamItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
                const int teamItemNewSortOrder = teamItem->data(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE).toInt() - 1;
                teamItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamItemNewSortOrder);
                teamItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamItemNewSortOrder));
                teamItem->setText(teamDataTree->columnCount()-1, QString::number(teamItemNewSortOrder));
            }
            teamItem = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(teamItem));
        }
        // set sort column data for teamA to teamB - 1
        teamAItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder - 1);
        teamAItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder - 1));
        teamAItem->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder - 1));
        // re-set sort column data for teamB (if not placing teamA at bottom of table)
        if(teamBItem != nullptr) {
            teamBItem->setData(teamDataTree->columnCount()-1, TEAMINFO_SORT_ROLE, teamBSortOrder);
            teamBItem->setData(teamDataTree->columnCount()-1, TEAMINFO_DISPLAY_ROLE, QString::number(teamBSortOrder));
            teamBItem->setText(teamDataTree->columnCount()-1, QString::number(teamBSortOrder));
        }
    }
    teamDataTree->sortByColumn(teamDataTree->columnCount()-1, Qt::AscendingOrder);

    // rewrite all of the sort column data
    refreshDisplayOrder();

    teamDataTree->setUpdatesEnabled(true);
    emit saveState();
}


void TeamsTabItem::undoRedoDragDrop()
{
    const bool undoingSomething = (sender() == undoButton);

    auto redoItemsCopy = redoItems;

    auto *itemStack = (undoingSomething? &undoItems : &redoItems);
    if(itemStack->isEmpty()) {
        return;
    }
    auto item = itemStack->takeFirst();

    (this->*(item.action))(item.arguments);

    redoItems = redoItemsCopy;      // performing the action (in the previous line) puts it back on the undo list and clears the redo list
    if(undoingSomething) {
        redoItems.prepend(undoItems.takeFirst());
        redoItems.first().ToolTip.replace(tr("Undo"), tr("Redo"));
    }
    else {    // redoingSomething
        redoItems.removeFirst();
    }

    redoButton->setEnabled(!redoItems.isEmpty());
    redoButton->setToolTip(redoItems.isEmpty()? "" : redoItems.first().ToolTip);
    undoButton->setEnabled(!undoItems.isEmpty());
    undoButton->setToolTip(undoItems.isEmpty()? "" : undoItems.first().ToolTip);
    emit saveState();
}


void TeamsTabItem::makeNewSetWithAllNewTeammates()
{
    if(teamingOptions->haveAnyRequiredTeammates || teamingOptions->haveAnyRequestedTeammates) {
        const bool yesDoIt = grueprGlobal::warningMessage(this, "gruepr", tr("This will remove all of the current ") + (teamingOptions->haveAnyRequiredTeammates? tr("required") : "") +
                                                                             ((teamingOptions->haveAnyRequiredTeammates && teamingOptions->haveAnyRequestedTeammates)? tr(" and ") : "") +
                                                                             ((teamingOptions->haveAnyRequestedTeammates)? tr("requested") : "") +
                                                                            tr(" teammate settings. Do you want to continue?"),
                                                            tr("Yes"), tr("No"));
        if(yesDoIt) {
            for(auto &student : *externalStudents) {
                student.requiredWith.clear();
                student.requestedWith.clear();
            }
        }
        else {
            return;
        }
    }

    for(const auto &team : qAsConst(teams)) {
        for(const auto ID1 : qAsConst(team.studentIDs)) {
            for(const auto ID2 : qAsConst(team.studentIDs)) {
                if(ID1 != ID2) {
                    StudentRecord* stu = nullptr;
                    for(auto &student1 : *externalStudents) {
                        if(student1.ID == ID1) {
                            stu = &student1;
                        }
                    }
                    if(stu == nullptr) {
                        continue;
                    }
                    stu->preventedWith << ID2;
                }
            }
        }
    }

    externalTeamingOptions->haveAnyPreventedTeammates = true;
    externalDoItButton->animateClick();
}


void TeamsTabItem::saveTeams()
{
    QStringList fileContents = createStdFileContents();
    const QStringList previews = {fileContents[studentFile].left(FILEPREVIEWLENGTH) + "\n...",
                                  fileContents[instructorFile].left(FILEPREVIEWLENGTH/2) + "\n...\n" +
                                      fileContents[instructorFile].mid(fileContents[instructorFile].indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, FILEPREVIEWLENGTH/2) + "\n...",
                                  fileContents[spreadsheetFile].left(FILEPREVIEWLENGTH) + "\n...",
                                  tr("(Custom contents)")};

    //Open specialized dialog box to choose which file(s) to save
    auto *window = new WhichFilesDialog(WhichFilesDialog::Action::save, &teams.dataOptions, teamingOptions->sectionType, previews, this);
    if(window->exec() == QDialog::Accepted) {
        if(window->fileType == WhichFilesDialog::FileType::custom) {
            fileContents[customFile] = createCustomFileContents(window->customFileOptions);
        }
        if(window->pdf) {
            //save as formatted pdf files
            printFiles(fileContents, window->fileType, PrintType::printToPDF);
        }
        else {
            //save to text files
            const QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a location and filename"), "",
                                                                  tr("Text File (*.txt);;All Files (*)"));
            if (!fileName.isEmpty()) {
                bool problemSaving = false;
                QFile saveFile(fileName);
                if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&saveFile);
                    if(window->fileType == WhichFilesDialog::FileType::instructor) {
                        out << fileContents[instructorFile];
                    }
                    else if(window->fileType == WhichFilesDialog::FileType::student) {
                        out << fileContents[studentFile];
                    }
                    else if(window->fileType == WhichFilesDialog::FileType::spreadsheet) {
                        out << fileContents[spreadsheetFile];
                    }
                    else if(window->fileType == WhichFilesDialog::FileType::custom) {
                        out << fileContents[customFile];
                    }
                    saveFile.close();
                }
                else {
                    problemSaving = true;
                }

                if(problemSaving) {
                    grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
                }
            }
        }
        delete window;
    }
}


void TeamsTabItem::printTeams()
{
    QStringList fileContents = createStdFileContents();
    const QStringList previews = {fileContents[studentFile].left(FILEPREVIEWLENGTH) + "\n...",
                                  fileContents[instructorFile].left(FILEPREVIEWLENGTH/2) + "\n...\n" +
                                      fileContents[instructorFile].mid(fileContents[instructorFile].indexOf("\n\n\nteam ", 0, Qt::CaseInsensitive)+3, FILEPREVIEWLENGTH/2) + "\n...",
                                  fileContents[spreadsheetFile].left(FILEPREVIEWLENGTH) + "\n...",
                                  tr("(Custom contents)")};

    //Open specialized dialog box to choose which file(s) to print
    auto *window = new WhichFilesDialog(WhichFilesDialog::Action::print, &teams.dataOptions, teamingOptions->sectionType, previews, this);
    if(window->exec() == QDialog::Accepted) {
        if(window->fileType == WhichFilesDialog::FileType::custom) {
            fileContents[customFile] = createCustomFileContents(window->customFileOptions);
        }
        printFiles(fileContents, window->fileType, PrintType::printer);
    }
    delete window;
}


void TeamsTabItem::postTeamsToCanvas()
{

    if(!grueprGlobal::internetIsGood()) {
        return;
    }

    //create canvasHandler and authenticate
    auto *canvas = new CanvasHandler(this);
    if(!canvas->authenticate()) {
        canvas->deleteLater();
        return;
    }

    //ask the user in which course we're creating the teams
    auto *busyBox = canvas->actionDialog();
    QList<CanvasHandler::CanvasCourse> canvasCourses = canvas->getCourses();
    canvas->actionComplete(busyBox);
    auto *canvasCoursesDialog = new QDialog(this);
    canvasCoursesDialog->setWindowTitle(tr("Choose Canvas course"));
    canvasCoursesDialog->setWindowIcon(QIcon(CanvasHandler::icon()));
    canvasCoursesDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout;
    int i = 1;
    auto *label = new QLabel(tr("In which course should these teams be created?"), canvasCoursesDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *coursesComboBox = new QComboBox(canvasCoursesDialog);
    coursesComboBox->setStyleSheet(COMBOBOXSTYLE);
    for(const auto &canvasCourse : qAsConst(canvasCourses)) {
        coursesComboBox->addItem(canvasCourse.name);
        coursesComboBox->setItemData(i++, QString::number(canvasCourse.numStudents) + " students", Qt::ToolTipRole);
    }
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, canvasCoursesDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesComboBox);
    vLayout->addWidget(buttonBox);
    canvasCoursesDialog->setLayout(vLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, canvasCoursesDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, canvasCoursesDialog, &QDialog::reject);
    if((canvasCoursesDialog->exec() == QDialog::Rejected)) {
        return;
    }

    // get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();
    // assemble each team in display order
    const int numTeams = teams.size();
    QStringList teamNames;
    teamNames.reserve(numTeams);
    QList<StudentRecord> teamRoster;
    teamRoster.reserve(numTeams);
    QList<QList<StudentRecord>> teamRosters;
    teamRosters.reserve(numTeams);
    for(const auto teamNum : qAsConst(teamDisplayNums)) {
        const auto &team = teams.at(teamNum);
        teamNames << team.name;
        teamRoster.clear();
        //loop through each teammate in the team
        for(const auto studentID : qAsConst(team.studentIDs)) {
            teamRoster << *findStudentFromID(studentID);
        }
        teamRosters << teamRoster;
    }

    busyBox = canvas->actionDialog();
    const QSize iconSize = canvas->actionDialogIcon->pixmap().size();
    QPixmap icon;
    QEventLoop loop;
    if(canvas->createTeams(coursesComboBox->currentText(), tabName, teamNames, teamRosters)) {
        canvas->actionDialogLabel->setText(tr("Success!"));
        icon.load(":/icons_new/ok.png");
        canvas->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->actionComplete(busyBox);
    }
    else {
        canvas->actionDialogLabel->setText(tr("Error. Teams not created."));
        icon.load(":/icons_new/error.png");
        canvas->actionDialogIcon->setPixmap(icon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
        loop.exec();
        canvas->actionComplete(busyBox);
        return;
    }
    delete canvasCoursesDialog;

    delete canvas;
}


void TeamsTabItem::refreshTeamDisplay()
{
    //Create TeamTreeWidgetItems for the sections, teams, and students
    QList<TeamTreeWidgetItem*> sectionItems;
    sectionItems.reserve(sectionNames.size());
    QList<TeamTreeWidgetItem*> teamItems;
    teamItems.reserve(teams.size());
    QList<TeamTreeWidgetItem*> studentItems;
    studentItems.reserve(numStudents);

    //iterate through sections or teams
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) {
        for(const auto &sectionName : qAsConst(sectionNames)) {
            sectionItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::section);
            teamDataTree->refreshSection(sectionItems.last(), sectionName);

            //remove all team items in the section
            for(auto &teamItem : sectionItems.last()->takeChildren()) {
                sectionItems.last()->removeChild(teamItem);
                delete teamItem;
            }

            //iterate through teams
            int teamNum = 0;
            for(const auto &team : qAsConst(teams)) {
                const StudentRecord *const firstStudent = findStudentFromID(team.studentIDs.at(0));
                const QString firstStudentName = firstStudent->lastname + firstStudent->firstname;
                if(firstStudent->section == sectionName) {
                    teamItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::team, teamDataTree->columnCount(), team.score);
                    teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::newTeam, teamItems.last(), team, teamNum,
                                              firstStudentName, &teams.dataOptions, teamingOptions);

                    //remove all student items in the team
                    for(auto &studentItem : teamItems.last()->takeChildren()) {
                        teamItems.last()->removeChild(studentItem);
                        delete studentItem;
                    }

                    //add new student items
                    for(const auto studentID : team.studentIDs) {
                        studentItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
                        teamDataTree->refreshStudent(studentItems.last(), *findStudentFromID(studentID), &teams.dataOptions, teamingOptions);
                        teamItems.last()->addChild(studentItems.last());
                    }

                    teamItems.last()->setExpanded(false);
                    sectionItems.last()->addChild(teamItems.last());
                }
                teamNum++;
            }
            sectionItems.last()->setExpanded(true);
        }
    }
    else {
        //iterate through teams
        int teamNum = 0;
        for(const auto &team : qAsConst(teams)) {
            teamItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::team, teamDataTree->columnCount(), team.score);
            const StudentRecord *const firstStudent = findStudentFromID(team.studentIDs.at(0));
            const QString firstStudentName = firstStudent->lastname + firstStudent->firstname;
            teamDataTree->refreshTeam(TeamTreeWidget::RefreshType::newTeam, teamItems.last(), team,
                                      teamNum, firstStudentName, &teams.dataOptions, teamingOptions);

            //remove all student items in the team
            for(auto &studentItem : teamItems.last()->takeChildren()) {
                teamItems.last()->removeChild(studentItem);
                delete studentItem;
            }

            //add new student items
            for(const auto studentID : team.studentIDs) {
                studentItems << new TeamTreeWidgetItem(TeamTreeWidgetItem::TreeItemType::student);
                teamDataTree->refreshStudent(studentItems.last(), *findStudentFromID(studentID), &teams.dataOptions, teamingOptions);
                teamItems.last()->addChild(studentItems.last());
            }

            teamItems.last()->setExpanded(false);
            teamNum++;
        }
    }

    // Finally, put each section or team in the table for display
    teamDataTree->setUpdatesEnabled(false);

    teamDataTree->clear();
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) {
        for(auto &sectionItem : sectionItems) {
            teamDataTree->addTopLevelItem(sectionItem);
        }
    }
    else {
        for(auto &teamItem : teamItems) {
            teamDataTree->addTopLevelItem(teamItem);
        }
    }

    for(int column = 0; column < teamDataTree->columnCount(); column++) {
        teamDataTree->resizeColumnToContents(column);
    }
    teamDataTree->setUpdatesEnabled(true);
    teamDataTree->setSortingEnabled(true);
}


void TeamsTabItem::refreshDisplayOrder()
{
    // Any time teams have been reordered, refresh the hidden display order column
    const int lastCol = teamDataTree->columnCount()-1;
    auto item = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    int teamRow = 0;
    while(item != nullptr) {
        if(item->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
            item->setData(lastCol, TEAMINFO_SORT_ROLE, teamRow);
            item->setData(lastCol, TEAMINFO_DISPLAY_ROLE, QString::number(teamRow));
            item->setText(lastCol, QString::number(teamRow));
            teamRow++;
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(teamDataTree->itemBelow(item));
    }
}


QList<int> TeamsTabItem::getTeamNumbersInDisplayOrder() const
{
    //NOTE: only includes the teams that are currently being displayed. If teams are hidden within a collapsed parent, they will not be included
    // this is not a problem currently because: a) teams are top level items, or b) teams have the section as parent and sections are prevented from being collapsed
    QList<int> teamDisplayNums;
    teamDisplayNums.reserve(teams.size());
    auto item = dynamic_cast<const TeamTreeWidgetItem*>(teamDataTree->topLevelItem(0));
    while(item != nullptr) {
        if(item->treeItemType == TeamTreeWidgetItem::TreeItemType::team) {
            teamDisplayNums << item->data(0, TEAM_NUMBER_ROLE).toInt();
        }
        item = dynamic_cast<const TeamTreeWidgetItem*>(teamDataTree->itemBelow(item));
    }
    return teamDisplayNums;
}


inline StudentRecord* TeamsTabItem::findStudentFromID(const long long ID)
{
    for(auto &student : students) {
        if(student.ID == ID) {
            return &student;
        }
    }
    return nullptr;
}


QStringList TeamsTabItem::createStdFileContents()
{
    QStringList fileContents(NUMEXPORTFILES);
    QString &studentsFileContents = fileContents[studentFile];
    QString &instructorsFileContents = fileContents[instructorFile];
    QString &spreadsheetFileContents = fileContents[spreadsheetFile];

    spreadsheetFileContents = tr("Section") + "\t" + tr("Team") + "\t" + tr("Name") + "\t" + tr("Email") + "\n";

    instructorsFileContents = tr("Source: ") + teams.dataOptions.dataSourceName;
    if(teams.dataOptions.sectionIncluded) {
        instructorsFileContents += "\n" + tr("Section: ") + teamingOptions->sectionName;
    }
    instructorsFileContents += "\n\n" + tr("Teaming Options") + ":";
    if(teams.dataOptions.genderIncluded) {
        instructorsFileContents += (teamingOptions->isolatedWomenPrevented? ("\n" + tr("Isolated women prevented")) : "");
        instructorsFileContents += (teamingOptions->isolatedMenPrevented? ("\n" + tr("Isolated men prevented")) : "");
        instructorsFileContents += (teamingOptions->isolatedNonbinaryPrevented? ("\n" + tr("Isolated nonbinary students prevented")) : "");
        instructorsFileContents += (teamingOptions->singleGenderPrevented? ("\n" + tr("Single gender teams prevented")) : "");
    }
    if(teams.dataOptions.URMIncluded && teamingOptions->isolatedURMPrevented) {
        instructorsFileContents += "\n" + tr("Isolated URM students prevented");
    }
    if(!teams.dataOptions.dayNames.isEmpty() && teamingOptions->scheduleWeight > 0) {
        instructorsFileContents += "\n" + tr("Meeting block size is ") + QString::number(teamingOptions->meetingBlockSize) +
                                                                         tr(" hour") + ((teamingOptions->meetingBlockSize == 1) ? "" : tr("s"));
        instructorsFileContents += "\n" + tr("Minimum number of meeting times = ") + QString::number(teamingOptions->minTimeBlocksOverlap);
        instructorsFileContents += "\n" + tr("Desired number of meeting times = ") + QString::number(teamingOptions->desiredTimeBlocksOverlap);
        instructorsFileContents += "\n" + tr("Schedule weight = ") + QString::number(double(teamingOptions->scheduleWeight));
    }
    for(int attrib = 0; attrib < teams.dataOptions.numAttributes; attrib++) {
        instructorsFileContents += "\n" + tr("Multiple choice Q") + QString::number(attrib+1) + ": "
                                   + tr("weight") + " = " + QString::number(double(teamingOptions->attributeWeights[attrib]));
        // Check the attribute diversity type explicitly
        if (teamingOptions->attributeDiversity[attrib] == TeamingOptions::AttributeDiversity::HOMOGENOUS) {
            instructorsFileContents += ", " + tr("homogeneous");
        } else if (teamingOptions->attributeDiversity[attrib] == TeamingOptions::AttributeDiversity::HETEROGENOUS) {
            instructorsFileContents += ", " + tr("heterogeneous");
        } else {
            instructorsFileContents += ", " + tr("ignored");
        }
    }
    instructorsFileContents += "\n\n\n";
    for(int attrib = 0; attrib < teams.dataOptions.numAttributes; attrib++) {
        QString questionWithResponses = tr("Multiple choice Q") + QString::number(attrib+1) + "\n" +
                                        teams.dataOptions.attributeQuestionText.at(attrib) + "\n" + tr("Responses:");
        for(int response = 0; response < teams.dataOptions.attributeQuestionResponses[attrib].size(); response++) {
            if((teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::ordered) ||
                (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::multiordered) ||
                (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::timezone)) {
                questionWithResponses += "\n\t" + teams.dataOptions.attributeQuestionResponses[attrib].at(response);
            }
            else if((teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::categorical) ||
                    (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::multicategorical)) {
                questionWithResponses += "\n\t" + (response < 26 ? QString(char(response + 'A')) :
                                                                   QString(char(response%26 + 'A')).repeated(1 + (response/26)));
                questionWithResponses += ". " + teams.dataOptions.attributeQuestionResponses[attrib].at(response);
            }
        }
        questionWithResponses += "\n\n\n";
        instructorsFileContents += questionWithResponses;
    }

    // get the relevant gender terminology
    QStringList genderOptions;
    if(teams.dataOptions.genderType == GenderType::biol) {
        genderOptions = QString(BIOLGENDERS7CHAR).split('/');
    }
    else if(teams.dataOptions.genderType == GenderType::adult) {
        genderOptions = QString(ADULTGENDERS7CHAR).split('/');
    }
    else if(teams.dataOptions.genderType == GenderType::child) {
        genderOptions = QString(CHILDGENDERS7CHAR).split('/');
    }
    else { //if(teams.dataOptions.genderType == GenderType::pronoun)
        genderOptions = QString(PRONOUNS7CHAR).split('/');
    }

    // get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    //loop through every team
    for(const auto teamNum : teamDisplayNums) {
        const auto &team = teams[teamNum];
        instructorsFileContents += tr("Team ") + team.name + "  -  " +
                                   tr("Score = ") + QString::number(double(team.score), 'f', 2) + "\n\n";
        studentsFileContents += tr("Team ") + team.name + "\n\n";

        //loop through each teammate in the team
        for(const auto studentID : team.studentIDs) {
            const auto *const student = findStudentFromID(studentID);
            if(student == nullptr) {
                continue;
            }
            if(teams.dataOptions.genderIncluded) {
                QString genderText;
                bool firstGender = true;
                for(const auto gen : student->gender) {
                    if(!firstGender) {
                        genderText += ", ";
                    }
                    genderText += genderOptions.at(static_cast<int>(gen));
                    firstGender = false;
                }
                instructorsFileContents += " " + genderText + " ";
            }
            if(teams.dataOptions.URMIncluded) {
                if(student->URM) {
                    instructorsFileContents += tr(" URM ");
                }
                else {
                    instructorsFileContents += "     ";
                }
            }
            for(int attribute = 0; attribute < teams.dataOptions.numAttributes; attribute++) {
                auto value = student->attributeVals[attribute].constBegin();
                if(*value != -1) {
                    if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::ordered) {
                        instructorsFileContents += (QString::number(*value)).leftJustified(3);
                    }
                    else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone) {
                        instructorsFileContents += (QString::number(student->timezone)).leftJustified(5);
                    }
                    else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::categorical) {
                        instructorsFileContents += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))).leftJustified(3) :
                                                                     (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)))).leftJustified(3);
                    }
                    else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                        const auto lastValue = student->attributeVals[attribute].constEnd();
                        QString attributeList;
                        while(value != lastValue) {
                            attributeList += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))) :
                                                               (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26))));
                            value++;
                            if(value != lastValue) {
                                 attributeList += ",";
                            }
                        }
                        instructorsFileContents += attributeList.leftJustified(3);
                    }
                    else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                        const auto lastValue = student->attributeVals[attribute].constEnd();
                        QString attributeList;
                        while(value != lastValue) {
                            attributeList += QString::number(*value);
                            value++;
                            if(value != lastValue) {
                                 attributeList += ",";
                            }
                        }
                        instructorsFileContents += attributeList.leftJustified(3);
                    }
                }
                else {
                    instructorsFileContents += (QString("?")).leftJustified(3);
                }
            }
            if(teams.dataOptions.sectionIncluded && teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
                instructorsFileContents += student->section;
            }
            const int nameSize = int((student->firstname + " " + student->lastname).size());
            instructorsFileContents += "\t" + student->firstname + " " + student->lastname +
                                        QString(std::max(2,30-nameSize), ' ') + student->email + "\n";
            studentsFileContents += student->firstname + " " + student->lastname +
                                    QString(std::max(2,30-nameSize), ' ') + student->email + "\n";
            spreadsheetFileContents += student->section + "\t" + team.name + "\t" + student->firstname +
                                       " " + student->lastname + "\t" + student->email + "\n";
        }
        if(!teams.dataOptions.dayNames.isEmpty()) {
            instructorsFileContents += "\n" + tr("Availability:") + "\n            ";
            studentsFileContents += "\n" + tr("Availability:") + "\n            ";

            for(const auto &dayName : qAsConst(teams.dataOptions.dayNames)) {
                // using first 3 characters in day name as abbreviation
                instructorsFileContents += "  " + dayName.left(3) + "  ";
                studentsFileContents += "  " + dayName.left(3) + "  ";
            }
            instructorsFileContents += "\n";
            studentsFileContents += "\n";

            for(int time = 0; time < teams.dataOptions.timeNames.size(); time++) {
                instructorsFileContents += teams.dataOptions.timeNames.at(time) + QString((11-teams.dataOptions.timeNames.at(time).size()), ' ');
                studentsFileContents += teams.dataOptions.timeNames.at(time) + QString((11-teams.dataOptions.timeNames.at(time).size()), ' ');
                for(int day = 0; day < teams.dataOptions.dayNames.size(); day++) {
                    QString percentage;
                    if(team.size > team.numStudentsWithAmbiguousSchedules) {
                        percentage = QString::number((100*team.numStudentsAvailable[day][time]) /
                                                     (team.size-team.numStudentsWithAmbiguousSchedules)) + "% ";
                    }
                    else {
                        percentage = "?";
                    }
                    const QStringView left3 = QStringView{teams.dataOptions.dayNames.at(day).left(3)};
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
    return fileContents;
}


QString TeamsTabItem::createCustomFileContents(WhichFilesDialog::CustomFileOptions customFileOptions)
{
    QString customFileContents = "";

    if(customFileOptions.includeFileData) {
        customFileContents = tr("Source: ") + teams.dataOptions.dataSourceName;
        if(teams.dataOptions.sectionIncluded) {
            customFileContents += "\n" + tr("Section: ") + teamingOptions->sectionName;
        }
        customFileContents += "\n\n";
    }
    if(customFileOptions.includeTeamingData) {
        customFileContents += tr("Teaming Options") + ":";
        if(teams.dataOptions.genderIncluded) {
            customFileContents += (teamingOptions->isolatedWomenPrevented? ("\n" + tr("Isolated women prevented")) : "");
            customFileContents += (teamingOptions->isolatedMenPrevented? ("\n" + tr("Isolated men prevented")) : "");
            customFileContents += (teamingOptions->isolatedNonbinaryPrevented? ("\n" + tr("Isolated nonbinary students prevented")) : "");
            customFileContents += (teamingOptions->singleGenderPrevented? ("\n" + tr("Single gender teams prevented")) : "");
        }
        if(teams.dataOptions.URMIncluded && teamingOptions->isolatedURMPrevented) {
            customFileContents += "\n" + tr("Isolated URM students prevented");
        }
        if(!teams.dataOptions.dayNames.isEmpty() && teamingOptions->scheduleWeight > 0) {
            customFileContents += "\n" + tr("Meeting block size is ") + QString::number(teamingOptions->meetingBlockSize)
                                       + tr(" hour") + ((teamingOptions->meetingBlockSize == 1) ? "" : tr("s"));
            customFileContents += "\n" + tr("Minimum number of meeting times = ") + QString::number(teamingOptions->minTimeBlocksOverlap);
            customFileContents += "\n" + tr("Desired number of meeting times = ") + QString::number(teamingOptions->desiredTimeBlocksOverlap);
            customFileContents += "\n" + tr("Schedule weight = ") + QString::number(double(teamingOptions->scheduleWeight));
        }
        for(int attrib = 0; attrib < teams.dataOptions.numAttributes; attrib++) {
            customFileContents += "\n" + tr("Multiple choice Q") + QString::number(attrib+1) + ": "
                                  + tr("weight") + " = " + QString::number(double(teamingOptions->attributeWeights[attrib]));
            if (teamingOptions->attributeDiversity[attrib] == TeamingOptions::AttributeDiversity::HOMOGENOUS) {
                customFileContents += ", " + tr("homogeneous");
            } else if (teamingOptions->attributeDiversity[attrib] == TeamingOptions::AttributeDiversity::HETEROGENOUS) {
                customFileContents += ", " + tr("heterogeneous");
            } else {
                customFileContents += ", " + tr("ignored");
            }
        }
        customFileContents += "\n\n\n";
        for(int attrib = 0; attrib < teams.dataOptions.numAttributes; attrib++) {
            QString questionWithResponses = tr("Multiple choice Q") + QString::number(attrib+1) + "\n" +
                                            teams.dataOptions.attributeQuestionText.at(attrib) + "\n" + tr("Responses:");
            for(int response = 0; response < teams.dataOptions.attributeQuestionResponses[attrib].size(); response++) {
                if((teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::ordered) ||
                    (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::multiordered) ||
                    (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::timezone)) {
                    questionWithResponses += "\n\t" + teams.dataOptions.attributeQuestionResponses[attrib].at(response);
                }
                else if((teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::categorical) ||
                         (teams.dataOptions.attributeType[attrib] == DataOptions::AttributeType::multicategorical)) {
                    questionWithResponses += "\n\t" + (response < 26 ? QString(char(response + 'A')) :
                                                           QString(char(response%26 + 'A')).repeated(1 + (response/26)));
                    questionWithResponses += ". " + teams.dataOptions.attributeQuestionResponses[attrib].at(response);
                }
            }
            questionWithResponses += "\n\n\n";
            customFileContents += questionWithResponses;
        }
    }

    // get the relevant gender terminology
    QStringList genderOptions;
    if(teams.dataOptions.genderType == GenderType::biol) {
        genderOptions = QString(BIOLGENDERS7CHAR).split('/');
    }
    else if(teams.dataOptions.genderType == GenderType::adult) {
        genderOptions = QString(ADULTGENDERS7CHAR).split('/');
    }
    else if(teams.dataOptions.genderType == GenderType::child) {
        genderOptions = QString(CHILDGENDERS7CHAR).split('/');
    }
    else { //if(teams.dataOptions.genderType == GenderType::pronoun)
        genderOptions = QString(PRONOUNS7CHAR).split('/');
    }

    // get team numbers in the order that they are currently displayed/sorted
    const QList<int> teamDisplayNums = getTeamNumbersInDisplayOrder();

    //loop through every team
    for(const auto teamNum : teamDisplayNums) {
        const auto &team = teams[teamNum];
        customFileContents += tr("Team ") + team.name;
        if(customFileOptions.includeTeamScore) {
            customFileContents += "  -  " + tr("Score = ") + QString::number(double(team.score), 'f', 2);
        }
        customFileContents += "\n\n";

        //loop through each teammate in the team
        for(const auto studentID : team.studentIDs) {
            const auto *const student = findStudentFromID(studentID);
            if(student == nullptr) {
                continue;
            }
            if(teams.dataOptions.genderIncluded && customFileOptions.includeGender) {
                QString genderText;
                bool firstGender = true;
                for(const auto gen : student->gender) {
                    if(!firstGender) {
                        genderText += ", ";
                    }
                    genderText += genderOptions.at(static_cast<int>(gen));
                    firstGender = false;
                }
                customFileContents += " " + genderText + " ";
            }
            if(teams.dataOptions.URMIncluded && customFileOptions.includeURM) {
                if(student->URM) {
                    customFileContents += tr(" URM ");
                }
                else {
                    customFileContents += "     ";
                }
            }
            for(int attribute = 0; attribute < teams.dataOptions.numAttributes; attribute++) {
                if(customFileOptions.includeMultiChoice.at(attribute)) {
                    auto value = student->attributeVals[attribute].constBegin();
                    if(*value != -1) {
                        if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::ordered) {
                            customFileContents += (QString::number(*value)).leftJustified(3);
                        }
                        else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::timezone) {
                            customFileContents += (QString::number(student->timezone)).leftJustified(5);
                        }
                        else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::categorical) {
                            customFileContents += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))).leftJustified(3) :
                                                                    (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26)))).leftJustified(3);
                        }
                        else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::multicategorical) {
                            const auto lastValue = student->attributeVals[attribute].constEnd();
                            QString attributeList;
                            while(value != lastValue) {
                                attributeList += ((*value) <= 26 ? (QString(char((*value)-1 + 'A'))) :
                                                      (QString(char(((*value)-1)%26 + 'A')).repeated(1+(((*value)-1)/26))));
                                value++;
                                if(value != lastValue) {
                                    attributeList += ",";
                                }
                            }
                            customFileContents += attributeList.leftJustified(3);
                        }
                        else if(teams.dataOptions.attributeType[attribute] == DataOptions::AttributeType::multiordered) {
                            const auto lastValue = student->attributeVals[attribute].constEnd();
                            QString attributeList;
                            while(value != lastValue) {
                                attributeList += QString::number(*value);
                                value++;
                                if(value != lastValue) {
                                    attributeList += ",";
                                }
                            }
                            customFileContents += attributeList.leftJustified(3);
                        }
                    }
                    else {
                        customFileContents += (QString("?")).leftJustified(3);
                    }
                }
            }
            if(teams.dataOptions.sectionIncluded && customFileOptions.includeSect) {
                customFileContents += student->section;
            }
            if((teams.dataOptions.firstNameField != DataOptions::FIELDNOTPRESENT && customFileOptions.includeFirstName) ||
               (teams.dataOptions.lastNameField != DataOptions::FIELDNOTPRESENT && customFileOptions.includeLastName)) {
                int nameSize = 0, nameWidth = 0;
                customFileContents += "\t";
                if(customFileOptions.includeFirstName) {
                    customFileContents += student->firstname;
                    nameSize += int(student->firstname.size());
                    nameWidth += 15;
                }
                if(customFileOptions.includeFirstName && customFileOptions.includeLastName) {
                    customFileContents += " ";
                }
                if(customFileOptions.includeLastName) {
                    customFileContents += student->lastname;
                    nameSize += int(student->lastname.size());
                    nameWidth += 15;
                }
                customFileContents += QString(std::max(2, nameWidth-nameSize), ' ');
            }
            if(teams.dataOptions.emailField != DataOptions::FIELDNOTPRESENT && customFileOptions.includeEmail) {
                customFileContents += student->email;
            }
            customFileContents += "\n";
        }
        if(!teams.dataOptions.dayNames.isEmpty() & customFileOptions.includeSechedule) {
            customFileContents += "\n" + tr("Availability:") + "\n            ";

            for(const auto &dayName : qAsConst(teams.dataOptions.dayNames)) {
                // using first 3 characters in day name as abbreviation
                customFileContents += "  " + dayName.left(3) + "  ";
            }
            customFileContents += "\n";

            for(int time = 0; time < teams.dataOptions.timeNames.size(); time++) {
                customFileContents += teams.dataOptions.timeNames.at(time) + QString((11-teams.dataOptions.timeNames.at(time).size()), ' ');
                for(int day = 0; day < teams.dataOptions.dayNames.size(); day++) {
                    QString percentage;
                    if(team.size > team.numStudentsWithAmbiguousSchedules) {
                        percentage = QString::number((100*team.numStudentsAvailable[day][time]) /
                                                     (team.size-team.numStudentsWithAmbiguousSchedules)) + "% ";
                    }
                    else {
                        percentage = "?";
                    }
                    const QStringView left3 = QStringView{teams.dataOptions.dayNames.at(day).left(3)};
                    customFileContents += QString((4+left3.size())-percentage.size(), ' ') + percentage;
                }
                customFileContents += "\n";
            }
        }
        customFileContents += "\n\n";
    }
    return customFileContents;
}


void TeamsTabItem::printFiles(const QStringList &fileContents, WhichFilesDialog::FileType filetype, PrintType printType)
{
    // connecting to the printer is spun off into a separate thread because sometimes it causes ~30 second hang
    // message box explains what's happening
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    auto *msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::NoIcon);
    msgBox->setText(printType == PrintType::printToPDF? tr("Setting up PDF writer...") : tr("Connecting to printer..."));
    msgBox->setStyleSheet(LABEL10PTSTYLE);
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

    bool doIt = false;
    QString fileName;
    if(printType == PrintType::printToPDF) {
        printer->setOutputFormat(QPrinter::PdfFormat);
        fileName = QFileDialog::getSaveFileName(this, tr("Choose a location and filename"), "", tr("PDF File (*.pdf);;All Files (*)"));
        if(!fileName.isEmpty()) {
            doIt = true;
            printer->setOutputFileName(fileName);
        }
    }
    else {
        printer->setOutputFormat(QPrinter::NativeFormat);
        QPrintDialog printDialog(printer);
        printDialog.setWindowTitle(tr("Print"));
        doIt = (printDialog.exec() == QDialog::Accepted);
    }

    if(doIt) {
        QFont printFont = PRINTFONT;

        if(filetype == WhichFilesDialog::FileType::instructor) {
            printOneFile(fileContents[instructorFile], "\n\n\n", printFont, printer);
        }
        else if(filetype == WhichFilesDialog::FileType::student) {
            printOneFile(fileContents[studentFile], "\n\n\n", printFont, printer);
        }
        else if(filetype == WhichFilesDialog::FileType::spreadsheet) {
            printFont.setPointSize(PRINTOUT_FONTSIZE);
            printer->setPageOrientation(QPageLayout::Landscape);
            if(printType == PrintType::printToPDF) {
                printOneFile(fileContents[spreadsheetFile], "\n\n\n", printFont, printer);
            }
            else {
                QTextDocument textDocument(fileContents[spreadsheetFile], this);
                textDocument.setDefaultFont(printFont);
                textDocument.print(printer);
            }
        }
        else if(filetype == WhichFilesDialog::FileType::custom) {
            printOneFile(fileContents[customFile], "\n\n\n", printFont, printer);
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
    const int LargeGap = printer->logicalDpiY()/2, MediumGap = LargeGap/2, SmallGap = MediumGap/2;
    const int pageHeight = painter.window().height() - 2*LargeGap;

    const QStringList eachTeam = file.split(delimiter, Qt::SkipEmptyParts);

    //paginate the output
    QStringList currentPage;
    QList<QStringList> pages;
    int y = 0;
    QStringList::const_iterator it = eachTeam.cbegin();
    while (it != eachTeam.cend()) {
        //calculate height on page of this team text
        const int textWidth = painter.window().width() - 2*LargeGap - 2*SmallGap;
        const int maxHeight = painter.window().height();
        const QRect textRect = painter.boundingRect(0, 0, textWidth, maxHeight, Qt::TextWordWrap, *it);
        const int height = textRect.height() + 2*SmallGap;
        if(y + height > pageHeight && !currentPage.isEmpty()) {
            pages.push_back(currentPage);
            currentPage.clear();
            y = 0;
        }
        currentPage.push_back(*it);
        y += height + MediumGap;
        ++it;
    }
    if (!currentPage.isEmpty()) {
        pages.push_back(currentPage);
    }

    //print each page, 1 at a time
    for (int pagenum = 0; pagenum < pages.size(); pagenum++) {
        if (pagenum > 0) {
            printer->newPage();
        }
        const QTransform savedTransform = painter.worldTransform();
        painter.translate(0, LargeGap);
        QStringList::const_iterator it = pages[pagenum].cbegin();
        while (it != pages[pagenum].cend()) {
            const QString title = it->left(it->indexOf('\n')) + " ";
            const QString body = it->right(it->size() - (it->indexOf('\n')+1));
            const int boxWidth = painter.window().width() - 2*LargeGap;
            const int textWidth = boxWidth - 2*SmallGap;
            const int maxHeight = painter.window().height();
            const QRect titleRect = painter.boundingRect(LargeGap+SmallGap, SmallGap, textWidth, maxHeight, Qt::TextWordWrap, title);
            const QRect bodyRect = painter.boundingRect(LargeGap+SmallGap, SmallGap+titleRect.height(), textWidth, maxHeight, Qt::TextWordWrap, body);
            const int boxHeight = titleRect.height() + bodyRect.height() + 2 * SmallGap;
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
