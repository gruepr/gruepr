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
#include "gruepr.h"
#include "criteria/assignmentPreferenceCriterion.h"
#include "criteria/attributeCriterion.h"
#include "criteria/genderCriterion.h"
#include "criteria/scheduleCriterion.h"
#include "criteria/sectionCriterion.h"
#include "criteria/teammatesCriterion.h"
#include "criteria/teamsizeCriterion.h"
#include "criteria/URMIdentityCriterion.h"
#include <QAbstractScrollArea>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>

namespace {
// Invalidates a layout and every layout nested inside it, depth first. When a widget's contents
// change, Qt invalidates only the layout installed directly on that widget's parent -- so a layout
// nested inside it can go on serving cached values, including a stale hasHeightForWidth().
void invalidateLayoutTree(QLayout *layout)
{
    for (int i = 0; i < layout->count(); ++i) {
        if (QLayout *sub = layout->itemAt(i)->layout()) {
            invalidateLayoutTree(sub);
        }
    }
    layout->invalidate();
}
}

int GroupingCriteriaCard::fixedCardOffset = 0;

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
        criterion = new URMIdentityCriterion(dataOptions, criterionType, 0, true, this);
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
    case Criterion::CriteriaType::assignmentPreference:
        if(dataOptions != nullptr) {
            criterion = new AssignmentPreferenceCriterion(dataOptions, criterionType, 0, false, this);
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
    case Criterion::CriteriaType::groupTogether:
    case Criterion::CriteriaType::splitApart:
        criterion = new TeammatesCriterion(criterionType, 0, true, this);
        break;
    }

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    //initialize timer
    dragTimer.setInterval(50);
    dragTimer.setSingleShot(false);

    connect(&dragTimer, &QTimer::timeout, this, [this]() {
        emit criteriaCardMoved(lastPosOfCard);
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
    contentArea->installEventFilter(this);

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

    // Only contentArea is animated. The card's own height follows from its sizeHint (see the Fixed
    // vertical size policy below), so the column of cards re-packs itself through ordinary layout
    // invalidation as the content grows or shrinks -- nothing has to reposition the cards by hand.
    toggleAnimation = new QParallelAnimationGroup(this);
    auto *contentAnimation = new QPropertyAnimation(contentArea, "maximumHeight");
    toggleAnimation->addAnimation(contentAnimation);
    // Keeping contentArea's minimum equal to its maximum is what makes the card's sizeHint exact:
    // QWidgetItem::sizeHint() bounds to the maximum and then expands to the minimum, so with the two
    // equal the card is always exactly as tall as its header plus its content -- at every frame, and
    // whether or not the content is word-wrapped (whose height sizeHint() alone underestimates).
    connect(contentAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        contentArea->setMinimumHeight(value.toInt());
        // Our sizeHint just changed; nothing infers that from a child's resize, so say so.
        updateGeometry();
        emit cardHeightChanged();
    });

    priorityOrderLabel = new QLabel;
    priorityOrderLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    priorityOrderLabel->setStyleSheet("border: none");
    switch(criterion->precedence) {
        case Criterion::Precedence::fixed:
            priorityOrderLabel->setText("");
            priorityOrderLabel->setFixedWidth(5);
        break;
        case Criterion::Precedence::need:
            priorityOrderLabel->setText(tr("Requirement"));// "# " + priorityOrder + " - " + tr("Need");
            priorityOrderLabel->setToolTip("gruepr will optimize for this as a requirement above all preferences");
        break;
        case Criterion::Precedence::want:
            priorityOrderLabel->setText(tr("Preference") + " # " + QString::number(priorityOrder + 1 - fixedCardOffset)); // + " - " + tr("Want");
            priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    }

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
    // Fixed vertically: the column gives this card exactly its sizeHint, which mainVerticalLayout
    // computes as the header row plus contentArea's current height. Nothing else may set this card's
    // minimumHeight or maximumHeight -- doing so is what used to leave the column laying out one set
    // of heights while the cards had another.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    connect(deleteGroupingCriteriaCardButton, &QPushButton::clicked, this, [this](){
        emit deleteCardRequested(priorityOrder);
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

    if (auto *grueprParent = qobject_cast<gruepr*>(parent)) {
        connect(this, &GroupingCriteriaCard::criteriaCardMoveRequested, grueprParent, &gruepr::moveCriteriaCard);
        connect(this, &GroupingCriteriaCard::dragStarting, grueprParent, &gruepr::showBottomDropZone);
        connect(this, &GroupingCriteriaCard::dragEnteredCard, grueprParent, &gruepr::showDropIndicator);
        connect(this, &GroupingCriteriaCard::dragFinished, grueprParent, &gruepr::hideDropIndicator);
        connect(this, &GroupingCriteriaCard::criteriaCardMoved, grueprParent, &gruepr::doAutoScroll);
        connect(this, &GroupingCriteriaCard::deleteCardRequested, grueprParent, &gruepr::deleteCriteriaCard);
        connect(this, &GroupingCriteriaCard::includePenaltyStateChanged, grueprParent, &gruepr::refreshCriteriaLayout);
        connect(this, &GroupingCriteriaCard::cardHeightChanged, grueprParent, &gruepr::layoutCriteriaCards);
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

    refreshContentHeight();
}

void GroupingCriteriaCard::refreshContentHeight()
{
    QLayout *layout = contentArea->layout();
    if (layout == nullptr) {
        return;
    }

    // A criterion label's setText() invalidates only its parent widget's layout -- contentArea's
    // mainLayout -- and not the nested criterion layout the label actually sits in. That nested layout
    // then keeps serving a stale cache, reporting hasHeightForWidth() == false even though it now
    // holds a word-wrapped label, which silently skips the wrapped measurement below. Invalidating the
    // whole tree makes every cached sizeHint and hasHfw flag come from the current contents.
    invalidateLayoutTree(layout);

    // A word-wrapped label's sizeHint() is its natural multi-line height, which has nothing to do with
    // the width it actually gets -- on a wide card that overestimates badly (a response label wants 51
    // or 85px by sizeHint but needs 17 at the card's real width). heightForWidth(w) is by definition
    // the height needed at width w, so use it outright once the card has been through a real layout
    // pass. Before that pass contentArea is still at its default 100px width, where measuring wrapped
    // text is meaningless, so fall back to sizeHint() until then.
    int contentHeight = layout->sizeHint().height();
    if (layout->hasHeightForWidth() && hasRealWidth) {
        contentHeight = layout->heightForWidth(contentArea->width());
    }
    lastMeasuredWidth = contentArea->width();

    auto *contentAnimation = static_cast<QPropertyAnimation*>(toggleAnimation->animationAt(0));
    contentAnimation->setDuration(animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);

    if (toggleButton->isChecked()) {
        contentArea->setMinimumHeight(contentHeight);
        contentArea->setMaximumHeight(contentHeight);
    }
    // Same as during the animation: the card's height is its sizeHint, and the column has to be told
    // to re-place everything when it changes.
    updateGeometry();
    emit cardHeightChanged();
}

// Drag and Drop Methods

void GroupingCriteriaCard::dragStarted() {
    QApplication::setOverrideCursor(Qt::ClosedHandCursor);
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

    // Insert a placeholder where this card is, then hide the card
    auto *parentLayout = qobject_cast<QVBoxLayout*>(parentWidget()->layout());
    if (parentLayout != nullptr) {
        const int layoutIndex = parentLayout->indexOf(this);
        dragPlaceholder = new QWidget(parentWidget());
        dragPlaceholder->setFixedHeight(height());
        dragPlaceholder->setStyleSheet("background-color: rgba(0, 0, 0, 0.05); border: 2px dashed rgba(0, 0, 0, 0.15); border-radius: 4px;");
        parentLayout->insertWidget(layoutIndex, dragPlaceholder);
        dragPlaceholder->show();   // a disabled layout won't show it for us
    }

    this->hide();  // hide card from layout during drag so the gap appears naturally
    emit cardHeightChanged();   // the placeholder replaced this card, so re-place the column
    emit dragStarting();
    drag->exec(Qt::MoveAction);

    // Remove placeholder
    if (dragPlaceholder != nullptr) {
        delete dragPlaceholder;
        dragPlaceholder = nullptr;
    }

    QApplication::restoreOverrideCursor();
    this->show();  // restore visibility after drag ends
    emit cardHeightChanged();   // the placeholder is gone and this card is back
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
        lastPosOfCard = mapToViewport(event->position());
        if (!dragTimer.isActive()) {
            dragTimer.start();
        }
        emit dragEnteredCard(this->priorityOrder);
    }
}

