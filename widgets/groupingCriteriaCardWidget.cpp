/*
    Elypson/qt-collapsible-section
    (c) 2016 Michael A. Voelkel - michael.alexander.voelkel@gmail.com

    This file is part of Elypson/qt-collapsible section.

    Elypson/qt-collapsible-section is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Elypson/qt-collapsible-section is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Elypson/qt-collapsible-section. If not, see <http://www.gnu.org/licenses/>.
*/

#include "groupingCriteriaCardWidget.h"
#include "criteria/attributeCriterion.h"
#include "criteria/genderCriterion.h"
#include "criteria/gradeBalanceCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/sectionCriterion.h"
#include "criteria/teammatesCriterion.h"
#include "criteria/teamsizeCriterion.h"
#include "criteria/URMIdentityCriterion.h"
#include <QAbstractScrollArea>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>

GroupingCriteriaCard::GroupingCriteriaCard(Criterion::CriteriaType criterionType, const DataOptions *const dataOptions, TeamingOptions *const teamingOptions,
                                           QWidget *parent, QString title, bool draggable, const int attribute)
    : QFrame(parent)
{
    switch(criterionType) {
    case Criterion::CriteriaType::section:
        criterion = new SectionCriterion(criterionType, 0, true, this);
        criterion->precedence = Criterion::Precedence::fixed;
        break;
    case Criterion::CriteriaType::teamSize:
        criterion = new TeamsizeCriterion(criterionType, 0, true, this);
        criterion->precedence = Criterion::Precedence::fixed;
        break;
    case Criterion::CriteriaType::genderIdentity:
        criterion = new GenderCriterion(dataOptions, criterionType, 0, true, this);
        break;
    case Criterion::CriteriaType::urmIdentity:
        criterion = new URMIdentityCriterion(dataOptions, criterionType, 0, false, this);
        break;
    case Criterion::CriteriaType::attributeQuestion:
        if(dataOptions != nullptr && attribute != -1) {
            criterion = new AttributeCriterion(dataOptions, criterionType, 0, false, this, attribute);
            break;
        }
        else {
            return;
            break;
        }
    case Criterion::CriteriaType::scheduleMeetingTimes:
        if(dataOptions != nullptr) {
            criterion = new ScheduleCriterion(dataOptions, criterionType, 0, false, this);
            break;
        }
        else {
            return;
            break;
        }
    case Criterion::CriteriaType::requiredTeammates:
    case Criterion::CriteriaType::preventedTeammates:
    case Criterion::CriteriaType::requestedTeammates:
        criterion = new TeammatesCriterion(criterionType, 0, true, this);
        break;
    case Criterion::CriteriaType::gradeBalance:
        criterion = new GradeBalanceCriterion(criterionType, 0, false, this);
        break;
    }

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    //initialize timer
    m_dragTimer.setInterval(50);
    m_dragTimer.setSingleShot(false);

    connect(&m_dragTimer, &QTimer::timeout, this, [this]() {
        emit criteriaCardMoved(m_lastPos);
    });

    //initialize parts of section
    toggleButton = new QToolButton(this);
    titleLabel = new LabelThatForwardsMouseClicks(this);
    contentArea = new QWidget(this);
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);
    dragHandleButton = new QPushButton(this);
    mainVerticalLayout = new QVBoxLayout();

    //toggleButton settings
    toggleButton->setStyleSheet(R"(
        QToolButton {
            border: none;
            font-family: 'DM Sans';  /* Set font family to DM Sans */
            font-size: 12pt;         /* Set font size to 12 */
        }
        QToolButton:hover {
            background-color: rgba(0, 0, 0, 0.1); /* subtle darkening */
            border-radius: 1px;
        }
    )");
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setArrowType(Qt::ArrowType::DownArrow);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false);
    toggleButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    titleLabel->setText(title);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(R"(
        QLabel {
            border: none;
            font-family: 'DM Sans';  /* Set font family to DM Sans */
            font-size: 12pt;         /* Set font size to 12 */
        }
        QLabel:hover {
            background-color: rgba(0, 0, 0, 0.1); /* subtle darkening */
            border-radius: 1px;
        }
    )");
    connect(titleLabel, &LabelThatForwardsMouseClicks::mousePressed, toggleButton, &QToolButton::click);
    toggleLayout = new QHBoxLayout();
    toggleLayout->setSpacing(0);
    toggleLayout->setContentsMargins(0, 0, 0, 0);
    toggleLayout->addWidget(toggleButton);
    toggleLayout->addWidget(titleLabel);
    toggleButton->installEventFilter(this);
    titleLabel->installEventFilter(this);

    //dragHandleButton settings
    dragHandleButton->setIcon(QIcon(":/icons_new/drag-handle.png"));
    dragHandleButton->setToolTip(QString("Drag and drop to reorder"));
    dragHandleButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
        }
        QPushButton:hover {
            background-color: rgba(0, 0, 0, 0.1); /* subtle darkening */
            border-radius: 1px;
        }
    )");
    // Ensure dragHandleButton only takes as much space as its content
    dragHandleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dragHandleButton->setMinimumWidth(24);
    dragHandleButton->setMinimumHeight(24);
    dragHandleButton->setCursor(Qt::OpenHandCursor);

    lockButton = new QPushButton(this);
    lockButton->setStyleSheet("border: none;");
    lockButton->setIcon(QIcon(":/icons_new/lock.png"));

    //contentArea settings
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentArea->setStyleSheet("QScrollArea { border: none; }");

    //start out collapsed
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);

    // let the entire widget grow and shrink with its content
    toggleAnimation = new QParallelAnimationGroup(this);
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "minimumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "maximumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(contentArea, "maximumHeight"));
    connect(toggleAnimation, &QAbstractAnimation::finished, this, [this]() {
        QTimer::singleShot(animationDuration + 10, this, [this]() {
            refreshParentLayout();
        });
    });

    const QString priorityOrder = QString::number(this->priorityOrder);
    priorityOrderLabel = new QLabel;
    QString labelText;
    switch(criterion->precedence) {
        case Criterion::Precedence::fixed:
            labelText = "";
            priorityOrderLabel->setFixedWidth(5);
        break;
        case Criterion::Precedence::need:
            labelText = "# " + priorityOrder + " - " + tr("Need");
            priorityOrderLabel->setFixedWidth(75);
            priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
        case Criterion::Precedence::want:
            labelText = "# " + priorityOrder + " - " + tr("Want");
            priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
            priorityOrderLabel->setFixedWidth(75);
        break;
    }
    priorityOrderLabel->setText(labelText);
    priorityOrderLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    priorityOrderLabel->setStyleSheet("border: none");

    headerRowLayout = new QHBoxLayout();
    auto *contentRowLayout = new QHBoxLayout();
    mainVerticalLayout->addLayout(headerRowLayout);

    deleteGroupingCriteriaCardButton = new QPushButton(this);
    deleteGroupingCriteriaCardButton->setIcon(QIcon(":/icons_new/trashButton.png"));
    deleteGroupingCriteriaCardButton->setStyleSheet(R"(
        QPushButton {
            border: none;
        }
        QPushButton:hover {
            background-color: rgba(0, 0, 0, 0.1); /* subtle darkening */
            border-radius: 1px;
        }
    )");
    deleteGroupingCriteriaCardButton->setFixedSize(40, 40);

    if (draggable){
        setDraggable(true);
    }
    else {
        setAcceptDrops(false);
        headerRowLayout->addWidget(lockButton, 0, Qt::AlignLeft);
        lockButton->setVisible(true);
        dragHandleButton->setVisible(false);
        headerRowLayout->addWidget(priorityOrderLabel, 0, Qt::AlignLeft);
        headerRowLayout->addLayout(toggleLayout, 1);
        deleteGroupingCriteriaCardButton->setVisible(false);
    }

    contentRowLayout->addWidget(contentArea);
    mainVerticalLayout->addLayout(contentRowLayout);

    setLayout(mainVerticalLayout);
    setContentsMargins(2,2,2,2);
    connect(deleteGroupingCriteriaCardButton, &QPushButton::clicked, this, [this](){
        emit deleteCardRequested(this->priorityOrder);
    });
    connect(toggleButton, &QToolButton::toggled, this, &GroupingCriteriaCard::toggle);
    connect(dragHandleButton, &QToolButton::pressed, this, &GroupingCriteriaCard::dragStarted);
    connect(dragHandleButton, &QToolButton::released, this, &QFrame::unsetCursor);

    //set initial toggle to be true
    toggleButton->blockSignals(true);
    toggleButton->setChecked(true);
    toggleButton->blockSignals(false);

    if(criterion != nullptr) {
        criterion->setParent(this);
        if(teamingOptions != nullptr) {
            criterion->generateCriteriaCard(teamingOptions);
        }
    }
}


