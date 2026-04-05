#include "gruepr.h"
#include "ui_gruepr.h"
#include "criteria/attributeCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/sectionCriterion.h"
#include "criteria/teamsizeCriterion.h"
#include "dialogs/customTeamsizesDialog.h"
#include "dialogs/editOrAddStudentDialog.h"
#include "dialogs/editSectionNamesDialog.h"
#include "dialogs/findMatchingNameDialog.h"
#include "widgets/groupingCriteriaCardWidget.h"
#include "widgets/sortableTableWidgetItem.h"
#include "widgets/teamsTabItem.h"
#include <memory>
#include <random>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMimeData>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QtConcurrentRun>
#include <QTextBrowser>


gruepr::gruepr(DataOptions &_dataOptions, QList<StudentRecord> &_students) :
    QMainWindow(),
    students(std::move(_students)),
    dataOptions(new DataOptions(std::move(_dataOptions))),
    ui(new Ui::gruepr)
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

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle {border: 1px solid lightgrey; border-radius: 4px; background-color: " OPENWATERHEX "; "
                                                "image: url(:/icons_new/drag-handle.png);}");
    splitter->setHandleWidth(8);

    auto *settingTeamCriteriaWidget = new QWidget(this);
    ui->criteriaLabel->setStyleSheet(LABEL12PTSTYLE);
    ui->teamingOptionsScrollArea->setWidgetResizable(true);
    ui->teamingOptionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->teamingOptionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->teamingOptionsScrollArea->viewport()->installEventFilter(this);

    auto *scrollLayout = qobject_cast<QVBoxLayout*>(ui->teamingOptionsScrollArea->widget()->layout());
    scrollLayout->setSpacing(5);
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
    splitter->handle(1)->setCursor(Qt::SplitHCursor);

    teamingOptions = nullptr;
    QJsonArray savedCriteriaCards;
    if(dataOptions->dataSource == DataOptions::DataSource::fromPrevWork) {
        QFile savedFile(dataOptions->saveStateFileName, this);
        if(savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            const QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
            savedFile.close();
            QJsonObject content = doc.object();
            teamingOptions = new TeamingOptions(content["teamingoptions"].toObject());
            savedCriteriaCards = content["criteriaCards"].toArray();
        }
        else {
            grueprGlobal::errorMessage(this, tr("Error"), tr("There was an error loading the previous data."));
        }
    }
    if(teamingOptions == nullptr) {             // either not from previous work, or loading from previous work failed
        teamingOptions = new TeamingOptions;
    }

    // Create the drop indicator for drag-and-drop visual feedback
    dropIndicator = new QFrame(this);
    dropIndicator->setFixedHeight(4);
    dropIndicator->setStyleSheet("QFrame{ background-color: " DEEPWATERHEX "; border-radius: 2px; margin: 0px 10px; }");
    dropIndicator->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    dropIndicator->hide();
    bottomDropZone = new QWidget(this);
    bottomDropZone->setFixedHeight(40);
    bottomDropZone->setAcceptDrops(true);
    bottomDropZone->installEventFilter(this);
    bottomDropZone->hide();

    //Defining all criteria cards
    criteriaCardsList.clear();

    //Section Criteria Card
    if (dataOptions->sectionIncluded) {
        sectionCriteriaCard = new GroupingCriteriaCard(Criterion::CriteriaType::section, dataOptions, teamingOptions, this,
                                                       QString("Section"), false);
        const auto &sectionCriterion = qobject_cast<SectionCriterion*>(sectionCriteriaCard->criterion);
        sectionSelectionBox = sectionCriterion->sectionSelectionBox;
        connect(sectionCriterion->editSectionNameButton, &QPushButton::clicked, this, &gruepr::editSectionNames);
        connect(sectionCriterion->sectionSelectionBox, &QComboBox::currentIndexChanged, this, &gruepr::changeSection);
        criteriaCardsList.append(sectionCriteriaCard);
    }

    //Team Size Criteria Card
    teamsizeCriteriaCard = new GroupingCriteriaCard(Criterion::CriteriaType::teamSize, dataOptions, teamingOptions, this,
                                                    QString("Team Size"), false);
    const auto &teamSizeCriterion = qobject_cast<TeamsizeCriterion*>(teamsizeCriteriaCard->criterion);
    idealTeamSizeBox = teamSizeCriterion->idealTeamSizeBox;
    teamSizeBox = teamSizeCriterion->teamSizeBox;
    idealTeamSizeBox->setValue(teamingOptions->idealTeamSize);
    connect(teamSizeCriterion->idealTeamSizeBox, &QSpinBox::valueChanged, this, &gruepr::changeIdealTeamSize);
    connect(teamSizeCriterion->teamSizeBox, &QComboBox::currentIndexChanged, this, &gruepr::chooseTeamSizes);
    criteriaCardsList.append(teamsizeCriteriaCard);

    GroupingCriteriaCard::fixedCardOffset = (dataOptions->sectionIncluded ? 1 : 0) + 1; // +1 for teamsize, always present

    //Construct button and menu to add new criteria cards
    addNewCriteriaMenu = new QMenu(this);
    addNewCriteriaMenu->setStyleSheet(MENUSTYLE);
    addNewCriteriaCardButton = new QPushButton("Add New Criteria", this);
    addNewCriteriaCardButton->setIcon(QIcon(":/icons_new/add.png"));
    addNewCriteriaCardButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(addNewCriteriaCardButton, &QPushButton::clicked, addNewCriteriaCardButton, [this]() {
        const QPoint centerOfCriteriaButton = addNewCriteriaCardButton->mapToGlobal(addNewCriteriaCardButton->rect().center());
        addNewCriteriaMenu->popup(QPoint(centerOfCriteriaButton.x() - addNewCriteriaMenu->sizeHint().width()/1.8, centerOfCriteriaButton.y()+10));
    });
    if (dataOptions->genderIncluded){
        genderMenuAction = addNewCriteriaMenu->addAction(tr("Gender"));
        connect(genderMenuAction, &QAction::triggered, addNewCriteriaCardButton, [this](){
            gruepr::addCriteriaCard(Criterion::CriteriaType::genderIdentity);});
    }
    if (dataOptions->URMIncluded){
        urmMenuAction = addNewCriteriaMenu->addAction(tr("Racial/Ethnic/Cultural Identity"));
        connect(urmMenuAction, &QAction::triggered, addNewCriteriaCardButton, [this](){
            gruepr::addCriteriaCard(Criterion::CriteriaType::urmIdentity);});
    }
    if (dataOptions->numAttributes > 0){
        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
            QAction *currentAttributeAction = addNewCriteriaMenu->addAction(dataOptions->attributeQuestionText[attribute]);
            connect(currentAttributeAction, &QAction::triggered, addNewCriteriaCardButton, [this, attribute](){
                gruepr::addCriteriaCard(Criterion::CriteriaType::attributeQuestion, attribute);});
            attributeMenuActions.append(currentAttributeAction);
        }
    }
    if(!dataOptions->assignmentPreferenceFields.empty()) {
        assignmentPreferenceMenuAction = addNewCriteriaMenu->addAction(tr("Team Assignment Preferences"));
        connect(assignmentPreferenceMenuAction, &QAction::triggered, this, [this](){
            gruepr::addCriteriaCard(Criterion::CriteriaType::assignmentPreference);});
    }
    if (!dataOptions->scheduleField.empty()){
        scheduleMenuAction = new QAction("Meeting Times", this);
        connect(scheduleMenuAction, &QAction::triggered, this, [this](){gruepr::addCriteriaCard(Criterion::CriteriaType::scheduleMeetingTimes);});
        addNewCriteriaMenu->addAction(scheduleMenuAction);
    }
    groupTogetherMenuAction = addNewCriteriaMenu->addAction("Students to group on to the same team");
    connect(groupTogetherMenuAction, &QAction::triggered, this, [this]() {
        gruepr::addCriteriaCard(Criterion::CriteriaType::groupTogether);
    });
    splitApartMenuAction = addNewCriteriaMenu->addAction("Students to split on to different teams");
    connect(splitApartMenuAction, &QAction::triggered, this, [this]() {
        gruepr::addCriteriaCard(Criterion::CriteriaType::splitApart);
    });

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
    ui->dataDisplayTabWidget->tabBar()->setElideMode(Qt::ElideNone);
    connect(ui->dataDisplayTabWidget, &QTabWidget::tabCloseRequested, this, &gruepr::dataDisplayTabClose);

    //Set alternate fonts on some UI features
    QFont altFont = this->font();
    altFont.setPointSize(altFont.pointSize() + 4);
    letsDoItButton->setFont(altFont);
    ui->addStudentPushButton->setFont(altFont);
    ui->compareRosterPushButton->setFont(altFont);
    ui->dataDisplayTabWidget->setFont(altFont);

    loadUI();
    // Restore additional criteria cards from previous work (savedCriteriaCards is empty if not loading from prevWork)
    for (const auto &cardJsonVal : std::as_const(savedCriteriaCards)) {
        const QJsonObject cardJson = cardJsonVal.toObject();
        auto criteriaTypeEnum = QMetaEnum::fromType<Criterion::CriteriaType>();
        const int typeInt = Criterion::resolveCriteriaTypeKey(criteriaTypeEnum, cardJson["criteriaType"].toString());
        if (typeInt == -1) {
            continue;
        }
        const auto type = static_cast<Criterion::CriteriaType>(typeInt);
        if (type == Criterion::CriteriaType::section || type == Criterion::CriteriaType::teamSize) {
            continue; // already created
        }

        if (type == Criterion::CriteriaType::attributeQuestion) {
            addCriteriaCard(type, cardJson["attributeIndex"].toInt());

        } else {
            addCriteriaCard(type);
        }

        // Apply saved settings to the just-added card's criterion
        GroupingCriteriaCard *addedCard = criteriaCardsList.last();
        if (cardJson.contains("settings")) {
            addedCard->criterion->settingsFromJson(cardJson["settings"].toObject());
        }

        // Refresh the attribute widget UI to reflect restored settings
        if (type == Criterion::CriteriaType::attributeQuestion) {
            auto *attributeCriterion = qobject_cast<AttributeCriterion*>(addedCard->criterion);
            if (attributeCriterion != nullptr && attributeCriterion->attributeWidget != nullptr) {
                attributeCriterion->attributeWidget->setValues(false);
            }
        }
    }

    // initialize the priority order of criteria cards
    initializeCriteriaCardPriorities();
    populateCriterionTypes();

    QList<QPushButton *> buttons = {letsDoItButton, ui->addStudentPushButton, ui->compareRosterPushButton};
    for(auto &button : buttons) {
        button->setIconSize(QSize(STD_ICON_SIZE, STD_ICON_SIZE));
    }
    ui->dataSourceIcon->setFixedSize(STD_ICON_SIZE, STD_ICON_SIZE);

    //connecting the buttons that are always shown
    connect(ui->addStudentPushButton, &QPushButton::clicked, this, &gruepr::addAStudent);
    connect(ui->compareRosterPushButton, &QPushButton::clicked, this, &gruepr::compareStudentsToRoster);
    connect(letsDoItButton, &QPushButton::clicked, this, &gruepr::startOptimization);

    //Connect genetic algorithm progress signals to slots
    connect(this, &gruepr::generationComplete, this, &gruepr::updateOptimizationProgress, Qt::BlockingQueuedConnection);
    connect(&futureWatcher, &QFutureWatcher<void>::finished, this, &gruepr::optimizationComplete);

    refreshCriteriaLayout();
}

