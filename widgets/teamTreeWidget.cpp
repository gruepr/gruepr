#include "teamTreeWidget.h"
#include "gruepr_globals.h"
#include <QDropEvent>
#include <QPainter>
#include <QTextLayout>
#include <QTimer>
#include <QToolTip>


//////////////////
// Tree display for teammates with swappable positions and sortable columns using hidden data
//////////////////
TeamTreeWidget::TeamTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
    headerView = new TeamTreeHeaderView(Qt::Horizontal, this);
    setHeader(headerView);
    setStyleSheet(QString(TEAMTREEWIDGETSTYLE) + SCROLLBARSTYLE);
    setMouseTracking(true);
    setHeaderHidden(false);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(false);
    setAlternatingRowColors(true);
    setAutoScroll(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setStyle(new NoHoverStyle(style()));
    setItemDelegate(new NoHoverDelegate(this));

    connect(this, &QTreeWidget::itemCollapsed, this, &TeamTreeWidget::itemCollapse);
    connect(this, &QTreeWidget::itemExpanded, this, &TeamTreeWidget::itemExpand);
}

void TeamTreeWidget::itemCollapse(QTreeWidgetItem *item)
{
    // only collapse teams (not students or sections)
    auto *newItem = dynamic_cast<TeamTreeWidgetItem*>(item);
    if(newItem == nullptr) {
        return;
    }
    const bool itemIsTeam = (newItem->treeItemType == TeamTreeWidgetItem::TreeItemType::team);
    if(itemIsTeam) {
        for(int column = 0; column < columnCount(); column++) {
            resizeColumnToContents(column);
        }
    }
    else {
        QTreeWidget::expandItem(newItem);
    }
}

void TeamTreeWidget::itemExpand(QTreeWidgetItem *item)
{
    auto *newItem = dynamic_cast<TeamTreeWidgetItem*>(item);
    if(newItem == nullptr) {
        return;
    }
    for(int column = 0; column < columnCount(); column++) {
        resizeColumnToContents(column);
    }
}

void TeamTreeWidget::collapseAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, collapsing only the teams
    auto *item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
    while(item != nullptr) {
        const bool itemIsTeam = (item->treeItemType == TeamTreeWidgetItem::TreeItemType::team);
        if(itemIsTeam) {
            QTreeWidget::collapseItem(item);
            for(int column = 0; column < columnCount(); column++) {
                resizeColumnToContents(column);
            }
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(itemBelow(item));
    }

    setUpdatesEnabled(true);
    repaint();
}

void TeamTreeWidget::expandAll()
{
    setUpdatesEnabled(false);

    // iterate through tree, expanding every section and team
    auto *item = dynamic_cast<TeamTreeWidgetItem*>(topLevelItem(0));
    while(item != nullptr) {
        const bool itemIsStudent = (item->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
        if(!itemIsStudent) {
            QTreeWidget::expandItem(item);
            for(int column = 0; column < columnCount(); column++) {
                resizeColumnToContents(column);
            }
        }
        item = dynamic_cast<TeamTreeWidgetItem*>(itemBelow(item));
    }
    setUpdatesEnabled(true);
    repaint();
}

void TeamTreeWidget::resetDisplay(const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    QStringList headerLabels;
    int i = 0;
    headerLabels << tr("Name");
    headerView->setColumnElideMode(i++, Qt::ElideNone);

    // Add section column if showing all sections together (not criterion-driven, just info)
    if (teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        headerLabels << tr("Sections");
        headerView->setColumnElideMode(i++, Qt::ElideRight);
    }

    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        const auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }
        headerLabels << criterion->headerLabel(dataOptions);
        headerView->setColumnElideMode(i++, criterion->headerElideMode());
    }

    headerLabels << tr("display_order");

    setColumnCount(int(headerLabels.size()));
    for(int i = 0; i < headerLabels.size() - 1; i++) {
        showColumn(i);
    }
    hideColumn(int(headerLabels.size()) - 1);  // don't show the sort order column (can comment this out when debugging sorting operations)

    setHeaderLabels(headerLabels);
    for(int i = 0; i < headerLabels.size(); i++) {
        headerView->setColumnIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
    }

    setSortingEnabled(false);
    setFocus();
    clear();
}

