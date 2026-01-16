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

#ifndef GROUPINGCRITERIACARDWIDGET_H
#define GROUPINGCRITERIACARDWIDGET_H

#include "criteria/criterion.h"
#include "dataOptions.h"
#include "teamingOptions.h"
#include "widgets/labelThatForwardsMouseClicks.h"
#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>
#include <QtUiPlugin/customwidget.h>
#include <QtDesigner>


class GroupingCriteriaCard : public QFrame, public QDesignerCustomWidgetInterface {
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
    //Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSection")

public:
    explicit GroupingCriteriaCard(Criterion::CriteriaType criterionType, const DataOptions *const dataOptions, TeamingOptions *const teamingOptions,
                                  QWidget *parent = nullptr, QString title = "", bool draggable = false, const int attribute = -1);
    GroupingCriteriaCard(const GroupingCriteriaCard&) = delete;
    GroupingCriteriaCard operator= (const GroupingCriteriaCard&) = delete;
    GroupingCriteriaCard(GroupingCriteriaCard&&) = delete;
    GroupingCriteriaCard& operator= (GroupingCriteriaCard&&) = delete;

    Criterion *criterion = nullptr;

    //QCheckBox *includePenaltyCheckBox = nullptr;
    void setContentAreaLayout(QLayout & contentLayout);
    QString name() const;
    QString includeFile() const;
    QString group() const;
    QIcon icon() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;
    QWidget *createWidget(QWidget *parent);
    QLabel *priorityOrderLabel;
    QHBoxLayout* headerRowLayout;
    QPushButton *deleteGroupingCriteriaCardButton;

    //Drag and Drop Methods
    void setDraggable(bool draggable);
    //QPushButton* getDeleteButton();
    void dragStarted();
    void dragEnterEvent(QDragEnterEvent *event);
    QPoint mapToViewport(const QPointF &local);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    int getPriorityOrder() const;
    void setPriorityOrder(int priorityOrder);
    Criterion::Precedence getPrecedence() const;
    void setPrecedence(Criterion::Precedence precedence);

signals:
    void criteriaCardMoved(QPoint point);
    void criteriaCardSwapRequested(int draggedIndex, int targetIndex);
    void deleteCardRequested(int deletedIndex);
    void includePenaltyStateChanged();

public slots:
    void toggle(bool collapsed);

private:
    int priorityOrder = 0;
    QString title;
    QVBoxLayout* mainVerticalLayout = nullptr;
    QToolButton* toggleButton = nullptr;
    LabelThatForwardsMouseClicks* titleLabel = nullptr;
    QPushButton* dragHandleButton = nullptr;
    QPushButton *lockButton = nullptr;
    QParallelAnimationGroup* toggleAnimation = nullptr;
    QScrollArea* contentArea = nullptr;
    int animationDuration = 100;
    QTimer m_dragTimer;
    QPoint m_lastPos;
};

#endif // SECTION_H