void gruepr::addSavedTeamsTabs()
{
    if(dataOptions->dataSource == DataOptions::DataSource::fromPrevWork) {
        QFile savedFile(dataOptions->saveStateFileName, this);
        if(savedFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            const QJsonDocument doc = QJsonDocument::fromJson(savedFile.readAll());
            savedFile.close();
            QJsonObject content = doc.object();
            const QJsonArray teamsetjsons = content["teamsets"].toArray();
            TeamsTabItem *teamTab = nullptr;
            for(const auto &teamsetjson : teamsetjsons) {
                teamTab = new TeamsTabItem(teamsetjson.toObject(), *teamingOptions, students, dataOptions->sectionNames, letsDoItButton, this);
                ui->dataDisplayTabWidget->addTab(teamTab, teamTab->tabName);
                numTeams = int(teams.size());
                connect(teamTab, &TeamsTabItem::saveState, this, &gruepr::saveState);
                connect(teamTab, &TeamsTabItem::addCriterionRequested, this, static_cast<void (gruepr::*)(Criterion::CriteriaType)>(&gruepr::addCriteriaCard));
            }
        }
    }
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
        const QRect area = ui->teamingOptionsScrollArea->viewport()->rect();
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

void gruepr::moveCriteriaCard(int draggedIndex, int targetIndex) {
    QLayout* layout = ui->teamingOptionsScrollAreaWidget->layout();
    layout->setSpacing(5);
    if (draggedIndex < 0 || targetIndex < 0 ||
        draggedIndex >= criteriaCardsList.size() ||
        targetIndex > criteriaCardsList.size() ||   // note: > not >=, to allow end position
        draggedIndex == targetIndex) {
        return;
    }

    hideDropIndicator();

    GroupingCriteriaCard* draggedCard = criteriaCardsList.takeAt(draggedIndex);

    int insertIndex;
    if (targetIndex == criteriaCardsList.size() + 1) {
        // Was the end position; after takeAt, list shrank by 1
        insertIndex = criteriaCardsList.size();
    } else {
        insertIndex = (draggedIndex < targetIndex) ? targetIndex - 1 : targetIndex;
    }

    criteriaCardsList.insert(insertIndex, draggedCard);

    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();

    for (auto *card : std::as_const(criteriaCardsList)) {
        card->stopDragTimer();
    }
}

bool gruepr::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == bottomDropZone) {
        if (event->type() == QEvent::DragEnter) {
            auto *e = static_cast<QDragEnterEvent*>(event);
            if (e->mimeData()->hasText()) {
                e->acceptProposedAction();
                showDropIndicator(criteriaCardsList.size());
            }
            return true;
        }
        if (event->type() == QEvent::DragMove) {
            auto *e = static_cast<QDragMoveEvent*>(event);
            e->acceptProposedAction();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            auto *e = static_cast<QDropEvent*>(event);
            const QString widgetID = e->mimeData()->text();
            const auto *draggedCard = reinterpret_cast<GroupingCriteriaCard*>(widgetID.toULongLong());
            e->acceptProposedAction();
            moveCriteriaCard(draggedCard->getPriorityOrder(), criteriaCardsList.size());
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void gruepr::showDropIndicator(int targetIndex) {
    auto *layout = qobject_cast<QVBoxLayout*>(ui->teamingOptionsScrollAreaWidget->layout());
    if (layout == nullptr) {
        return;
    }

    layout->removeWidget(dropIndicator);

    if (targetIndex >= 0 && targetIndex < criteriaCardsList.size()) {
        // Insert indicator above the target card
        const int layoutIndex = layout->indexOf(criteriaCardsList[targetIndex]);
        if (layoutIndex >= 0) {
            layout->insertWidget(layoutIndex, dropIndicator);
            dropIndicator->show();
        }
    }
    else if (targetIndex == criteriaCardsList.size()) {
        // Insert indicator after the last card (above the bottom drop zone)
        const int layoutIndex = layout->indexOf(bottomDropZone);
        if (layoutIndex >= 0) {
            layout->insertWidget(layoutIndex, dropIndicator);
            dropIndicator->show();
        }
    }
}

void gruepr::showBottomDropZone() {
    bottomDropZone->show();
}

void gruepr::hideDropIndicator() {
    if (dropIndicator == nullptr) {
        return;
    }
    auto *layout = qobject_cast<QVBoxLayout*>(ui->teamingOptionsScrollAreaWidget->layout());
    if (layout != nullptr) {
        layout->removeWidget(dropIndicator);
    }
    dropIndicator->hide();
    bottomDropZone->hide();
}

void gruepr::addCriteriaCard(Criterion::CriteriaType criteriaType){
    // Note: standard drag/drop/delete signals are connected automatically in GroupingCriteriaCard constructor
    switch(criteriaType) {
        case Criterion::CriteriaType::genderIdentity: {
            if(genderIdentityCriteriaCard == nullptr) {
                genderIdentityCriteriaCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this,
                                                                      QString("Gender identity"), true);
                criteriaCardsList.append(genderIdentityCriteriaCard);
                if(genderMenuAction != nullptr) {
                    genderMenuAction->setVisible(false);
                }
            }
            break;
        }
        case Criterion::CriteriaType::urmIdentity: {
            if(urmIdentityCard == nullptr) {
                urmIdentityCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this,
                                                           QString("Racial/Ethnic Identity"), true);
                criteriaCardsList.append(urmIdentityCard);
                if(urmMenuAction != nullptr) {
                    urmMenuAction->setVisible(false);
                }
            }
            break;
        }
        case Criterion::CriteriaType::assignmentPreference: {
            if(assignmentPreferenceCriteriaCard == nullptr) {
                assignmentPreferenceCriteriaCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this,
                                                                            QString("Team Assignment Preferences"), true);
                criteriaCardsList.append(assignmentPreferenceCriteriaCard);
                if(assignmentPreferenceMenuAction != nullptr) {
                    assignmentPreferenceMenuAction->setVisible(false);
                }
            }
            break;
        }
        case Criterion::CriteriaType::scheduleMeetingTimes: {
            if (meetingScheduleCriteriaCard == nullptr){
                meetingScheduleCriteriaCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this,
                                                                       QString("Number of weekly meeting times"), true);
                criteriaCardsList.append(meetingScheduleCriteriaCard);
                if(scheduleMenuAction != nullptr) {
                    scheduleMenuAction->setVisible(false);
                }
            }
            break;
        }
        case Criterion::CriteriaType::groupTogether:
        case Criterion::CriteriaType::splitApart: {
            if (!teammateRulesExistence.contains(criteriaType)) {
                teammateRulesExistence.append(criteriaType);
                const QString typeString = (criteriaType == Criterion::CriteriaType::groupTogether) ? tr("group together") : tr("split apart");
                auto *teammatesCard = new GroupingCriteriaCard(criteriaType, dataOptions, teamingOptions, this,
                                                               tr("Students to ") + typeString, true);
                criteriaCardsList.append(teammatesCard);
                if (criteriaType == Criterion::CriteriaType::groupTogether && groupTogetherMenuAction != nullptr) {
                    groupTogetherMenuAction->setVisible(false);
                }
                else if (criteriaType == Criterion::CriteriaType::splitApart && splitApartMenuAction != nullptr) {
                    splitApartMenuAction->setVisible(false);
                }
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
            GroupingCriteriaCard *currentAttributeCard = initializedAttributeCriteriaCards[attribute];
            addedAttributeNumbersList.append(attribute);
            criteriaCardsList.append(currentAttributeCard);
            if(attribute < attributeMenuActions.size() && attributeMenuActions[attribute] != nullptr) {
                attributeMenuActions[attribute]->setVisible(false);
            }
        }
    }
    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();
}