void GroupingCriteriaCard::setDraggable(bool draggable)
{
    // Clear and Rebuild Layout
    while (headerRowLayout->count() > 1) {
        headerRowLayout->removeItem(headerRowLayout->itemAt(1));
    }

    if (draggable) {
        setAcceptDrops(true);
        dragHandleButton->setVisible(true);
        lockButton->setVisible(false);
        headerRowLayout->addWidget(dragHandleButton, 0, Qt::AlignLeft);
        headerRowLayout->addWidget(priorityOrderLabel, 0, Qt::AlignLeft);
        headerRowLayout->addLayout(toggleLayout,1);
        headerRowLayout->addWidget(deleteGroupingCriteriaCardButton, 0, Qt::AlignRight);
    }
    else {
        setAcceptDrops(false);
        dragHandleButton->setVisible(false);
        lockButton->setVisible(true);
        headerRowLayout->addWidget(lockButton, 0, Qt::AlignLeft);
        headerRowLayout->addWidget(priorityOrderLabel, 0, Qt::AlignLeft);
        headerRowLayout->addLayout(toggleLayout,1);
        headerRowLayout->addWidget(deleteGroupingCriteriaCardButton, 0, Qt::AlignRight);
    }
}

void GroupingCriteriaCard::toggle(bool collapsed)
{
    toggleButton->setArrowType(collapsed ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    toggleAnimation->setDirection(collapsed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    toggleAnimation->start();
}

void GroupingCriteriaCard::refreshParentLayout()
{
    if (parentSplitter == nullptr) {
        auto *w = parentWidget();
        while (w != nullptr && qobject_cast<QSplitter*>(w) == nullptr) {
            w = w->parentWidget();
        }
        parentSplitter = qobject_cast<QSplitter*>(w);
    }
    if (parentSplitter != nullptr) {
        QList<int> sizes = parentSplitter->sizes();
        static int direction = -1;
        sizes[0] += direction;
        sizes[1] -= direction;
        direction = -direction;
        parentSplitter->setSizes(sizes);
    }
}

//Sets ContentLayout for the contentArea, this function always needs to be called otherwise, the expanded portion does not have a layout
void GroupingCriteriaCard::setContentAreaLayout(QLayout &contentLayout)
{
    delete contentArea->layout();

    auto *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(&contentLayout);

    // if (criteriaType == CriteriaType::teamSize || criteriaType == CriteriaType::section){
    //     mainLayout->addLayout(&contentLayout);
    // } else {
    //     mainLayout->addLayout(&contentLayout);
    //     includePenaltyCheckBox = new QCheckBox(this);
    //     includePenaltyCheckBox->setText(QString("Set Criteria as Mandatory"));
    //     includePenaltyCheckBox->setToolTip(QString("If condition is unmet, gruepr applies a penalty to the team score"));
    //     includePenaltyCheckBox->setStyleSheet(CHECKBOXSTYLE);
    //     connect(includePenaltyCheckBox, &QCheckBox::stateChanged, this, [this](){
    //         this->criterion->penaltyStatus = (includePenaltyCheckBox->checkState() == Qt::Checked);
    //         if (includePenaltyCheckBox->checkState() == Qt::Checked){
    //             this->setStyleSheet(QString(MANDATORYFRAME) + LABEL10PTMANDATORYSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    //             this->setDraggable(false);
    //         } else {
    //             this->setStyleSheet(QString(BLUEFRAME) + LABEL10PTSTYLE + CHECKBOXSTYLE + COMBOBOXSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + SMALLBUTTONSTYLETRANSPARENT);
    //             this->setDraggable(true);
    //         }
    //         emit includePenaltyStateChanged();
    //     });
    //     mainLayout->addWidget(includePenaltyCheckBox);
    // }

    contentArea->setLayout(mainLayout);

    const int contentHeight = mainLayout->sizeHint().height();
    const int collapsedHeight = sizeHint().height() - contentArea->maximumHeight();

    for (int i = 0; i < toggleAnimation->animationCount() - 1; ++i) {
        auto *SectionAnimation = static_cast<QPropertyAnimation*>(toggleAnimation->animationAt(i));
        SectionAnimation->setDuration(animationDuration);
        SectionAnimation->setStartValue(collapsedHeight);
        SectionAnimation->setEndValue(collapsedHeight + contentHeight);
    }

    auto *contentAnimation = static_cast<QPropertyAnimation*>(toggleAnimation->animationAt(toggleAnimation->animationCount() - 1));
    contentAnimation->setDuration(animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);

    if (toggleButton->isChecked()) {
        contentArea->setMaximumHeight(contentHeight);
        setMinimumHeight(collapsedHeight + contentHeight);
        setMaximumHeight(collapsedHeight + contentHeight);
    }
}

// Drag and Drop Methods

void GroupingCriteriaCard::dragStarted() {
    setCursor(Qt::ClosedHandCursor);
    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setText(QString::number(reinterpret_cast<quintptr>(this)));
    drag->setMimeData(mimeData);
    const QPixmap pixmap = grab();

    // Create a transparentPixmap over which we will paint the pixmap in 50% opacity
    QPixmap transparentPixmap(pixmap.size());
    transparentPixmap.fill(Qt::transparent);

    QPainter painter(&transparentPixmap);
    painter.setOpacity(0.5);
    painter.drawPixmap(0, 0, pixmap);  // Draw the original pixmap with opacity
    painter.end();

    drag->setPixmap(transparentPixmap); //Capture the widget's appearance

    const QPoint hotSpot = QPoint(10, 10);
    drag->setHotSpot(hotSpot);

    this->hide();  // hide card from layout during drag so the gap appears naturally
    emit dragStarting();
    drag->exec(Qt::MoveAction);
    this->show();  // restore visibility after drag ends (refreshCriteriaLayout will reposition)
    emit dragFinished();
}

QPoint GroupingCriteriaCard::mapToViewport(const QPointF &local)
{
    QWidget *w = parentWidget();
    while (w != nullptr && qobject_cast<QAbstractScrollArea*>(w) == nullptr) {
        w = w->parentWidget();
    }
    auto *vp = qobject_cast<QAbstractScrollArea*>(w);
    if (vp != nullptr) {
        return vp->viewport()->mapFromGlobal(mapToGlobal(local.toPoint()));
    }
    return local.toPoint();
}


void GroupingCriteriaCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        m_lastPos = mapToViewport(event->position());
        if (!m_dragTimer.isActive())
            m_dragTimer.start();
        emit dragEnteredCard(this->priorityOrder);
    }
}