void TeamTreeWidget::refreshSection(TeamTreeWidgetItem *sectionItem, const QString &sectionName)
{
    if(sectionItem->treeItemType != TeamTreeWidgetItem::TreeItemType::section) {
        return;
    }
    sectionItem->setText(0, tr("Section ") + sectionName);
    sectionItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
}

void TeamTreeWidget::refreshTeam(RefreshType refreshType, TeamTreeWidgetItem *teamItem, const TeamRecord &team, const int teamNum,
                                 const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions,
                                 const QList<StudentRecord> &students, const QSet<long long> &IDsBeingTeamed)
{
    if(teamItem->treeItemType != TeamTreeWidgetItem::TreeItemType::team) {
        return;
    }

    // create team items and fill in information
    int column = 0;

    // Name column
    teamItem->setText(column, tr("Team ") + team.name);
    teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, tr("Team ") + team.name);
    teamItem->setData(column, TEAMINFO_SORT_ROLE, team.name); //sort based on team name
    teamItem->setData(column, TEAM_NUMBER_ROLE, teamNum);
    teamItem->setToolTip(column, team.tooltip);
    column++;

    // Sections column
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        teamItem->setText(column, QString::number(team.numSections));
        teamItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(team.numSections));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, team.numSections);
        teamItem->setToolTip(column, team.tooltip);
        column++;
    }

    // One more column per scoring criterion
    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }

        const float score = criterion->scoreForOneTeamInDisplay(students, team, teamingOptions, dataOptions, IDsBeingTeamed);
        const QString text = criterion->teamDisplayText(team, dataOptions, score, students);
        const QVariant sortVal = criterion->teamSortValue(team, dataOptions, score, students);

        teamItem->setText(column, text);
        teamItem->setTextAlignment(column, criterion->teamTextAlignment());
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, text);
        teamItem->setData(column, TEAMINFO_SORT_ROLE, sortVal);
        teamItem->setToolTip(column, team.tooltip);
        teamItem->setBackground(column, criterion->teamDisplayColor(score));

        column++;
    }

    // Display order column
    if(refreshType == RefreshType::newTeam) {
        teamItem->setText(column, QString::number(teamNum));
        teamItem->setTextAlignment(column, Qt::AlignCenter);
        teamItem->setData(column, TEAMINFO_DISPLAY_ROLE, QString::number(teamNum));
        teamItem->setData(column, TEAMINFO_SORT_ROLE, teamNum);
    }
}

void TeamTreeWidget::refreshStudent(TeamTreeWidgetItem *studentItem, const StudentRecord &stu, const DataOptions *const dataOptions, const TeamingOptions *const teamingOptions)
{
    if(studentItem->treeItemType != TeamTreeWidgetItem::TreeItemType::student) {
        return;
    }

    int column = 0;

    // Name column
    studentItem->setText(column, stu.firstname + " " + stu.lastname);
    studentItem->setData(column, Qt::UserRole, stu.ID);
    studentItem->setToolTip(column, stu.tooltip);
    studentItem->setTextAlignment(column, Qt::AlignLeft | Qt::AlignVCenter);
    column++;

    // Section column
    if(teamingOptions->sectionType == TeamingOptions::SectionType::allTogether) {
        studentItem->setText(column, stu.section);
        studentItem->setToolTip(column, stu.tooltip);
        column++;
    }

    // One more column per scoring criterion
    for (int c = 0; c < teamingOptions->realNumScoringFactors; c++) {
        const auto *criterion = teamingOptions->criteria[c];
        if (criterion == nullptr) {
            continue;
        }

        studentItem->setText(column, criterion->studentDisplayText(stu, dataOptions));
        studentItem->setToolTip(column, stu.tooltip);
        studentItem->setTextAlignment(column, criterion->studentTextAlignment());
        column++;
    }
}

void TeamTreeWidget::setColumnHeaderIcon(int column, const QIcon &icon)
{
    headerView->setColumnIcon(column, icon);
}

void TeamTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    draggedItem = dynamic_cast<TeamTreeWidgetItem*>(currentItem());
    if(draggedItem == nullptr) {
        return;
    }
    QTreeWidget::dragEnterEvent(event);

    dragDropEventLabel = new QLabel(this);
    dragDropEventLabel->setWindowFlag(Qt::ToolTip);
    dragDropEventLabel->setTextFormat(Qt::RichText);
}

void TeamTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    QTreeWidget::dragLeaveEvent(event);

    if(dragDropEventLabel != nullptr) {
        dragDropEventLabel->hide();
        delete dragDropEventLabel;
        dragDropEventLabel = nullptr;
    }
}

void TeamTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    if(dragDropEventLabel == nullptr) {
        dragDropEventLabel = new QLabel(this);
        dragDropEventLabel->setWindowFlag(Qt::ToolTip);
        dragDropEventLabel->setTextFormat(Qt::RichText);
    }

    // get the item being dragged and ensure that the item is a TeamTreeWidgetItem
    if(draggedItem == nullptr) {
        dragDropEventLabel->hide();
        return;
    }

    // get the item currently under the cursor and ensure that it is a TeamTreeWidgetItem and not a section
    QTreeWidgetItem* itemUnderCursor = itemAt(event->position().toPoint());
    droppedItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);
    if((droppedItem == nullptr) || (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::section)) {
        dragDropEventLabel->hide();
        return;
    }

    // adjust the location and text of the tooltip
    const int iconSize = 32;
    const QString iconSizeStr = QString::number(iconSize);
    dragDropEventLabel->move(QCursor::pos() + QPoint(iconSize, iconSize));

    const bool draggedItemIsStudent = (draggedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const bool droppedItemIsStudent = (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const auto *const draggedItemParent = draggedItem->parent();
    const auto *const droppedItemParent = droppedItem->parent();

    if((draggedItem == droppedItem) || (!draggedItemIsStudent && droppedItemIsStudent) || (draggedItemParent == droppedItem)) {
        // ignore if dragging item onto self, team->student, or student->own team
        dragDropEventLabel->hide();
    }
    else if(draggedItemIsStudent && droppedItemIsStudent) {
        // dragging student->student
        // show warning if there are separated sections and dragging between different sections
        if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
            draggedItemParent->parent() != nullptr && droppedItemParent->parent() != nullptr &&
            draggedItemParent->parent() != droppedItemParent->parent()) {
                dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                            + tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + droppedItem->text(0) + "</b><br>"
                                            + tr("NOTE: these students are on teams in different sections.") + "</div>");
                dragDropEventLabel->setStyleSheet(DRAGDROPLABELWARNSTYLE);
                dragDropEventLabel->show();
                dragDropEventLabel->adjustSize();
        }
        else {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Swap the placement of") + " <b>" + draggedItem->text(0) + "</b> " + tr("and") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }

    }
    else if(draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging student->team
        // disallow if this is the only student left on the team (leaving team empty)
        // and warn if this team is in a differen section
        if(draggedItemParent->childCount() == 1) {
            dragDropEventLabel->setText(tr("Cannot move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto another team.<br>")
                                         + " <b>" + draggedItem->parent()->text(0) + "</b> " + tr("cannot be left empty."));
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELSTOPSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
                draggedItemParent->parent() != nullptr && draggedItemParent->parent() != droppedItemParent) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/swap.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + droppedItem->text(0) + "</b><br>"
                                        + tr("NOTE: this students is on a team in different a section.") + "</div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELWARNSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();

        }
        else {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                         + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("onto") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
    }
    else if(!draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging team->team
        // disallow if dragging a team to a different section
        if((draggedItemParent == nullptr) || (droppedItemParent == nullptr) ||
          ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent == droppedItemParent))) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Move") + " <b>" + draggedItem->text(0) + "</b> " + tr("above") + " <b>" + droppedItem->text(0) + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELGOODSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else if ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent != droppedItemParent)) {
            dragDropEventLabel->setText(R"(<img style="vertical-align:middle" src=":/icons_new/move.png" width=")" + iconSizeStr + "\" height=\"" + iconSizeStr + "\">"
                                        + tr("Cannot move") + " <b>" + draggedItem->text(0) + "</b> " + tr("to a different section.") + "</b></div>");
            dragDropEventLabel->setStyleSheet(DRAGDROPLABELSTOPSTYLE);
            dragDropEventLabel->show();
            dragDropEventLabel->adjustSize();
        }
        else {
            dragDropEventLabel->hide();
        }
    }
}

