#include "gruepr.h"
#include "ui_gruepr.h"
#include "criteria/sectionCriterion.h"
#include "criteria/teamsizeCriterion.h"
#include "dialogs/customTeamsizesDialog.h"
#include "dialogs/editOrAddStudentDialog.h"
#include "dialogs/editSectionNamesDialog.h"
#include "dialogs/findMatchingNameDialog.h"
#include "dialogs/teammatesRulesDialog.h"
#include "widgets/customsplitter.h"
#include "widgets/pushButtonWithMouseEnter.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include "widgets/sortableTableWidgetItem.h"
#include "widgets/teamsTabItem.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QtConcurrentRun>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QTextBrowser>
#include <QSlider>
#include <random>


gruepr::gruepr(DataOptions &_dataOptions, QList<StudentRecord> &_students) :
    QMainWindow(),
    ui(new Ui::gruepr),
    dataOptions(new DataOptions(std::move(_dataOptions))),
    students(std::move(_students))
{
    //Setup the main window
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowIcon(QIcon(":/icons_new/icon.svg"));
    setWindowTitle(tr("gruepr - Form teams"));
    qRegisterMetaType<QList<float> >("QList<float>");

    ui->dataSourceFrame->setStyleSheet(DATASOURCEFRAMESTYLE);
    ui->dataSourcePrelabel->setStyleSheet(DATASOURCEPRELABELSTYLE);
    ui->dataSourceLabel->setStyleSheet(DATASOURCELABELSTYLE);

    CustomSplitter *splitter = new CustomSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle {background: lightgray;}");
    splitter->setHandleWidth(8);

    QWidget *settingTeamCriteriaWidget = new QWidget(this);
    ui->criteriaLabel->setStyleSheet(LABEL12PTSTYLE);
    QWidget *scrollWidget = ui->teamingOptionsScrollArea->widget();
    ui->teamingOptionsScrollArea->setWidgetResizable(true);
    ui->teamingOptionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->teamingOptionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->teamingOptionsScrollArea->viewport()->installEventFilter(this);

    auto *scrollLayout = qobject_cast<QVBoxLayout*>(scrollWidget->layout());
    scrollLayout->setSpacing(5);
    scrollLayout->setAlignment(Qt::AlignTop);
    ui->topLayout->addWidget(splitter);

    auto *teamingCriteriaLayout = new QVBoxLayout();
    teamingCriteriaLayout->addWidget(ui->dataSourceFrame);
    teamingCriteriaLayout->addWidget(ui->criteriaLabel);
    teamingCriteriaLayout->addWidget(ui->teamingOptionsScrollArea);
    settingTeamCriteriaWidget->setLayout(teamingCriteriaLayout);

    letsDoItButton = new QPushButton(this);
    letsDoItButton->setText("Create Teams");
    letsDoItButton->setIcon(QIcon(":/icons_new/createTeams.png"));
    letsDoItButton->setIconSize(QSize(24, 24));
    letsDoItButton->setToolTip("Click to form teams");
    teamingCriteriaLayout->addWidget(letsDoItButton);
    splitter->addWidget(settingTeamCriteriaWidget);
    splitter->addWidget(ui->dataDisplayTabWidget);
    splitter->setSizes({this->width()/2, this->width()/2});

    teamingOptions = nullptr;
    if(dataOptions->dataSource == DataOptions::DataSource::fromPrevWork) {
        QFile savedFile(dataOptions->saveStateFileName, this);
        if(savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            const QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
            savedFile.close();
            QJsonObject content = doc.object();
            teamingOptions = new TeamingOptions(content["teamingoptions"].toObject());
            const QJsonArray teamsetjsons = content["teamsets"].toArray();
            TeamsTabItem *teamTab = nullptr;
            for(const auto &teamsetjson : teamsetjsons) {
                teamTab = new TeamsTabItem(teamsetjson.toObject(), *teamingOptions, students, dataOptions->sectionNames, letsDoItButton, this);
                ui->dataDisplayTabWidget->addTab(teamTab, teamTab->tabName);
                numTeams = int(teams.size());
                connect(teamTab, &TeamsTabItem::saveState, this, &gruepr::saveState);
            }
        }
        else {
            grueprGlobal::errorMessage(this, tr("Error"), tr("There was an error loading the previous data."));
        }
    }
    if(teamingOptions == nullptr) {             // either not from previous work, or loading from previous work failed
        teamingOptions = new TeamingOptions;
        loadDefaultSettings();
    }

    //Defining all criteria cards
    criteriaCardsList.clear();

    //Section Criteria Card
    if (dataOptions->sectionIncluded) {
        sectionCriteriaCard = new GroupingCriteriaCard(Criterion::CriteriaType::section, dataOptions, teamingOptions, this, QString("Section"), false);
        const auto &sectionCriterion = qobject_cast<SectionCriterion*>(sectionCriteriaCard->criterion);
        sectionSelectionBox = sectionCriterion->sectionSelectionBox;
        connect(sectionCriterion->editSectionNameButton, &QPushButton::clicked, this, &gruepr::editSectionNames);
        connect(sectionCriterion->sectionSelectionBox, &QComboBox::currentIndexChanged, this, &gruepr::changeSection);
        connect(sectionCriteriaCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
        connect(sectionCriteriaCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
        connect(sectionCriteriaCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
        criteriaCardsList.append(sectionCriteriaCard);
    }

    //Team Size Criteria Card
    teamsizeCriteriaCard = new GroupingCriteriaCard(Criterion::CriteriaType::teamSize, dataOptions, teamingOptions, this, QString("Team Size"), false);
    const auto &teamSizeCriterion = qobject_cast<TeamsizeCriterion*>(teamsizeCriteriaCard->criterion);
    idealTeamSizeBox = teamSizeCriterion->idealTeamSizeBox;
    teamSizeBox = teamSizeCriterion->teamSizeBox;
    if(dataOptions->dataSource != DataOptions::DataSource::fromPrevWork) {
        QSettings savedSettings;
        idealTeamSizeBox->setValue(savedSettings.value("idealTeamSize", 4).toInt());
    }
    connect(teamSizeCriterion->idealTeamSizeBox, &QSpinBox::valueChanged, this, &gruepr::changeIdealTeamSize);
    connect(teamSizeCriterion->teamSizeBox, &QComboBox::currentIndexChanged, this, &gruepr::chooseTeamSizes);
    criteriaCardsList.append(teamsizeCriteriaCard);

    //Construct button and menu to add new criteria cards
    addNewCriteriaMenu = new QMenu(this);
    addNewCriteriaMenu->setStyleSheet(MENUSTYLE);
    addNewCriteriaCardButton = new QPushButton("Add New Criteria", this);
    addNewCriteriaCardButton->setIcon(QIcon(":/icons_new/add.png"));
    addNewCriteriaCardButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(addNewCriteriaCardButton, &QPushButton::clicked, addNewCriteriaCardButton, [this]() {
        QPoint centerOfCriteriaButton = addNewCriteriaCardButton->mapToGlobal(addNewCriteriaCardButton->rect().center());
        addNewCriteriaMenu->popup(QPoint(centerOfCriteriaButton.x() - addNewCriteriaMenu->sizeHint().width()/1.8, centerOfCriteriaButton.y()+10));
    });
    if (dataOptions->genderIncluded){
        QAction *genderAction = addNewCriteriaMenu->addAction(tr("Gender"));
        connect(genderAction, &QAction::triggered, addNewCriteriaCardButton, [this](){
            gruepr::addCriteriaCard(Criterion::CriteriaType::genderIdentity);});
    }
    if (dataOptions->URMIncluded){
        QAction *urmAction = addNewCriteriaMenu->addAction(tr("Racial/Ethnic/Cultural Identity"));
        connect(urmAction, &QAction::triggered, addNewCriteriaCardButton, [this](){
            gruepr::addCriteriaCard(Criterion::CriteriaType::urmIdentity);});
    }
    if (dataOptions->numAttributes > 0){
        //add an action for each question
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            QAction *currentMCQAction = addNewCriteriaMenu->addAction(dataOptions->attributeQuestionText[attribute]);
            connect(currentMCQAction, &QAction::triggered, addNewCriteriaCardButton, [this, attribute](){
                gruepr::addCriteriaCard(Criterion::CriteriaType::attributeQuestion, attribute);});
        }
    }
    if (dataOptions->gradeIncluded){
        QAction *gradeAction = addNewCriteriaMenu->addAction("Grade");
        connect(gradeAction, &QAction::triggered, this, [this](){gruepr::addCriteriaCard(Criterion::CriteriaType::gradeBalance);});
        addNewCriteriaMenu->addAction(gradeAction);
    }
    if (!dataOptions->scheduleField.empty()){
        QAction* scheduleMeetingTimesAction = new QAction("Meeting Times", this);
        connect(scheduleMeetingTimesAction, &QAction::triggered, this, [this](){gruepr::addCriteriaCard(Criterion::CriteriaType::scheduleMeetingTimes);});
        addNewCriteriaMenu->addAction(scheduleMeetingTimesAction);
    }
    QAction* requiredTeammatesAction = addNewCriteriaMenu->addAction("Required Teammates");
    connect(requiredTeammatesAction, &QAction::triggered, this, [this](){
        gruepr::addCriteriaCard(Criterion::CriteriaType::requiredTeammates);});
    QAction* preventedTeammatesAction = addNewCriteriaMenu->addAction("Prevented Teammates");
    connect(preventedTeammatesAction, &QAction::triggered, this, [this](){
        gruepr::addCriteriaCard(Criterion::CriteriaType::preventedTeammates);});
    QAction* requestedTeammatesAction = addNewCriteriaMenu->addAction("Requested Teammates");
    connect(requestedTeammatesAction, &QAction::triggered, this, [this](){
        gruepr::addCriteriaCard(Criterion::CriteriaType::requestedTeammates);});

    ui->teamingOptionsScrollArea->setStyleSheet(SCROLLBARSTYLE);
    letsDoItButton->setStyleSheet(GETSTARTEDBUTTONSTYLE);
    ui->addStudentPushButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->compareRosterPushButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    ui->dataDisplayTabWidget->setStyleSheet(DATADISPTABSTYLE);
    ui->dataDisplayTabWidget->tabBar()->setStyleSheet(DATADISPBARSTYLE);
    ui->dataDisplayTabWidget->tabBar()->setDrawBase(false);

    //Make the teams tabs double-clickable and closable (hide the close button on the students tab)
    connect(ui->dataDisplayTabWidget, &QTabWidget::tabBarDoubleClicked, this, &gruepr::editDataDisplayTabName);
    ui->dataDisplayTabWidget->setTabsClosable(true);
    ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    ui->dataDisplayTabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);
    connect(ui->dataDisplayTabWidget, &QTabWidget::tabCloseRequested, this, &gruepr::dataDisplayTabClose);

    //Set alternate fonts on some UI features
    QFont altFont = this->font();
    altFont.setPointSize(altFont.pointSize() + 4);
    letsDoItButton->setFont(altFont);
    ui->addStudentPushButton->setFont(altFont);
    ui->compareRosterPushButton->setFont(altFont);
    ui->dataDisplayTabWidget->setFont(altFont);

    loadUI();

    // initialize the priority order of criteria cards
    initializeCriteriaCardPriorities();

    QList<QPushButton *> buttons = {letsDoItButton, ui->addStudentPushButton, ui->compareRosterPushButton};
    for(auto &button : buttons) {
        button->setIconSize(QSize(STD_ICON_SIZE, STD_ICON_SIZE));
    }
    ui->dataSourceIcon->setFixedSize(STD_ICON_SIZE, STD_ICON_SIZE);

    //Connect other UI items to more involved actions
    // Connect signals from each draggableQFrame to swapFrames
    for(auto &criteriaCard : criteriaCardsList) {
        connect(criteriaCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
        connect(criteriaCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
    }

    //connecting the buttons that are always shown
    connect(ui->addStudentPushButton, &QPushButton::clicked, this, &gruepr::addAStudent);
    connect(ui->compareRosterPushButton, &QPushButton::clicked, this, &gruepr::compareStudentsToRoster);
    connect(letsDoItButton, &QPushButton::clicked, this, &gruepr::startOptimization);

    //Connect genetic algorithm progress signals to slots
    connect(this, &gruepr::generationComplete, this, &gruepr::updateOptimizationProgress, Qt::BlockingQueuedConnection);
    connect(&futureWatcher, &QFutureWatcher<void>::finished, this, &gruepr::optimizationComplete);

    refreshCriteriaLayout();
    saveState();
}

gruepr::~gruepr()
{
    delete dataOptions;
    delete teamingOptions;
    delete ui;
}

void gruepr::doAutoScroll(QPoint point){
    const int margin = 30, step = 10;
        QRect area = ui->teamingOptionsScrollArea->viewport()->rect();
        if (point.y() < area.top() + margin) {
            ui->teamingOptionsScrollArea->verticalScrollBar()->setValue(
                ui->teamingOptionsScrollArea->verticalScrollBar()->value() - step
            );
        } else if (point.y() > area.bottom() - margin) {
            ui->teamingOptionsScrollArea->verticalScrollBar()->setValue(
                ui->teamingOptionsScrollArea->verticalScrollBar()->value() + step
            );
        }
}

void gruepr::initializeCriteriaCardPriorities(){
    int count = 0;
    for(auto &criteriaCard : criteriaCardsList) {
        criteriaCard->setPriorityOrder(count);
        criteriaCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        count += 1;
    }
}

void gruepr::swapCriteriaCards(int draggedIndex, int targetIndex) {
    // Access the existing layout (verticalLayout_2)
    QLayout* layout = ui->teamingOptionsScrollAreaWidget->layout();
    layout->setSpacing(5);
    if (draggedIndex < 0 || targetIndex < 0 || draggedIndex >= criteriaCardsList.size() || targetIndex >= criteriaCardsList.size()) {
        //qDebug() << "Invalid indices for swapping!";
        return;
    }
    // Remove the frames from the layout
    GroupingCriteriaCard* draggedCard = criteriaCardsList[draggedIndex];
    GroupingCriteriaCard* targetCard = criteriaCardsList[targetIndex];
    // qDebug() << "criteriaCardsList size:" << criteriaCardsList.size();
    // qDebug() << "draggedIndex:" << draggedIndex << "targetIndex:" << targetIndex;
    // qDebug() << "draggedCard pointer:" << draggedCard << "targetCard pointer:" << targetCard;
    criteriaCardsList[targetIndex] = draggedCard;
    criteriaCardsList[draggedIndex] = targetCard;
    criteriaCardsList[targetIndex]->setPriorityOrder(targetIndex);

    criteriaCardsList[draggedIndex]->setPriorityOrder(draggedIndex);
    refreshCriteriaLayout();
}

void gruepr::addCriteriaCard(Criterion::CriteriaType criteriaType){
    switch(criteriaType) {
        case Criterion::CriteriaType::genderIdentity: {
            if(genderIdentityCriteriaCard == nullptr) {
                genderIdentityCriteriaCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this, QString("Gender identity"), true);
                connect(genderIdentityCriteriaCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
                connect(genderIdentityCriteriaCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
                connect(genderIdentityCriteriaCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
                connect(genderIdentityCriteriaCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);
                criteriaCardsList.append(genderIdentityCriteriaCard);
            }
            else {
                grueprGlobal::errorMessage(this, tr("Criterion already exists"), tr("Gender identity criteria already exists"));
            }
            break;
        }
        case Criterion::CriteriaType::urmIdentity: {
            if(urmIdentityCard == nullptr) {
                urmIdentityCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this, QString("Racial/Ethnic/Cultural Identity"), true);
                connect(urmIdentityCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
                connect(urmIdentityCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
                connect(urmIdentityCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
                connect(urmIdentityCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);
                criteriaCardsList.append(urmIdentityCard);
            }
            else {
                grueprGlobal::errorMessage(this, tr("Criterion already exists"), tr("URM Identity Criteria already exists"));
            }
            break;
        }
        case Criterion::CriteriaType::scheduleMeetingTimes: {
            if (meetingScheduleCriteriaCard == nullptr){
                meetingScheduleCriteriaCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this, QString("Number of weekly meeting times"), true);
                connect(meetingScheduleCriteriaCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
                connect(meetingScheduleCriteriaCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
                connect(meetingScheduleCriteriaCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
                connect(meetingScheduleCriteriaCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);
                criteriaCardsList.append(meetingScheduleCriteriaCard);
            } else {
                grueprGlobal::errorMessage(this, tr("Criterion already exists"), tr("Schedule Meeting Times Criteria already exists"));
            }
            break;
        }
        case Criterion::CriteriaType::requiredTeammates:
        case Criterion::CriteriaType::preventedTeammates:
        case Criterion::CriteriaType::requestedTeammates: {
            QString typeString;
            TeammatesRulesDialog::TypeOfTeammates type;
            if(criteriaType == Criterion::CriteriaType::requiredTeammates) {
                typeString = "Required";
                type = TeammatesRulesDialog::TypeOfTeammates::required;
            }
            else if(criteriaType == Criterion::CriteriaType::preventedTeammates) {
                typeString = "Prevented";
                type = TeammatesRulesDialog::TypeOfTeammates::prevented;
            }
            else {
                typeString = "Requested";
                type = TeammatesRulesDialog::TypeOfTeammates::requested;
            }
            if (!teammateRulesExistence.contains(criteriaType)){
                teammateRulesExistence.append(criteriaType);
                auto *teammatesCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this, typeString + tr(" Teammates"), true);
                const auto &teammatesCriterion = qobject_cast<TeammatesCriterion*>(teammatesCard->criterion);
                const int numTabs = ui->dataDisplayTabWidget->count();
                QStringList teamTabNames;
                teamTabNames.reserve(numTabs);
                for(int tab = 1; tab < numTabs; tab++) {
                    teamTabNames << ui->dataDisplayTabWidget->tabText(tab);
                }
                connect(teammatesCriterion->setTeammateRulesButton, &QPushButton::clicked, this, [this, type, teamTabNames]() {
                    auto *win = new TeammatesRulesDialog(students, *dataOptions, *teamingOptions,
                                                         ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
                                                          (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) ||
                                                          (teamingOptions->sectionType == TeamingOptions::SectionType::noSections))? "" : teamingOptions->sectionName,
                                                         teamTabNames, type, this);
                    //If user clicks OK, replace student database with copy that has had pairings added
                    const int reply = win->exec();
                    if(reply == QDialog::Accepted) {
                        for(int index = 0; index < students.size(); index++) {
                            students[index] = win->students[index];
                        }
                        teamingOptions->haveAnyRequiredTeammates = win->required_teammatesSpecified;
                        teamingOptions->haveAnyPreventedTeammates = win->prevented_teammatesSpecified;
                        teamingOptions->haveAnyRequestedTeammates = win->requested_teammatesSpecified;
                        teamingOptions->numberRequestedTeammatesGiven = win->numberRequestedTeammatesGiven;

                        saveState();
                    }

                    delete win;
                });
                connect(teammatesCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
                connect(teammatesCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
                connect(teammatesCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
                connect(teammatesCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);
                criteriaCardsList.append(teammatesCard);
            } else {
                grueprGlobal::errorMessage(this, tr("Criterion already exists"), typeString + tr(" Teammates Criteria Card already exists"));
            }
            break;
        }
        case Criterion::CriteriaType::gradeBalance: {
            if (gradeBalanceCriteriaCard == nullptr) {
                /*FROMDEV
                //Meeting Schedule Criteria Card Styling
                gradeBalanceCriteriaCard = new GroupingCriteriaCard(this, QString("Grade Balance"), true, criteriaType, GroupingCriteriaCard::Precedence::want);
                gradeBalanceCriteriaCard->criterion = new GradeBalanceCriterion(0.0, false);

                QHBoxLayout* gradeBalanceContentLayout = new QHBoxLayout();
                QFrame *gradeInfoFrame = new QFrame();
                gradeBalanceContentLayout->addWidget(gradeInfoFrame, 1);
                //mean grade box that is elevated + distribution of student grades

                QVBoxLayout* spinBoxVLayout = new QVBoxLayout();
                QHBoxLayout* spinBoxHLayout = new QHBoxLayout();
                gradeBalanceContentLayout->addLayout(spinBoxVLayout, 3);

                QLabel* gradeRangeLabel = new QLabel("Target group average grade range: ");
                spinBoxVLayout->addWidget(gradeRangeLabel);
                spinBoxVLayout->addLayout(spinBoxHLayout);

                minimumMeanGradeSpinBox = new QDoubleSpinBox(this);
                maximumMeanGradeSpinBox = new QDoubleSpinBox(this);

                spinBoxHLayout->addWidget(minimumMeanGradeSpinBox);
                spinBoxHLayout->addWidget(maximumMeanGradeSpinBox);


                minimumMeanGradeSpinBox->setPrefix(QString("Min: "));
                minimumMeanGradeSpinBox->setMinimum(0.0);
                minimumMeanGradeSpinBox->setValue(0.0);
                minimumMeanGradeSpinBox->setSingleStep(1.0);
                minimumMeanGradeSpinBox->setDecimals(2);
                minimumMeanGradeSpinBox->setMinimumHeight(40);

                maximumMeanGradeSpinBox->setPrefix(QString("Max: "));
                maximumMeanGradeSpinBox->setMinimum(0.0);
                maximumMeanGradeSpinBox->setMaximum(100.0);
                maximumMeanGradeSpinBox->setValue(100.0);
                maximumMeanGradeSpinBox->setSingleStep(1.0);
                maximumMeanGradeSpinBox->setDecimals(2);
                maximumMeanGradeSpinBox->setMinimumHeight(40);
                //add standard deviation and average data of grade based on population

                //mean grade?
                float meangrade = 0.0;
                for (const auto &student : std::as_const(students)){
                    meangrade += student.grade;
                }
                meangrade = meangrade / students.size();

                gradeInfoFrame->setFrameShadow(QFrame::Raised);
                QVBoxLayout *gradeInfoFrameLayout = new QVBoxLayout();
                gradeInfoFrameLayout->addWidget(new QLabel("<u>Mean Student Grade</u>"));
                gradeInfoFrameLayout->addWidget(new QLabel(QString::number(meangrade, 'f', 2)));
                gradeInfoFrame->setLayout(gradeInfoFrameLayout);

                //students over total students for mean grade

                gradeBalanceCriteriaCard->setContentAreaLayout(*gradeBalanceContentLayout);
                gradeBalanceCriteriaCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);

                //adding to criteria card list
                criteriaCardsList.append(gradeBalanceCriteriaCard);

                connect(minimumMeanGradeSpinBox, &QDoubleSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(minimumMeanGradeSpinBox);});
                connect(maximumMeanGradeSpinBox, &QDoubleSpinBox::valueChanged, this, [this](){simpleUIItemUpdate(maximumMeanGradeSpinBox);});
                //adding to layout
                connect(gradeBalanceCriteriaCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
                connect(gradeBalanceCriteriaCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
                connect(gradeBalanceCriteriaCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
                connect(gradeBalanceCriteriaCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);
            */
            }
            else {
                grueprGlobal::errorMessage(this, tr("Criterion already exists"), tr("Grade Balance Criteria already exists"));
            }
            break;
        }
        default: {
            break;
        }
    }

    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();
}

void gruepr::addCriteriaCard(Criterion::CriteriaType criteriaType, int attribute)
{
    if (criteriaType == Criterion::CriteriaType::attributeQuestion && (attribute < dataOptions->numAttributes)) {
        if (!addedAttributeNumbersList.contains(attribute)) {
            GroupingCriteriaCard *currentMultipleChoiceCard = initializedAttributeCriteriaCards[attribute];
            addedAttributeNumbersList.append(attribute);
            criteriaCardsList.append(currentMultipleChoiceCard);
            teamingOptions->attributeSelected.contains(attribute);
        } else {
            const QString message = tr("This criteria has already been added.");
            grueprGlobal::errorMessage(this, tr("Criterion already added"), message);
        }
    }
    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();
}

void gruepr::deleteCriteriaCard(int deletedIndex)
{
    // Get the card to be deleted
    //qDebug() << deletedIndex;
    //qDebug() << criteriaCardsList.size();
    GroupingCriteriaCard *cardToDelete = criteriaCardsList[deletedIndex];
    Criterion::CriteriaType criteriaType = cardToDelete->criterion->criteriaType;

    // Remove the card from the list and delete it
    criteriaCardsList.removeAt(deletedIndex);

    if (criteriaType == Criterion::CriteriaType::genderIdentity){
        delete cardToDelete;
        genderIdentityCriteriaCard = nullptr;
    }
    else if (criteriaType == Criterion::CriteriaType::urmIdentity){
        //FROMDEV
        // SingleURMIdentityCriterion *criterion = dynamic_cast<SingleURMIdentityCriterion*>(cardToDelete->criterion);
        // QString urmResponse = criterion->urmName;
        // uiCheckBoxMap.remove(urmResponse + "PreventIsolatedCheckBox");
        delete cardToDelete;
        urmIdentityCard = nullptr;
    }
    else if (criteriaType == Criterion::CriteriaType::attributeQuestion){
        auto *criterion = qobject_cast<AttributeCriterion*>(cardToDelete->criterion);
        int attributeIndex = criterion->attributeIndex;
        int indexToRemove = addedAttributeNumbersList.indexOf(attributeIndex);
        addedAttributeNumbersList.remove(indexToRemove);
        cardToDelete->setVisible(false);
        //setVisible to false?
    }
    else if (criteriaType == Criterion::CriteriaType::section){
        delete cardToDelete;
        sectionCriteriaCard = nullptr;
    }
    else if (criteriaType == Criterion::CriteriaType::scheduleMeetingTimes){
        delete cardToDelete;
        meetingScheduleCriteriaCard = nullptr;
    }
    else if ((criteriaType == Criterion::CriteriaType::requiredTeammates ||
              criteriaType == Criterion::CriteriaType::preventedTeammates) ||
              criteriaType == Criterion::CriteriaType::requestedTeammates){
        int indexToRemove = teammateRulesExistence.indexOf(criteriaType);
        teammateRulesExistence.remove(indexToRemove);
        delete cardToDelete;
    }
    else if (criteriaType == Criterion::CriteriaType::gradeBalance){
        delete cardToDelete;
        gradeBalanceCriteriaCard = nullptr;
    }
    else {
        //qDebug() << deletedIndex << " does not exist.";
    }
    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();
}

void gruepr::refreshCriteriaLayout(){
    QLayout* layout = ui->teamingOptionsScrollAreaWidget->layout();
    while (layout->count() > 1) {
        layout->removeItem(layout->itemAt(1));
    }
    for (auto *const criteriaCard : std::as_const(criteriaCardsList)) {
        if (criteriaCard->criterion->precedence == Criterion::Precedence::fixed) {
            criteriaCard->setStyleSheet(QString(FIXEDCRITERIAFRAME) + LABEL10PTFIXEDSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        // } else if (prevCriteriaCard->criterion->penaltyStatus == true && criteriaCard->criterion->penaltyStatus == true){
        //     if (prevCriteriaCard->includePenaltyCheckBox!=nullptr){
        //         prevCriteriaCard->includePenaltyCheckBox->setDisabled(true);
        //     }
        //     criteriaCard->includePenaltyCheckBox->setDisabled(false);
        //     criteriaCard->includePenaltyCheckBox->setVisible(true);
        //     criteriaCard->setStyleSheet(QString(FIXEDCRITERIAFRAME) + LABEL10PTMANDATORYSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        // } else if (prevCriteriaCard->criterion->penaltyStatus == true && criteriaCard->criterion->penaltyStatus == false){
        //     criteriaCard->includePenaltyCheckBox->setVisible(true);
        //     criteriaCard->includePenaltyCheckBox->setDisabled(false);
        //     criteriaCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        // } else if (prevCriteriaCard->criterion->penaltyStatus == false && criteriaCard->criterion->penaltyStatus == false){
        //     criteriaCard->includePenaltyCheckBox->setVisible(false);
        //     criteriaCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        // } else {
        //     //criteriaCard->includePenaltyCheckBox->setVisible(false);
        //     criteriaCard->setStyleSheet(QString(FIXEDCRITERIAFRAME) + LABEL10PTMANDATORYSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        }
        else {
            //criteriaCard->includePenaltyCheckBox->setVisible(false);
            criteriaCard->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
        }
        criteriaCard->setVisible(true);
        layout->addWidget(criteriaCard);
        //prevCriteriaCard = criteriaCard;
    }
    layout->addWidget(addNewCriteriaCardButton);
}

////////////////////
// A static public wrapper for the getGenomeScore function used internally
// The calculated scores are updated into the .scores members of the _teams array sent to the function
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
////////////////////
void gruepr::calcTeamScores(const QList<StudentRecord> &_students, const long long _numStudents,
                            TeamSet &_teams, const TeamingOptions *const _teamingOptions)
{
    const int _numTeams = _teams.size();
    const auto &_dataOptions = _teams.dataOptions;
    std::vector<float>teamScores(_numTeams);
    std::vector<std::vector<float>> criteriaScores(_teamingOptions->realNumScoringFactors, std::vector<float>(_numTeams));
    std::vector<int> penaltyPoints(_numTeams);
    std::vector<std::vector<bool>> availabilityChart(_dataOptions.dayNames.size(), std::vector<bool>(_dataOptions.timeNames.size()));
    std::vector<int> teamSizes(_numTeams);
    std::vector<int> genome(_numStudents);
    int ID = 0;
    for(int teamnum = 0; teamnum < _numTeams; teamnum++) {
        teamSizes[teamnum] = _teams[teamnum].size;
        for(const auto studentID : std::as_const(_teams[teamnum].studentIDs)) {
            int index = 0;
            while(index < _students.size() && _students.at(index).ID != studentID) {
                index++;
            }
            genome[ID] = index;
            ID++;
        }
    }

    getGenomeScore(_students.constData(), genome.data(), _numTeams, teamSizes.data(),
                   _teamingOptions, &_dataOptions, teamScores.data(),
                   criteriaScores, availabilityChart, penaltyPoints);

    for(int criterion = 0; criterion < _teamingOptions->realNumScoringFactors; criterion++){
        for (int team = 0; team <_numTeams; team++){
            _teams[team].criteriaScores[criterion] = criteriaScores[criterion][team];
            //penalty
            int penaltyScore = -penaltyPoints[criterion];
            //unweighted score
            float actualScore = criteriaScores[criterion][team]/_teamingOptions->weights[criterion];
            //qDebug() << "weight from weights[]:" << _teamingOptions->weights[criterion];
            //qDebug() << "weight from criterion->weight:" << _teamingOptions->criterionTypes[criterion]->weight;

            //does not take into account penalty scores yet
            _teams[team].criteriaScores[criterion] = actualScore;

            // qDebug() << "team:" << team;
            // qDebug() << "criterion:" << criterion;
            // qDebug() << "penalty:" << -penaltyPoints[criterion];
            // qDebug() << "actual score:" << actualScore;
            // qDebug() << "score:" << criteriaScores[criterion][team];
            // qDebug() << "score final:" << _teams[team].criterionScores[criterion];

        }
    }

    for(int teamnum = 0; teamnum < _numTeams; teamnum++) {
        _teams[teamnum].score = teamScores[teamnum];
    }
}


void gruepr::changeSection(int index)
{
    const QString desiredSection = sectionSelectionBox->itemText(index);
    if(dataOptions->sectionIncluded && desiredSection.isEmpty()) {
        const QString prevSection = teamingOptions->sectionName;
        teamingOptions->sectionName = desiredSection;
        teamingOptions->sectionType = TeamingOptions::SectionType::oneSection;
        refreshStudentDisplay();
        ui->studentTable->clearSortIndicator();
        teamingOptions->sectionName = prevSection;

        numActiveStudents = 0;
        letsDoItButton->setEnabled(false);
        return;
    }

    teamingOptions->sectionName = desiredSection;
    if(!multipleSectionsInProgress) {
        if(!dataOptions->sectionIncluded) {
            teamingOptions->sectionType = TeamingOptions::SectionType::noSections;
        }
        else if(sectionSelectionBox->currentIndex() == 1) {
            teamingOptions->sectionType = TeamingOptions::SectionType::allSeparately;
        }
        else if(sectionSelectionBox->currentIndex() == 0) {
            teamingOptions->sectionType = TeamingOptions::SectionType::allTogether;
        }
        else {
            teamingOptions->sectionType = TeamingOptions::SectionType::oneSection;
        }
    }

    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();

    // update the response counts in the attribute tabs
    if (!attributeWidgets.isEmpty()){ //check if user has added any attributes
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            QString currentAttributeQuestionText = dataOptions->attributeQuestionText.at(attribute);
                //do attribute widgets correspond to the attribute number?
            const auto &attributeType = dataOptions->attributeType[attribute];

            // record a tally for each response, starting with a 0 count for each response found in all of the survey data
            std::map<QString, int> currentResponseCounts;
            for(const auto &responseCount : std::as_const(dataOptions->attributeQuestionResponseCounts[attribute])) {
                currentResponseCounts[responseCount.first] = 0;
            }
            for(const auto &student : std::as_const(students)) {
                if(!student.deleted &&
                   ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
                    (teamingOptions->sectionType == TeamingOptions::SectionType::noSections) ||
                    ((teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) && !multipleSectionsInProgress) ||
                    (student.section == teamingOptions->sectionName))) {
                    const QString &currentStudentResponse = student.attributeResponse[attribute];

                    if(!student.attributeResponse[attribute].isEmpty()) {
                        if((attributeType == DataOptions::AttributeType::multicategorical) ||
                            (attributeType == DataOptions::AttributeType::multiordered)) {
                            //multivalued - tally each value
                            const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                            for(const auto &responseFromStudent : setOfResponsesFromStudent) {
                                currentResponseCounts[responseFromStudent.trimmed()]++;
                            }
                        }
                        else {
                            currentResponseCounts[currentStudentResponse]++;
                        }
                    }
                }
            }

            // put this new tally in the responses textbox of the attribute tab
            attributeWidgets[attribute]->updateResponses();
        }
    }

    idealTeamSizeBox->setMaximum(std::max(2ll, numActiveStudents / 2));
    changeIdealTeamSize();    // load new team sizes in selection box, if necessary
}


void gruepr::editSectionNames()
{
    auto *window = new editSectionNamesDialog(dataOptions->sectionNames, this);

    // If user clicks OK, use these section names
    const int reply = window->exec();
    if(reply == QDialog::Accepted) {
        //build a map of old name -> new name
        std::map<QString, QString> mapOfOldToNewSectionNames;
        for(int i = 0; i < dataOptions->sectionNames.size(); i++) {
            mapOfOldToNewSectionNames[dataOptions->sectionNames.at(i)] = window->sectionNames.at(i);
        }
        //replace section names for each student
        for(auto &student : students) {
            student.section = mapOfOldToNewSectionNames[student.section];
        }
        //replace section names in section selection box and dataOptions
        rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();

        saveState();
    }

    delete window;
}


inline StudentRecord* gruepr::findStudentFromID(const long long ID)
{
    for(auto &student : students) {
        if(student.ID == ID) {
            return &student;
        }
    }
    return nullptr;
}


void gruepr::editAStudent()
{
    // first, find the student, using the ID property of this edit button
    StudentRecord *studentBeingEdited = findStudentFromID(sender()->property("StudentID").toInt());
    if(studentBeingEdited == nullptr) {
        // student not found, somehow
        return;
    }

    // remove this student's current attribute responses from the counts in dataOptions
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        const QString &currentStudentResponse = studentBeingEdited->attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty()) {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : std::as_const(setOfResponsesFromStudent)) {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
            else {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
        }
    }

    //Open window with the student record in it
    auto *win = new editOrAddStudentDialog(*studentBeingEdited, dataOptions, this, false);

    //If user clicks OK, replace student in the database with edited copy
    const int reply = win->exec();
    if(reply == QDialog::Accepted) {
        studentBeingEdited->createTooltip(*dataOptions);
        studentBeingEdited->URM = teamingOptions->URMResponsesConsideredUR.contains(studentBeingEdited->URMResponse);

        rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    }

    // add back in this student's attribute responses from the counts in dataOptions and update the attribute tabs to show the counts
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        const QString &currentStudentResponse = studentBeingEdited->attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty()) {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : std::as_const(setOfResponsesFromStudent)) {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                }
            }
            else {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
            }            
        }
        attributeWidgets[attribute]->setValues();
    }

    delete win;
    saveState();
}


void gruepr::removeAStudent(const QString &name)
{
    if(!name.isEmpty()) {
        // use the name to find the ID
        long long ID = -1;
        // don't have index, need to search and locate based on name
        for(const auto &student : std::as_const(students)) {
            if(name.compare((student.firstname + " " + student.lastname), Qt::CaseInsensitive) == 0) {
                ID = student.ID;
                break;
            }
        }
        if(ID != -1) {
            removeAStudent(ID, true);
        }
    }
}


void gruepr::removeAStudent(const long long ID, const bool delayVisualUpdate)
{
    StudentRecord *studentBeingRemoved = findStudentFromID(ID);
    if(studentBeingRemoved == nullptr) {
        // student not found, somehow
        return;
    }

    if(teamingOptions->haveAnyRequiredTeammates || teamingOptions->haveAnyRequestedTeammates) {
        // remove this student from all other students who might have them as required/prevented/requested
        for(auto &student : students) {
            student.requiredWith.remove(ID);
            student.preventedWith.remove(ID);
            student.requestedWith.remove(ID);
        }
    }

    // update in dataOptions and then the attribute tab the count of each attribute response
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        const QString &currentStudentResponse = studentBeingRemoved->attributeResponse[attribute];
        if(!currentStudentResponse.isEmpty()) {
            if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                //need to process each one
                const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                for(const auto &responseFromStudent : std::as_const(setOfResponsesFromStudent)) {
                    dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]--;
                }
            }
            else {
                dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]--;
            }
        }
        attributeWidgets[attribute]->setValues();
    }

    //Remove the student
    studentBeingRemoved->deleted = true;

    if(delayVisualUpdate) {
        return;
    }

    rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    saveState();
}


void gruepr::addAStudent()
{
    if(students.size() < MAX_STUDENTS) {
        //Open window with a blank student record in it
        StudentRecord newStudent;
        auto *win = new editOrAddStudentDialog(newStudent, dataOptions, this, true);

        //If user clicks OK, add student to the database
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            newStudent.ID = students.size();
            newStudent.createTooltip(*dataOptions);
            newStudent.URM = teamingOptions->URMResponsesConsideredUR.contains(newStudent.URMResponse);
            newStudent.ambiguousSchedule = (newStudent.availabilityChart.count("√") == 0 ||
                                           (newStudent.availabilityChart.count("√") == (dataOptions->dayNames.size() * dataOptions->timeNames.size())));
            students << newStudent;

            // update in dataOptions and then the attribute tab the count of each attribute response
            for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
                const QString &currentStudentResponse = newStudent.attributeResponse[attribute];
                if(!currentStudentResponse.isEmpty()) {
                    if((dataOptions->attributeType[attribute] == DataOptions::AttributeType::multicategorical) ||
                        (dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                        //need to process each one
                        const QStringList setOfResponsesFromStudent = currentStudentResponse.split(',', Qt::SkipEmptyParts);
                        for(const auto &responseFromStudent : std::as_const(setOfResponsesFromStudent)) {
                            dataOptions->attributeQuestionResponseCounts[attribute][responseFromStudent.trimmed()]++;
                        }
                    }
                    else {
                        dataOptions->attributeQuestionResponseCounts[attribute][currentStudentResponse]++;
                    }
                }
                attributeWidgets[attribute]->setValues();
            }
            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
        }
        delete win;
    }
    else {
        grueprGlobal::errorMessage(this, tr("Cannot add student."),
                                   tr("Sorry, we cannot add another student.\nThis version of gruepr allows a maximum of ") +
                                   QString::number(MAX_STUDENTS) + ".");
    }
    saveState();
}