void GroupingCriteriaCard::dragMoveEvent(QDragMoveEvent *e)
{
    m_lastPos = mapToViewport(e->position()); // just refresh the cached pos
}

void GroupingCriteriaCard::dropEvent(QDropEvent *event)
{
    // Get the widget ID (pointer stored in mime data)
    const QString widgetID = event->mimeData()->text();
    const auto *draggedFrame = reinterpret_cast<GroupingCriteriaCard*>(widgetID.toULongLong());
    m_dragTimer.stop();
    if (draggedFrame != this) {
        // Find the index of both frames in the layout
        const int draggedPriorityOrder = draggedFrame->getPriorityOrder();
        const int targetPriorityOrder = this->getPriorityOrder();

        //qDebug() << this->getPriorityOrder();
        //qDebug() << draggedFrame->getPriorityOrder();

        // Insert the widgets back at their new positions
        event->acceptProposedAction();
        emit criteriaCardMoveRequested(draggedPriorityOrder, targetPriorityOrder);  // Emitting the signal
    }
}

int GroupingCriteriaCard::getPriorityOrder() const
{
    return priorityOrder;
}

void GroupingCriteriaCard::setPriorityOrder(int priorityOrder)
{
    //qDebug() << "set priority for " << priorityOrder;
    this->priorityOrder = priorityOrder;
    QString labelText;
    switch(criterion->precedence) {
    case Criterion::Precedence::fixed:
        labelText = "";
        break;
    case Criterion::Precedence::need:
        labelText = "# " + QString::number(this->priorityOrder+1) + " - " + tr("Need");
        priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    case Criterion::Precedence::want:
        labelText = "# " + QString::number(this->priorityOrder+1) + " - " + tr("Want");
        priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    }
    priorityOrderLabel->setText(labelText);
    headerRowLayout->update();
}

