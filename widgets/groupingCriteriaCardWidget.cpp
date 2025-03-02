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

#include <QPropertyAnimation>

#include "groupingCriteriaCardWidget.h"

GroupingCriteriaCard::GroupingCriteriaCard(QWidget *parent, QString title, bool draggable)
    : QFrame(parent) {

    //make it draggable and droppable
    setAcceptDrops(draggable);

    //initialize parts of section
    toggleButton = new QToolButton(this);
    toggleAnimation = new QParallelAnimationGroup(this);
    contentArea = new QScrollArea(this);
    dragHandleButton = new QPushButton(this);
    mainVerticalLayout = new QVBoxLayout();

    //toggleButton settings
    toggleButton->setStyleSheet(R"(
    QToolButton {border: none;}
    QToolButton:hover {
        background-color: rgba(0, 0, 0, 0.1); /* subtle darkening */
        border-radius: 1px;
    })");
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setArrowType(Qt::ArrowType::RightArrow);
    toggleButton->setText(title);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false);
    // Ensure toggleButton don't stretch
    toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
    dragHandleButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    dragHandleButton->setMinimumWidth(24);
    dragHandleButton->setMinimumHeight(24);

    //contentArea settings
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentArea->setStyleSheet("QScrollArea { border: none; }");

    // start out collapsed
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);

    // let the entire widget grow and shrink with its content
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "minimumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "maximumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(contentArea, "maximumHeight"));

    QHBoxLayout* headerRowLayout = new QHBoxLayout();
    QHBoxLayout* contentRowLayout = new QHBoxLayout();
    mainVerticalLayout->addLayout(headerRowLayout);
    headerRowLayout->addWidget(dragHandleButton);
    headerRowLayout->addWidget(toggleButton, Qt::AlignLeft);
    contentRowLayout->addWidget(contentArea);
    mainVerticalLayout->addLayout(contentRowLayout);

    setLayout(mainVerticalLayout);
    setContentsMargins(2,2,2,2);
    connect(toggleButton, &QToolButton::toggled, this, &GroupingCriteriaCard::toggle);
    connect(dragHandleButton, &QToolButton::pressed, this, &GroupingCriteriaCard::dragStarted);
    connect(dragHandleButton, &QToolButton::released, this, &QFrame::unsetCursor);
    //set initial toggle to be true
    toggle(true);
}


void GroupingCriteriaCard::toggle(bool collapsed) {
    toggleButton->setArrowType(collapsed ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
    toggleAnimation->setDirection(collapsed ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    toggleAnimation->start();
}


//Sets ContentLayout for the contentArea
void GroupingCriteriaCard::setContentAreaLayout(QLayout &contentLayout) {
    delete contentArea->layout();
    contentArea->setLayout(&contentLayout);
    const auto collapsedHeight = sizeHint().height() - contentArea->maximumHeight();
    auto contentHeight = contentLayout.sizeHint().height();

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
}

// Drag and Drop Methods

void GroupingCriteriaCard::dragStarted() {
    setCursor(Qt::ClosedHandCursor);
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString::number(reinterpret_cast<quintptr>(this)));
    qDebug() << mimeData;
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

void GroupingCriteriaCard::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasText()) {
        qDebug() << "drag enter event accepted";
        event->acceptProposedAction();
    }
}

void GroupingCriteriaCard::dropEvent(QDropEvent *event) {
    // Get the widget ID (pointer stored in mime data)
    QString widgetID = event->mimeData()->text();
    GroupingCriteriaCard *draggedFrame = reinterpret_cast<GroupingCriteriaCard*>(widgetID.toULongLong());

    if (draggedFrame!=this) {
        // Find the index of both frames in the layout
        int draggedPriorityOrder = draggedFrame->getPriorityOrder();
        int targetPriorityOrder = this->getPriorityOrder();

        qDebug() << this->getPriorityOrder();
        qDebug() << draggedFrame->getPriorityOrder();

        // a layout that stores the objects in a list <DraggableQFrame> then depending on the order, outputs!
        // Insert the widgets back at their new positions
        event->acceptProposedAction();
        emit frameSwapRequested(draggedPriorityOrder, targetPriorityOrder);  // Emitting the signal
    }
}

int GroupingCriteriaCard::getPriorityOrder(){
    return this->priorityOrder;
}
void GroupingCriteriaCard::setPriorityOrder(int priorityOrder){
    this->priorityOrder = priorityOrder;
}

// --------------------------------------------------------------------------------
//                          QCustomWidget methods
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
    return new GroupingCriteriaCard( parent);
}