void gruepr::compareStudentsToRoster()
{
    // Open the roster file
    const QSettings savedSettings;
    CsvFile rosterFile(CsvFile::Delimiter::comma, this);
    if(!rosterFile.open(this, CsvFile::Operation::read, tr("Open Student Roster File"), savedSettings.value("saveFileLocation").toString(), tr("Roster File"))) {
        return;
    }

    QStringList names, emails;
    if(loadRosterData(rosterFile, names, emails)) {
        bool dataHasChanged = false;

        // load all current names from the survey so we can later remove them as they're found in the roster and be left with problem cases
        QStringList namesNotFound;
        namesNotFound.reserve(students.size());
        for(const auto &student : std::as_const(students)) {
            namesNotFound << student.firstname + " " + student.lastname;
        }

        // create a place to save info for names with mismatched emails
        QList<StudentRecord*> studentsWithDiffEmail;
        studentsWithDiffEmail.reserve(students.size());

        for(auto &name : names) {
            // first try to find student in the existing students data
            // start at first student in database and look until we find a matching firstname + " " +last name
            StudentRecord *stu = nullptr;
            for(auto &student : students) {
                if(name.compare(student.firstname + " " + student.lastname, Qt::CaseInsensitive) == 0) {
                    stu = &student;
                    break;
                }
            }

            // get the email corresponding to this name on the roster
            const auto index = names.indexOf(name);
            const QString &rosterEmail = ((index >= 0 && index < emails.size())? emails.at(index) : "");

            if(stu != nullptr) {
                // Exact match for name was found in existing students
                namesNotFound.removeAll(stu->firstname + " " + stu->lastname);
                if(!emails.isEmpty()) {
                    if(stu->email.compare(rosterEmail, Qt::CaseInsensitive) != 0) {
                        // Email in survey doesn't match roster
                        studentsWithDiffEmail << stu;
                    }
                }
            }
            else {
                // No exact match, so list possible matches sorted by Levenshtein distance and allow user to pick a match, add as a new student, or ignore
                auto *choiceWindow = new findMatchingNameDialog(students, name, this, "", true, emails.isEmpty()? "" : rosterEmail);
                if(choiceWindow->exec() == QDialog::Accepted) {  // not ignoring this student
                    if(choiceWindow->addStudent) {   // add as a new student
                        dataHasChanged = true;

                        StudentRecord newStudent;
                        newStudent.surveyTimestamp = QDateTime();
                        newStudent.ID = students.size();
                        newStudent.firstname = name.split(" ").first();
                        newStudent.lastname = name.split(" ").mid(1).join(" ");
                        if(!emails.isEmpty()) {
                            newStudent.email = rosterEmail;
                        }
                        newStudent.URM = teamingOptions->URMResponsesConsideredUR.contains(newStudent.URMResponse);
                        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
                            newStudent.attributeVals[attribute] << -1;
                        }
                        newStudent.ambiguousSchedule = true;
                        newStudent.createTooltip(*dataOptions);

                        students << newStudent;

                        numActiveStudents = students.size();
                    }
                    else {  // selected an inexact match
                        const QString surveyName = choiceWindow->currSurveyName;
                        namesNotFound.removeAll(surveyName);
                        for(auto &student : students) {
                            if(surveyName != (student.firstname + " " + student.lastname)) {
                                stu = &student;
                                break;
                            }
                        }
                        if(stu != nullptr) {
                            if(choiceWindow->useRosterEmail) {
                                dataHasChanged = true;
                                stu->email = emails.isEmpty()? "" : rosterEmail;
                                stu->createTooltip(*dataOptions);
                            }
                            if(choiceWindow->useRosterName) {
                                dataHasChanged = true;
                                stu->firstname = name.split(" ").first();
                                stu->lastname = name.split(" ").mid(1).join(" ");
                                stu->createTooltip(*dataOptions);
                            }
                        }
                    }
                }
                delete choiceWindow;
            }
        }

        bool keepAsking = true, makeTheChange = false;
        int i = 0;

        if(!emails.isEmpty()) {
            // Now handle the times where the roster and survey have different email addresses
            for(auto &student : studentsWithDiffEmail) {
                const QString surveyName = student->firstname + " " + student->lastname;
                const QString surveyEmail = student->email;
                if(keepAsking) {
                    auto *whichEmailWindow = new QMessageBox(QMessageBox::Question, tr("Email addresses do not match"),
                                                             tr("This student on the roster:") +
                                                                 "<br><b>" + surveyName + "</b><br>" +
                                                                 tr("has a different email address in the survey.") + "<br><br>" +
                                                                 tr("Select one of the following email addresses:") + "<br>" +
                                                                 tr("Survey: ") + "<b>" + surveyEmail + "</b><br>" +
                                                                 tr("Roster: ") + "<b>" +  emails.at(names.indexOf(surveyName))  + "</b><br>",
                                                             QMessageBox::Ok | QMessageBox::Cancel, this);
                    whichEmailWindow->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                                               Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    whichEmailWindow->setStyleSheet(LABEL10PTSTYLE);
                    whichEmailWindow->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
                    whichEmailWindow->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
                    auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(studentsWithDiffEmail.size() - i) + tr(" students)"), whichEmailWindow);
                    applyToAll->setStyleSheet(CHECKBOXSTYLE);
                    whichEmailWindow->setCheckBox(applyToAll);
                    connect(applyToAll, &QCheckBox::clicked, whichEmailWindow, [&keepAsking] (bool checked) {keepAsking = !checked;});
                    whichEmailWindow->button(QMessageBox::Ok)->setText(tr("Use survey email address"));
                    connect(whichEmailWindow->button(QMessageBox::Ok), &QPushButton::clicked, whichEmailWindow, &QDialog::accept);
                    whichEmailWindow->button(QMessageBox::Cancel)->setText(tr("Use roster email address"));
                    connect(whichEmailWindow->button(QMessageBox::Cancel), &QPushButton::clicked, whichEmailWindow, &QDialog::reject);

                    if(whichEmailWindow->exec() == QDialog::Rejected) {
                        dataHasChanged = true;
                        makeTheChange = true;
                        student->email = emails.at(names.indexOf(surveyName));
                        student->createTooltip(*dataOptions);
                    }
                    else {
                        makeTheChange = false;
                    }
                    delete whichEmailWindow;
                }
                else if(makeTheChange) {
                    student->email = emails.at(names.indexOf(surveyName));
                    student->createTooltip(*dataOptions);
                }
                i++;
            }
        }

        // Finally, handle the names on the survey that were not found in the roster
        keepAsking = true, makeTheChange = false;
        i = 0;
        for(auto &name : namesNotFound) {
            if(keepAsking) {
                auto *keepOrDeleteWindow = new QMessageBox(QMessageBox::Question, tr("Student not in roster file"),
                                                           tr("This student:") +
                                                               "<br><b>" + name + "</b><br>" +
                                                               tr("submitted a survey but was not found in the roster file.") + "<br><br>" +
                                                               tr("Should we keep this student or remove them?"),
                                                           QMessageBox::Ok | QMessageBox::Cancel, this);
                keepOrDeleteWindow->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                                      Qt::KeepAspectRatio, Qt::SmoothTransformation));
                keepOrDeleteWindow->setStyleSheet(LABEL10PTSTYLE);
                keepOrDeleteWindow->button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
                keepOrDeleteWindow->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
                auto *applyToAll = new QCheckBox(tr("Apply to all remaining (") + QString::number(namesNotFound.size() - i) + tr(" students)"), keepOrDeleteWindow);
                applyToAll->setStyleSheet(CHECKBOXSTYLE);
                keepOrDeleteWindow->setCheckBox(applyToAll);
                connect(applyToAll, &QCheckBox::clicked, keepOrDeleteWindow, [&keepAsking] (bool checked) {keepAsking = !checked;});
                keepOrDeleteWindow->button(QMessageBox::Ok)->setText(tr("Keep ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Ok), &QPushButton::clicked, keepOrDeleteWindow, &QMessageBox::accept);
                keepOrDeleteWindow->button(QMessageBox::Cancel)->setText(tr("Remove ") + name);
                connect(keepOrDeleteWindow->button(QMessageBox::Cancel), &QPushButton::clicked, keepOrDeleteWindow, &QMessageBox::reject);

                if(keepOrDeleteWindow->exec() == QMessageBox::Rejected) {
                    dataHasChanged = true;
                    makeTheChange = true;
                    removeAStudent(name);
                }
                else {
                    makeTheChange = false;
                }

                delete keepOrDeleteWindow;
            }
            else if(makeTheChange) {
                removeAStudent(name);
            }
            i++;
        }

        if(dataHasChanged) {
            rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
            saveState();
        }
    }
    rosterFile.close();
}