Criterion::Precedence GroupingCriteriaCard::getPrecedence() const
{
    return criterion->precedence;
}

void GroupingCriteriaCard::setPrecedence(Criterion::Precedence precedence)
{
    criterion->precedence = precedence;
    QString labelText;
    switch(criterion->precedence) {
    case Criterion::Precedence::fixed:
        labelText = "";
        break;
    case Criterion::Precedence::need:
        labelText = "# " + QString::number(this->priorityOrder+1) + " - " + tr("Need");
        priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    case Criterion::Precedence::want:
        labelText = "# " + QString::number(this->priorityOrder+1) + " - " + tr("Want");
        priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    }    headerRowLayout->update();
}

// --------------------------------------------------------------------------------
//                          QCustomWidget needed methods
// --------------------------------------------------------------------------------

QString GroupingCriteriaCard::name() const {
    return title;
}

QString GroupingCriteriaCard::includeFile() const {
    return "groupingCriteriaCardWidget.h";
}

QString GroupingCriteriaCard::group() const {
    return tr("Containers");
}

QIcon GroupingCriteriaCard::icon() const {
    return QIcon();
}

QString GroupingCriteriaCard::toolTip() const {
    return tr("Collapsible and expandable section");
}

QString GroupingCriteriaCard::whatsThis() const
{
    return tr("A collapsible and expandable section widget");
}