void TeamTreeWidget::dropEvent(QDropEvent *event)
{
    if(dragDropEventLabel != nullptr) {
        dragDropEventLabel->hide();
        delete dragDropEventLabel;
        dragDropEventLabel = nullptr;
    }

    // ensure that the dragged item is a TeamTreeWidgetItem
    if(draggedItem == nullptr) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    // get the item currently under the cursor and ensure that it is a TeamTreeWidgetItem and not a section
    QTreeWidgetItem* itemUnderCursor = itemAt(event->position().toPoint());
    droppedItem = dynamic_cast<TeamTreeWidgetItem*>(itemUnderCursor);
    if((droppedItem == nullptr) || (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::section)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    const bool draggedItemIsStudent = (draggedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const bool droppedItemIsStudent = (droppedItem->treeItemType == TeamTreeWidgetItem::TreeItemType::student);
    const auto *const draggedItemParent = draggedItem->parent();
    const auto *const droppedItemParent = droppedItem->parent();

    // ignore if dragging item onto self, team->student, or student->own team, or if something went wrong looking up the team for this student
    if((draggedItem == droppedItem) || (!draggedItemIsStudent && droppedItemIsStudent) || (draggedItem->parent() == droppedItem) ||
        (draggedItemIsStudent && (draggedItemParent == nullptr))) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
    else if(draggedItemIsStudent && droppedItemIsStudent) {
        // swapping two students
        // verify they want this if separated sections and dragging between different sections
        if(draggedItemParent->parent() != nullptr && droppedItemParent != nullptr && droppedItemParent->parent() != nullptr &&
           draggedItemParent->parent() != droppedItemParent->parent()) {
                const bool sureAboutThat = grueprGlobal::warningMessage(this, "gruepr",
                                                                    tr("You are swapping students between different sections.\n"
                                                                       "Are you sure you want to continue?"),
                                                                    tr("Yes"), tr("No"));
            if(!sureAboutThat) {
                    event->setDropAction(Qt::IgnoreAction);
                    event->ignore();
                    return;
                }
                emit swapStudents({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                                   (draggedItem->data(0, Qt::UserRole)).toInt(),
                                   droppedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                                   (droppedItem->data(0, Qt::UserRole)).toInt()});
                return;
        }
        if(droppedItemParent != nullptr) {
            emit swapStudents({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                              (draggedItem->data(0, Qt::UserRole)).toInt(),
                               droppedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                              (droppedItem->data(0, Qt::UserRole)).toInt()});
            return;
        }
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }
    else if(draggedItemIsStudent && !droppedItemIsStudent && (draggedItemParent->childCount() != 1)) {
        // dragging student onto team and not the only student left on the team
        // verify they want this if separated sections and dragging between different sections
        if(draggedItemParent != nullptr && droppedItemParent != nullptr &&
           draggedItemParent->parent() != nullptr && draggedItemParent->parent() != droppedItemParent) {
            const bool sureAboutThat = grueprGlobal::warningMessage(this, "gruepr",
                                                                    tr("You are moving a student to a team in a different section.\n"
                                                                       "Are you sure you want to continue?"),
                                                                    tr("Yes"), tr("No"));
            if(!sureAboutThat) {
                event->setDropAction(Qt::IgnoreAction);
                event->ignore();
                return;
            }
            emit moveStudent({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                              (draggedItem->data(0, Qt::UserRole)).toInt(),
                              droppedItem->data(0, TEAM_NUMBER_ROLE).toInt()});
            return;
        }
        emit moveStudent({draggedItemParent->data(0, TEAM_NUMBER_ROLE).toInt(),
                          (draggedItem->data(0, Qt::UserRole)).toInt(),
                          droppedItem->data(0, TEAM_NUMBER_ROLE).toInt()});
        return;
    }
    else if(!draggedItemIsStudent && !droppedItemIsStudent) {
        // dragging team onto teams in order to reorder
        // if these are teams with separated sections, only allow dragging within the section
        if((draggedItemParent == nullptr) || (droppedItemParent == nullptr) ||
          ((draggedItemParent != nullptr) && (droppedItemParent != nullptr) && (draggedItemParent == droppedItemParent))) {
            emit reorderTeams({(draggedItem->data(0, TEAM_NUMBER_ROLE)).toInt(),
                               (droppedItem->data(0, TEAM_NUMBER_ROLE)).toInt()});
            return;
        }
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }
    event->setDropAction(Qt::IgnoreAction);
    event->ignore();
    return;
}

void TeamTreeWidget::resorting(int column)
{
    auto sortOrder = headerView->sortIndicatorOrder();
    for(int i = 0; i < columnCount(); i++) {
        if(i != column) {
            headerView->setColumnIcon(i, QIcon(":/icons_new/upDownButton_white.png"));
        }
        else if(sortOrder == Qt::AscendingOrder){
            headerView->setColumnIcon(column, QIcon(":/icons_new/downButton_white.png"));
        }
        else { // sortOrder == Qt::DescendingOrder
            headerView->setColumnIcon(column, QIcon(":/icons_new/upButton_white.png"));
        }
    }
    emit updateTeamOrder();
}


///////////////////////////////////////////////////////////////////////

TeamTreeHeaderView::TeamTreeHeaderView(Qt::Orientation orientation, TeamTreeWidget *parent)
    :QHeaderView(orientation, parent), m_elideMode(Qt::ElideMiddle), m_lineCount(1), m_iconSize(ICONSIZE, ICONSIZE)
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setSectionResizeMode(QHeaderView::Interactive);
    setDefaultAlignment(Qt::AlignCenter);
    setStretchLastSection(false);
    setStyleSheet(TEAMTREEWIDGETHEADERSTYLE);
    setMaximumSectionSize(MAX_SECTION_WIDTH);

    connect(this, &TeamTreeHeaderView::sectionClicked, parent, &TeamTreeWidget::resorting);
    connect(this, &TeamTreeHeaderView::sectionResized, this, &TeamTreeHeaderView::updateHeaderHeight);
}

void TeamTreeHeaderView::setElideMode(Qt::TextElideMode mode) {
    m_elideMode = mode;
    viewport()->update();
}

void TeamTreeHeaderView::setColumnElideMode(int column, Qt::TextElideMode mode) {
    m_columnElideModes[column] = mode;
    viewport()->update();
}

void TeamTreeHeaderView::setColumnIcon(int column, const QIcon &icon) {
    m_columnIcons[column] = icon;
    viewport()->update();
}

void TeamTreeHeaderView::setIconSize(const QSize &size) {
    m_iconSize = size;
    updateHeaderHeight();
    viewport()->update();
}

void TeamTreeHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid()) {
        return;
    }

    // Get the full text
    const QString fullText = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    m_fullTexts[logicalIndex] = fullText;

    // Prepare style option
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = rect;
    opt.section = logicalIndex;

    // Check if we have an icon
    const bool hasIcon = m_columnIcons.contains(logicalIndex) && !m_columnIcons[logicalIndex].isNull();

    // Calculate available text width
    int textWidth = rect.width() - 4; // padding
    if (hasIcon) {
        textWidth -= (m_iconSize.width() + 4);
    }

    // Get the elide mode for this section
    Qt::TextElideMode columnElideMode = m_elideMode;
    if (m_columnElideModes.contains(logicalIndex)) {
        columnElideMode = m_columnElideModes[logicalIndex];
    }

    // Check if text would be elided
    const QFontMetrics fm(font());
    const bool wouldElide = fm.horizontalAdvance(fullText) > textWidth;

    // For middle eliding, switch to word wrap if text would be elided
    if (wouldElide) {
        const int prevLineCount = m_lineCountPerColumn.value(logicalIndex, 1);
        const QString wrappedText = wrapText(logicalIndex, fullText, textWidth, fm);
        opt.text = wrappedText;

        if (m_lineCountPerColumn.value(logicalIndex, 1) != prevLineCount) {
            QTimer::singleShot(0, const_cast<TeamTreeHeaderView*>(this),
                               &TeamTreeHeaderView::updateHeaderHeight);
        }
    }
    else {
        // Use normal eliding for other modes
        m_lineCountPerColumn[logicalIndex] = 1;
        opt.text = fm.elidedText(fullText, columnElideMode, textWidth);
    }
    opt.textAlignment = Qt::AlignLeft | Qt::AlignVCenter;

    // Set icon in the option if we have one
    if (hasIcon) {
        opt.icon = m_columnIcons[logicalIndex];
        opt.iconAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }

    // Let the style draw everything including the icon
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
}

