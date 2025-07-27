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
#include "criteria/genderCriterion.h"
#include "criteria/gradeBalanceCriterion.h"
#include "criteria/attributeCriterion.h"
#include "criteria/TeammatesCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/sectionCriterion.h"
#include "criteria/singleURMIdentityCriterion.h"
#include "criteria/teamsizeCriterion.h"
#include <QPropertyAnimation>

GroupingCriteriaCard::GroupingCriteriaCard(Criterion::CriteriaType criterionType, const DataOptions *const dataOptions, TeamingOptions *const teamingOptions,
                                           QWidget *parent, QString title, bool draggable)
    : QFrame(parent)
{
    switch(criterionType) {
    case Criterion::CriteriaType::section:
        criterion = new SectionCriterion(criterionType, 0, true, this);
        criterion->precedence = Criterion::Precedence::need;
        break;
    case Criterion::CriteriaType::teamSize:
        criterion = new TeamsizeCriterion(criterionType, 0, true, this);
        criterion->precedence = Criterion::Precedence::need;
        break;
    case Criterion::CriteriaType::genderIdentity:
        criterion = new GenderCriterion(criterionType, 0, false, this);
        break;
    case Criterion::CriteriaType::urmIdentity:
        criterion = new SingleURMIdentityCriterion(criterionType, 0, false, this);
        break;
    case Criterion::CriteriaType::attributeQuestion:
        if(dataOptions != nullptr) {
            criterion = new AttributeCriterion(dataOptions, criterionType, 0, false, this);
            break;
        }
        else {
            return;
        }
    case Criterion::CriteriaType::scheduleMeetingTimes:
        if(dataOptions != nullptr) {
            criterion = new ScheduleCriterion(dataOptions, criterionType, 0, false, this);
            break;
        }
        else {
            return;
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

    //make it draggable and droppable
    //setAcceptDrops(draggable);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    //initialize timer
    m_dragTimer.setInterval(50);
    m_dragTimer.setSingleShot(false);

    connect(&m_dragTimer, &QTimer::timeout, this, [this]()
            {
                emit criteriaCardMoved(m_lastPos);
            });

    //initialize parts of section
    toggleButton = new QToolButton(this);
    titleLabel = new LabelThatForwardsMouseClicks(this);
    toggleAnimation = new QParallelAnimationGroup(this);
    contentArea = new QScrollArea(this);
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
    toggleButton->setArrowType(Qt::ArrowType::RightArrow);
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
    //set initial toggle to true

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

    lockButton = new QPushButton(this);
    lockButton->setStyleSheet("border: none;");
    lockButton->setIcon(QIcon(":/icons_new/lock.png"));

    //contentArea settings
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    contentArea->setStyleSheet("QScrollArea { border: none; }");

    //start out collapsed
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);

    // let the entire widget grow and shrink with its content
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "minimumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "maximumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(contentArea, "maximumHeight"));

    QString priorityOrder = QString::number(this->priorityOrder);
    priorityOrderLabel = new QLabel((criterion->precedence == Criterion::Precedence::need? tr("Need") : tr("Want")) + " #" + priorityOrder);
    priorityOrderLabel->setFixedWidth(75);
    priorityOrderLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
    priorityOrderLabel->setStyleSheet("border: none");

    headerRowLayout = new QHBoxLayout();
    QHBoxLayout* contentRowLayout = new QHBoxLayout();
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
        headerRowLayout->addWidget(lockButton);
        lockButton->setVisible(true);
        dragHandleButton->setVisible(false);
        headerRowLayout->addWidget(priorityOrderLabel, Qt::AlignLeft);
        headerRowLayout->addWidget(toggleButton, Qt::AlignLeft);
        headerRowLayout->addWidget(titleLabel, Qt::AlignLeft);
        headerRowLayout->addStretch();
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
    toggleButton->setChecked(true);

    if(criterion != nullptr) {
        criterion->setParent(this);
        if(teamingOptions != nullptr) {
            criterion->generateCriteriaCard(teamingOptions);
        }
    }
}

// QPushButton* GroupingCriteriaCard::getDeleteButton(){
//     return deleteGroupingCriteriaCardButton;
// }

//delete button! and checkbox button!

//if draggable is false, it locks the criteria!



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
        headerRowLayout->addWidget(dragHandleButton);
        headerRowLayout->addWidget(priorityOrderLabel);
        headerRowLayout->addWidget(toggleButton, Qt::AlignLeft);
        headerRowLayout->addWidget(titleLabel, Qt::AlignLeft);
        headerRowLayout->addWidget(deleteGroupingCriteriaCardButton, Qt::AlignRight);
        headerRowLayout->addStretch();
    }
    else {
        setAcceptDrops(false);
        dragHandleButton->setVisible(false);
        lockButton->setVisible(true);
        headerRowLayout->addWidget(lockButton);
        headerRowLayout->addWidget(priorityOrderLabel);
        headerRowLayout->addWidget(toggleButton, Qt::AlignLeft);
        headerRowLayout->addWidget(titleLabel, Qt::AlignLeft);
        headerRowLayout->addWidget(deleteGroupingCriteriaCardButton, Qt::AlignRight);
        headerRowLayout->addStretch();
    }
}