bool GroupingCriteriaCard::isContainer() const
{
    return true;
}

QWidget *GroupingCriteriaCard::createWidget(QWidget *parent)
{
    return new GroupingCriteriaCard(Criterion::CriteriaType::attributeQuestion, nullptr, nullptr, parent);
}

bool GroupingCriteriaCard::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == toggleButton || watched == titleLabel) {
        if (event->type() == QEvent::Enter) {
            toggleButton->setStyleSheet(R"(
                QToolButton {
                    border: none;
                    font-family: 'DM Sans';
                    font-size: 12pt;
                    background-color: rgba(0, 0, 0, 0.1);
                    border-radius: 1px;
                })");
            titleLabel->setStyleSheet(R"(
                QLabel {
                    border: none;
                    font-family: 'DM Sans';
                    font-size: 12pt;
                    background-color: rgba(0, 0, 0, 0.1);
                    border-radius: 1px;
                })");
        }
        else if (event->type() == QEvent::Leave) {
            toggleButton->setStyleSheet(R"(
                QToolButton {
                    border: none;
                    font-family: 'DM Sans';
                    font-size: 12pt;
                }
                QToolButton:hover {
                    background-color: rgba(0, 0, 0, 0.1);
                    border-radius: 1px;
                })");
            titleLabel->setStyleSheet(R"(
                QLabel {
                    border: none;
                    font-family: 'DM Sans';
                    font-size: 12pt;
                }
                QLabel:hover {
                    background-color: rgba(0, 0, 0, 0.1);
                    border-radius: 1px;
                })");
        }
    }
    return QFrame::eventFilter(watched, event);
}