QSize TeamTreeHeaderView::sectionSizeFromContents(int logicalIndex) const {
    QSize size = QHeaderView::sectionSizeFromContents(logicalIndex);
    if (m_lineCountPerColumn.contains(logicalIndex) && m_lineCountPerColumn[logicalIndex] > 1) {
        const int height = (fontMetrics().height() * m_lineCountPerColumn[logicalIndex]) + 12;
        size.setHeight(qMax(size.height(), height));
    }
    return size;
}

bool TeamTreeHeaderView::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        const auto *const helpEvent = static_cast<QHelpEvent *>(e);
        const int column = logicalIndexAt(helpEvent->pos());

        if (column >= 0 && m_fullTexts.contains(column)) {
            const QString fullText = m_fullTexts[column];

            // Only show tooltip if text is elided
            const QFontMetrics fm(font());
            int sectionWidth = sectionSize(column) - 4;
            if (m_columnIcons.contains(column)) {
                sectionWidth -= m_iconSize.width() + 4;
            }

            if (fm.horizontalAdvance(fullText) > sectionWidth) {
                QToolTip::showText(helpEvent->globalPos(), fullText);
                return true;
            }
        }
        QToolTip::hideText();
        return true;
    }
    return QHeaderView::event(e);
}

void TeamTreeHeaderView::resizeEvent(QResizeEvent *e)
{
    QHeaderView::resizeEvent(e);
    updateHeaderHeight();
}