void gruepr::rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable()
{
    // go back through all records to see if any are duplicates; assume each isn't and then check
    for(int index = 0; index < students.size(); index++) {
        auto &student1 = students[index];
        student1.duplicateRecord = false;
        for(int index2 = 0; index2 < index; index2++) {
            auto &student2 = students[index2];
            if(!student1.deleted && !student2.deleted &&
               (((student1.firstname + student1.lastname == student2.firstname + student2.lastname) && !((student1.firstname + student1.lastname).isEmpty())) ||
                ((student1.email == student2.email) && !student1.email.isEmpty()))) {
                student1.duplicateRecord = true;
                student2.duplicateRecord = true;
                student2.createTooltip(*dataOptions);
            }
        }
        student1.createTooltip(*dataOptions);
    }

    // Re-build the URM info
    if(dataOptions->URMIncluded) {
        dataOptions->URMResponses.clear();
        for(const auto &student : std::as_const(students)) {
            if(!dataOptions->URMResponses.contains(student.URMResponse, Qt::CaseInsensitive)) {
                dataOptions->URMResponses << student.URMResponse;
            }
        }
        QCollator sortAlphanumerically;
        sortAlphanumerically.setNumericMode(true);
        sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(dataOptions->URMResponses.begin(), dataOptions->URMResponses.end(), sortAlphanumerically);
        if(dataOptions->URMResponses.contains("--")) {
            // put the blank response option at the end of the list
            dataOptions->URMResponses.removeAll("--");
            dataOptions->URMResponses << "--";
        }
    }

    // Re-build the section options in the selection box
    if(dataOptions->sectionIncluded) {
        sectionSelectionBox->blockSignals(true);
        sectionSelectionBox->clear();
        dataOptions->sectionNames.clear();
        for(const auto &student : std::as_const(students)) {
            if(!student.deleted && !dataOptions->sectionNames.contains(student.section, Qt::CaseInsensitive)) {
                dataOptions->sectionNames << student.section;
            }
        }
        if(dataOptions->sectionNames.size() > 1) {
            QCollator sortAlphanumerically;
            sortAlphanumerically.setNumericMode(true);
            sortAlphanumerically.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(dataOptions->sectionNames.begin(), dataOptions->sectionNames.end(), sortAlphanumerically);
            sectionSelectionBox->addItem(tr("Students in all sections together"));
            sectionSelectionBox->addItem(tr("Students in all sections, each section separately"));
            sectionSelectionBox->insertSeparator(2);
            sectionSelectionBox->addItems(dataOptions->sectionNames);
        }
        else {
            sectionSelectionBox->addItem(tr("Only one section in the data."));
        }
        sectionSelectionBox->blockSignals(false);

        if(sectionSelectionBox->findText(teamingOptions->sectionName) != -1) {
            sectionSelectionBox->setCurrentText(teamingOptions->sectionName);
        }
    }

    // Refresh student table data
    refreshStudentDisplay();
    ui->studentTable->clearSortIndicator();

    // Load new team sizes in selection box
    idealTeamSizeBox->setMaximum(std::max(2ll,numActiveStudents/2));
    changeIdealTeamSize();
}


