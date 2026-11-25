#include "peerReviewStartDialog.h"
#include "gruepr_globals.h"
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to start a new peer review, or to choose a saved one to start a new round of, load the results of, or delete
/////////////////////////////////////////////////////////////////////////////////////////////////////////

PeerReviewStartDialog::PeerReviewStartDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("gruepr - Get peer review"));
    setStyleSheet(QString(WHITEDIALOGSTYLE) + SCROLLBARSTYLE);
    setSizeGripEnabled(true);
    setMinimumSize(LG_DLG_SIZE, SM_DLG_SIZE);

    auto *theGrid = new QGridLayout(this);
    int row = 0;
    auto *headerLabel = new QLabel(tr("Peer review"), this);
    headerLabel->setStyleSheet(LABEL14PTSTYLE);
    theGrid->addWidget(headerLabel, row++, 0, 1, -1);
    auto *explanationLabel = new QLabel(tr("Ask the members of each team for feedback about each other, then review the results and share "
                                           "feedback with each student. Start a new peer review, or choose a saved one below."), this);
    explanationLabel->setStyleSheet(LABEL10PTSTYLE);
    explanationLabel->setWordWrap(true);
    theGrid->addWidget(explanationLabel, row++, 0, 1, -1);
    theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT / 2);

    auto *newButton = new QPushButton(tr("New Peer Review"), this);
    newButton->setStyleSheet(SMALLBUTTONSTYLE);
    newButton->setIcon(QIcon(":/icons_new/addButton.png"));
    connect(newButton, &QPushButton::clicked, this, [this](){choose(Action::newPeerReview);});
    theGrid->addWidget(newButton, row++, 0, 1, 1, Qt::AlignLeft);
    theGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT / 2);

    auto *savedLabel = new QLabel(tr("Saved peer reviews:"), this);
    savedLabel->setStyleSheet(LABEL10PTSTYLE);
    theGrid->addWidget(savedLabel, row++, 0, 1, -1);
    reviewsTable = new QTableWidget(this);
    reviewsTable->setColumnCount(6);
    reviewsTable->setHorizontalHeaderLabels({tr("Name"), tr("Created"), tr("Students"), tr("Form"), "", ""});
    reviewsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reviewsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    reviewsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    reviewsTable->verticalHeader()->hide();
    reviewsTable->horizontalHeader()->setHighlightSections(false);
    reviewsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    reviewsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    reviewsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    reviewsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    reviewsTable->horizontalHeader()->setSectionResizeMode(NEWROUNDCOLUMN, QHeaderView::Fixed);
    reviewsTable->horizontalHeader()->setSectionResizeMode(DELETECOLUMN, QHeaderView::Fixed);
    reviewsTable->setColumnWidth(NEWROUNDCOLUMN, 32);
    reviewsTable->setColumnWidth(DELETECOLUMN, 32);
    reviewsTable->setStyleSheet(QString("QTableView{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: " DEEPWATERHEX ";}"
                                        "QTableView::item:selected{background-color: " OPENWATERHEX "; color: white;}"
                                        "QHeaderView::section{background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; "
                                                             "font-size: 10pt; padding: 4px; border: none;}") + SCROLLBARSTYLE);
    connect(reviewsTable, &QTableWidget::itemSelectionChanged, this, &PeerReviewStartDialog::updateButtons);
    connect(reviewsTable, &QTableWidget::cellDoubleClicked, this, [this](int row){
        if((row >= 0) && (row < storedReviews.size()) && storedReviews.at(row).hasForm) {
            choose(Action::loadResults, row);
        }
    });
    theGrid->addWidget(reviewsTable, row++, 0, 1, -1);
    theGrid->setRowStretch(row - 1, 1);

    auto *closeButton = new QPushButton(tr("Close"), this);
    closeButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    loadResultsButton = new QPushButton(tr("Load Results"), this);
    loadResultsButton->setStyleSheet(SMALLBUTTONSTYLE);
    loadResultsButton->setToolTip(tr("See who has responded, the results, and the feedback for each student"));
    connect(loadResultsButton, &QPushButton::clicked, this, [this](){choose(Action::loadResults, reviewsTable->currentRow());});
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(loadResultsButton);
    theGrid->addLayout(buttonLayout, row++, 0, 1, -1);

    refreshList();
    adjustSize();
}