void TeamTreeHeaderView::updateHeaderHeight()
{
    if (!model()) {
        return;
    }

    int maxLines = 1;
    for (const auto lines : std::as_const(m_lineCountPerColumn)) {
        maxLines = std::max(maxLines, lines);
    }
    const int maxHeight = qMin((fontMetrics().height() * maxLines) + 12, MAX_HEADER_HEIGHT);

    setMinimumHeight(maxHeight);
    updateGeometry();
}

QString TeamTreeHeaderView::wrapText(int logicalIndex, const QString &text, int availableWidth, const QFontMetrics &fm) const
{
    // Respect explicit newlines first
    const QStringList explicitLines = text.split('\n');
    QStringList finalLines;

    for (const auto &line : explicitLines) {
        const QStringList words = line.split(' ', Qt::SkipEmptyParts);
        if (words.isEmpty()) {
            finalLines << line;
            continue;
        }

        QString currentLine = words.first();
        for (int i = 1; i < words.size(); i++) {
            const QString candidate = currentLine + " " + words[i];
            if (fm.horizontalAdvance(candidate) <= availableWidth) {
                currentLine = candidate;
            } else {
                finalLines << currentLine;
                currentLine = words[i];
            }
        }
        finalLines << currentLine;
    }

    // Truncate to the number of lines that fit within MAX_HEADER_HEIGHT
    const int maxLines = qMax(1, (MAX_HEADER_HEIGHT - 12) / fm.height());
    if (finalLines.size() > maxLines) {
        finalLines = finalLines.mid(0, maxLines);
        finalLines.last() += QStringLiteral("\u2026");  // add an elipsis if truncating the height
    }

    // Elide any lines that are still too wide
    for (auto &line : finalLines) {
        if (fm.horizontalAdvance(line) > availableWidth) {
            line = fm.elidedText(line, Qt::ElideRight, availableWidth);
        }
    }

    m_lineCountPerColumn[logicalIndex] = finalLines.size();
    return finalLines.join("\n");
}


///////////////////////////////////////////////////////////////////////

TeamTreeWidgetItem::TeamTreeWidgetItem(TreeItemType type, int columns)
{
    treeItemType = type;
    if(treeItemType == TreeItemType::team && columns > 0) {
        for(int col = 0; col < columns; col++) {
            setForeground(col, Qt::black);
        }
    }
    if(treeItemType == TreeItemType::section) {
        //sections are fixed--they cannot be dragged or dropped onto
        setFlags(flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }
}

bool TeamTreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    if(treeItemType != TreeItemType::team) {      // only sort the teams
        return false;
    }

    const int sortColumn = treeWidget()->sortColumn();

    // sort using sortorder data in column, and use existing order to break ties
    return((data(sortColumn, TEAMINFO_SORT_ROLE).toDouble() != other.data(sortColumn, TEAMINFO_SORT_ROLE).toDouble()) ?
                (data(sortColumn, TEAMINFO_SORT_ROLE).toDouble() < other.data(sortColumn, TEAMINFO_SORT_ROLE).toDouble()) :
                (data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt() < other.data(columnCount()-1, TEAMINFO_SORT_ROLE).toInt()));
}
