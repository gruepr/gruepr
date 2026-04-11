#ifndef STUDENTTABLEWIDGET_H
#define STUDENTTABLEWIDGET_H

#include <QTableWidget>
#include "gruepr_globals.h"

class StudentTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    StudentTableWidget(QWidget *parent = nullptr);
    void resetTable();
    void clearSortIndicator();

public slots:
    void sortByColumn(int column);

protected:
    void leaveEvent(QEvent *event) override;

private slots:
    void itemEntered(const QModelIndex &index);         // select entire row when hovering over any part of it

private:
    int prevSortColumn = 0;
    Qt::SortOrder prevSortOrder = Qt::AscendingOrder;

    inline static const char STUDENTTABLEWIDGETHORIZONTALHEADERSTYLE[] =
        "QHeaderView{border-top: none; border-left: none; border-right: 1px solid lightGray; border-bottom: none;"
                    "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; "
                    "color: white; text-align:left;}"
        "QHeaderView::section{border-top: none; border-left: none; border-right: 1px solid lightGray; "
                             "border-bottom: none;"
                             "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; "
                             "color: white; text-align:left;}"
        "QHeaderView::down-arrow{image: url(:/icons_new/downButton_white.png); width: 15px; "
                                "subcontrol-origin: padding; subcontrol-position: bottom left;}"
        "QHeaderView::up-arrow{image: url(:/icons_new/upButton_white.png); width: 15px; "
                              "subcontrol-origin: padding; subcontrol-position: top left;}";

    inline static const char STUDENTTABLEWIDGETVERTICALALHEADERSTYLE[] =
        "QHeaderView{border-top: none; border-left: none; border-right: none; border-bottom: none; padding: 1px;"
                    "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; "
                    "color: white; text-align:center;}"
        "QHeaderView::section{border-top: none; border-left: none; border-right: none; "
                             "border-bottom: none; padding: 1px;"
                             "background-color:" DEEPWATERHEX "; font-family: 'DM Sans'; font-size: 12pt; "
                             "color: white; text-align:center;}";

    inline static const char STUDENTTABLEWIDGETSTYLE[] =
        "QTableView{gridline-color: lightGray; font-family: 'DM Sans'; font-size: 12pt;}"
        "QTableCornerButton::section{border-top: none; border-left: none; border-right: 1px solid gray; "
                                    "border-bottom: none; background-color: " DEEPWATERHEX ";}"
        "QTableWidget::item{border-right: 1px solid lightGray; color: black;}"
        "QTableWidget::item:selected{background-color: " BUBBLYHEX ";}"
        "QTableWidget::item:hover{background-color: " BUBBLYHEX ";}";
};

#endif // STUDENTTABLEWIDGET_H