void gruepr::changeIdealTeamSize()
{
    const int idealSize = idealTeamSizeBox->value();

    // First, make sure we don't end up penalizing teams for having fewer requested teammates than the team size allows
    if(teamingOptions->numberRequestedTeammatesGiven > idealSize) {
        teamingOptions->numberRequestedTeammatesGiven = idealSize;
    }

    // put suitable options in the team size selection box, depending on whether the number of students is evenly divisible by this desired team size
    teamSizeBox->setUpdatesEnabled(false);

    // typically just figuring out team sizes for one section or for all students together,
    // but need to re-calculate for each section if we will team all sections independently
    const bool calculatingSeparateSections = (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) && !multipleSectionsInProgress;
    const int numSectionsToCalculate = (calculatingSeparateSections? int(dataOptions->sectionNames.size()) : 1);
    long long numStudentsBeingTeamed = numActiveStudents;
    int smallerTeamsSizeA=0, smallerTeamsSizeB=0, numSmallerATeams=0, largerTeamsSizeA=0, largerTeamsSizeB=0, numLargerATeams=0;
    int cumNumSmallerATeams=0, cumNumSmallerBTeams = 0, cumNumLargerATeams=0, cumNumLargerBTeams = 0;
    for(int section = 0; section < numSectionsToCalculate; section++) {
        teamSizeBox->clear();

        if(calculatingSeparateSections) {
            // if teaming all sections separately, figure out how many students in this section
            const QString sectionName = sectionSelectionBox->itemText(section + 3);
            numStudentsBeingTeamed = 0;
            for(const auto &student : std::as_const(students)) {
                if(student.section == sectionName && !student.deleted) {
                    numStudentsBeingTeamed++;
                }
            }
        }
        else if(multipleSectionsInProgress) {
            const QString sectionName = sectionSelectionBox->currentText();
            numStudentsBeingTeamed = 0;
            for(const auto &student : std::as_const(students)) {
                if(student.section == sectionName && !student.deleted) {
                    numStudentsBeingTeamed++;
                }
            }
        }
        else {
            numStudentsBeingTeamed = numActiveStudents;
        }

        // reset the potential team sizes, and reserve sufficient memory
        teamingOptions->numTeamsDesired = std::max(1ll, numStudentsBeingTeamed/idealSize);
        // qDebug() << "Num teams desired being calculated:" << teamingOptions->numTeamsDesired;
        // qDebug() << "Num students being teamed: "<< numStudentsBeingTeamed;
        teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired;
        teamingOptions->smallerTeamsSizes.clear();
        teamingOptions->smallerTeamsSizes.reserve(MAX_STUDENTS);
        teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;
        teamingOptions->largerTeamsSizes.clear();
        teamingOptions->largerTeamsSizes.reserve(MAX_STUDENTS);
        for(int teamNum = 0; teamNum < MAX_STUDENTS; teamNum++) {
            teamingOptions->smallerTeamsSizes << 0;
            teamingOptions->largerTeamsSizes << 0;
        }

        if(numStudentsBeingTeamed%idealSize != 0) {      //if teams can't be evenly divided into this size
            // What are the team sizes when desiredTeamSize represents a maximum size?
            teamingOptions->smallerTeamsNumTeams = teamingOptions->numTeamsDesired+1;
            for(int student = 0; student < numStudentsBeingTeamed; student++) {     // run through every student
                // add one student to each team (with 1 additional team relative to before) in turn until we run out of students
                (teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams])++;
                smallerTeamsSizeA = teamingOptions->smallerTeamsSizes[student%teamingOptions->smallerTeamsNumTeams];  // the larger of the two (uneven) team sizes
                numSmallerATeams = (student%teamingOptions->smallerTeamsNumTeams)+1;                                  // the number of larger teams
            }
            smallerTeamsSizeB = smallerTeamsSizeA - 1;                  // the smaller of the two (uneven) team sizes

            // And what are the team sizes when desiredTeamSize represents a minimum size?
            teamingOptions->largerTeamsNumTeams = teamingOptions->numTeamsDesired;
            for(int student = 0; student < numStudentsBeingTeamed; student++) {	// run through every student
                // add one student to each team in turn until we run out of students
                (teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams])++;
                largerTeamsSizeA = teamingOptions->largerTeamsSizes[student%teamingOptions->largerTeamsNumTeams];     // the larger of the two (uneven) team sizes
                numLargerATeams = (student%teamingOptions->largerTeamsNumTeams)+1;                                    // the number of larger teams
            }
            largerTeamsSizeB = largerTeamsSizeA - 1;					// the smaller of the two (uneven) team sizes

            // Add first option to selection box
            const QString smallerTeamOption = writeTeamSizeOption(numSmallerATeams, smallerTeamsSizeA, teamingOptions->numTeamsDesired+1-numSmallerATeams, smallerTeamsSizeB);
            if(numSmallerATeams > 0) {
                cumNumSmallerATeams += numSmallerATeams;
            }
            if((teamingOptions->numTeamsDesired+1-numSmallerATeams) > 0) {
                cumNumSmallerBTeams += teamingOptions->numTeamsDesired+1-numSmallerATeams;
            }

            // Add second option to selection box
            const QString largerTeamOption = writeTeamSizeOption(teamingOptions->numTeamsDesired-numLargerATeams, largerTeamsSizeB, numLargerATeams, largerTeamsSizeA);
            if((teamingOptions->numTeamsDesired-numLargerATeams) > 0) {
                cumNumLargerBTeams += teamingOptions->numTeamsDesired-numLargerATeams;
            }
            if(numLargerATeams > 0) {
                cumNumLargerATeams += numLargerATeams;
            }

            if(!calculatingSeparateSections) {
                teamSizeBox->addItem(smallerTeamOption);
                teamSizeBox->addItem(largerTeamOption);
            }
        }
        else {  // evenly divisible number of students
            cumNumSmallerATeams += teamingOptions->numTeamsDesired;
            smallerTeamsSizeA = idealSize;
            cumNumLargerBTeams += teamingOptions->numTeamsDesired;
            largerTeamsSizeB = idealSize;
            for(int teamNum = 0; teamNum < teamingOptions->numTeamsDesired; teamNum++) {
                teamingOptions->smallerTeamsSizes[teamNum] = idealSize;
                teamingOptions->largerTeamsSizes[teamNum] = idealSize;
            }

            if(!calculatingSeparateSections) {
                teamSizeBox->addItem(QString::number(teamingOptions->numTeamsDesired) + tr(" teams (") + QString::number(idealSize) + tr(" students each)"));
            }
        }

        teamingOptions->smallerTeamsSizes.removeAll(0);
        teamingOptions->smallerTeamsSizes.squeeze();
        teamingOptions->largerTeamsSizes.removeAll(0);
        teamingOptions->largerTeamsSizes.squeeze();
    }

    if(calculatingSeparateSections) {
        // load new team sizes in selection box by adding together the sizes from each section
        const QString smallerTeamOption = writeTeamSizeOption(cumNumSmallerATeams, smallerTeamsSizeA, cumNumSmallerBTeams, smallerTeamsSizeB);
        const QString largerTeamOption = writeTeamSizeOption(cumNumLargerBTeams, largerTeamsSizeB, cumNumLargerATeams, largerTeamsSizeA);

        teamSizeBox->addItem(smallerTeamOption);
        if(smallerTeamOption != largerTeamOption) {
            teamSizeBox->addItem(largerTeamOption);
        }
    }
    else {
        // allow custom team sizes (too complicated to allow this if teaming all sections separately
        teamSizeBox->insertSeparator(teamSizeBox->count());
        teamSizeBox->addItem(tr("Custom team sizes"));
    }

    // if we have fewer than MIN_STUDENTS students somehow, disable the form teams button
    letsDoItButton->setEnabled(numStudentsBeingTeamed >= MIN_STUDENTS);
    //qDebug() << teamSizeBox->currentText();
    teamSizeBox->setUpdatesEnabled(true);
}


QString gruepr::writeTeamSizeOption(const int numTeamsA, const int teamsizeA, const int numTeamsB, const int teamsizeB)
{
    if((numTeamsA == 0) || (numTeamsB == 0)) {
        const QString numTeamsString = QString::number((numTeamsA == 0)? numTeamsB : numTeamsA);
        const QString numStudentsString = QString::number((numTeamsA == 0)? teamsizeB : teamsizeA);
        return numTeamsString + tr(" teams (") + numStudentsString + tr(" students each)");
    }
    QString teamOption = QString::number(numTeamsA + numTeamsB) + ((numTeamsA + numTeamsB > 1)? tr(" teams") : tr(" team")) + " (";
    if(numTeamsA > 0) {
        teamOption += QString::number(numTeamsA) + tr(" of ") + QString::number(teamsizeA) + tr(" student");
        if(teamsizeA > 1) {
            teamOption += "s";
        }
    }
    if((numTeamsA > 0) && (numTeamsB > 0)) {
        teamOption += ";  ";
    }
    if(numTeamsB > 0) {
        teamOption += QString::number(numTeamsB) + " of " + QString::number(teamsizeB) + tr(" student");
        if(teamsizeB > 1) {
            teamOption += "s";
        }
    }
    teamOption += ")";

    return teamOption;
}


void gruepr::chooseTeamSizes(int index)
{
    auto sizecriterion = qobject_cast<TeamsizeCriterion*>(teamsizeCriteriaCard->criterion);
    if(sizecriterion->teamSizeBox->currentText() == QString::number(teamingOptions->numTeamsDesired) +
                                         tr(" teams (") + QString::number(sizecriterion->idealTeamSizeBox->value()) +
                                         tr(" students each)")) {
        // Evenly divisible teams, all same size
        // qDebug() << "teams desired:";
        // qDebug() << QString::number(teamingOptions->numTeamsDesired);
        setTeamSizes(sizecriterion->idealTeamSizeBox->value());
    }
    else if(sizecriterion->teamSizeBox->currentText() == tr("Custom team sizes")) {
        //Open specialized dialog box to collect teamsizes
        auto *win = new customTeamsizesDialog(numActiveStudents, sizecriterion->idealTeamSizeBox->value(), this);

        //If user clicks OK, use these team sizes, otherwise revert to option 1, smaller team sizes
        const int reply = win->exec();
        if(reply == QDialog::Accepted) {
            teamingOptions->numTeamsDesired = win->numTeams;
            setTeamSizes(win->teamsizes);
        }
        else {
            // Set to index 0 if cancelled
            sizecriterion->teamSizeBox->setCurrentIndex(0);
        }
        delete win;
        return;
    }
    else if(index == 0) {
        // Smaller teams desired
        teamingOptions->numTeamsDesired = teamingOptions->smallerTeamsNumTeams;
        setTeamSizes(teamingOptions->smallerTeamsSizes);
    }
    else if (index == 1) {
        // Larger teams desired
        teamingOptions->numTeamsDesired = teamingOptions->largerTeamsNumTeams;
        setTeamSizes(teamingOptions->largerTeamsSizes);
    }
}


void gruepr::startOptimization()
{
    // User wants no isolated URM, so note the URM students
    if(dataOptions->URMIncluded && teamingOptions->isolatedURMPrevented) {
        for(auto &student : students) {
            student.URM = teamingOptions->URMResponsesConsideredUR.contains(student.URMResponse);
        }
    }

    // Normalize all score factor weights using norm factor = number of factors / total weights of all factors

    //Initialize weights based on priority
    float weight = 10;
    float sumOfWeights = 0;
    int index = 0;
    for (auto *criteriaCard : criteriaCardsList){
        if (criteriaCard->criterion->criteriaType == Criterion::CriteriaType::section ||
            criteriaCard->criterion->criteriaType == Criterion::CriteriaType::teamSize) {
            continue;
        } else {
            teamingOptions->weights[index] = weight;
            criteriaCard->criterion->weight = weight;
            teamingOptions->penaltyStatus[index] = criteriaCard->criterion->penaltyStatus;
            teamingOptions->criterionTypes[index] = criteriaCard->criterion;
        }
        sumOfWeights = sumOfWeights + weight;
        weight = weight/2;
        index++;
    }

    teamingOptions->realNumScoringFactors = index; //initialize realNumScoringFactors to contain number of criteria to optimize

    if (index == 0){
        //just do the randomization bcs no priority
    }

    float normFactor = teamingOptions->realNumScoringFactors / sumOfWeights;
    if(!std::isfinite(normFactor)) {
        // pretty sure this should never be true given the math above!
        normFactor = 0;
    }

    // convert weights to realWeights
    for (int i = 0; i < teamingOptions->realNumScoringFactors; i++) {
        teamingOptions->weights[i] *= normFactor;
        teamingOptions->criterionTypes[i]->weight = teamingOptions->weights[i];
    }

    teamingOptions->realMeetingBlockSize = std::ceil(teamingOptions->meetingBlockSize / dataOptions->scheduleResolution); // divide by length of time block in hours, rounded up

    bestTeamSet.clear();
    finalTeams.clear();
    finalTeams.dataOptions = *dataOptions;

    const bool teamingMultipleSections = (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately);
    multipleSectionsInProgress = teamingMultipleSections;
    const int numSectionsToTeam = (teamingMultipleSections? int(dataOptions->sectionNames.size()) : 1);
    const bool smallerTeamSizesInSelector = (teamSizeBox->currentIndex() == 0);
    for(int section = 0; section < numSectionsToTeam; section++) {
        if(teamingMultipleSections) {
            // team each section one at a time by changing the section
            sectionSelectionBox->setCurrentIndex(section + 3);  // go to the next section (index: 0 = allTogether, 1 = allSeparately, 2 = separator line, 3 = first section)
        }

        // Get the indexes of non-deleted students from desired section(s) and change numStudents accordingly
        int numStudentsInSection = 0;
        studentIndexes.reserve(students.size());
        for(int index = 0; index < students.size(); index++) {
            if(!students[index].deleted &&
                ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
                (teamingOptions->sectionType == TeamingOptions::SectionType::noSections) ||
                (teamingOptions->sectionName == students[index].section))) {
                studentIndexes << index;
                numStudentsInSection++;
            }
        }
        numActiveStudents = numStudentsInSection;
        if(numActiveStudents < 4) {
            continue;
        }

        if(teamingMultipleSections) {
            // now pick the correct team sizes
            if(smallerTeamSizesInSelector || (teamSizeBox->count() == 3)) {
                teamSizeBox->setCurrentIndex(0);
            }
            else {
                teamSizeBox->setCurrentIndex(1);
            }
        }

        // Create a new set of TeamRecords to hold the eventual results
        numTeams = teamingOptions->numTeamsDesired;
        teams.clear();
        teams.dataOptions = *dataOptions;
        teams.reserve(numTeams);
        for(const auto teamSize : std::as_const(teamingOptions->teamSizesDesired)) {
            teams.emplaceBack(&teams.dataOptions, teamSize);
        }

        // Create progress display plot
        progressChart = new BoxWhiskerPlot("", "Generation", "Scores");
        auto *chartView = new QChartView(progressChart);
        chartView->setRenderHint(QPainter::Antialiasing);

        // Create window to display progress, and connect the stop optimization button in the window to the actual stopping of the optimization thread
        const QString sectionName = (teamingMultipleSections? (tr("section ") + QString::number(section + 1) + " / " + QString::number(numSectionsToTeam) + ": " +
                                                          teamingOptions->sectionName) : "");
        progressWindow = new progressDialog(sectionName, chartView, this);
        progressWindow->show();
        connect(progressWindow, &progressDialog::letsStop, this, [this] {QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
                                                                         connect(this, &gruepr::turnOffBusyCursor, this, &QApplication::restoreOverrideCursor);
                                                                         optimizationStoppedmutex.lock();
                                                                         optimizationStopped = true;
                                                                         optimizationStoppedmutex.unlock();
                                                                        });

        // Set up the flag to allow a stoppage and set up futureWatcher to know when results are available
        optimizationStopped = false;
        future = QtConcurrent::run(&gruepr::optimizeTeams, this, studentIndexes);       // spin optimization off into a separate thread
        futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
        multipleSectionsInProgress = (section < (numSectionsToTeam - 1));

        // set the working value of the genetic algorithm's population size and tournament selection probability
        ga.setGAParameters(numActiveStudents);

        // hold here until the optimization is done. This feels really hacky and probably can be improved with something simple!
        QEventLoop loop;
        connect(this, &gruepr::sectionOptimizationFullyComplete, this, [this, &loop, teamingMultipleSections, smallerTeamSizesInSelector] {
            if(teamingMultipleSections && !multipleSectionsInProgress) {
                sectionSelectionBox->setCurrentIndex(1);            // go back to each section separately
                if(smallerTeamSizesInSelector || (teamSizeBox->count() == 3)) {  // pick the correct team sizes
                    teamSizeBox->setCurrentIndex(0);
                }
                else {
                    teamSizeBox->setCurrentIndex(1);
                }
            }
            loop.quit();
        }, static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
        loop.exec();
    }
}