void gruepr::populateCriterionTypes()
{
    teamingOptions->criteria.clear();
    for (auto *const criteriaCard : std::as_const(criteriaCardsList)) {
        if (criteriaCard->criterion->criteriaType == Criterion::CriteriaType::section ||
            criteriaCard->criterion->criteriaType == Criterion::CriteriaType::teamSize) {
            continue;
        }
        teamingOptions->criteria << criteriaCard->criterion;
    }
}

void gruepr::deleteCriteriaCard(int deletedIndex)
{
    // Get the card to be deleted
    //qDebug() << deletedIndex;
    //qDebug() << criteriaCardsList.size();
    GroupingCriteriaCard *cardToDelete = criteriaCardsList[deletedIndex];
    const Criterion::CriteriaType criteriaType = cardToDelete->criterion->criteriaType;

    // Remove the card from the list and delete it
    criteriaCardsList.removeAt(deletedIndex);

    switch(criteriaType) {
        case Criterion::CriteriaType::genderIdentity: {
            delete cardToDelete;
            genderIdentityCriteriaCard = nullptr;
            if(genderMenuAction != nullptr) {
                genderMenuAction->setVisible(true);
            }
            break;
        }
        case Criterion::CriteriaType::urmIdentity: {
            delete cardToDelete;
            urmIdentityCard = nullptr;
            if(urmMenuAction != nullptr) {
                urmMenuAction->setVisible(true);
            }
            break;
        }
        case Criterion::CriteriaType::attributeQuestion: {
            auto *criterion = qobject_cast<AttributeCriterion*>(cardToDelete->criterion);
            const int attributeIndex = criterion->attributeIndex;
            const int indexToRemove = addedAttributeNumbersList.indexOf(attributeIndex);
            addedAttributeNumbersList.remove(indexToRemove);
            cardToDelete->setVisible(false);
            if(attributeIndex < attributeMenuActions.size() && attributeMenuActions[attributeIndex] != nullptr) {
                attributeMenuActions[attributeIndex]->setVisible(true);
            }
            break;
        }
        case Criterion::CriteriaType::assignmentPreference: {
            delete cardToDelete;
            assignmentPreferenceCriteriaCard = nullptr;
            if(assignmentPreferenceMenuAction != nullptr) {
                assignmentPreferenceMenuAction->setVisible(true);
            }
            break;
        }
        case Criterion::CriteriaType::scheduleMeetingTimes: {
            delete cardToDelete;
            meetingScheduleCriteriaCard = nullptr;
            if(scheduleMenuAction != nullptr) {
                scheduleMenuAction->setVisible(true);
            }
            break;
        }
        case Criterion::CriteriaType::groupTogether:
        case Criterion::CriteriaType::splitApart: {
            const int indexToRemove = teammateRulesExistence.indexOf(criteriaType);
            teammateRulesExistence.remove(indexToRemove);
            delete cardToDelete;
            if(criteriaType == Criterion::CriteriaType::groupTogether && groupTogetherMenuAction != nullptr) {
                groupTogetherMenuAction->setVisible(true);
            }
            else if(criteriaType == Criterion::CriteriaType::splitApart && splitApartMenuAction != nullptr) {
                splitApartMenuAction->setVisible(true);
            }
            break;
        }
        default: {
            return;
            //qDebug() << deletedIndex << " does not exist or is non-deletable section or teamsize card.";
        }
    }

    initializeCriteriaCardPriorities();
    refreshCriteriaLayout();
}