void GroupingCriteriaCard::dragMoveEvent(QDragMoveEvent *e)
{
    lastPosOfCard = mapToViewport(e->position()); // just refresh the cached pos
}

void GroupingCriteriaCard::dragLeaveEvent(QDragLeaveEvent* /*event*/)
{
    dragTimer.stop();
}

void GroupingCriteriaCard::dropEvent(QDropEvent *event)
{
    // Get the widget ID (pointer stored in mime data)
    const QString widgetID = event->mimeData()->text();
    const auto *draggedFrame = reinterpret_cast<GroupingCriteriaCard*>(widgetID.toULongLong());
    dragTimer.stop();
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

void GroupingCriteriaCard::setPriorityOrder(int newPriorityOrder)
{
    //qDebug() << "set priority for " << newPriorityOrder;
    priorityOrder = newPriorityOrder;
    QString labelText;
    switch(criterion->precedence) {
    case Criterion::Precedence::fixed:
        labelText = "";
        break;
    case Criterion::Precedence::need:
        labelText = tr("Requirement");// "# " + QString::number(priorityOrder+1) + " - " + tr("Need");
        priorityOrderLabel->setToolTip("gruepr will optimize for this as a requirement above all preferences");
        break;
    case Criterion::Precedence::want:
        labelText = tr("Preference") + " # " + QString::number(priorityOrder + 1 - fixedCardOffset); // + " - " + tr("Want");
        priorityOrderLabel->setToolTip("Precedence of this criterion when creating teams");
        break;
    }
    priorityOrderLabel->setText(labelText);
    headerRowLayout->update();
}

void GroupingCriteriaCard::stopDragTimer()
{
    dragTimer.stop();
}

void GroupingCriteriaCard::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        refreshContentHeight();
    });
}

bool GroupingCriteriaCard::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == contentArea && event->type() == QEvent::Resize) {
        // Wrapped content's height depends on its width, so a width change makes the animation
        // endpoints baked in by refreshContentHeight() stale. Deliberately keyed on width alone:
        // refreshContentHeight() sets contentArea's own height, so reacting to height changes here
        // would recurse.
        hasRealWidth = true;
        if (contentArea->width() != lastMeasuredWidth) {
            refreshContentHeight();
        }
    }

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