void GroupingCriteriaCard::toggle(bool collapsed)
{
    toggleButton->setArrowType(collapsed ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    toggleAnimation->setDirection(collapsed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    toggleAnimation->start();
}


//Sets ContentLayout for the contentArea, this function always needs to be called otherwise, the expanded portion does not have a layout
void GroupingCriteriaCard::setContentAreaLayout(QLayout &contentLayout)
{
    delete contentArea->layout();

    //if cannot cast contentLayout to VBoxLayout, then
    QVBoxLayout *mainLayout = new QVBoxLayout();
    //don't allow user to set penalty checkbox if it criteria is teamsize or section

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

    auto contentHeight = mainLayout->sizeHint().height();
    const auto collapsedHeight = sizeHint().height() - contentArea->maximumHeight();

    for (int i = 0; i < toggleAnimation->animationCount() - 1; ++i) {
        QPropertyAnimation *SectionAnimation = static_cast<QPropertyAnimation *>(toggleAnimation->animationAt(i));
        SectionAnimation->setDuration(animationDuration);
        SectionAnimation->setStartValue(collapsedHeight);
        SectionAnimation->setEndValue(collapsedHeight + contentHeight);
    }

    QPropertyAnimation *contentAnimation = static_cast<QPropertyAnimation *>(toggleAnimation->animationAt(
        toggleAnimation->animationCount() - 1));
    contentAnimation->setDuration(animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);

    if (toggleButton->isChecked()) {
        contentArea->setMaximumHeight(contentHeight);
    }
}

// Drag and Drop Methods

void GroupingCriteriaCard::dragStarted() {
    setCursor(Qt::ClosedHandCursor);
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString::number(reinterpret_cast<quintptr>(this)));
    //qDebug() << mimeData;
    drag->setMimeData(mimeData);
    QPixmap pixmap = grab();

    // Create a transparentPixmap over which we will paint the pixmap in 50% opacity
    QPixmap transparentPixmap(pixmap.size());
    transparentPixmap.fill(Qt::transparent);

    QPainter painter(&transparentPixmap);
    painter.setOpacity(0.5);
    painter.drawPixmap(0, 0, pixmap);  // Draw the original pixmap with opacity
    painter.end();

    drag->setPixmap(transparentPixmap); //Capture the widget's appearance

    QPoint hotSpot = QPoint(10, 10);
    drag->setHotSpot(hotSpot);
    drag->exec(Qt::MoveAction);
}

QPoint GroupingCriteriaCard::mapToViewport(const QPointF &local)
{
    QAbstractScrollArea *vp = nullptr;
    for (QWidget *w = parentWidget(); w; w = w->parentWidget())
        if ((vp = qobject_cast<QAbstractScrollArea*>(w)))
            return vp->viewport()->mapFromGlobal(mapToGlobal(local.toPoint()));
    return local.toPoint();
}


void GroupingCriteriaCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        m_lastPos = mapToViewport(event->position());
        if (!m_dragTimer.isActive())
            m_dragTimer.start();
    }
}

void GroupingCriteriaCard::dragMoveEvent(QDragMoveEvent *e)
{
    m_lastPos = mapToViewport(e->position()); // just refresh the cached pos
}

void GroupingCriteriaCard::dropEvent(QDropEvent *event)
{
    // Get the widget ID (pointer stored in mime data)
    QString widgetID = event->mimeData()->text();
    GroupingCriteriaCard *draggedFrame = reinterpret_cast<GroupingCriteriaCard*>(widgetID.toULongLong());
    m_dragTimer.stop();
    if (draggedFrame!=this) {
        // Find the index of both frames in the layout
        int draggedPriorityOrder = draggedFrame->getPriorityOrder();
        int targetPriorityOrder = this->getPriorityOrder();

        //qDebug() << this->getPriorityOrder();
        //qDebug() << draggedFrame->getPriorityOrder();

        // Insert the widgets back at their new positions
        event->acceptProposedAction();
        emit criteriaCardSwapRequested(draggedPriorityOrder, targetPriorityOrder);  // Emitting the signal
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
    priorityOrderLabel->setText((criterion->precedence == Criterion::Precedence::need? tr("Need") : tr("Want")) + " #" + QString::number(this->priorityOrder+1));
    headerRowLayout->update();
}

Criterion::Precedence GroupingCriteriaCard::getPrecedence() const
{
    return criterion->precedence;
}

void GroupingCriteriaCard::setPrecedence(Criterion::Precedence precedence)
{
    criterion->precedence = precedence;
    priorityOrderLabel->setText((precedence == Criterion::Precedence::need? tr("Need") : tr("Want")) + " #" + QString::number(this->priorityOrder+1));
    headerRowLayout->update();
}

// --------------------------------------------------------------------------------
//                          QCustomWidget nneded methods
// --------------------------------------------------------------------------------

QString GroupingCriteriaCard::name() const {
    return title;
}

QString GroupingCriteriaCard::includeFile() const {
    return "Section.h";
}

QString GroupingCriteriaCard::group() const {
    return tr("Containers");
}

QIcon GroupingCriteriaCard::icon() const {
    return QIcon("icon.png");
}

QString GroupingCriteriaCard::toolTip() const {
    return tr("Collapsible and expandable section");
}

QString GroupingCriteriaCard::whatsThis() const
{
    return tr("Cool collapsible and expandable section widget");
}

bool GroupingCriteriaCard::isContainer() const
{
    return true;
}

QWidget *GroupingCriteriaCard::createWidget(QWidget *parent)
{
    return new GroupingCriteriaCard(Criterion::CriteriaType::attributeQuestion, nullptr, nullptr, parent);
}