void gruepr::refreshCriteriaLayout(){
    auto *layout = qobject_cast<QVBoxLayout*>(ui->teamingOptionsScrollAreaWidget->layout());
    while (layout->count() > 1) {
        auto *item = layout->takeAt(1);
        if (item->spacerItem() != nullptr) {
            delete item;
        }
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
    layout->addWidget(bottomDropZone);
    layout->addWidget(addNewCriteriaCardButton);
    layout->addStretch(1);
}

////////////////////
// A static public wrapper for the getGenomeScore function
// The calculated scores are updated into the .scores members of the _teams array sent to the function
// This is a static function, and parameters are named with leading underscore to differentiate from gruepr member variables
////////////////////
void gruepr::calcTeamScores(const QList<StudentRecord> &_students, const long long _numStudents,
                            TeamSet &_teams, const TeamingOptions *const _teamingOptions)
{
    const int _numTeams = _teams.size();
    const auto &_dataOptions = _teams.dataOptions;
    QList<float> teamScores(_numTeams);
    QList<QList<float>> criteriaScores(_teamingOptions->criteria.size(), QList<float>(_numTeams));
    QList<float> penaltyPoints(_numTeams);
    QList<int> teamSizes(_numTeams);
    QList<int> genome(_numStudents);
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
                   _teamingOptions, &_dataOptions, teamScores.data(), criteriaScores, penaltyPoints);

    for(int teamnum = 0; teamnum < _numTeams; teamnum++) {
        _teams[teamnum].score = teamScores[teamnum];
    }

/*  for(int criterion = 0; criterion < _teamingOptions->realNumScoringFactors; criterion++){
        for (int team = 0; team <_numTeams; team++){
            const float actualScore = criteriaScores[criterion][team]/_teamingOptions->weights[criterion];
            qDebug() << "weight from weights[]:" << _teamingOptions->weights[criterion];
            qDebug() << "team:" << team;
            qDebug() << "criterion:" << criterion;
            qDebug() << "penalty:" << -penaltyPoints[criterion];
            qDebug() << "actual score:" << actualScore;
            qDebug() << "score:" << criteriaScores[criterion][team];
        }
    } */
}

QStringList gruepr::getTeamTabNames() const {
    QStringList names;
    for (int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++) {
        names << ui->dataDisplayTabWidget->tabText(tab);
    }
    return names;
}