void gruepr::updateOptimizationProgress(const float *const allScores, const int *const orderedIndex,
                                        const int generation, const float scoreStability, const bool unpenalizedGenomePresent)
{
    if((generation % (BoxWhiskerPlot::PLOTFREQUENCY)) == 0) {
        progressChart->loadNextVals(allScores, orderedIndex, ga.populationsize, unpenalizedGenomePresent);
    }

    float maxScore = *std::max_element(allScores, allScores + ga.populationsize);

    if(generation > GA::MAX_GENERATIONS) {
        progressWindow->setText(tr("We have reached ") + QString::number(GA::MAX_GENERATIONS) + tr(" generations."),
                                generation, *std::max_element(allScores, allScores+ga.populationsize), true);
        progressWindow->highlightStopButton();
    }
    else if((generation >= GA::MIN_GENERATIONS) && (scoreStability > GA::MIN_SCORE_STABILITY)) {
        progressWindow->setText(tr("Score appears to be stable!"), generation, *std::max_element(allScores, allScores+ga.populationsize), true);
        progressWindow->highlightStopButton();
    }
    else {
        progressWindow->setText(tr("Please wait while your grueps are created!"), generation, *std::max_element(allScores, allScores+ga.populationsize), false);
    }
}


void gruepr::optimizationComplete()
{
    // update UI
    delete progressChart;
    delete progressWindow;

    // Get the results
    bestTeamSet << future.result();
    finalTeams << teams;
    studentIndexes.clear();

    emit sectionOptimizationFullyComplete();

    if(multipleSectionsInProgress) {
        return;
    }

    //alert
    QApplication::beep();
    QApplication::alert(this);

    // Load students into teams
    teams = finalTeams;
    int indexInTeamset = 0;
    for(auto &team : teams) {
        auto &IDList = team.studentIDs;
        IDList.clear();
        for(int studentNum = 0, size = team.size; studentNum < size; studentNum++) {
            IDList << students.at(bestTeamSet.at(indexInTeamset)).ID;
            indexInTeamset++;
        }
        //sort teammates within a team alphabetically by lastname,firstname
        std::sort(IDList.begin(), IDList.end(), [this] (const int a, const int b)
                  { const StudentRecord *const studentA = findStudentFromID(a);
                      const StudentRecord *const studentB = findStudentFromID(b);
                      return ((studentA->lastname + studentA->firstname) <
                              (studentB->lastname + studentB->firstname));
                  });
    }

    // Load scores and info into the teams
    calcTeamScores(students, numActiveStudents, teams, teamingOptions);
    for(auto &team : teams) {
        team.refreshTeamInfo(students, teamingOptions->realMeetingBlockSize);
    }

    for(int team = 0; team < teams.size(); team++) {
        teams[team].name = QString::number(team+1);
        teams[team].createTooltip();
    }

    // Sort teams by 1st student's name, then set default teamnames and create tooltips
    // std::sort(teams.begin(), teams.end(), [this](const TeamRecord &a, const TeamRecord &b)
    //           { const StudentRecord *const firstStudentOnTeamA = findStudentFromID(a.studentIDs.at(0));
    //             const StudentRecord *const firstStudentOnTeamB = findStudentFromID(b.studentIDs.at(0));
    //             return ((firstStudentOnTeamA->lastname + firstStudentOnTeamA->firstname) <
    //                     (firstStudentOnTeamB->lastname + firstStudentOnTeamB->firstname));
    //           });

    // Display the results in a new tab
    // Eventually maybe this should let the tab take ownership of the teams pointer, deleting when the tab is closed!
    const QString teamSetName = tr("Team set ") + QString::number(teamingOptions->teamsetNumber);
    auto *teamTab = new TeamsTabItem(*teamingOptions, teams, students, dataOptions->sectionNames, teamSetName, letsDoItButton, this);
    ui->dataDisplayTabWidget->addTab(teamTab, teamSetName);
    numTeams = int(teams.size());
    teamingOptions->teamsetNumber++;
    connect(teamTab, &TeamsTabItem::saveState, this, &gruepr::saveState);

    ui->dataDisplayTabWidget->setCurrentWidget(teamTab);
    saveState();
}


void gruepr::dataDisplayTabClose(int closingTabIndex)
{
    // don't close the student tab!
    if(closingTabIndex < 1) {
        return;
    }

    auto *tab = ui->dataDisplayTabWidget->widget(closingTabIndex);
    ui->dataDisplayTabWidget->removeTab(closingTabIndex);
    tab->deleteLater();
    saveState();
}

void gruepr::editDataDisplayTabName(int tabIndex)
{
    // don't do anything if they double-clicked on the student tab
    if(tabIndex < 1) {
        return;
    }

    // pop up at the cursor location a little window to edit the tab title
    auto *win = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    win->setWindowTitle(tr("Rename this team set"));
    win->setSizeGripEnabled(true);
    win->move(QCursor::pos());
    auto *layout = new QVBoxLayout(win);
    auto *newNameEditor = new QLineEdit(win);
    newNameEditor->setStyleSheet(LINEEDITSTYLE);
    newNameEditor->setText(ui->dataDisplayTabWidget->tabText(tabIndex));
    newNameEditor->setPlaceholderText(ui->dataDisplayTabWidget->tabText(tabIndex));
    layout->addWidget(newNameEditor);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, win);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, win, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, win, &QDialog::reject);
    layout->addWidget(buttonBox);
    newNameEditor->selectAll();
    if(win->exec() == QDialog::Accepted && !newNameEditor->text().isEmpty()) {
        ui->dataDisplayTabWidget->setTabText(tabIndex, newNameEditor->text());
        auto *tab = qobject_cast<TeamsTabItem*>(ui->dataDisplayTabWidget->widget(tabIndex));
        tab->tabName = newNameEditor->text();
    }
    win->deleteLater();
    saveState();
}


//////////////////
//Load window geometry and default teaming options saved from previous run. If non-existant, load app defaults.
//////////////////
void gruepr::loadDefaultSettings()
{
    QSettings savedSettings;
    //Restore teaming options
    const int numGenderRules = savedSettings.beginReadArray("genderIdentityRules");
    for(int rule = 0; rule < numGenderRules; rule++) {
        savedSettings.setArrayIndex(rule);
        const QStringList ruleVal = savedSettings.value("ruleVal", "").toString().split(',');
        if(ruleVal.size() == 3) {
            teamingOptions->genderIdentityRules[grueprGlobal::stringToGender(ruleVal.at(0))][ruleVal.at(1)].append(ruleVal.at(2).toInt());
        }
    }
    savedSettings.endArray();
    teamingOptions->singleGenderPrevented = savedSettings.value("singleGenderPrevented", false).toBool();
    teamingOptions->isolatedURMPrevented = savedSettings.value("isolatedURMPrevented", false).toBool();
    teamingOptions->minTimeBlocksOverlap = savedSettings.value("minTimeBlocksOverlap", 4).toInt();
    teamingOptions->desiredTimeBlocksOverlap = savedSettings.value("desiredTimeBlocksOverlap", 8).toInt();
    teamingOptions->meetingBlockSize = savedSettings.value("meetingBlockSize", 1).toFloat();
    teamingOptions->realMeetingBlockSize = savedSettings.value("realMeetingBlockSize", 1).toInt();
    teamingOptions->scheduleWeight = savedSettings.value("scheduleWeight", 4).toFloat();
    savedSettings.beginReadArray("Attributes");
    auto diversity = QMetaEnum::fromType<Criterion::AttributeDiversity>();
    for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum) {
        savedSettings.setArrayIndex(attribNum);
        teamingOptions->attributeDiversity[attribNum] =
                static_cast<Criterion::AttributeDiversity>(diversity.keyToValue(qPrintable(savedSettings.value("attributeDiversity", 0).toString())));
        teamingOptions->attributeWeights[attribNum] = savedSettings.value("Weight", 1).toFloat();
        // Shouldn't re-load the incompatible and required responses if working with new survey data
        if(dataOptions->dataSource == DataOptions::DataSource::fromPrevWork) {
            const int numIncompats = savedSettings.beginReadArray("incompatibleResponses");
            for(int incompResp = 0; incompResp < numIncompats; incompResp++) {
                savedSettings.setArrayIndex(incompResp);
                const QStringList incompats = savedSettings.value("incompatibleResponses", "").toString().split(',');
                teamingOptions->incompatibleAttributeValues[attribNum].append({incompats.at(0).toInt(),incompats.at(1).toInt()});
            }
            savedSettings.endArray();
            const int numRequireds = savedSettings.beginReadArray("requiredResponses");
            for(int requiredResp = 0; requiredResp < numRequireds; requiredResp++) {
                savedSettings.setArrayIndex(requiredResp);
                const int required = savedSettings.value("requiredResponse", "").toInt();
                teamingOptions->requiredAttributeValues[attribNum] << required;
            }
            savedSettings.endArray();
        }
    }
    savedSettings.endArray();
    teamingOptions->numberRequestedTeammatesGiven = savedSettings.value("requestedTeammateNumber", 1).toInt();
}


//////////////////
//Enable the appropriate UI settings when loading a set of students
//////////////////
void gruepr::loadUI()
{
    //Restore window geometry
    adjustSize();
    const QSettings savedSettings;
    restoreGeometry(savedSettings.value("windowGeometry").toByteArray());

    //Set the label and icon for the data source
    ui->dataSourceLabel->setText(dataOptions->dataSourceName);
    if(dataOptions->dataSource == DataOptions::DataSource::fromGoogle) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/google.png"));
    }
    else if(dataOptions->dataSource == DataOptions::DataSource::fromCanvas) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/canvas.png"));
    }
    else if(dataOptions->dataSource == DataOptions::DataSource::fromPrevWork) {
        ui->dataSourceIcon->setPixmap(QPixmap(":/icons_new/icon.svg"));
    }

    idealTeamSizeBox->setMaximum(std::max(2ll,numActiveStudents/2));
    //qDebug() << "Calling change ideal team size in loadUI:";

    if(dataOptions->sectionIncluded) {
        sectionSelectionBox->blockSignals(true);
        if(dataOptions->sectionNames.size() > 1) {
            sectionSelectionBox->addItem(tr("Students in all sections together"));
            sectionSelectionBox->addItem(tr("Students in all sections, each section separately"));
            sectionSelectionBox->insertSeparator(2);
            sectionSelectionBox->addItems(dataOptions->sectionNames);
            if(teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) {
                sectionSelectionBox->setCurrentIndex(1);
            }
            else if(teamingOptions->sectionType == TeamingOptions::SectionType::oneSection) {
                sectionSelectionBox->setCurrentText(teamingOptions->sectionName);
            }
            else {
                sectionSelectionBox->setCurrentIndex(0);
            }
        }
        else {
            if(dataOptions->sectionNames.size() > 0) {     // (must be only one section, but checking not empty just so it doesn't crash on .first()...)
                sectionSelectionBox->addItem(dataOptions->sectionNames.first());
            }
            else {
                sectionSelectionBox->addItem(tr("No section data."));
            }
            teamingOptions->sectionType = TeamingOptions::SectionType::noSections;
            sectionCriteriaCard->hide();
        }
        teamingOptions->sectionName = sectionSelectionBox->currentText();
        sectionSelectionBox->blockSignals(false);
    }
    else {
        teamingOptions->sectionType = TeamingOptions::SectionType::noSections;
    }

    refreshStudentDisplay();
    ui->studentTable->resetTable();

    //Initialize all cards, but do not add them to the layout
    for (int attribute = 0; attribute < dataOptions->numAttributes; attribute++){
        teamingOptions->attributeSelected.removeAll(attribute);
        QString title = "Multiple Choice: "+ dataOptions->attributeQuestionText.at(attribute);
        initializedAttributeCriteriaCards << new GroupingCriteriaCard(Criterion::CriteriaType::attributeQuestion, dataOptions, teamingOptions, this, title, true, attribute);
        auto *currentAttributeCard = initializedAttributeCriteriaCards.last();
        const auto &currentAttributeCriterion = qobject_cast<AttributeCriterion*>(currentAttributeCard->criterion);
        attributeWidgets << currentAttributeCriterion->attributeWidget;
        currentAttributeCriterion->attributeWidget->setValues(); //update issue
        connect(currentAttributeCard, &GroupingCriteriaCard::criteriaCardSwapRequested, this, &gruepr::swapCriteriaCards);
        connect(currentAttributeCard, &GroupingCriteriaCard::criteriaCardMoved, this, &gruepr::doAutoScroll);
        connect(currentAttributeCard, &GroupingCriteriaCard::deleteCardRequested, this, &gruepr::deleteCriteriaCard);
        connect(currentAttributeCard, &GroupingCriteriaCard::includePenaltyStateChanged, this, &gruepr::refreshCriteriaLayout);

        //(re)set the weight to zero for any attributes with just one value in the data
        if(dataOptions->attributeVals[attribute].size() == 1) {
            teamingOptions->attributeWeights[attribute] = 0;
        }
        currentAttributeCard->setVisible(false); //just initialize, but initially false visibility
    }

    //Remove duplicates
    if(std::any_of(students.constBegin(), students.constEnd(), [](const StudentRecord &student){return student.duplicateRecord;})) {
        grueprGlobal::warningMessage(this, "gruepr", tr("There appears to be at least one student with multiple survey submissions. "
                                                        "Possible duplicates are marked with a yellow background in the edit and remove buttons."), tr("OK"));
    }
    refreshStudentDisplay();
    ui->studentTable->resetTable();
    changeIdealTeamSize();    // load new team sizes in selection box
    chooseTeamSizes(0);
}

//////////////////
// Save everything for future re-opening
//////////////////
void gruepr::saveState()
{
    QSettings savedSettings;

    QFile saveFile(dataOptions->saveStateFileName, this);
    if(saveFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
        QJsonObject content;
        content["teamingoptions"] = teamingOptions->toJson();
        content["dataoptions"] = dataOptions->toJson();
        QJsonArray studentjsons;
        for(const auto &student : std::as_const(students)) {
            studentjsons.append(student.toJson());
        }
        content["students"] = studentjsons;
        QJsonArray teamsetjsons;
        for(int tabIndex = 1; tabIndex < ui->dataDisplayTabWidget->count(); tabIndex++) {
            const auto *tab = qobject_cast<TeamsTabItem*>(ui->dataDisplayTabWidget->widget(tabIndex));
            teamsetjsons.append(tab->toJson());
        }
        content["teamsets"] = teamsetjsons;
        const QJsonDocument doc(content);
        saveFile.write(doc.toJson(QJsonDocument::Compact));
        saveFile.close();

        //find which savestate this is in the settings
        const int numIndexes = savedSettings.beginReadArray("prevWorks");
        int index = -1;
        for(int i = 0; i < numIndexes; i++) {
            savedSettings.setArrayIndex(i);
            if(savedSettings.value("prevWorkFile", "").toString().compare(saveFile.fileName(), Qt::CaseInsensitive) == 0) {
                index = i;
            }
        }
        savedSettings.endArray();
        savedSettings.beginWriteArray("prevWorks");
        if(index == -1) {
            savedSettings.setArrayIndex(numIndexes);
            savedSettings.setValue("prevWorkName", dataOptions->dataSourceName);
            savedSettings.setValue("prevWorkFile", saveFile.fileName());
            savedSettings.setValue("prevWorkDate", QDateTime::currentDateTime().toString(QLocale::system().dateTimeFormat(QLocale::LongFormat)));
        }
        else {
            savedSettings.setArrayIndex(index);
            savedSettings.setValue("prevWorkDate", QDateTime::currentDateTime().toString(QLocale::system().dateTimeFormat(QLocale::LongFormat)));
            savedSettings.setArrayIndex(numIndexes-1); // go to the end of the array so that we still have access to all values next time
        }
        savedSettings.endArray();
    }
}


//////////////////
// Set the "official" team sizes using an array of different sizes or a single, constant size
//////////////////
inline void gruepr::setTeamSizes(const QList<int> &teamSizes)
{
    teamingOptions->teamSizesDesired.clear();
    teamingOptions->teamSizesDesired.reserve(teamingOptions->numTeamsDesired);
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++) {
        teamingOptions->teamSizesDesired << teamSizes.at(team);
    }
}
inline void gruepr::setTeamSizes(const int singleSize)
{
    teamingOptions->teamSizesDesired.clear();
    teamingOptions->teamSizesDesired.reserve(teamingOptions->numTeamsDesired);
    for(int team = 0; team < teamingOptions->numTeamsDesired; team++) {
        teamingOptions->teamSizesDesired << singleSize;
    }
}