void PeerReviewStartDialog::refreshList()
{
    storedReviews = PeerReview::storedPeerReviews();
    reviewsTable->clearContents();
    reviewsTable->setRowCount(int(storedReviews.size()));
    int row = 0;
    for(const auto &storedReview : std::as_const(storedReviews)) {
        reviewsTable->setItem(row, 0, new QTableWidgetItem(storedReview.name));
        reviewsTable->setItem(row, 1, new QTableWidgetItem(storedReview.createdTime.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat))));
        reviewsTable->setItem(row, 2, new QTableWidgetItem(QString::number(storedReview.numStudents)));
        reviewsTable->setItem(row, 3, new QTableWidgetItem(!storedReview.hasForm? "" : (storedReview.onCanvas? tr("Canvas quizzes") : tr("Google Form"))));

        auto *newRoundButton = rowButton(":/icons_new/redo.png", tr("Start a new round: a new peer review with the same teams and questions"));
        connect(newRoundButton, &QPushButton::clicked, this, [this, row](){choose(Action::newRound, row);});
        reviewsTable->setCellWidget(row, NEWROUNDCOLUMN, newRoundButton);
        auto *deleteButton = rowButton(":/icons_new/trashButton.png", storedReview.onCanvas?
                                           tr("Delete this peer review from gruepr (its quizzes stay in the Canvas course)") :
                                           tr("Delete this peer review from gruepr (its form stays in your Google Drive)"));
        connect(deleteButton, &QPushButton::clicked, this, [this, row](){deleteReview(row);});
        reviewsTable->setCellWidget(row, DELETECOLUMN, deleteButton);
        row++;
    }
    if(!storedReviews.isEmpty()) {
        reviewsTable->selectRow(0);
    }
    updateButtons();
}

QPushButton *PeerReviewStartDialog::rowButton(const QString &iconFileName, const QString &toolTip)
{
    // a flat icon button, like the edit and remove buttons in each row of the main window's student table
    auto *button = new QPushButton(QIcon(iconFileName), "", reviewsTable);
    button->setFlat(true);
    button->setStyleSheet("QPushButton{background-color: transparent; border: none;}");
    button->setIconSize(QSize(20, 20));
    button->setToolTip(toolTip);
    return button;
}

void PeerReviewStartDialog::updateButtons()
{
    const int row = reviewsTable->currentRow();
    const bool chosen = !reviewsTable->selectedItems().isEmpty() && (row >= 0) && (row < storedReviews.size());
    loadResultsButton->setEnabled(chosen && storedReviews.at(row).hasForm);
}

void PeerReviewStartDialog::choose(const Action chosenAction, const int row)
{
    action = chosenAction;
    if(action != Action::newPeerReview) {
        if((row < 0) || (row >= storedReviews.size())) {
            return;
        }
        chosenFileName = storedReviews.at(row).fileName;
    }
    accept();
}

void PeerReviewStartDialog::deleteReview(const int row)
{
    if((row < 0) || (row >= storedReviews.size())) {
        return;
    }
    const QString whereItStays = storedReviews.at(row).onCanvas? tr("Its quizzes and responses stay in the Canvas course") :
                                                                 tr("Its form and responses stay in your Google Drive");
    if(!grueprGlobal::warningMessage(this, "gruepr", tr("Are you sure you want to delete the peer review \"") + storedReviews.at(row).name + "\"?\n" +
                                                      whereItStays + ", " + tr("but gruepr will no longer be able to load its results.") + "\n" +
                                                      tr("This cannot be undone."),
                                     tr("Delete"), tr("Cancel"))) {
        return;
    }
    if(!PeerReview::deleteStored(storedReviews.at(row).fileName)) {
        grueprGlobal::errorMessage(this, tr("Error"), tr("The peer review file could not be deleted. It may be in use; please try again."));
        return;
    }
    refreshList();
}