QList<QList<long long>> gruepr::getTeamSetData(const QString &tabName) const {
    QList<QList<long long>> teamIDLists;
    for (int tab = 1; tab < ui->dataDisplayTabWidget->count(); tab++) {
        if (ui->dataDisplayTabWidget->tabText(tab) == tabName) {
            const auto *teamTab = qobject_cast<TeamsTabItem*>(ui->dataDisplayTabWidget->widget(tab));
            if (teamTab != nullptr) {
                teamIDLists.reserve(teamTab->getTeams().size());
                for (const auto &team : teamTab->getTeams()) {
                    teamIDLists << team.studentIDs;
                }
            }
            break;
        }
    }
    return teamIDLists;
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
            const QString currentAttributeQuestionText = dataOptions->attributeQuestionText.at(attribute);
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
            attributeWidgets[attribute]->setValues();
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
        rebuildDuplicatesTeamsizeURMAndSectionDataAndRefreshStudentTable();
    }

    // add back in this student's attribute responses from the counts in dataOptions and update the attribute tabs to show the counts
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::numerical) {
            dataOptions->attributeVals_continuous[attribute].clear();
            for(const auto &student : std::as_const(students)) {
                if(!student.deleted && !student.attributeVals_continuous[attribute].isEmpty()) {
                    dataOptions->attributeVals_continuous[attribute].insert(student.attributeVals_continuous[attribute].first());
                }
            }
        }
        else {
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

    //Remove the student
    studentBeingRemoved->deleted = true;

    // remove this student from all other students who might have them as groupTogether / SplitApart
    for(auto &student : students) {
        student.splitApart.remove(ID);
        student.groupTogether.remove(ID);
    }

    // update in dataOptions and then the attribute tab the count of each attribute response
    for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
        if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::numerical) {
            dataOptions->attributeVals_continuous[attribute].clear();
            for(const auto &student : std::as_const(students)) {
                if(!student.deleted && !student.attributeVals_continuous[attribute].isEmpty()) {
                    dataOptions->attributeVals_continuous[attribute].insert(
                        student.attributeVals_continuous[attribute].first());
                }
            }
        }
        else {
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
        }
        attributeWidgets[attribute]->setValues();
    }

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
            newStudent.ambiguousSchedule = (newStudent.availabilityChart.count("√") == 0 ||
                                           (newStudent.availabilityChart.count("√") == (dataOptions->dayNames.size() * dataOptions->timeNames.size())));
            students << newStudent;

            // update in dataOptions and then the attribute tab the count of each attribute response
            for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
                if(dataOptions->attributeType[attribute] == DataOptions::AttributeType::numerical) {
                    dataOptions->attributeVals_continuous[attribute].clear();
                    for(const auto &student : std::as_const(students)) {
                        if(!student.deleted && !student.attributeVals_continuous[attribute].isEmpty()) {
                            dataOptions->attributeVals_continuous[attribute].insert(student.attributeVals_continuous[attribute].first());
                        }
                    }
                }
                else {
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
    CsvFile rosterFile(CsvFile::Delimiter::comma);
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
            StudentRecord *student = nullptr;
            for(auto &thisStudent : students) {
                if(name.compare(thisStudent.firstname + " " + thisStudent.lastname, Qt::CaseInsensitive) == 0) {
                    student = &thisStudent;
                    break;
                }
            }

            // get the email corresponding to this name on the roster
            const auto index = names.indexOf(name);
            const QString rosterEmail = ((index >= 0 && index < emails.size())? emails.at(index) : "");

            if(student != nullptr) {
                // Exact match for name was found in existing students
                namesNotFound.removeAll(student->firstname + " " + student->lastname);
                if(!emails.isEmpty()) {
                    if(student->email.compare(rosterEmail, Qt::CaseInsensitive) != 0) {
                        // Email in survey doesn't match roster
                        studentsWithDiffEmail << student;
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
                        for(int attribute = 0; attribute < dataOptions->numAttributes; attribute++) {
                            newStudent.attributeVals_discrete[attribute] << -1;
                            newStudent.attributeVals_continuous[attribute] << 0;
                        }
                        newStudent.ambiguousSchedule = true;
                        newStudent.createTooltip(*dataOptions);

                        students << newStudent;

                        numActiveStudents = students.size();
                    }
                    else {  // selected an inexact match
                        const QString surveyName = choiceWindow->currSurveyName;
                        namesNotFound.removeAll(surveyName);
                        for(auto &thisStudent : students) {
                            if(surveyName != (thisStudent.firstname + " " + thisStudent.lastname)) {
                                student = &thisStudent;
                                break;
                            }
                        }
                        if(student != nullptr) {
                            if(choiceWindow->useRosterEmail) {
                                dataHasChanged = true;
                                student->email = emails.isEmpty()? "" : rosterEmail;
                                student->createTooltip(*dataOptions);
                            }
                            if(choiceWindow->useRosterName) {
                                dataHasChanged = true;
                                student->firstname = name.split(" ").first();
                                student->lastname = name.split(" ").mid(1).join(" ");
                                student->createTooltip(*dataOptions);
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
    // First pass: reset all duplicate flags
    for(auto &student : students) {
        student.duplicateRecord = false;
    }
    // Now build hash maps of name -> indices and email -> indices (skipping deleted students and empty keys)
    QHash<QString, QList<int>> nameToIndices;
    QHash<QString, QList<int>> emailToIndices;
    for(int index = 0; index < students.size(); index++) {
        const auto &student = students[index];
        if(student.deleted) {
            continue;
        }
        const QString fullName = (student.firstname + student.lastname).toLower();
        if(!fullName.isEmpty()) {
            nameToIndices[fullName] << index;
        }
        if(!student.email.isEmpty()) {
            emailToIndices[student.email.toLower()] << index;
        }
    }
    // Any key with more than one index means those students are duplicates
    for(auto it = nameToIndices.constBegin(); it != nameToIndices.constEnd(); ++it) {
        if(it.value().size() > 1) {
            for(const int index : it.value()) {
                students[index].duplicateRecord = true;
            }
        }
    }
    for(auto it = emailToIndices.constBegin(); it != emailToIndices.constEnd(); ++it) {
        if(it.value().size() > 1) {
            for(const int index : it.value()) {
                students[index].duplicateRecord = true;
            }
        }
    }
    // Rebuild tooltips once per student
    for(auto &student : students) {
        student.createTooltip(*dataOptions);
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
    teamingOptions->idealTeamSize = idealSize;

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
    auto *sizecriterion = qobject_cast<TeamsizeCriterion*>(teamsizeCriteriaCard->criterion);
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
    // Initialize weights based on priority then normalize all score factor weights using norm factor = number of factors / total weights of all factors
    // First criterion has weight 10, then each subsequent criterion is 3/4 the weight of the prev. one
    teamingOptions->criteria.clear();
    float weight = 10;
    float sumOfWeights = 0;
    for (auto *const criteriaCard : std::as_const(criteriaCardsList)){
        if (criteriaCard->criterion->criteriaType == Criterion::CriteriaType::section ||
            criteriaCard->criterion->criteriaType == Criterion::CriteriaType::teamSize) {
            continue;
        }
        teamingOptions->criteria << criteriaCard->criterion;
        criteriaCard->criterion->weight = weight;
        sumOfWeights += weight;
        weight *= 0.75;
    }

    const int numCriteria = teamingOptions->criteria.size();
    float normFactor = numCriteria / sumOfWeights;
    if(!std::isfinite(normFactor)) {
        normFactor = 0;
    }
    // convert weights to realWeights
    for (auto *criterion : std::as_const(teamingOptions->criteria)) {
        criterion->weight *= normFactor;
    }

    // prepare the criteria for the optimization process (mostly cache pre-determined values)
    for (auto *criterion : std::as_const(teamingOptions->criteria)) {
        criterion->prepareForOptimization(students.constData(), numActiveStudents, dataOptions);
    }

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

        // Create window to display progress, and connect the stop optimization button in the window to the actual stopping of the optimization thread
        const QString sectionName = (teamingMultipleSections? (tr("section ") + QString::number(section + 1) + " / " + QString::number(numSectionsToTeam) + ": " +
                                                          teamingOptions->sectionName) : "");
        progressWindow = new progressDialog(sectionName, progressChart, this);
        progressWindow->show();
        connect(progressWindow, &progressDialog::letsStop, this, [this] {QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
                                                                         connect(this, &gruepr::turnOffBusyCursor, this, &QApplication::restoreOverrideCursor);
                                                                         optimizationStoppedmutex.lock();
                                                                         optimizationStopped = true;
                                                                         optimizationStoppedmutex.unlock();
                                                                        });

        // set the working value of the genetic algorithm's population size and tournament selection probability
        ga.setGAParameters(numActiveStudents);

        // Set up the flag to allow a stoppage and set up futureWatcher to know when results are available
        optimizationStopped = false;
        future = QtConcurrent::run(&gruepr::optimizeTeams, this, studentIndexes);       // spin optimization off into a separate thread
        futureWatcher.setFuture(future);                                // connect the watcher to get notified when optimization completes
        multipleSectionsInProgress = (section < (numSectionsToTeam - 1));

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
        team.refreshTeamInfo(students, ScheduleCriterion::getNumBlocksForOneMeeting(teamingOptions));
    }

    for(int team = 0; team < teams.size(); team++) {
        teams[team].name = QString::number(team+1);
    }

    // Sort teams by 1st student's name
    std::sort(teams.begin(), teams.end(), [this](const TeamRecord &a, const TeamRecord &b)
              { const StudentRecord *const firstStudentOnTeamA = findStudentFromID(a.studentIDs.at(0));
                const StudentRecord *const firstStudentOnTeamB = findStudentFromID(b.studentIDs.at(0));
                return ((firstStudentOnTeamA->lastname + firstStudentOnTeamA->firstname) <
                        (firstStudentOnTeamB->lastname + firstStudentOnTeamB->firstname));
              });

    // Display the results in a new tab
    // Eventually maybe this should let the tab take ownership of the teams pointer, deleting when the tab is closed!
    const QString teamSetName = tr("Team set ") + QString::number(teamingOptions->teamsetNumber);
    auto *teamTab = new TeamsTabItem(*teamingOptions, teams, students, dataOptions->sectionNames, teamSetName, letsDoItButton, this);
    ui->dataDisplayTabWidget->addTab(teamTab, teamSetName);
    numTeams = int(teams.size());
    teamingOptions->teamsetNumber++;
    connect(teamTab, &TeamsTabItem::saveState, this, &gruepr::saveState);
    connect(teamTab, &TeamsTabItem::addCriterionRequested, this, static_cast<void (gruepr::*)(Criterion::CriteriaType)>(&gruepr::addCriteriaCard));
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

    //Initialize all cards, but do not add them to the layout
    for (int attribute = 0; attribute < dataOptions->numAttributes; attribute++){
        const QString title = "Attribute: "+ dataOptions->attributeQuestionText.at(attribute);
        initializedAttributeCriteriaCards << new GroupingCriteriaCard(Criterion::CriteriaType::attributeQuestion, dataOptions,
                                                                       teamingOptions, this, title, true, attribute);
        auto *currentAttributeCard = initializedAttributeCriteriaCards.last();
        const auto &currentAttributeCriterion = qobject_cast<AttributeCriterion*>(currentAttributeCard->criterion);
        attributeWidgets << currentAttributeCriterion->attributeWidget;
        currentAttributeCriterion->attributeWidget->setValues();
        currentAttributeCard->setVisible(false); //just initialize, don't show yet
    }

    //Remove duplicates
    if(std::any_of(students.constBegin(), students.constEnd(), [](const StudentRecord &student){return student.duplicateRecord;})) {
        grueprGlobal::warningMessage(this, "gruepr", tr("There appears to be at least one student with multiple survey submissions."), tr("OK"));
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
        QJsonArray criteriacardsjsons;
        for (const auto *card : std::as_const(criteriaCardsList)) {
            QJsonObject cardjson;
            auto criteriaTypeEnum = QMetaEnum::fromType<Criterion::CriteriaType>();
            cardjson["criteriaType"] = criteriaTypeEnum.valueToKey(static_cast<int>(card->criterion->criteriaType));
            cardjson["settings"] = card->criterion->settingsToJson();
            if (card->criterion->criteriaType == Criterion::CriteriaType::attributeQuestion) {
                const auto *attrCriterion = qobject_cast<AttributeCriterion*>(card->criterion);
                cardjson["attributeIndex"] = attrCriterion->attributeIndex;
            }
            criteriacardsjsons.append(cardjson);
        }
        content["criteriaCards"] = criteriacardsjsons;
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
    // delete all the widgets and clear the table
    for(int row = ui->studentTable->rowCount() - 1; row >= 0; row--) {
        for(int col = ui->studentTable->columnCount() - 1; col >= 0; col--) {
            QWidget *w = ui->studentTable->cellWidget(row, col);
            if(w != nullptr) {
                ui->studentTable->removeCellWidget(row, col);
                delete w;
            }
        }
    }
    ui->studentTable->clear();
    ui->studentTable->setSortingEnabled(false);

    const bool anyDuplicates = std::any_of(students.constBegin(), students.constEnd(),
                                           [](const StudentRecord &s){ return !s.deleted && s.duplicateRecord; });

    // Build duplicate group sort keys so clicking the status column clusters likely duplicates
    QHash<long long, QString> duplicateSortKeys;
    if(anyDuplicates) {
        QHash<QString, QList<long long>> nameGroups;
        QHash<QString, QList<long long>> emailGroups;
        for(const auto &student : std::as_const(students)) {
            if(student.deleted) {
                continue;
            }
            const QString fullName = (student.firstname + student.lastname).toLower();
            if(!fullName.isEmpty()) {
                nameGroups[fullName] << student.ID;
            }
            if(!student.email.isEmpty()) {
                emailGroups[student.email.toLower()] << student.ID;
            }
        }
        for(auto it = nameGroups.constBegin(); it != nameGroups.constEnd(); ++it) {
            if(it.value().size() > 1) {
                for(const long long id : it.value()) {
                    duplicateSortKeys[id] = it.key();
                }
            }
        }
        for(auto it = emailGroups.constBegin(); it != emailGroups.constEnd(); ++it) {
            if(it.value().size() > 1) {
                for(const long long id : it.value()) {
                    if(!duplicateSortKeys.contains(id)) {
                        duplicateSortKeys[id] = it.key();
                    }
                }
            }
        }
    }

    ui->studentTable->setColumnCount((anyDuplicates ? 1 : 0) + 2 +
                                     (dataOptions->timestampField != DataOptions::FIELDNOTPRESENT ? 1 : 0) +
                                     (dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT ? 1 : 0) +
                                     (dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT ? 1 : 0) +
                                     (dataOptions->sectionIncluded ? 1 : 0));
    const QIcon unsortedIcon(":/icons_new/upDownButton_white.png");
    int column = 0;
    if(anyDuplicates) {
        ui->studentTable->setHorizontalHeaderItem(column++, new QTableWidgetItem(unsortedIcon, tr("  Duplicate?  ")));
    }
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
    const QIcon duplicateIcon(":/icons_new/important_yellow.png");
    for(const auto &student : std::as_const(students)) {
        column = 0;
        if((numActiveStudents < students.size()) &&            // make sure student exists, hasn't been deleted, and is in the section(s) being teamed
            (!student.deleted) &&
            ((teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) ||
             (teamingOptions->sectionType == TeamingOptions::SectionType::allSeparately) ||
             (teamingOptions->sectionType == TeamingOptions::SectionType::noSections) ||
             (student.section == teamingOptions->sectionName))) {

            QList<QTableWidgetItem*> items;

            if(anyDuplicates) {
                auto *statusItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, "");
                if(student.duplicateRecord) {
                    statusItem->setSortKey("0_" + duplicateSortKeys.value(student.ID) + "_" +
                                           student.surveyTimestamp.toString(Qt::ISODate));
                    statusItem->setIcon(duplicateIcon);
                    statusItem->setToolTip(tr("Possible duplicate submission"));
                }
                else {
                    statusItem->setSortKey("1");    // putting non-duplicates strictly after the duplicates
                }
                ui->studentTable->setItem(numActiveStudents, column++, statusItem);
                items << statusItem;
            }
            if(dataOptions->timestampField != DataOptions::FIELDNOTPRESENT) {
                auto *timestamp = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::datetime,
                                                              QLocale::system().toString(student.surveyTimestamp, QLocale::ShortFormat));
                ui->studentTable->setItem(numActiveStudents, column++, timestamp);
                items << timestamp;
            }
            if(dataOptions->firstNameField != DataOptions::FIELDNOTPRESENT) {
                auto *firstName = new QTableWidgetItem(student.firstname);
                ui->studentTable->setItem(numActiveStudents, column++, firstName);
                items << firstName;
            }
            if(dataOptions->lastNameField != DataOptions::FIELDNOTPRESENT) {
                auto *lastName = new QTableWidgetItem(student.lastname);
                ui->studentTable->setItem(numActiveStudents, column++, lastName);
                items << lastName;
            }
            if(dataOptions->sectionIncluded) {
                auto *section = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, student.section);
                ui->studentTable->setItem(numActiveStudents, column++, section);
                items << section;
            }

            for(auto &item : items) {
                if(item->toolTip().isEmpty()) {
                    item->setToolTip(student.tooltip);
                }
            }

            auto *editButton = new QPushButton(QIcon(":/icons_new/edit.png"), "", ui->studentTable);
            editButton->setFlat(true);
            editButton->setIconSize(QSize(20, 20));
            editButton->setToolTip("<html>" + tr("Edit") + " " + student.firstname + " " + student.lastname + tr("'s data.") + "</html>");
            editButton->setProperty("StudentID", student.ID);
            connect(editButton, &QPushButton::clicked, this, &gruepr::editAStudent);
            ui->studentTable->setCellWidget(numActiveStudents, column++, editButton);

            auto *removerButton = new QPushButton(QIcon(":/icons_new/trashButton.png"), "", ui->studentTable);
            removerButton->setFlat(true);
            removerButton->setIconSize(QSize(20, 20));
            removerButton->setToolTip("<html>" + tr("Remove") + " " + student.firstname + " " + student.lastname + " " +
                                      tr("from the student roster.") + "</html>");
            removerButton->setProperty("StudentID", student.ID);
            connect(removerButton, &QPushButton::clicked, ui->studentTable, [this, ID = student.ID, removerButton]{removerButton->disconnect(); removeAStudent(ID);});
            ui->studentTable->setCellWidget(numActiveStudents, column, removerButton);

            numActiveStudents++;
        }
    }
    ui->studentTable->setRowCount(std::min(students.size(), numActiveStudents));

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

    // allocate memory for gene pools and ancestor pools (RAII — freed automatically)
    GA::GenePool genePool(ga, numActiveStudents);
    GA::GenePool nextGenGenePool(ga, numActiveStudents);
    GA::AncestorPool ancestors(ga);
    GA::AncestorPool nextGenAncestors(ga);

    // array of indexes, sorted in order of score (so genePool[orderedIndex[0]] is the one with the top score)
    auto orderedIndex = std::make_unique<int[]>(ga.populationsize);
    for(int genome = 0; genome < ga.populationsize; genome++) {
        orderedIndex[genome] = genome;
    }

    // create an initial population
    // start with an array of all the student IDs in order
    auto randPerm = std::make_unique<int[]>(numActiveStudents);
    for(int i = 0; i < numActiveStudents; i++) {
        randPerm[i] = studentIndexes[i];
    }
    // then make "populationSize" number of random permutations for the initial population, store in genePool
    // just use random values for their initial "ancestor" values
    std::uniform_int_distribution<unsigned int> randAncestor(0, ga.populationsize);
    for(int genome = 0; genome < ga.populationsize; genome++) {
        std::shuffle(randPerm.get(), randPerm.get()+numActiveStudents, pRNG);
        auto *const thisGenome = genePool[genome];
        for(int ID = 0; ID < numActiveStudents; ID++) {
            thisGenome[ID] = randPerm[ID];
        }
        auto *const thisGenomesAncestors = ancestors[genome];
        for(int ancestor = 0; ancestor < ancestors.numAncestors(); ancestor++) {
            thisGenomesAncestors[ancestor] = int(randAncestor(pRNG));
        }
    }

    QList<int> teamSizes(numTeams);
    for(int team = 0; team < numTeams; team++) {
        teamSizes[team] = teams[team].size;
    }

    auto worstTeam = std::make_unique<int[]>(ga.populationsize);
    auto teamStartPositions = std::make_unique<int[]>(numTeams + 1);
    teamStartPositions[0] = 0;
    for(int team = 0; team < numTeams; team++) {
        teamStartPositions[team + 1] = teamStartPositions[team] + teamSizes[team];
    }

    // calculate this first generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
    auto scores = std::make_unique<float[]>(ga.populationsize);
    bool unpenalizedGenomePresent = false;
    // make local copies of member variables to satisfy openMP's needs
    const auto &sharedStudents = students;
    const auto &sharedNumTeams = numTeams;
    const auto *const sharedTeamingOptions = teamingOptions;
    const auto *const sharedDataOptions = dataOptions;

    //parallel initialization of needed variables.
#pragma omp parallel \
        default(none) \
        shared(scores, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions) \
        reduction(||:unpenalizedGenomePresent)
    {
        QList<float> teamScores(sharedNumTeams);
        QList<QList<float>> criteriaScores(sharedTeamingOptions->criteria.size(), QList<float>(sharedNumTeams));
        QList<float> penaltyPoints(sharedNumTeams);
#pragma omp for
        for(int genome = 0; genome < ga.populationsize; genome++) {
            scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes.data(),
                                            sharedTeamingOptions, sharedDataOptions, teamScores.data(),
                                            criteriaScores, penaltyPoints);
            unpenalizedGenomePresent = unpenalizedGenomePresent || std::all_of(penaltyPoints.cbegin(), penaltyPoints.cend(), [](const int p){return p == 0;});
        }
    }

    // get genome indexes in order of score, largest to smallest
    std::sort(orderedIndex.get(), orderedIndex.get() + ga.populationsize,
              [&scores](const int i, const int j){return (scores[i] > scores[j]);});
    emit generationComplete(scores.get(), orderedIndex.get(), 0, 0, unpenalizedGenomePresent);

    const int *mom=nullptr, *dad=nullptr;               // pointer to genome of mom and dad
    float bestScores[GA::GENERATIONS_OF_STABILITY]={0};	// historical record of best score in the genome, going back generationsOfStability generations
    float scoreStability = 0;
    int generation = 0;
    bool localOptimizationStopped = false;

    // now optimize
    do {        // allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
        do {        // keep optimizing until reach stability or maxGenerations
            // clone the elites in genePool into nextGenGenePool, shifting their ancestor arrays as if "self-mating"
            for(int genome = 0; genome < GA::NUM_ELITES; genome++) {
                ga.clone(genePool[orderedIndex[genome]], ancestors[orderedIndex[genome]], orderedIndex[genome],
                         nextGenGenePool[genome], nextGenAncestors[genome], numActiveStudents);
            }

            // create rest of population in nextGenGenePool by mating
            for(int genome = GA::NUM_ELITES; genome < ga.populationsize; genome++) {
                //get a couple of parents
                ga.tournamentSelectParents(genePool.data(), orderedIndex.get(), ancestors.data(), mom, dad, nextGenAncestors[genome], pRNG);

                //mate them and put child in nextGenGenePool
                ga.mate(mom, dad, teamStartPositions.get(), sharedNumTeams, nextGenGenePool[genome], numActiveStudents, pRNG);
            }

            // swap pointers to make nextGen's genePool and ancestors into this generation's
            swap(genePool, nextGenGenePool);
            swap(ancestors, nextGenAncestors);

            generation++;

            // calculate this generation's scores (multi-threaded using OpenMP, preallocating one set of scoring variables per thread)
            unpenalizedGenomePresent = false;
#pragma omp parallel \
            default(none) \
                shared(scores, worstTeam, sharedStudents, genePool, sharedNumTeams, teamSizes, sharedTeamingOptions, sharedDataOptions) \
                reduction(||:unpenalizedGenomePresent)
            {
                QList<float> teamScores(sharedNumTeams);
                QList<QList<float>> criteriaScores(sharedTeamingOptions->criteria.size(), QList<float>(sharedNumTeams));
                QList<float> penaltyPoints(sharedNumTeams);
#pragma omp for
                for(int genome = 0; genome < ga.populationsize; genome++) {
                    scores[genome] = getGenomeScore(sharedStudents.constData(), genePool[genome], sharedNumTeams, teamSizes.data(),
                                                    sharedTeamingOptions, sharedDataOptions, teamScores.data(),
                                                    criteriaScores, penaltyPoints);
                    // find this genome's worst team
                    int worst = 0;
                    for(int team = 1; team < sharedNumTeams; team++) {
                        if(teamScores[team] < teamScores[worst]) {
                            worst = team;
                        }
                    }
                    worstTeam[genome] = worst;
                    unpenalizedGenomePresent = unpenalizedGenomePresent ||
                                               std::all_of(penaltyPoints.cbegin(), penaltyPoints.cend(), [](const int p){return p == 0;});
                }
            }

            // get genome indexes in order of score, largest to smallest
            std::sort(orderedIndex.get(), orderedIndex.get() + ga.populationsize,
                      [&scores](const int i, const int j){return (scores[i] > scores[j]);});

            // mutate all but the single top-scoring genome with some probability
            std::uniform_int_distribution<unsigned int> randProbability(1, 100);
            for(int genome = 0; genome < ga.populationsize; genome++) {
                if(genome == orderedIndex[0]) {
                    continue;
                }
                while(randProbability(pRNG) < ga.mutationlikelihood) {
                    ga.mutateWorstTeam(genePool[genome], teamStartPositions.get(), worstTeam[genome], numActiveStudents, pRNG);
                }
            }

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
            emit generationComplete(scores.get(), orderedIndex.get(), generation, scoreStability, unpenalizedGenomePresent);

            optimizationStoppedmutex.lock();
            localOptimizationStopped = optimizationStopped;
            optimizationStoppedmutex.unlock();
        }
        while(!localOptimizationStopped && ((generation < GA::MIN_GENERATIONS) ||
                                            ((generation < GA::MAX_GENERATIONS) && (scoreStability < GA::MIN_SCORE_STABILITY))));

        if(localOptimizationStopped || teamingOptions->criteria.empty()) { //if no criteria to group by, return immediately
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
                             QList<QList<float>> &_criteriaScores, QList<float> &_penaltyPoints)
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

    for (int criterion = 0; criterion < _teamingOptions->criteria.size(); criterion++) {
        _teamingOptions->criteria[criterion]->calculateScore(_students, _teammates, _numTeams, _teamSizes,
                                                             _teamingOptions, _dataOptions, _criteriaScores[criterion], _penaltyPoints);
    }

    // Bring together for a final score for each team:
    // Score is normalized to be out of 100 (but with possible "extra credit" for more than criterion match)
    for(int team = 0; team < _numTeams; team++) {
        for(int criterion = 0; criterion < _teamingOptions->criteria.size(); criterion++) {
            // remove any criterion's "extra credit" (score > weight) if **any** penalties are being applied,
            // so that very high extra credit doesn't cancel out the penalty
            if(_criteriaScores[criterion][team] > _teamingOptions->criteria[criterion]->weight &&
                _penaltyPoints[team] > 0) {
                _teamScores[team] += _teamingOptions->criteria[criterion]->weight;
            }
            else {
                _teamScores[team] += _criteriaScores[criterion][team];
            }
        }

        if(_penaltyPoints[team] > 0) {
            _penaltyPoints[team] = std::max(_penaltyPoints[team], MINIMUM_PENALTY);
        }
        _teamScores[team] = 100 * ((_teamScores[team] / float(_teamingOptions->criteria.size())) - _penaltyPoints[team]);
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

void gruepr::closeEvent(QCloseEvent *event)
{
    QSettings savedSettings;
    savedSettings.setValue("windowGeometry", saveGeometry());
    saveState();    // save current work for possible future use
    event->accept();
    emit closed();
}