//////////////////
// Read the roster datafile, returning true if successful and false if file is invalid
//////////////////
bool gruepr::loadRosterData(CsvFile &rosterFile, QStringList &names, QStringList &emails)
{
    // Read the header row
    if(!rosterFile.readHeader()) {
        // header row could not be read as valid data
        grueprGlobal::errorMessage(this, tr("File error."), tr("This file is empty or there is an error in its format."));
        return false;
    }

    // Ask user what the columns mean
    // Preloading the selector boxes with "unused" except first time "email", "first name", "last name", and "name" are found
    const QList<possFieldMeaning> rosterFieldOptions  = {{"First Name", "((first)|(given)|(preferred)).*(name)", 1},
                                                         {"Last Name", "((last)|(sur)|(family)).*(name)", 1},
                                                         {"Email Address", "(e).*(mail)", 1},
                                                         {"Full Name (First Last)", "(name)", 1},
                                                         {"Full Name (Last, First)", "(name)", 1}};;
    if(rosterFile.chooseFieldMeaningsDialog(rosterFieldOptions, this)->exec() == QDialog::Rejected) {
        return false;
    }

    // set field values now according to uer's selection of field meanings (defulting to -1 if not chosen)
    const int emailField = int(rosterFile.fieldMeanings.indexOf("Email Address"));
    const int firstNameField = int(rosterFile.fieldMeanings.indexOf("First Name"));
    const int lastNameField = int(rosterFile.fieldMeanings.indexOf("Last Name"));
    const int firstLastNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (First Last)"));
    const int lastFirstNameField = int(rosterFile.fieldMeanings.indexOf("Full Name (Last, First)"));

    // Process each row until there's an empty one. Load names and email addresses
    names.clear();
    emails.clear();
    if(rosterFile.hasHeaderRow) {
        rosterFile.readDataRow();
    }
    else {
        rosterFile.readDataRow(CsvFile::ReadLocation::beginningOfFile);
    }
    do {
        QString name;
        if((firstLastNameField >= 0) && (firstLastNameField < rosterFile.fieldValues.size())) {
            name = rosterFile.fieldValues.at(firstLastNameField).trimmed();
        }
        else if((lastFirstNameField >= 0) && (lastFirstNameField < rosterFile.fieldValues.size())) {
            if(rosterFile.fieldValues.at(lastFirstNameField).contains(',')) {
                const QStringList lastandfirstname = rosterFile.fieldValues.at(lastFirstNameField).split(',');
                name = QString(lastandfirstname.at(1) + " " + lastandfirstname.at(0)).trimmed();
            }
            else {
                name = QString(rosterFile.fieldValues.at(lastFirstNameField)).trimmed();
            }
        }
        else if(firstNameField >= 0 || lastNameField >= 0) {
            if((firstNameField >= 0) && firstNameField < rosterFile.fieldValues.size()) {
                name = QString(rosterFile.fieldValues.at(firstNameField)).trimmed();
            }
            if(firstNameField >= 0 && lastNameField >= 0) {
                name = name + " ";
            }
            if((lastNameField >= 0) && (lastNameField < rosterFile.fieldValues.size())) {
                name = name + QString(rosterFile.fieldValues.at(lastNameField)).trimmed();
            }
        }
        else {
            grueprGlobal::errorMessage(this, tr("File error."), tr("This roster does not contain student names."));
            return false;
        }

        if(!name.isEmpty()) {
            names << name;
            if((emailField >= 0) && (emailField < rosterFile.fieldValues.size())){
                emails << rosterFile.fieldValues.at(emailField).trimmed();
            }
        }
    }
    while(rosterFile.readDataRow());


    return true;
}


//////////////////
// Update current student info in table
//////////////////
void gruepr::refreshStudentDisplay()
{
    ui->studentTable->setUpdatesEnabled(false);
    ui->dataDisplayTabWidget->setCurrentIndex(0);
    ui->studentTable->clear();
    ui->studentTable->setSortingEnabled(false);

    ui->studentTable->setColumnCount(2 + (dataOptions->timestampField != DataOptions::FIELDNOTPRESENT? 1 : 0) + (dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT? 1 : 0) +
                                     (dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT? 1 : 0) + (dataOptions->sectionIncluded? 1 : 0));
    const QIcon unsortedIcon(":/icons_new/upDownButton_white.png");
    int column = 0;
    if(dataOptions->timestampField != DataOptions::FIELDNOTPRESENT) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Survey  \n  Timestamp  ")));
    }
    if(dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  First  \n  Name  ")));
    }
    if(dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Last  \n  Name  ")));
    }
    if(dataOptions->sectionIncluded) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Section  ")));
    }
    ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(tr("  Edit")));
    ui->studentTable->setHorizontalHeaderItem(column, new QTableWidgetItem(tr("  Remove")));

    ui->studentTable->setRowCount(students.size());
    numActiveStudents = 0;
    for(const auto &student : std::as_const(students)) {
        column = 0;
        if((numActiveStudents < students.size()) &&            // make sure student exists, hasn't been deleted, and is in the section(s) being teamed
           (!student.deleted) &&
           ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
            (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) ||
            (teamingOptions->sectionType == TeamingOptions::SectionType::noSections) ||
            (student.section == teamingOptions->sectionName))) {

            auto *timestamp = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::datetime,
                                                          QLocale::system().toString(student.surveyTimestamp, QLocale::ShortFormat));
            if(dataOptions->timestampField != DataOptions::FIELDNOTPRESENT) {
                ui->studentTable->setItem(numActiveStudents, column++, timestamp);
            }
            auto *firstName = new QTableWidgetItem(student.firstname);
            if(dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT) {
                ui->studentTable->setItem(numActiveStudents, column++, firstName);
            }
            auto *lastName = new QTableWidgetItem(student.lastname);
            if(dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT) {
                ui->studentTable->setItem(numActiveStudents, column++, lastName);
            }
            auto *section = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, student.section);
            if(dataOptions->sectionIncluded) {
                ui->studentTable->setItem(numActiveStudents, column++, section);
            }

            const bool duplicate = student.duplicateRecord;

            QList<QTableWidgetItem*> items = {timestamp, firstName, lastName, section};
            for(auto &item : items) {
                item->setToolTip(student.tooltip);
            }

            auto *editButton = new PushButtonWithMouseEnter(QIcon(":/icons_new/edit.png"), "", this);
            editButton->setToolTip("<html>" + tr("Edit") + " " + student.firstname + " " + student.lastname + tr("'s data.") + "</html>");
            editButton->setProperty("StudentID", student.ID);
            editButton->setProperty("duplicate", duplicate);
            if(duplicate) {
                editButton->setStyleSheet(EDITREMOVEBUTTONDUPLICATESTYLE);
            }
            connect(editButton, &PushButtonWithMouseEnter::clicked, this, &gruepr::editAStudent);
            // pass on mouse enter events onto cell in table
            connect(editButton, &PushButtonWithMouseEnter::mouseEntered, this, [this, editButton]
                                                                        {int row=0;
                                                                         while(editButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-2))
                                                                              {row++;}
                                                                         ui->studentTable->cellEntered(row);});
            connect(editButton, &PushButtonWithMouseEnter::mouseLeft, this, [this, editButton]
                                                                     {int row=0;
                                                                      while(editButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-2))
                                                                           {row++;}
                                                                      ui->studentTable->cellLeft(row);});
            ui->studentTable->setCellWidget(numActiveStudents, column++, editButton);

            auto *removerButton = new PushButtonWithMouseEnter(QIcon(":/icons_new/trashButton.png"), "", this);
            removerButton->setToolTip("<html>" + tr("Remove") + " " + student.firstname + " " + student.lastname + " " +
                                                 tr("from the list.") + "</html>");
            removerButton->setProperty("StudentID", student.ID);
            removerButton->setProperty("duplicate", duplicate);
            if(duplicate) {
                removerButton->setStyleSheet(EDITREMOVEBUTTONDUPLICATESTYLE);
            }
            connect(removerButton, &PushButtonWithMouseEnter::clicked, this, [this, ID = student.ID, removerButton]{removerButton->disconnect(); removeAStudent(ID);});
            // pass on mouse enter events onto cell in table
            connect(removerButton, &PushButtonWithMouseEnter::mouseEntered, this, [this, removerButton]
                                                                           {int row=0;
                                                                            while(removerButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-1))
                                                                                 {row++;}
                                                                            ui->studentTable->cellEntered(row);});
            connect(removerButton, &PushButtonWithMouseEnter::mouseLeft, this, [this, removerButton]
                                                                        {int row=0;
                                                                         while(removerButton != ui->studentTable->cellWidget(row, ui->studentTable->columnCount()-1))
                                                                              {row++;}
                                                                         ui->studentTable->cellLeft(row);});
            ui->studentTable->setCellWidget(numActiveStudents, column, removerButton);

            numActiveStudents++;
        }
    }
    ui->studentTable->setRowCount(std::min(students.size(), numActiveStudents));   // not sure how numActiveStudents could be larger, but safer to take min

    ui->studentTable->setUpdatesEnabled(true);
    ui->studentTable->resizeColumnsToContents();
    ui->studentTable->setSortingEnabled(true);
}


////////////////////////////////////////////
// Create and optimize teams using genetic algorithm
////////////////////////////////////////////
QList<int> gruepr::optimizeTeams(QList<int> studentIndexes)
{
    // create and seed the pRNG (need to specifically do it here because this is happening in a new thread)
    std::random_device randDev;
    std::mt19937 pRNG(randDev());

    // Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
    // Each genome in this generation stores (by permutation) which students are in which team.
    // Array has one entry per student and lists, in order, the index of the student in the students[] array.
    // For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
    // students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

    // allocate memory for a genepool for current generation and a next generation as it is being created
    int **genePool = new int*[ga.populationsize];
    int **nextGenGenePool = new int*[ga.populationsize];
    // allocate memory for current and next generation's ancestors
    int **ancestors = new int*[ga.populationsize];
    int **nextGenAncestors = new int*[ga.populationsize];
    int numAncestors = 2;           //always track mom & dad
    for(int generation = 0; generation < ga.numgenerationsofancestors; generation++) {
        numAncestors += (4<<generation);   //add an additional 2^(n+1) ancestors for the next level of (great)grandparents
    }
    for(int genome = 0; genome < ga.populationsize; genome++) {
        genePool[genome] = new int[numActiveStudents];
        nextGenGenePool[genome] = new int[numActiveStudents];
        ancestors[genome] = new int[numAncestors];
        nextGenAncestors[genome] = new int[numAncestors];
    }
    // allocate memory for array of indexes, to be sorted in order of score (so genePool[orderedIndex[0]] is the one with the top score)
    int *orderedIndex = new int[ga.populationsize];
    for(int genome = 0; genome < ga.populationsize; genome++) {
        orderedIndex[genome] = genome;
    }

    // create an initial population
    // start with an array of all the student IDs in order
    int *randPerm = new int[numActiveStudents];
    for(int i = 0; i < numActiveStudents; i++) {
        randPerm[i] = studentIndexes[i];
    }
    // then make "populationSize" number of random permutations for the initial population, store in genePool
    // just use random values for their initial "ancestor" values
    std::uniform_int_distribution<unsigned int> randAncestor(0, ga.populationsize);
    for(int genome = 0; genome < ga.populationsize; genome++) {
        std::shuffle(randPerm, randPerm+numActiveStudents, pRNG);
        const auto &thisGenome = genePool[genome];
        for(int ID = 0; ID < numActiveStudents; ID++) {
            thisGenome[ID] = randPerm[ID];
        }
        const auto &thisGenomesAncestors = ancestors[genome];
        for(int ancestor = 0; ancestor < numAncestors; ancestor++) {
            thisGenomesAncestors[ancestor] = int(randAncestor(pRNG));
        }
    }
    delete[] randPerm;

    int *teamSizes = new int[MAX_TEAMS];
    for(int team = 0; team < numTeams; team++) {
        teamSizes[team] = teams[team].size;
    }

    // calculate this first generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
    auto *scores = new float[ga.populationsize];
    std::vector<std::vector<float>> criteriaScores;
    std::vector<std::vector<bool>> availabilityChart;
    std::vector<int> penaltyPoints;
    std::vector<float> unusedTeamScores;
    bool unpenalizedGenomePresent = false;
    auto sharedStudents = students;
    auto sharedNumTeams = numTeams;
    const auto *sharedTeamingOptions = teamingOptions;
    const auto *sharedDataOptions = dataOptions;

    //parallel initialization of needed variables.
#pragma omp parallel \
        default(none) \
        shared(scores, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions, unpenalizedGenomePresent) \
        private(unusedTeamScores, criteriaScores, availabilityChart, penaltyPoints)
    {
        unusedTeamScores.resize(sharedNumTeams);
        criteriaScores.resize(teamingOptions->realNumScoringFactors, std::vector<float>(numTeams));
        availabilityChart.resize(dataOptions->dayNames.size(), std::vector<bool>(dataOptions->timeNames.size()));
        penaltyPoints.resize(numTeams);
#pragma omp for nowait
        for(int genome = 0; genome < ga.populationsize; genome++) {
            scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes,
                                            sharedTeamingOptions, sharedDataOptions, unusedTeamScores.data(),
                                            criteriaScores, availabilityChart, penaltyPoints);
            int totalPenaltyPoints = 0;
            for(int team = 0; team < sharedNumTeams; team++) {
                totalPenaltyPoints += penaltyPoints[team];
            }
            unpenalizedGenomePresent = unpenalizedGenomePresent || (totalPenaltyPoints == 0);
        }
    }

    // get genome indexes in order of score, largest to smallest
    std::sort(orderedIndex, orderedIndex+ga.populationsize, [&scores](const int i, const int j){return (scores[i] > scores[j]);});
    emit generationComplete(scores, orderedIndex, 0, 0, unpenalizedGenomePresent);


    const int *mom=nullptr, *dad=nullptr;               // pointer to genome of mom and dad
    float bestScores[GA::GENERATIONS_OF_STABILITY]={0};	// historical record of best score in the genome, going back generationsOfStability generations
    float scoreStability = 0;
    int generation = 0;
    bool localOptimizationStopped = false;

    // now optimize
    do {                    // allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
        do {                // keep optimizing until reach stability or maxGenerations
            // clone the elites in genePool into nextGenGenePool, shifting their ancestor arrays as if "self-mating"
            for(int genome = 0; genome < GA::NUM_ELITES; genome++) {
                ga.clone(genePool[orderedIndex[genome]], ancestors[orderedIndex[genome]], orderedIndex[genome],
                         nextGenGenePool[genome], nextGenAncestors[genome], numActiveStudents);
            }

            // create rest of population in nextGenGenePool by mating
            for(int genome = GA::NUM_ELITES; genome < ga.populationsize; genome++) {
                //get a couple of parents
                ga.tournamentSelectParents(genePool, orderedIndex, ancestors, mom, dad, nextGenAncestors[genome], pRNG);

                //mate them and put child in nextGenGenePool
                ga.mate(mom, dad, teamSizes, numTeams, nextGenGenePool[genome], numActiveStudents, pRNG);
            }

            // mutate all but the single top-scoring elite genome with some probability; if mutation occurs, mutate same genome again with same probability
            std::uniform_int_distribution<unsigned int> randProbability(1, 100);
            for(int genome = 1; genome < ga.populationsize; genome++) {
                while(randProbability(pRNG) < ga.mutationlikelihood) {
                    ga.mutate(&nextGenGenePool[genome][0], numActiveStudents, pRNG);
                }
            }

            // swap pointers to make nextGen's genePool and ancestors into this generation's
            std::swap(genePool, nextGenGenePool);
            std::swap(ancestors, nextGenAncestors);

            generation++;

            // calculate this generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
            unpenalizedGenomePresent = false;
            sharedStudents = students;
            sharedNumTeams = numTeams;
            sharedTeamingOptions = teamingOptions;
            sharedDataOptions = dataOptions;
#pragma omp parallel \
            default(none) \
                shared(scores, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions, unpenalizedGenomePresent) \
                private(unusedTeamScores, criteriaScores, availabilityChart, penaltyPoints)
            {
                unusedTeamScores.resize(sharedNumTeams);
                criteriaScores.resize(teamingOptions->realNumScoringFactors, std::vector<float>(numTeams));
                availabilityChart.resize(dataOptions->dayNames.size(), std::vector<bool>(dataOptions->timeNames.size()));
                penaltyPoints.resize(numTeams);
#pragma omp for nowait
                for(int genome = 0; genome < ga.populationsize; genome++) {
                    scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes,
                                                    sharedTeamingOptions, sharedDataOptions, unusedTeamScores.data(),
                                                    criteriaScores, availabilityChart, penaltyPoints);
                    int totalPenaltyPoints = 0;
                    for(int team = 0; team < sharedNumTeams; team++) {
                        totalPenaltyPoints += penaltyPoints[team];
                    }
                    unpenalizedGenomePresent = unpenalizedGenomePresent || (totalPenaltyPoints == 0);
                }
            }

            // get genome indexes in order of score, largest to smallest
            std::sort(orderedIndex, orderedIndex+ga.populationsize, [scores](const int i, const int j){return (scores[i] > scores[j]);});

            // determine best score, save in historical record, and calculate score stability
            const float maxScoreInThisGeneration = scores[orderedIndex[0]];
            const float maxScoreFromGenerationsAgo = bestScores[(generation+1) % (GA::GENERATIONS_OF_STABILITY)];
            bestScores[generation % (GA::GENERATIONS_OF_STABILITY)] = maxScoreInThisGeneration;	//best scores from most recent generationsOfStability, wrapping storage location

            if(maxScoreInThisGeneration == maxScoreFromGenerationsAgo) {
                scoreStability = maxScoreInThisGeneration / 0.0001F;
            }
            else {
                scoreStability = maxScoreInThisGeneration / (maxScoreInThisGeneration - maxScoreFromGenerationsAgo);
            }
            emit generationComplete(scores, orderedIndex, generation, scoreStability, unpenalizedGenomePresent);

            optimizationStoppedmutex.lock();
            localOptimizationStopped = optimizationStopped;
            optimizationStoppedmutex.unlock();
        }
        while(!localOptimizationStopped && ((generation < GA::MIN_GENERATIONS) ||
                                            ((generation < GA::MAX_GENERATIONS) && (scoreStability < GA::MIN_SCORE_STABILITY))));

        if(localOptimizationStopped || teamingOptions->realNumScoringFactors == 0) { //if no criteria to group by, return immediately
            keepOptimizing = false;
            emit turnOffBusyCursor();
        }
        else {
            keepOptimizing = true;
        }
    }
    while(keepOptimizing);

    finalGeneration = generation;
    teamSetScore = bestScores[generation % (GA::GENERATIONS_OF_STABILITY)];

    //copy best team set into a QList to return
    QList<int> bestTeamSet;
    bestTeamSet.reserve(numActiveStudents);
    const auto &bestGenome = genePool[orderedIndex[0]];
    for(int ID = 0; ID < numActiveStudents; ID++) {
        bestTeamSet << bestGenome[ID];
    }

    // deallocate memory
    for(int genome = 0; genome < ga.populationsize; ++genome) {
        delete[] nextGenGenePool[genome];
        delete[] genePool[genome];
        delete[] nextGenAncestors[genome];
        delete[] ancestors[genome];
    }
    delete[] nextGenGenePool;
    delete[] genePool;
    delete[] nextGenAncestors;
    delete[] ancestors;
    delete[] orderedIndex;
    delete[] teamSizes;

    return bestTeamSet;
}


//////////////////
// Calculate score for one teamset (one genome)
// Returns the total net score (which is, typically, the harmonic mean of all team scores)
// Modifies the teamScores[] to give scores for each individual team in the genome, too
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
//////////////////
float gruepr::getGenomeScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                             const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, float _teamScores[],
                             std::vector<std::vector<float>> &_criteriaScores, std::vector<std::vector<bool>> &_availabilityChart, std::vector<int> &_penaltyPoints)
{
    // Initialize each component and team score
    for(auto &penalty : _penaltyPoints) {
        penalty = 0;
    }
    for(auto &criteria : _criteriaScores) {
        for(auto &score : criteria) {
            score = 0;
        }
    }
    for(int team = 0; team < _numTeams; team++) {
        _teamScores[team] = 0;
    }

    // Calculate attribute scores and / or penalties for each attribute for each team:
    std::multiset<int> attributeLevelsInTeam;
    std::multiset<float> timezoneLevelsInTeam;

    for (int i = 0; i< _teamingOptions->realNumScoringFactors; i++) {
        auto _criterionBeingScored = _teamingOptions->criterionTypes[i];
        switch(_criterionBeingScored->criteriaType) {
            case Criterion::CriteriaType::genderIdentity: {
                auto criterionCasted = qobject_cast<GenderCriterion*>(_criterionBeingScored);
                getGenderScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::urmIdentity: {
                /* FROMDEV
                auto criterionCasted = dynamic_cast<SingleURMIdentityCriterion*>(_criterionBeingScored);
                getURMScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                */
                break;
            }
            case Criterion::CriteriaType::attributeQuestion: {
                auto criterionCasted = qobject_cast<AttributeCriterion*>(_criterionBeingScored);
                getAttributeScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, _dataOptions, criterionCasted, _criteriaScores[i],
                                  criterionCasted->attributeIndex, attributeLevelsInTeam, timezoneLevelsInTeam, _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::scheduleMeetingTimes: {
                auto criterionCasted = qobject_cast<ScheduleCriterion*>(_criterionBeingScored);
                getScheduleScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, _dataOptions, criterionCasted, _criteriaScores[i], _availabilityChart, _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::requiredTeammates: {
                auto criterionCasted = qobject_cast<TeammatesCriterion*>(_criterionBeingScored);
                getRequiredTeammatesScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::preventedTeammates: {
                auto criterionCasted = qobject_cast<TeammatesCriterion*>(_criterionBeingScored);
                getPreventedTeammatesScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::requestedTeammates: {
                auto criterionCasted = qobject_cast<TeammatesCriterion*>(_criterionBeingScored);
                getRequestedTeammatesScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                break;
            }
            case Criterion::CriteriaType::gradeBalance: {
                /* FROMDEV
                auto criterionCasted = dynamic_cast<GradeBalanceCriterion*>(_criterionBeingScored);
                getGradeBalanceScore(_students, _teammates, _numTeams, _teamSizes, _teamingOptions, criterionCasted, _criteriaScores[i], _penaltyPoints);
                */
                break;
            }
            case Criterion::CriteriaType::section:
            case Criterion::CriteriaType::teamSize: {
                break;
            }
        }
    }

    // Bring together for a final score for each team:
    // Score is normalized to be out of 100 (but with possible "extra credit" for more than desiredTimeBlocksOverlap hours w/ 100% team availability)
    for(int team = 0; team < _numTeams; team++) {
        // remove the schedule extra credit if any penalties are being applied, so that a very high schedule overlap doesn't cancel out the penalty
        for(int criterion = 0; criterion < _teamingOptions->realNumScoringFactors; criterion++) {
            if(_teamingOptions->criterionTypes[criterion]->criteriaType == Criterion::CriteriaType::scheduleMeetingTimes &&
                _penaltyPoints[team] > 0) {
                _teamScores[team] += _teamingOptions->criterionTypes[criterion]->weight;
            }
            else {
                _teamScores[team] += _criteriaScores[criterion][team];
            }
        }
        _teamScores[team] = 100 * ((_teamScores[team] / float(_teamingOptions->realNumScoringFactors)) - _penaltyPoints[team]);
    }

    // Finally, bring all team scores together for a total genome score.
    // Use the harmonic mean, the inverse of the average of the inverses, so score is skewed towards the smaller members.
    // This makes it so we optimize for better values of the worse teams rather than run-away best teams.
    // Very poor teams have 0 or negative scores, and this makes the harmonic mean impossible to calculate.
    // Thus, if any teamScore is <= 0, we instead use the arithmetic mean punished by reducing towards negative infinity by half the arithmetic mean.
    float harmonicSum = 0, regularSum = 0;
    int numTeamsScored = 0;
    bool allTeamsPositive = true;
    for(int team = 0; team < _numTeams; team++) {
        //ignore unpenalized teams of one since their score of 0 is not meaningful
        if(_teamSizes[team] == 1 && _teamScores[team] == 0) {
            continue;
        }
        numTeamsScored++;
        regularSum += _teamScores[team];

        if(_teamScores[team] <= 0) {
            allTeamsPositive = false;
        }
        else {
            harmonicSum += 1/_teamScores[team];
        }
    }

    if(allTeamsPositive) {
        return(float(numTeamsScored)/harmonicSum);      //harmonic mean
    }
    const float mean = regularSum / float(numTeamsScored);
    return(mean - (std::abs(mean)/2));   //"punished" arithmetic mean
}

//function to get a score for an attribute type
void gruepr::getAttributeScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                               const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, AttributeCriterion *criterion,
                               std::vector<float> &_criteriaScores, const int attribute, std::multiset<int> &attributeLevelsInTeam,
                               std::multiset<float> &timezoneLevelsInTeam, std::vector<int> &_penaltyPoints)
{
    const bool thisIsTimezone = criterion->typeOfAttribute == DataOptions::AttributeType::timezone; //(_dataOptions->attributeField[attribute] == _dataOptions->timezoneField);
    const bool penaltyStatus = criterion->penaltyStatus || _teamingOptions->haveAnyRequiredAttributes[attribute] ||
                               _teamingOptions->haveAnyIncompatibleAttributes[attribute];
    const int totNumAttributeLevels = _dataOptions->attributeVals[attribute].size();
    const int totRangeAttributeLevels = *(_dataOptions->attributeVals[attribute].crbegin()) - *(_dataOptions->attributeVals[attribute].cbegin());

    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        // gather all the attribute values on the team
        attributeLevelsInTeam.clear();
        timezoneLevelsInTeam.clear();
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            attributeLevelsInTeam.insert(_students[_teammates[studentNum]].attributeVals[attribute].constBegin(),
                                         _students[_teammates[studentNum]].attributeVals[attribute].constEnd());
            if(thisIsTimezone) {
                timezoneLevelsInTeam.insert(_students[_teammates[studentNum]].timezone);
            }
            studentNum++;
        }

        if (penaltyStatus){
/*            if((criterion->weight > 0) && (!attributeLevelsInTeam.empty())) {
                //get the values of all, put a penalty for each
                if (_teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::similar){ //homogenous
                    if(thisIsTimezone) { //similar, penalize if uniqueItems > 1 (we can only have 1 unique)
                        std::set<int> uniqueItems(timezoneLevelsInTeam.begin(), timezoneLevelsInTeam.end());
                        int duplicatedValues = timezoneLevelsInTeam.size() - uniqueItems.size();  // Count unique values
                        int uniqueCount = uniqueItems.size();
                        if (uniqueCount > 1) {
                            _penaltyPoints[team] += uniqueCount - 1; // Penalize for extra unique values
                        }
                    } else { //if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                        //(_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)){
                        std::set<int> uniqueItems(attributeLevelsInTeam.begin(), attributeLevelsInTeam.end());
                        int duplicatedValues = attributeLevelsInTeam.size() - uniqueItems.size();  // Count unique values
                        int uniqueCount = uniqueItems.size();
                        if (uniqueCount > 1) {
                            _penaltyPoints[team] += uniqueCount - 1; // Penalize for extra unique values
                        }
                    }
                } else { //heterogenous, penalize same values
                    if(thisIsTimezone) {
                        std::set<int> uniqueItems(timezoneLevelsInTeam.begin(), timezoneLevelsInTeam.end());
                        int duplicatedValues = std::max(0, int(timezoneLevelsInTeam.size() - uniqueItems.size()));
                        _penaltyPoints[team] += duplicatedValues;

                    } else { //if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                        //(_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)){
                        std::set<int> uniqueItems(attributeLevelsInTeam.begin(), attributeLevelsInTeam.end());
                        int duplicatedValues = std::max(0, int(attributeLevelsInTeam.size() - uniqueItems.size()));
                        _penaltyPoints[team] += duplicatedValues;

                    }
                }
            }*/

            if(_teamingOptions->haveAnyIncompatibleAttributes[attribute]) {
                // go through each pair found in teamingOptions->incompatibleAttributeValues[attribute] list and see if both are found in attributeLevelsInTeam
                for(const auto &pair : std::as_const(_teamingOptions->incompatibleAttributeValues[attribute])) {
                    //getting the attribute level count for each incompatible attribute value
                    const int n = int(attributeLevelsInTeam.count(pair.first));
                    if(pair.first == pair.second) {
                        _penaltyPoints[team] += (n * (n-1))/ 2;  // number of incompatible pairings is the sum 1 -> n-1 (0 if n == 0 or n == 1)
                    }
                    else {
                        const int m = int(attributeLevelsInTeam.count(pair.second));
                        _penaltyPoints[team] += n * m;           // number of incompatible pairings is the # of n -> m interactions (0 if n == 0 or m == 0)
                    }
                }
            }

            // Add a penalty per required attribute response not found
            if(_teamingOptions->haveAnyRequiredAttributes[attribute]) {
                // go through each value found in teamingOptions->requiredAttributeValues[attrib] list and see whether it's found in attributeLevelsInTeam
                for(const auto value : std::as_const(_teamingOptions->requiredAttributeValues[attribute])) {
                    if(attributeLevelsInTeam.count(value) == 0) {
                        _penaltyPoints[team]++;
                    }
                }
            }
        }

        // Remove attribute values of -1 (unknown/not set) and then determine attribute scores
        attributeLevelsInTeam.erase(-1);

        //calculating score from homogeneity/heterogeneity
        if((criterion->weight > 0) && (!attributeLevelsInTeam.empty())) {
            if(thisIsTimezone) {
                // "attribute" is timezone, so use range of timezone values
                _criteriaScores[team] = (*timezoneLevelsInTeam.crbegin() - *timezoneLevelsInTeam.cbegin()) / totRangeAttributeLevels;
            }
            else if((_dataOptions->attributeType[attribute] == DataOptions::AttributeType::ordered) ||
                    (_dataOptions->attributeType[attribute] == DataOptions::AttributeType::multiordered)) {
                // attribute has meaningful ordering/numerical values--diverse range means mostly
                // max. spread between max and min values BUT ALSO, to a lesser extent, the number of unique values
                int rangeOfVals = *attributeLevelsInTeam.crbegin() - *attributeLevelsInTeam.cbegin();  // crbegin is last = largest val; cbegin is 1st = smallest
                int numUniqueVals = 0;
                int prevVal = -1;
                for(const auto currVal : attributeLevelsInTeam) {
                    if(currVal != prevVal) {
                        numUniqueVals++;
                    }
                    prevVal = currVal;
                }
                _criteriaScores[team] = (0.75 * rangeOfVals / totRangeAttributeLevels) + (0.25 * (numUniqueVals - 1) / (totNumAttributeLevels - 1));
            }
            else {
                // attribute is categorical or multicategorical--diverse range means maximum number of unique values
                int numUniqueVals = 0;
                int prevVal = -1;
                for(const auto currVal : attributeLevelsInTeam) {
                    if(currVal != prevVal) {
                        numUniqueVals++;
                    }
                    prevVal = currVal;
                }
                _criteriaScores[team] = (numUniqueVals - 1.0f) / (totNumAttributeLevels - 1.0f);
            }

            //Calculation assumes diverse (i.e., 0 if team is fully similar and +1 if fully diverse); flip it if want similar
            if(_teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::similar) {
                _criteriaScores[team] = 1 - _criteriaScores[team];
            }
            else if(_teamingOptions->attributeDiversity[attribute] == Criterion::AttributeDiversity::ignored) {
                _criteriaScores[team] = 0;
            }
        }
    }
}


void gruepr::getScheduleScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                              const TeamingOptions *const _teamingOptions, const DataOptions *const _dataOptions, ScheduleCriterion *criterion,
                              std::vector<float> &_criteriaScores, std::vector<std::vector<bool>> &_availabilityChart, std::vector<int> &_penaltyPoints)
{
    const int numDays = int(_dataOptions->dayNames.size());
    const int numTimes = int(_dataOptions->timeNames.size());
    const int numBlocksNeeded = _teamingOptions->realMeetingBlockSize;

    // combine each student's schedule array into a team schedule array
    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        if(_teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // start compiling a team availability chart; begin with that of the first student on team (unless they have ambiguous schedule)
        int numStudentsWithAmbiguousSchedules = 0;
        const auto &firstStudentOnTeam = _students[_teammates[studentNum]];
        if(!firstStudentOnTeam.ambiguousSchedule) {
            const auto &firstStudentUnavailability = firstStudentOnTeam.unavailable;
            for(int day = 0; day < numDays; day++) {
                const auto &firstStudentUnavailabilityThisDay = firstStudentUnavailability[day];
                auto &_availabilityChartThisDay = _availabilityChart[day];
                for(int time = 0; time < numTimes; time++) {
                    _availabilityChartThisDay[time] = !firstStudentUnavailabilityThisDay[time];
                }
            }
        }
        else {
            // ambiguous schedule, so note it and start with all timeslots available
            numStudentsWithAmbiguousSchedules++;
            for(int day = 0; day < numDays; day++) {
                auto &_availabilityChartThisDay = _availabilityChart[day];
                for(int time = 0; time < numTimes; time++) {
                    _availabilityChartThisDay[time] = true;
                }
            }
        }
        studentNum++;

        // now move on to each subsequent student and, unless they have ambiguous schedule, merge their availability into the team's
        for(int teammate = 1; teammate < _teamSizes[team]; teammate++) {
            const auto &currStudent = _students[_teammates[studentNum]];
            if(currStudent.ambiguousSchedule) {
                numStudentsWithAmbiguousSchedules++;
                studentNum++;
                continue;
            }
            const auto &currStudentUnavailability = currStudent.unavailable;
            for(int day = 0; day < numDays; day++) {
                const auto &currStudentUnavailabilityThisDay = currStudentUnavailability[day];
                auto &_availabilityChartThisDay = _availabilityChart[day];
                for(int time = 0; time < numTimes; time++) {
                    // "and" each student's not-unavailability
                    _availabilityChartThisDay[time] = _availabilityChartThisDay[time] && !currStudentUnavailabilityThisDay[time];
                }
            }
            studentNum++;
        }

        // keep schedule score at 0 unless 2+ students have unambiguous sched (avoid runaway score by grouping students w/ambiguous scheds)
        if((_teamSizes[team] - numStudentsWithAmbiguousSchedules) < 2) {
            _criteriaScores[team] = 0;
            continue;
        }

        //count when there's the correct number of consecutive time blocks, but don't count wrap-around past end of 1 day!
        for(int day = 0; day < numDays; day++) {
            const auto &_availabilityChartThisDay = _availabilityChart[day];
            for(int time = 0; time < numTimes; time++) {
                int block = 0;
                while((_availabilityChartThisDay[time]) && (block < numBlocksNeeded) && (time < numTimes)) {
                    block++;
                    if(block < numBlocksNeeded) {
                        time++;
                    }
                }

                if((block == numBlocksNeeded) && (block > 0)){
                    _criteriaScores[team]++;
                }
            }
        }

        // convert counts to a schedule score
        // normal schedule score is number of overlaps / desired number of overlaps
        if(_criteriaScores[team] > _teamingOptions->desiredTimeBlocksOverlap) {     // if team has > desiredTimeBlocksOverlap, each added overlap counts less
            const int numAdditionalOverlaps = int(_criteriaScores[team]) - _teamingOptions->desiredTimeBlocksOverlap;
            _criteriaScores[team] = _teamingOptions->desiredTimeBlocksOverlap;
            float factor = 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            for(int n = 1 ; n <= numAdditionalOverlaps; n++) {
                _criteriaScores[team] += factor;
                factor *= 1.0f / (HIGHSCHEDULEOVERLAPSCALE);
            }
        }
        else if(_criteriaScores[team] < _teamingOptions->minTimeBlocksOverlap) {    // if team has fewer than minTimeBlocksOverlap, zero out the score and apply penalty
            _criteriaScores[team] = 0;
            _penaltyPoints[team]++;
        }
        _criteriaScores[team] /= _teamingOptions->desiredTimeBlocksOverlap;
        _criteriaScores[team] *= criterion->weight;
    }
}

void gruepr::getGenderScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                            const TeamingOptions *const _teamingOptions, GenderCriterion *criterion,
                            std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints)
{
    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        _criteriaScores[team] = 0;  // was (1 * criterion->weight), but for now no positive "score", just penalties related to gender

        if(_teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // Count how many of each gender on the team
        int numWomen = 0;
        int numMen = 0;
        int numNonbinary = 0;
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            if(_students[_teammates[studentNum]].gender.contains(Gender::man)) {
                numMen++;
            }
            else if(_students[_teammates[studentNum]].gender.contains(Gender::woman)) {
                numWomen++;
            }
            else if(_students[_teammates[studentNum]].gender.contains(Gender::nonbinary)) {
                numNonbinary++;
            }
            studentNum++;
        }

        if(!_teamingOptions->genderIdentityRules[Gender::woman]["!="].isEmpty()) {
            const QList<int> &unallowed_values = _teamingOptions->genderIdentityRules[Gender::woman]["!="];
            for (const int unallowed_value : std::as_const(unallowed_values)) {
                if (numWomen == unallowed_value){
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                    break;
                }
            }
        }

        if(!_teamingOptions->genderIdentityRules[Gender::man]["!="].isEmpty()) {
            const QList<int> &unallowed_values = _teamingOptions->genderIdentityRules[Gender::man]["!="];
            for (const int unallowed_value : std::as_const(unallowed_values)) {
                if (numMen == unallowed_value){
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                    break;
                }
            }
        }

        if(!_teamingOptions->genderIdentityRules[Gender::nonbinary]["!="].isEmpty()) {
            const QList<int> &unallowed_values = _teamingOptions->genderIdentityRules[Gender::nonbinary]["!="];
            for (const int unallowed_value : std::as_const(unallowed_values)) {
                if (numNonbinary == unallowed_value){
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                    break;
                }
            }
        }

        if(_teamingOptions->singleGenderPrevented && (numMen == 0 || numWomen == 0)) {
            if (criterion->penaltyStatus){
                _penaltyPoints[team]++;
            }
        }
    }
}

void gruepr::getURMScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                         const TeamingOptions *const _teamingOptions, URMIdentityCriterion *criterion,
                         std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints)
{
    if(!_teamingOptions->isolatedURMPrevented) {
        return;
    }

    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
         //for now no positive "score", just penalties related to URM isolation
        _criteriaScores[team] = criterion->weight;
        if(_teamSizes[team] == 1) {
            studentNum++;
            continue;
        }

        // Count how many URM students on the team
        int numURM = 0;
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            if(_students[_teammates[studentNum]].URM) {
                numURM++;
            }
            studentNum++;
        }

        if(numURM == 1) {
            _penaltyPoints[team]++;
        }
/*
        // Count how many URM on the team
        QMap <QString, int> urmToCount;
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            if (urmToCount.contains(_students[_teammates[studentNum]].URMResponse)){
                urmToCount[_students[_teammates[studentNum]].URMResponse] += 1;
            } else {
                urmToCount[_students[_teammates[studentNum]].URMResponse] = 1;
            }
            studentNum++;
        }

        const QList<int> &unallowed_values = _teamingOptions->urmIdentityRules[criterion->urmName]["!="];
        for (int unallowed_value : unallowed_values) {
            if (urmToCount[criterion->urmName] == unallowed_value) {
                if (criterion->penaltyStatus){
                    _penaltyPoints[team]++;
                }
                break;
            }
        }
*/
    }
}


void gruepr::getPreventedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                        const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                        std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints)
{
    std::set<long long> IDsBeingTeamed, IDsOnTeam;
    std::multiset<long long> preventedIDsOnTeam;   //multiset so that penalties are in proportion to number of missed requirements

    // Get all IDs being teamed (so that we can make sure we only check the requireds/prevented/requesteds that are actually within this teamset)
    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            IDsBeingTeamed.insert(_students[_teammates[studentNum]].ID);
            studentNum++;
        }
    }

    // Loop through each team
    studentNum = 0;
    const StudentRecord *currStudent = nullptr;
    for(int team = 0; team < _numTeams; team++) {
        IDsOnTeam.clear();
        preventedIDsOnTeam.clear();
        //loop through each student on team and collect their ID and their required/prevented/requested IDs
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            currStudent = &_students[_teammates[studentNum]];
            IDsOnTeam.insert(currStudent->ID);
            for(const auto ID : IDsBeingTeamed) {
                if(currStudent->preventedWith.contains(ID)) {
                    preventedIDsOnTeam.insert(ID);
                }
            }
            studentNum++;
        }

        _criteriaScores[team] = 1;

        if(_teamingOptions->haveAnyPreventedTeammates) {
            //loop through all the prevented IDs to see if each is missing on the team--if not, increment penalty
            for(const auto preventedIDOnTeam : preventedIDsOnTeam) {
                if(IDsOnTeam.count(preventedIDOnTeam) != 0) {
                    _criteriaScores[team] = 0;
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                }
            }
        }
        _criteriaScores[team] *= criterion->weight;
    }
}

void gruepr::getGradeBalanceScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                  const TeamingOptions *const _teamingOptions, GradeBalanceCriterion *criterion,
                                  std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints){
    // optimize based on _teamingOptions->maximumDeviationOfGroupGradeAverage;

    // calculate group average grade. if absolute (overallmeangrade - currentgroupaveragegrade) > standarddeviationallowed, this group has a lower score

    int studentNum = 0;
    float overallMeanGrade = 0.0;
    QList<float> meanGroupGrades(_numTeams);
    //calculate grade average of each team
    for(int team = 0; team < _numTeams; team++) {
        float totalGroupGrade = 0.0;
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            totalGroupGrade += _students[_teammates[studentNum]].grade;
            studentNum++;
        }
        meanGroupGrades[team] = totalGroupGrade / _teamSizes[team];
        overallMeanGrade += meanGroupGrades[team];
    }
    overallMeanGrade = overallMeanGrade / _numTeams;

    //get overall mean grade
    for(int team = 0; team < _numTeams; team++){
        if (meanGroupGrades[team] > _teamingOptions->targetMaximumGroupGradeAverage || meanGroupGrades[team] < _teamingOptions->targetMinimumGroupGradeAverage){
            _criteriaScores[team] = 0;
        } else {
            _criteriaScores[team] = 1;
        }
        _criteriaScores[team] *= criterion->weight;
    }
}

void gruepr::getRequiredTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                       const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                       std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints)
{
    std::set<long long> IDsBeingTeamed, IDsOnTeam, requestedIDsByStudent;
    std::multiset<long long> requiredIDsOnTeam, preventedIDsOnTeam;   //multiset so that penalties are in proportion to number of missed requirements
    std::vector< std::set<long long> > requestedIDs;  // each set is the requests of one student; vector is all the students on the team

    // Get all IDs being teamed (so that we can make sure we only check the requireds/prevented/requesteds that are actually within this teamset)
    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            IDsBeingTeamed.insert(_students[_teammates[studentNum]].ID);
            studentNum++;
        }
    }

    // Loop through each team
    studentNum = 0;
    const StudentRecord *currStudent = nullptr;
    for(int team = 0; team < _numTeams; team++) {
        IDsOnTeam.clear();
        requiredIDsOnTeam.clear();
        preventedIDsOnTeam.clear();
        requestedIDs.clear();
        //loop through each student on team and collect their ID and their required/prevented/requested IDs
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            currStudent = &_students[_teammates[studentNum]];
            IDsOnTeam.insert(currStudent->ID);
            requestedIDsByStudent.clear();
            for(const auto ID : IDsBeingTeamed) {
                if(currStudent->requiredWith.contains(ID)) {
                    requiredIDsOnTeam.insert(ID);
                }
                if(currStudent->preventedWith.contains(ID)) {
                    preventedIDsOnTeam.insert(ID);
                }
                if(currStudent->requestedWith.contains(ID)) {
                    requestedIDsByStudent.insert(ID);
                }
            }
            requestedIDs.push_back(requestedIDsByStudent);
            studentNum++;
        }

        _criteriaScores[team] = 1;

        if(_teamingOptions->haveAnyRequiredTeammates) {
            //loop through all the required IDs to see if each is present on the team--if not, increment penalty
            for(const auto requiredIDOnTeam : requiredIDsOnTeam) {
                if(IDsOnTeam.count(requiredIDOnTeam) == 0) {
                    _criteriaScores[team] = 0;
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                }
            }
        }
        _criteriaScores[team] *= criterion->weight;
    }
}

void gruepr::getRequestedTeammatesScore(const StudentRecord *const _students, const int _teammates[], const int _numTeams, const int _teamSizes[],
                                        const TeamingOptions *const _teamingOptions, TeammatesCriterion *criterion,
                                        std::vector<float> &_criteriaScores, std::vector<int> &_penaltyPoints)
{
    std::set<long long> IDsBeingTeamed, IDsOnTeam, requestedIDsByStudent;
    std::multiset<long long> requiredIDsOnTeam, preventedIDsOnTeam;   //multiset so that penalties are in proportion to number of missed requirements
    std::vector< std::set<long long> > requestedIDs;  // each set is the requests of one student; vector is all the students on the team

    // Get all IDs being teamed (so that we can make sure we only check the requireds/prevented/requesteds that are actually within this teamset)
    int studentNum = 0;
    for(int team = 0; team < _numTeams; team++) {
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            IDsBeingTeamed.insert(_students[_teammates[studentNum]].ID);
            studentNum++;
        }
    }

    // Loop through each team
    studentNum = 0;
    const StudentRecord *currStudent = nullptr;
    for(int team = 0; team < _numTeams; team++) {
        IDsOnTeam.clear();
        requiredIDsOnTeam.clear();
        preventedIDsOnTeam.clear();
        requestedIDs.clear();
        //loop through each student on team and collect their ID and their required/prevented/requested IDs
        for(int teammate = 0; teammate < _teamSizes[team]; teammate++) {
            currStudent = &_students[_teammates[studentNum]];
            IDsOnTeam.insert(currStudent->ID);
            requestedIDsByStudent.clear();
            for(const auto ID : IDsBeingTeamed) {
                if(currStudent->requiredWith.contains(ID)) {
                    requiredIDsOnTeam.insert(ID);
                }
                if(currStudent->preventedWith.contains(ID)) {
                    preventedIDsOnTeam.insert(ID);
                }
                if(currStudent->requestedWith.contains(ID)) {
                    requestedIDsByStudent.insert(ID);
                }
            }
            requestedIDs.push_back(requestedIDsByStudent);
            studentNum++;
        }

        _criteriaScores[team] = 1;

        if(_teamingOptions->haveAnyRequestedTeammates) {
            for(const auto &requestedIDSet : requestedIDs) {
                int numRequestedTeammates = 0, numRequestedTeammatesFound = 0;
                for(const auto requestedIDOnTeam : requestedIDSet) {
                    numRequestedTeammates++;
                    if(IDsOnTeam.count(requestedIDOnTeam) != 0) {
                        numRequestedTeammatesFound++;
                    }
                }
                //apply penalty if student has unfulfilled requests that exceed the number allowed
                if(numRequestedTeammatesFound < std::min(numRequestedTeammates, _teamingOptions->numberRequestedTeammatesGiven)) {
                    _criteriaScores[team] = 0;
                    if (criterion->penaltyStatus){
                        _penaltyPoints[team]++;
                    }
                }
            }
        }
        _criteriaScores[team] *= criterion->weight;
    }
}


//////////////////
// Before closing the main application window, see if we want to save the current settings as defaults
//////////////////
void gruepr::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    savedSettings.setValue("windowGeometry", saveGeometry());
    saveState();    // save current work for possible future use

    bool dontActuallyExit = false;
    bool saveSettings = savedSettings.value("saveDefaultsOnExit", false).toBool();

    if(savedSettings.value("askToSaveDefaultsOnExit", true).toBool()) {
        QApplication::beep();
        auto *saveOptionsOnClose = new QMessageBox(this);
        saveOptionsOnClose->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint);
        saveOptionsOnClose->setStyleSheet(LABEL10PTSTYLE);
        auto *neverShowAgain = new QCheckBox(tr("Don't ask me this again"), saveOptionsOnClose);
        neverShowAgain->setStyleSheet(CHECKBOXSTYLE);

        saveOptionsOnClose->setIconPixmap(QPixmap(":/icons_new/question.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE,
                                                                              Qt::KeepAspectRatio, Qt::SmoothTransformation));
        saveOptionsOnClose->setWindowTitle(tr("Save Options?"));
        saveOptionsOnClose->setText(tr("Before exiting, should we save the\ncurrent teaming options as defaults?"));
        saveOptionsOnClose->setCheckBox(neverShowAgain);
        saveOptionsOnClose->setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        auto *dontSave = saveOptionsOnClose->addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
        saveOptionsOnClose->button(QMessageBox::Save)->setStyleSheet(SMALLBUTTONSTYLE);
        saveOptionsOnClose->button(QMessageBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        dontSave->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
        saveOptionsOnClose->exec();

        if(saveOptionsOnClose->result() == QMessageBox::Save) {
            saveSettings = true;
        }
        else if(saveOptionsOnClose->result() == QMessageBox::Cancel) {
            dontActuallyExit = true;
        }

        if(neverShowAgain->checkState() == Qt::Checked) {
            savedSettings.setValue("askToSaveDefaultsOnExit", false);
            savedSettings.setValue("saveDefaultsOnExit", saveSettings);
        }
        saveOptionsOnClose->deleteLater();
    }

    if(dontActuallyExit) {
        event->ignore();
    }
    else {
        if(saveSettings) {
            savedSettings.setValue("idealTeamSize", qobject_cast<TeamsizeCriterion*>(teamsizeCriteriaCard->criterion)->idealTeamSizeBox->value());
            savedSettings.remove("genderIdentityRules");  //clear any existing values
            savedSettings.beginWriteArray("genderIdentityRules");
            int rule = 0;
            for(const auto [gender, identityRule] : teamingOptions->genderIdentityRules.asKeyValueRange()) {
                for(const auto [operation, values] : identityRule.asKeyValueRange()) {
                    for(const auto value : std::as_const(values)) {
                        savedSettings.setArrayIndex(rule);
                        savedSettings.setValue("ruleVal", grueprGlobal::genderToString(gender) + "," +
                                                          operation + "," + QString::number(value));
                        rule++;
                    }
                }
            }
            savedSettings.endArray();
            savedSettings.setValue("singleGenderPrevented", teamingOptions->singleGenderPrevented);
            savedSettings.setValue("isolatedURMPrevented", teamingOptions->isolatedURMPrevented);
            savedSettings.setValue("minTimeBlocksOverlap", teamingOptions->minTimeBlocksOverlap);
            savedSettings.setValue("desiredTimeBlocksOverlap", teamingOptions->desiredTimeBlocksOverlap);
            savedSettings.setValue("meetingBlockSize", teamingOptions->meetingBlockSize);
            savedSettings.setValue("realMeetingBlockSize", teamingOptions->realMeetingBlockSize);
            savedSettings.setValue("scheduleWeight", teamingOptions->scheduleWeight);
            savedSettings.beginWriteArray("Attributes");
            auto diversity = QMetaEnum::fromType<Criterion::AttributeDiversity>();
            for (int attribNum = 0; attribNum < MAX_ATTRIBUTES; ++attribNum) {
                savedSettings.setArrayIndex(attribNum);
                savedSettings.setValue("attributeDiversity", diversity.valueToKey(static_cast<int>(teamingOptions->attributeDiversity[attribNum])));
                savedSettings.setValue("weight", teamingOptions->attributeWeights[attribNum]);
                savedSettings.remove("incompatibleResponses");  //clear any existing values
                savedSettings.beginWriteArray("incompatibleResponses");
                for(int incompResp = 0; incompResp < teamingOptions->incompatibleAttributeValues[attribNum].size(); incompResp++) {
                    savedSettings.setArrayIndex(incompResp);
                    savedSettings.setValue("incompatibleResponses",
                                           (QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).first) + "," +
                                            QString::number(teamingOptions->incompatibleAttributeValues[attribNum].at(incompResp).second)));
                }
                savedSettings.endArray();
                savedSettings.remove("requiredResponses");  //clear any existing values
                savedSettings.beginWriteArray("requiredResponses");
                for(int requiredResp = 0; requiredResp < teamingOptions->requiredAttributeValues[attribNum].size(); requiredResp++) {
                    savedSettings.setArrayIndex(requiredResp);
                    savedSettings.setValue("requiredResponse",
                                           (QString::number(teamingOptions->requiredAttributeValues[attribNum].at(requiredResp))));
                }
                savedSettings.endArray();
            }
            savedSettings.endArray();
            savedSettings.setValue("requestedTeammateNumber", teamingOptions->numberRequestedTeammatesGiven);
        }

        event->accept();
        emit closed();
    }
}

