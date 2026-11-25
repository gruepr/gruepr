#include "peerReviewResultsDialog.h"
#include "gruepr_globals.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include "dialogs/feedbackReleaseDialog.h"
#include "dialogs/reminderDialog.h"
#include "widgets/sortableTableWidgetItem.h"
#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTimer>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog showing the results of a peer review, from which feedback is released to the students
/////////////////////////////////////////////////////////////////////////////////////////////////////////

PeerReviewResultsDialog::PeerReviewResultsDialog(PeerReview &peerReview, QWidget *parent)
    : QDialog(parent),
    peerReview(peerReview)
{
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle(tr("gruepr - Peer review results") + " - " + peerReview.title);
    setStyleSheet(QString(WHITEDIALOGSTYLE) + CHECKBOXSTYLE + SCROLLBARSTYLE);
    setSizeGripEnabled(true);
    const QSettings savedSettings;
    if(!restoreGeometry(savedSettings.value("peerReviewResultsWindowGeometry").toByteArray())) {
        resize(SCREENWIDTH * 2 / 3, SCREENHEIGHT * 2 / 3);
    }

    auto *theGrid = new QGridLayout(this);
    int row = 0;
    auto *titleLabel = new QLabel(peerReview.title, this);
    titleLabel->setStyleSheet(LABEL14PTSTYLE);
    theGrid->addWidget(titleLabel, row++, 0, 1, -1);
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet(LABEL10PTSTYLE);
    statusLabel->setTextFormat(Qt::RichText);
    statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    statusLabel->setOpenExternalLinks(true);
    statusLabel->setWordWrap(true);
    theGrid->addWidget(statusLabel, row++, 0, 1, -1);

    // the actions (the latest responses are downloaded each time this window opens)
    auto *reminderButton = new QPushButton(tr("Remind Non-Responders..."), this);
    reminderButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    reminderButton->setToolTip(tr("Remind the students who haven't responded: copy their email addresses, save a mail merge file "
                                  "with each one's link and message, or send each one a private message in Canvas"));
    connect(reminderButton, &QPushButton::clicked, this, &PeerReviewResultsDialog::remindStudents);
    acceptingButton = new QPushButton(this);
    acceptingButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(acceptingButton, &QPushButton::clicked, this, &PeerReviewResultsDialog::toggleAcceptingResponses);
    auto *factorButton = new QPushButton(tr("Adjustment Factor..."), this);
    factorButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    factorButton->setToolTip(tr("Change how the adjustment factor is calculated and when students are flagged"));
    connect(factorButton, &QPushButton::clicked, this, &PeerReviewResultsDialog::editAdjustmentFactor);
    auto *exportButton = new QPushButton(tr("Save Spreadsheet..."), this);
    exportButton->setStyleSheet(SMALLBUTTONSTYLE);
    exportButton->setToolTip(tr("A spreadsheet of the results, with one row per student"));
    connect(exportButton, &QPushButton::clicked, this, &PeerReviewResultsDialog::exportSummarySpreadsheet);
    auto *feedbackButton = new QPushButton(tr("Provide Feedback to Students..."), this);
    feedbackButton->setStyleSheet(SMALLBUTTONSTYLE);
    feedbackButton->setToolTip(tr("Preview each student's feedback report, edit it, and save or send it"));
    connect(feedbackButton, &QPushButton::clicked, this, &PeerReviewResultsDialog::provideFeedback);
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(reminderButton);
    buttonLayout->addWidget(acceptingButton);
    buttonLayout->addWidget(factorButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(feedbackButton);
    theGrid->addLayout(buttonLayout, row++, 0, 1, -1);

    tabs = new QTabWidget(this);
    tabs->setStyleSheet(TABWIDGETSTYLE);

    // tab 1: who has responded
    auto *responsesTab = new QWidget;
    auto *responsesLayout = new QVBoxLayout(responsesTab);
    responsesSummaryLabel = new QLabel(responsesTab);
    responsesSummaryLabel->setStyleSheet(LABEL12PTSTYLE);
    responsesLayout->addWidget(responsesSummaryLabel);
    responsesTable = new QTableWidget(responsesTab);
    responsesTable->setColumnCount(5);
    responsesTable->setHorizontalHeaderLabels({tr("Submitted"), tr("Student"), tr("Team"), tr("Email"), tr("Response notes")});
    const QString bullet = "\n  " + QString(BULLET) + " ";
    responsesTable->horizontalHeaderItem(4)->setToolTip(tr("Blank unless:") +
                                                        bullet + tr("the student submitted more than once (only the latest response is used),") +
                                                        bullet + tr("sign-in was required and they signed in with an email that isn't theirs in the teams, or") +
                                                        bullet + tr("you entered the response while signed in with your own Google account"));
    responsesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    responsesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    responsesTable->verticalHeader()->hide();
    responsesTable->horizontalHeader()->setStretchLastSection(true);
    responsesTable->setStyleSheet(QString(TABLESTYLE) + SCROLLBARSTYLE);
    responsesTable->setSortingEnabled(true);
    setUpSortableHeader(responsesTable);
    responsesLayout->addWidget(responsesTable, 1);
    unmatchedLabel = new QLabel(responsesTab);
    unmatchedLabel->setStyleSheet(LABEL10PTSTYLE);
    unmatchedLabel->setWordWrap(true);
    responsesLayout->addWidget(unmatchedLabel);
    tabs->addTab(responsesTab, tr("Responses Received"));

    // tab 2: each student's results
    auto *studentsSplitter = new QSplitter(Qt::Vertical);
    studentsTable = new QTableWidget(studentsSplitter);
    studentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    studentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    studentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    studentsTable->verticalHeader()->hide();
    studentsTable->horizontalHeader()->setStretchLastSection(true);
    studentsTable->setStyleSheet(QString(TABLESTYLE) + SCROLLBARSTYLE);
    studentsTable->setSortingEnabled(true);
    setUpSortableHeader(studentsTable);
    connect(studentsTable, &QTableWidget::itemSelectionChanged, this, &PeerReviewResultsDialog::refreshStudentDetails);
    studentDetails = new QTextBrowser(studentsSplitter);
    studentDetails->setStyleSheet(QString(BROWSERSTYLE) + SCROLLBARSTYLE);
    studentsSplitter->addWidget(studentsTable);
    studentsSplitter->addWidget(studentDetails);
    studentsSplitter->setStretchFactor(0, 3);
    studentsSplitter->setStretchFactor(1, 2);
    tabs->addTab(studentsSplitter, tr("By Student"));

    // tab 3: the answers to each question
    auto *questionTab = new QWidget;
    auto *questionLayout = new QVBoxLayout(questionTab);
    questionComboBox = new StyledComboBox(questionTab);
    for(int questionNum = 0; questionNum < peerReview.questions.size(); questionNum++) {
        questionComboBox->addItem(QString::number(questionNum + 1) + ". " + peerReview.questions.at(questionNum).text, questionNum);
    }
    connect(questionComboBox, &QComboBox::currentIndexChanged, this, &PeerReviewResultsDialog::refreshQuestionTab);
    questionLayout->addWidget(questionComboBox);
    questionView = new QTextBrowser(questionTab);
    questionView->setStyleSheet(QString(BROWSERSTYLE) + SCROLLBARSTYLE);
    questionLayout->addWidget(questionView, 1);
    tabs->addTab(questionTab, tr("By Question"));

    // tab 4: contribution, if it was asked
    contributionView = new QTextBrowser;
    contributionView->setStyleSheet(QString(BROWSERSTYLE) + SCROLLBARSTYLE);
    contributionTabIndex = tabs->addTab(contributionView, tr("Contribution"));
    const bool askedContribution = std::any_of(peerReview.questions.cbegin(), peerReview.questions.cend(), [](const PeerReviewQuestion &question)
                                               {return question.kind == PeerReviewQuestion::Kind::contribution;});
    tabs->setTabVisible(contributionTabIndex, askedContribution);

    // tab 5: the written comments that go into students' feedback reports, if any questions share them
    auto *commentsTab = new QWidget;
    auto *commentsLayout = new QVBoxLayout(commentsTab);
    auto *commentsLabel = new QLabel(tr("These written comments go into students' feedback reports. Double-click a comment to edit or delete it."),
                                     commentsTab);
    commentsLabel->setStyleSheet(LABEL10PTSTYLE);
    commentsLabel->setWordWrap(true);
    commentsLayout->addWidget(commentsLabel);
    commentsTable = new QTableWidget(commentsTab);
    commentsTable->setColumnCount(6);
    commentsTable->setHorizontalHeaderLabels({tr("Question"), tr("Team"), tr("From"), tr("About"), tr("Comment"), tr("Status")});
    commentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    commentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    commentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    commentsTable->verticalHeader()->hide();
    commentsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);     // rows grow to fit the wrapped comments
    commentsTable->setWordWrap(true);
    commentsTable->setStyleSheet(QString(TABLESTYLE) + SCROLLBARSTYLE);
    commentsTable->setSortingEnabled(true);
    setUpSortableHeader(commentsTable);
    connect(commentsTable, &QTableWidget::cellDoubleClicked, this, [this](int row){editComment(row);});
    commentsLayout->addWidget(commentsTable, 1);
    commentsTabIndex = tabs->addTab(commentsTab, tr("Shared Comments"));
    const bool sharesComments = std::any_of(peerReview.questions.cbegin(), peerReview.questions.cend(), [](const PeerReviewQuestion &question)
                                            {return (question.kind == PeerReviewQuestion::Kind::written) &&
                                                    (question.shareWith != PeerReviewQuestion::ShareWith::instructor);});
    tabs->setTabVisible(commentsTabIndex, sharesComments);

    theGrid->addWidget(tabs, row++, 0, 1, -1);
    theGrid->setRowStretch(row - 1, 1);

    // no Close button: the window is closed with its title bar button, like gruepr's main window
    connect(this, &QDialog::finished, this, [this](){
        QSettings savedSettings;
        savedSettings.setValue("peerReviewResultsWindowGeometry", saveGeometry());
    });

    // show the responses downloaded last time, then download the latest ones once this window is showing
    refreshResults();
    QTimer::singleShot(0, this, &PeerReviewResultsDialog::downloadResponses);
}

void PeerReviewResultsDialog::downloadResponses()
{
    if(!grueprGlobal::internetIsGood()) {
        return;
    }

    if(peerReview.platform == PeerReview::Platform::canvasQuizzes) {
        //create canvasHandler and authenticate, then download the submissions to the team quizzes
        auto *canvas = new CanvasHandler(this);
        if(!canvas->authenticate()) {
            canvas->deleteLater();
            return;
        }
        auto *busyBox = canvas->actionDialog(this);
        bool ok = false;
        const QJsonArray submissions = canvas->downloadPeerReviewSubmissions(peerReview, &ok, [canvas](int quizzesDone, int numQuizzes){
            canvas->showActionProgress(tr("Loading responses: team") + " " + QString::number(quizzesDone + 1) + " " + tr("of") + " " +
                                       QString::number(numQuizzes) + "...", quizzesDone, numQuizzes);
        });
        const QString lastErrorMessage = canvas->lastErrorMessage;
        canvas->actionComplete(busyBox);
        canvas->deleteLater();

        if(!ok) {
            QString errormsg = tr("The responses could not be downloaded from the team quizzes in") + " " + peerReview.canvasCourseName.toHtmlEscaped() + ".";
            if(!lastErrorMessage.isEmpty()) {
                errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage.toHtmlEscaped() + "</code>";
            }
            grueprGlobal::errorMessage(this, tr("Error"), errormsg);
            return;
        }
        peerReview.lastResponses = submissions;
        peerReview.lastResponsesTime = QDateTime::currentDateTime();
        peerReview.save();
        refreshResults();
        return;
    }

    //create googleHandler and authenticate
    auto *google = new GoogleHandler(this);
    if(!google->authenticate()) {
        google->deleteLater();
        return;
    }

    auto *busyBox = google->actionDialog(this);
    bool ok = false;
    const QJsonArray responses = google->downloadPeerReviewResponses(peerReview.formID, &ok);
    const QString lastErrorMessage = google->lastErrorMessage;
    const QString signedInAccount = google->getAccountName();
    google->actionComplete(busyBox);
    google->deleteLater();

    if(!ok) {
        QString errormsg = tr("The responses could not be downloaded.");
        if(!peerReview.accountName.isEmpty() && (signedInAccount.compare(peerReview.accountName, Qt::CaseInsensitive) != 0)) {
            errormsg += "<br><br>" + tr("This peer review's form belongs to the Google account") + " <b>" + peerReview.accountName.toHtmlEscaped() +
                        "</b>, " + tr("but you are signed in as") + " <b>" + signedInAccount.toHtmlEscaped() + "</b>.";
        }
        if(!lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage.toHtmlEscaped() + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        return;
    }

    peerReview.lastResponses = responses;
    peerReview.lastResponsesTime = QDateTime::currentDateTime();
    peerReview.save();
    refreshResults();
}

void PeerReviewResultsDialog::toggleAcceptingResponses()
{
    if(!grueprGlobal::internetIsGood()) {
        return;
    }

    if(peerReview.platform == PeerReview::Platform::canvasQuizzes) {
        // locking the team quizzes stops responses; unlocking them lets students respond again
        auto *canvas = new CanvasHandler(this);
        if(!canvas->authenticate()) {
            canvas->deleteLater();
            return;
        }
        auto *busyBox = canvas->actionDialog(this);
        const bool locking = peerReview.acceptingResponses;
        const bool success = canvas->setPeerReviewQuizzesLocked(peerReview, locking, [canvas, locking](int quizzesDone, int numQuizzes, bool undoing){
            const QString action = undoing? tr("Changing back team quiz") : (locking? tr("Closing team quiz") : tr("Reopening team quiz"));
            canvas->showActionProgress(action + " " + QString::number(quizzesDone + 1) + " " + tr("of") + " " + QString::number(numQuizzes) + "...",
                                       quizzesDone, numQuizzes);
        });
        const QString lastErrorMessage = canvas->lastErrorMessage;
        canvas->actionComplete(busyBox);
        canvas->deleteLater();

        if(!success) {
            QString errormsg = tr("The team quizzes could not be changed, so they were all left as they were. Please try again.");
            if(!lastErrorMessage.isEmpty()) {
                errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage.toHtmlEscaped() + "</code>";
            }
            grueprGlobal::errorMessage(this, tr("Error"), errormsg);
            return;
        }
        peerReview.acceptingResponses = !peerReview.acceptingResponses;
        peerReview.save();
        refreshStatus();
        return;
    }

    auto *google = new GoogleHandler(this);
    if(!google->authenticate()) {
        google->deleteLater();
        return;
    }

    auto *busyBox = google->actionDialog(this);
    const bool success = google->setPeerReviewAcceptingResponses(peerReview.formID, !peerReview.acceptingResponses);
    const QString lastErrorMessage = google->lastErrorMessage;
    google->actionComplete(busyBox);
    google->deleteLater();

    if(!success) {
        QString errormsg = tr("The form could not be changed.");
        if(!lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage.toHtmlEscaped() + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        return;
    }

    peerReview.acceptingResponses = !peerReview.acceptingResponses;
    peerReview.formPublished = true;
    peerReview.save();
    refreshStatus();
}

void PeerReviewResultsDialog::editAdjustmentFactor()
{
    auto &thresholds = peerReview.thresholds;
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle(tr("Adjustment factor"));
    settingsDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    settingsDialog.setStyleSheet(QString(WHITEDIALOGSTYLE) + LABEL10PTSTYLE + SPINBOXSTYLE + DOUBLESPINBOXSTYLE + CHECKBOXSTYLE + RADIOBUTTONSTYLE);
    auto *settingsGrid = new QGridLayout(&settingsDialog);
    int row = 0;
    auto *factorLabel = new QLabel(tr("The adjustment factor compares each student's ratings from their teammates to the rest of their team: "
                                      "1.00 is the team's average."), &settingsDialog);
    factorLabel->setWordWrap(true);
    settingsGrid->addWidget(factorLabel, row++, 0, 1, -1);

    // with a contribution question, the factor can be based on it alone or on it averaged with the rating questions
    QRadioButton *contributionOnlyRadioButton = nullptr;
    if(results->contributionQuestionNum != -1) {
        auto *basisLabel = new QLabel(tr("Base the adjustment factor on:"), &settingsDialog);
        settingsGrid->addWidget(basisLabel, row++, 0, 1, -1);
        contributionOnlyRadioButton = new QRadioButton(tr("the contribution question only (1.00 is an equal share)"), &settingsDialog);
        auto *allQuestionsRadioButton = new QRadioButton(tr("all the rating questions and the contribution question, averaged"), &settingsDialog);
        allQuestionsRadioButton->setToolTip(tr("The contribution question counts as one question alongside the rating questions"));
        auto *basisGroup = new QButtonGroup(&settingsDialog);
        basisGroup->addButton(contributionOnlyRadioButton);
        basisGroup->addButton(allQuestionsRadioButton);
        contributionOnlyRadioButton->setChecked(!thresholds.factorFromAllQuestions);
        allQuestionsRadioButton->setChecked(thresholds.factorFromAllQuestions);
        settingsGrid->addWidget(contributionOnlyRadioButton, row++, 0, 1, -1);
        settingsGrid->addWidget(allQuestionsRadioButton, row++, 0, 1, -1);
    }
    auto *capCheckBox = new QCheckBox(tr("Limit the adjustment factor to at most"), &settingsDialog);
    capCheckBox->setChecked(thresholds.capFactor);
    auto *capSpinBox = new QDoubleSpinBox(&settingsDialog);
    capSpinBox->setRange(1.0, 2.0);
    capSpinBox->setSingleStep(0.05);
    capSpinBox->setValue(thresholds.factorCap);
    capSpinBox->setEnabled(thresholds.capFactor);
    connect(capCheckBox, &QCheckBox::toggled, capSpinBox, &QDoubleSpinBox::setEnabled);
    settingsGrid->addWidget(capCheckBox, row, 0, 1, 1);
    settingsGrid->addWidget(capSpinBox, row++, 1, 1, 1);
    settingsGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
    auto *flagsLabel = new QLabel(tr("Flag a student when:"), &settingsDialog);
    settingsGrid->addWidget(flagsLabel, row++, 0, 1, -1);
    auto *lowLabel = new QLabel(tr("their adjustment factor is at or below"), &settingsDialog);
    auto *lowSpinBox = new QDoubleSpinBox(&settingsDialog);
    lowSpinBox->setRange(0.0, 1.0);
    lowSpinBox->setSingleStep(0.05);
    lowSpinBox->setValue(thresholds.lowContributor);
    settingsGrid->addWidget(lowLabel, row, 0, 1, 1);
    settingsGrid->addWidget(lowSpinBox, row++, 1, 1, 1);
    auto *highLabel = new QLabel(tr("their adjustment factor is at or above"), &settingsDialog);
    auto *highSpinBox = new QDoubleSpinBox(&settingsDialog);
    highSpinBox->setRange(1.0, 3.0);
    highSpinBox->setSingleStep(0.05);
    highSpinBox->setValue(thresholds.highContributor);
    settingsGrid->addWidget(highLabel, row, 0, 1, 1);
    settingsGrid->addWidget(highSpinBox, row++, 1, 1, 1);
    auto *gapLabel = new QLabel(tr("they rate themselves higher or lower than their teammates do, by this percent of the scale"), &settingsDialog);
    gapLabel->setWordWrap(true);
    auto *gapSpinBox = new QSpinBox(&settingsDialog);
    gapSpinBox->setRange(5, 100);
    gapSpinBox->setSuffix("%");
    gapSpinBox->setValue(thresholds.selfGapPercent);
    settingsGrid->addWidget(gapLabel, row, 0, 1, 1);
    settingsGrid->addWidget(gapSpinBox, row++, 1, 1, 1);
    auto *spreadLabel = new QLabel(tr("their teammates' ratings of them spread over this percent of the scale"), &settingsDialog);
    spreadLabel->setWordWrap(true);
    auto *spreadSpinBox = new QSpinBox(&settingsDialog);
    spreadSpinBox->setRange(10, 100);
    spreadSpinBox->setSuffix("%");
    spreadSpinBox->setValue(thresholds.disagreementPercent);
    settingsGrid->addWidget(spreadLabel, row, 0, 1, 1);
    settingsGrid->addWidget(spreadSpinBox, row++, 1, 1, 1);
    settingsGrid->setRowMinimumHeight(row++, DIALOG_SPACER_ROWHEIGHT);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &settingsDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &settingsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &settingsDialog, &QDialog::reject);
    settingsGrid->addWidget(buttonBox, row++, 0, 1, -1);
    settingsGrid->setColumnStretch(0, 1);
    settingsDialog.setMinimumWidth(LG_DLG_SIZE);

    if(settingsDialog.exec() == QDialog::Rejected) {
        return;
    }
    if(contributionOnlyRadioButton != nullptr) {
        thresholds.factorFromAllQuestions = !contributionOnlyRadioButton->isChecked();
    }
    thresholds.capFactor = capCheckBox->isChecked();
    thresholds.factorCap = capSpinBox->value();
    thresholds.lowContributor = lowSpinBox->value();
    thresholds.highContributor = highSpinBox->value();
    thresholds.selfGapPercent = gapSpinBox->value();
    thresholds.disagreementPercent = spreadSpinBox->value();
    peerReview.save();
    refreshResults();
}

void PeerReviewResultsDialog::refreshResults()
{
    results = std::make_unique<PeerReviewResults>(peerReview, peerReview.parseResponses(peerReview.lastResponses));
    refreshStatus();
    refreshResponsesTab();
    refreshStudentsTab();
    refreshQuestionTab();
    refreshContributionTab();
    refreshCommentsTab();
}

void PeerReviewResultsDialog::refreshStatus()
{
    const bool onCanvas = (peerReview.platform == PeerReview::Platform::canvasQuizzes);
    QString status = tr("Teams") + ": " + peerReview.teamsSource.toHtmlEscaped() + "<br>";
    if(onCanvas) {
        const int numQuizzes = int(std::count_if(peerReview.canvasQuizzes.cbegin(), peerReview.canvasQuizzes.cend(),
                                                 [](const PeerReview::CanvasTeamQuiz &canvasQuiz){return canvasQuiz.quizID != -1;}));
        status += tr("Canvas course") + ": <a href=\"" + peerReview.canvasCourseURL.toHtmlEscaped() + "/quizzes\">" +
                  peerReview.canvasCourseName.toHtmlEscaped() + "</a>, " + QString::number(numQuizzes) + " " + tr("team quizzes");
    }
    else {
        status += tr("Form") + ": <a href=\"" + peerReview.responderURL.toHtmlEscaped() + "\">" + peerReview.responderURL.toHtmlEscaped() + "</a>";
    }
    if(peerReview.lastResponsesTime.isValid()) {
        status += "<br>" + tr("Responses downloaded") + ": " + peerReview.lastResponsesTime.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    }
    if(onCanvas) {
        status += "<br>" + (peerReview.acceptingResponses? tr("The team quizzes are still accepting responses.") :
                                                           tr("The team quizzes are NOT accepting responses (they are locked)."));
    }
    else {
        status += "<br>" + (peerReview.acceptingResponses? tr("The form is still accepting responses.") : tr("The form is NOT accepting responses."));
    }
    statusLabel->setText(status);
    acceptingButton->setText(peerReview.acceptingResponses? tr("Stop Accepting Responses") : tr("Resume Accepting Responses"));
}

QString PeerReviewResultsDialog::studentName(const long long studentID) const
{
    const auto *thisStudent = peerReview.student(studentID);
    return (thisStudent == nullptr)? tr("Unknown") : thisStudent->fullName();
}

void PeerReviewResultsDialog::setUpSortableHeader(QTableWidget *table)
{
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setSortIndicatorShown(true);
    connect(table->horizontalHeader(), &QHeaderView::sortIndicatorChanged, table, [table](){updateSortIcons(table);});
}

void PeerReviewResultsDialog::updateSortIcons(QTableWidget *table)
{
    // as in the main window: the sorted column shows its arrow (drawn by the header style), and the other columns show an up-down icon
    const int sortColumn = table->horizontalHeader()->sortIndicatorSection();
    for(int col = 0; col < table->columnCount(); col++) {
        auto *headerItem = table->horizontalHeaderItem(col);
        if(headerItem != nullptr) {
            headerItem->setIcon(QIcon((col == sortColumn)? ":/icons_new/blank_arrow.png" : ":/icons_new/upDownButton_white.png"));
        }
    }
}

QList<const PeerReviewStudent *> PeerReviewResultsDialog::missingStudents() const
{
    QList<const PeerReviewStudent *> missing;
    for(const auto &studentResult : results->students) {
        if(studentResult.numResponses == 0) {
            missing << peerReview.student(studentResult.ID);
        }
    }
    missing.removeAll(nullptr);
    return missing;
}

void PeerReviewResultsDialog::refreshResponsesTab()
{
    const int numStudents = int(results->students.size());
    const int numResponded = results->numResponded();
    responsesSummaryLabel->setText(QString::number(numResponded) + " " + tr("of") + " " + QString::number(numStudents) + " " + tr("students have responded"));

    responsesTable->setSortingEnabled(false);
    responsesTable->clearContents();
    responsesTable->setRowCount(numStudents);
    int row = 0;
    for(const auto &studentResult : results->students) {
        const auto *thisStudent = peerReview.student(studentResult.ID);
        QDateTime submitted;
        for(const auto &response : results->responses) {
            if((response.giverID == studentResult.ID) && !response.superseded) {
                submitted = response.submittedTime.toLocalTime();
            }
        }
        QStringList notes;
        if(studentResult.numResponses > 1) {
            notes << tr("Submitted %1 times (the latest is used)").arg(studentResult.numResponses);
        }
        if(!studentResult.mismatchedEmail.isEmpty()) {
            notes << tr("Signed in as %1").arg(studentResult.mismatchedEmail);
        }
        if(studentResult.enteredByInstructor) {
            notes << tr("Entered by instructor");
        }
        QDateTime lastReminded;
        for(const auto &reminder : std::as_const(peerReview.reminders)) {
            if((reminder.studentID == studentResult.ID) && (!lastReminded.isValid() || (reminder.time > lastReminded))) {
                lastReminded = reminder.time;
            }
        }
        if(lastReminded.isValid()) {
            notes << tr("Reminded %1").arg(lastReminded.toString(QLocale::system().dateFormat(QLocale::ShortFormat)));
        }

        // submitted: sorted with the students who haven't responded first, then from the earliest response to the latest
        auto *submittedItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric,
                                                          submitted.isValid()? submitted.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat)) : "");
        submittedItem->setSortKey(submitted.isValid()? ("1" + submitted.toUTC().toString(Qt::ISODate)) : "0");
        responsesTable->setItem(row, 0, submittedItem);
        responsesTable->setItem(row, 1, new QTableWidgetItem(thisStudent? thisStudent->fullName() : ""));
        auto *teamItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, peerReview.teams.at(studentResult.teamNum).name);
        teamItem->setSortKey(QString::number(studentResult.teamNum).rightJustified(5, '0') + (thisStudent? thisStudent->lastname : ""));
        responsesTable->setItem(row, 2, teamItem);
        responsesTable->setItem(row, 3, new QTableWidgetItem(thisStudent? thisStudent->email : ""));
        responsesTable->setItem(row, 4, new QTableWidgetItem(notes.join("; ")));
        if(studentResult.numResponses == 0) {
            for(int col = 0; col < responsesTable->columnCount(); col++) {
                responsesTable->item(row, col)->setBackground(QColor(STOPRED));
            }
        }
        row++;
    }
    responsesTable->setSortingEnabled(true);
    responsesTable->sortByColumn(0, Qt::AscendingOrder);
    updateSortIcons(responsesTable);
    responsesTable->resizeColumnsToContents();

    if(results->unmatchedNames.isEmpty()) {
        unmatchedLabel->clear();
        unmatchedLabel->hide();
    }
    else {
        unmatchedLabel->setText(((peerReview.platform == PeerReview::Platform::canvasQuizzes)?
                                     tr("These students took a team quiz but aren't in this peer review's teams, so their responses aren't used:") :
                                     tr("These names were chosen in the form but aren't in this peer review's teams, so their responses aren't used:")) +
                                " " + results->unmatchedNames.join(", "));
        unmatchedLabel->show();
    }
}

// a light background color for a rating: red at the bottom of the scale, through yellow in the middle, to green at the top
static QString ratingTint(double fractionOfScale)
{
    fractionOfScale = std::clamp(fractionOfScale, 0.0, 1.0);
    const QColor red(248, 203, 203), yellow(253, 240, 181), green(200, 235, 200);
    const QColor &from = (fractionOfScale < 0.5)? red : yellow;
    const QColor &to = (fractionOfScale < 0.5)? yellow : green;
    const double t = (fractionOfScale < 0.5)? (fractionOfScale / 0.5) : ((fractionOfScale - 0.5) / 0.5);
    return QColor(int(std::round(from.red() + t * (to.red() - from.red()))),
                  int(std::round(from.green() + t * (to.green() - from.green()))),
                  int(std::round(from.blue() + t * (to.blue() - from.blue())))).name();
}

// a score as shown in the results: contribution as a whole number (100 is an equal share), rating scales with one decimal place
static QString scoreValueText(const PeerReviewQuestion &question, const double value)
{
    if(std::isnan(value)) {
        return QString(QChar(0x2014));
    }
    if(question.kind == PeerReviewQuestion::Kind::contribution) {
        return QString::number(std::round(value));
    }
    if((question.kind == PeerReviewQuestion::Kind::likert) || (question.kind == PeerReviewQuestion::Kind::numberScale)) {
        return QString::number(value, 'f', 1);
    }
    return QString::number(value, 'g', 4);
}

QStringList PeerReviewResultsDialog::scoreLines(const int questionNum, const PeerReviewResults::StudentResult &studentResult) const
{
    const auto &question = peerReview.questions.at(questionNum);
    const bool isContribution = (questionNum == results->contributionQuestionNum);
    const double peer = isContribution? studentResult.perceived : studentResult.scores.at(questionNum).average;
    const double self = isContribution? studentResult.claimed : studentResult.scores.at(questionNum).self;
    if(std::isnan(peer) && std::isnan(self)) {
        return {};
    }
    return {tr("Peer") + " " + scoreValueText(question, peer), tr("Self") + " " + scoreValueText(question, self)};
}

void PeerReviewResultsDialog::refreshStudentsTab()
{
    // the questions with a number for each student: rating, contribution, and number entry questions about teammates
    QList<int> numberedQuestions;
    for(int questionNum = 0; questionNum < peerReview.questions.size(); questionNum++) {
        const auto &question = peerReview.questions.at(questionNum);
        if(question.isAboutTeammates() && (question.isScored() || (question.kind == PeerReviewQuestion::Kind::numberEntry))) {
            numberedQuestions << questionNum;
        }
    }

    QStringList headers = {tr("Team"), tr("Student")};
    for(const auto questionNum : std::as_const(numberedQuestions)) {
        headers << tr("Q") + QString::number(questionNum + 1);
    }
    headers << tr("Adjustment") + "\n" + tr("Factor") << tr("Flags");

    const int selectedRow = studentsTable->currentRow();
    studentsTable->setSortingEnabled(false);
    studentsTable->clear();
    studentsTable->setColumnCount(int(headers.size()));
    studentsTable->setHorizontalHeaderLabels(headers);
    for(int col = 0; col < numberedQuestions.size(); col++) {
        const int questionNum = numberedQuestions.at(col);
        studentsTable->horizontalHeaderItem(col + 2)->setToolTip(peerReview.questions.at(questionNum).text +
                                                                 ((questionNum == results->contributionQuestionNum)? ("\n" + tr("100 is an equal share")) : ""));
    }
    QString factorToolTip;
    if(results->contributionQuestionNum == -1) {
        factorToolTip = tr("The student's average rating from teammates, divided by the team's average (so 1.00 is the team's average)");
    }
    else if(peerReview.thresholds.factorFromAllQuestions) {
        factorToolTip = tr("The rating questions and the contribution question, averaged: the contribution question counts as one question "
                           "alongside the rating questions");
    }
    else {
        factorToolTip = tr("The teammates' view of the student's contribution, divided by 100 (an equal share)");
    }
    studentsTable->horizontalHeaderItem(int(headers.size()) - 2)->setToolTip(factorToolTip);
    studentsTable->setRowCount(int(results->students.size()));

    auto numberItem = [](const double value, const QString &text) {
        auto *item = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, text);
        item->setSortKey(std::isnan(value)? QString() : QString::number(value + 1000, 'f', 4).rightJustified(12, '0'));
        return item;
    };

    int row = 0;
    for(const auto &studentResult : results->students) {
        const auto *thisStudent = peerReview.student(studentResult.ID);
        int col = 0;
        auto *teamItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, peerReview.teams.at(studentResult.teamNum).name);
        teamItem->setSortKey(QString::number(studentResult.teamNum).rightJustified(5, '0') + (thisStudent? thisStudent->lastname : ""));
        teamItem->setData(Qt::UserRole + 1, studentResult.ID);
        studentsTable->setItem(row, col++, teamItem);
        studentsTable->setItem(row, col++, new QTableWidgetItem(thisStudent? thisStudent->fullName() : ""));
        for(const auto questionNum : std::as_const(numberedQuestions)) {
            const double sortValue = (questionNum == results->contributionQuestionNum)? studentResult.perceived : studentResult.scores.at(questionNum).average;
            studentsTable->setItem(row, col++, numberItem(sortValue, scoreLines(questionNum, studentResult).join("\n")));
        }
        auto *factorItem = numberItem(studentResult.factor, std::isnan(studentResult.factor)? "" : QString::number(studentResult.factor, 'f', 2));
        if(!std::isnan(studentResult.factor)) {
            // tinted like a contribution rating: 0.50 or less is red, 0.75 is yellow, and 1.00 (an equal share or the team's average) or more is green
            factorItem->setBackground(QColor(ratingTint((studentResult.factor - 0.5) / 0.5)));
        }
        studentsTable->setItem(row, col++, factorItem);
        studentsTable->setItem(row, col++, new QTableWidgetItem(studentResult.flags.join("; ")));
        if(!studentResult.flags.isEmpty()) {
            studentsTable->item(row, col - 1)->setBackground(QColor(TROPICALHEX));
        }
        row++;
    }
    studentsTable->setSortingEnabled(true);
    studentsTable->sortByColumn(0, Qt::AscendingOrder);
    updateSortIcons(studentsTable);
    studentsTable->resizeColumnsToContents();
    studentsTable->resizeRowsToContents();      // room for the two lines (peer and self) of the question columns
    if((selectedRow >= 0) && (selectedRow < studentsTable->rowCount())) {
        studentsTable->selectRow(selectedRow);
    }
    else if(studentsTable->rowCount() > 0) {
        studentsTable->selectRow(0);
    }
    refreshStudentDetails();
}

QString PeerReviewResultsDialog::answerText(const PeerReviewAnswer &answer) const
{
    const auto &question = peerReview.questions.at(answer.questionNum);
    if(question.isScored() && answer.hasScore && (question.kind != PeerReviewQuestion::Kind::contribution)) {
        return answer.values.join(", ") + " (" + QString::number(answer.score, 'g', 3) + ")";
    }
    return answer.values.join(", ");
}

void PeerReviewResultsDialog::refreshStudentDetails()
{
    const int row = studentsTable->currentRow();
    if((row < 0) || (studentsTable->item(row, 0) == nullptr)) {
        studentDetails->clear();
        return;
    }
    const long long studentID = studentsTable->item(row, 0)->data(Qt::UserRole + 1).toLongLong();
    const auto *studentResult = results->result(studentID);
    if(studentResult == nullptr) {
        studentDetails->clear();
        return;
    }

    // everything said about this student (and by them, for the questions asked once or about themselves), with names
    QString html = "<h3>" + studentName(studentID).toHtmlEscaped() + "</h3><p>" + peerReview.teams.at(studentResult->teamNum).name.toHtmlEscaped();
    if(!studentResult->flags.isEmpty()) {
        html += "<br><b>" + tr("Flags") + ":</b> " + studentResult->flags.join("; ").toHtmlEscaped();
    }
    html += "</p>";
    const auto &members = peerReview.teams.at(studentResult->teamNum).members;
    for(int questionNum = 0; questionNum < peerReview.questions.size(); questionNum++) {
        const auto &question = peerReview.questions.at(questionNum);
        QStringList lines;
        if(question.isAboutTeammates()) {
            // the student's own answer first, then each teammate's, in the team's order
            QHash<long long, QString> answersByGiver;
            for(const auto &answer : results->answers) {
                if((answer.questionNum == questionNum) && (answer.aboutID == studentID)) {
                    answersByGiver.insert(answer.giverID, answerText(answer).toHtmlEscaped().replace("\n", "<br>"));
                }
            }
            if(answersByGiver.contains(studentID)) {
                lines << "<i>" + tr("self") + "</i>: " + answersByGiver.value(studentID);
            }
            for(const auto &member : members) {
                if((member.ID != studentID) && answersByGiver.contains(member.ID)) {
                    lines << studentName(member.ID).toHtmlEscaped() + ": " + answersByGiver.value(member.ID);
                }
            }
        }
        else {
            for(const auto &answer : results->answers) {
                if((answer.questionNum == questionNum) && (answer.giverID == studentID)) {
                    lines << answerText(answer).toHtmlEscaped().replace("\n", "<br>");
                }
            }
        }
        if(lines.isEmpty()) {
            continue;
        }
        html += "<p><b>" + QString::number(questionNum + 1) + ". " + question.text.toHtmlEscaped() + "</b><br>" + lines.join("<br>") + "</p>";
    }
    studentDetails->setHtml(html);
}

QString PeerReviewResultsDialog::ratingCell(const PeerReviewQuestion &question, const double value) const
{
    if(std::isnan(value)) {
        return "<td align=\"center\">" + QString(QChar(0x2014)) + "</td>";
    }

    // where the value falls on the question's scale (contribution: 50% of an equal share or less is the bottom, an equal share or more is the top)
    double fractionOfScale = PeerReviewResults::NOVALUE;
    if((question.kind == PeerReviewQuestion::Kind::likert) && (question.options.size() > 1)) {
        fractionOfScale = (value - 1) / double(question.options.size() - 1);
    }
    else if(question.kind == PeerReviewQuestion::Kind::numberScale) {
        fractionOfScale = (value - question.scaleLow) / double(question.scaleHigh - question.scaleLow);
    }
    else if(question.kind == PeerReviewQuestion::Kind::contribution) {
        fractionOfScale = (value - 50) / 50.0;
    }
    const QString text = scoreValueText(question, value);

    if(std::isnan(fractionOfScale)) {
        return "<td align=\"center\">" + text + "</td>";
    }
    return "<td align=\"center\" bgcolor=\"" + ratingTint(fractionOfScale) + "\">" + text + "</td>";
}

void PeerReviewResultsDialog::refreshQuestionTab()
{
    const int questionNum = questionComboBox->currentData().toInt();
    if((questionNum < 0) || (questionNum >= peerReview.questions.size()) || (results == nullptr)) {
        questionView->clear();
        return;
    }
    const auto &question = peerReview.questions.at(questionNum);

    // (the question itself is in the dropdown above, so it isn't repeated here)
    QString html = "<p>" + PeerReviewQuestion::kindText(question.kind) + "; " + tr("about:") + " " +
                   PeerReviewQuestion::aboutText(question.about).toLower() + "; " + tr("shared with:") + " " +
                   PeerReviewQuestion::shareWithText(question.shareWith, question.about, question.showsWriterNames()).toLower() + "</p>";
    const bool hasSelfAndPeerColumns = question.isAboutTeammates() && (question.kind != PeerReviewQuestion::Kind::written);
    if(question.isAboutTeammates() && (question.kind == PeerReviewQuestion::Kind::contribution)) {
        html += "<p>" + tr("100 is an equal share of the work.") + "</p>";
    }
    for(int teamNum = 0; teamNum < peerReview.teams.size(); teamNum++) {
        html += "<h4>" + peerReview.teams.at(teamNum).name.toHtmlEscaped() + "</h4>";
        if(question.isAboutTeammates()) {
            html += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\"><tr><th>" + tr("About") + "</th>";
            if(question.isScored()) {
                html += "<th>" + tr("Self Rating") + "</th><th>" + tr("Peer Rating") + "</th>";
            }
            else if(question.kind == PeerReviewQuestion::Kind::numberEntry) {
                html += "<th>" + tr("Self") + "</th><th>" + tr("Peer Average") + "</th>";
            }
            else if(question.kind == PeerReviewQuestion::Kind::choice) {
                html += "<th>" + tr("Self") + "</th><th>" + tr("Peers' Choices") + "</th>";
            }
            html += "<th>" + tr("Answers") + "</th></tr>";

            for(const auto &member : peerReview.teams.at(teamNum).members) {
                const auto *studentResult = results->result(member.ID);
                if(studentResult == nullptr) {
                    continue;
                }
                html += "<tr><td>" + member.fullName().toHtmlEscaped() + "</td>";
                if(question.isScored()) {
                    const bool isContribution = (questionNum == results->contributionQuestionNum);
                    const auto &summary = studentResult->scores.at(questionNum);
                    html += ratingCell(question, isContribution? studentResult->claimed : summary.self) +
                            ratingCell(question, isContribution? studentResult->perceived : summary.average);
                }
                else if(question.kind == PeerReviewQuestion::Kind::numberEntry) {
                    const auto &summary = studentResult->scores.at(questionNum);
                    html += ratingCell(question, summary.self) + ratingCell(question, summary.average);
                }
                else if(question.kind == PeerReviewQuestion::Kind::choice) {
                    QString selfChoice;
                    for(const auto &answer : results->answers) {
                        if((answer.questionNum == questionNum) && (answer.aboutID == member.ID) && (answer.giverID == member.ID)) {
                            selfChoice = answer.values.join(", ");
                        }
                    }
                    QStringList counts;
                    for(const auto &count : studentResult->choiceCounts.at(questionNum)) {
                        if(count.second > 0) {
                            counts << count.first + ": " + QString::number(count.second);
                        }
                    }
                    html += "<td>" + selfChoice.toHtmlEscaped() + "</td><td>" + counts.join("; ").toHtmlEscaped() + "</td>";
                }

                // the individual answers, with the teammates' names (the self answer is already in its own column, if there is one)
                QStringList answers;
                for(const auto &answer : results->answers) {
                    if((answer.questionNum != questionNum) || (answer.aboutID != member.ID) || (hasSelfAndPeerColumns && (answer.giverID == member.ID))) {
                        continue;
                    }
                    const QString giver = (answer.giverID == member.ID)? tr("self") : studentName(answer.giverID);
                    answers << giver.toHtmlEscaped() + ": " + answerText(answer).toHtmlEscaped().replace("\n", "<br>");
                }
                html += "<td>" + answers.join("<br>") + "</td></tr>";
            }
            html += "</table>";
        }
        else {
            QStringList answers;
            double sum = 0;
            int count = 0;
            for(const auto &answer : results->answers) {
                if((answer.questionNum == questionNum) && (answer.teamNum == teamNum)) {
                    answers << studentName(answer.giverID).toHtmlEscaped() + ": " + answerText(answer).toHtmlEscaped().replace("\n", "<br>");
                    if(answer.hasScore) {
                        sum += answer.score;
                        count++;
                    }
                }
            }
            if(count > 0) {
                html += "<p>" + tr("Team average:") + " " + question.scoreDescription(sum / count).toHtmlEscaped() + "</p>";
            }
            html += "<p>" + (answers.isEmpty()? ("<i>" + tr("No answers") + "</i>") : answers.join("<br>")) + "</p>";
        }
    }
    questionView->setHtml(html);
}

void PeerReviewResultsDialog::refreshContributionTab()
{
    const int questionNum = results->contributionQuestionNum;
    if(questionNum == -1) {
        contributionView->clear();
        return;
    }
    const auto &question = peerReview.questions.at(questionNum);

    QString html = "<h3>" + question.text.toHtmlEscaped() + "</h3>"
                   "<p>" + tr("100 means an equal share of the work. Each student's ratings of their teammates are scaled to average 100, "
                              "leaving out their rating of themself, so that rating everyone else low doesn't raise their own share. "
                              "Each student's <b>teammates' view</b> is the average of what their teammates gave them, scaled so the team "
                              "averages 100; their <b>own estimate</b> is their rating of themself, scaled the same way, so it can differ from the answer they chose. "
                              "(This is the method used by TEAMMATES.)") + "</p>";
    for(int teamNum = 0; teamNum < peerReview.teams.size(); teamNum++) {
        const auto &members = peerReview.teams.at(teamNum).members;
        const auto &contribution = results->contributions.at(teamNum);
        html += "<h4>" + peerReview.teams.at(teamNum).name.toHtmlEscaped() + (contribution.disagreement? (" <i>(" + tr("teammates disagree") + ")</i>") : "") +
                "</h4><table border=\"1\" cellspacing=\"0\" cellpadding=\"4\"><tr><th>" + tr("Rating by") + " \\ " + tr("of") + "</th>";
        for(const auto &member : members) {
            html += "<th>" + member.fullName().toHtmlEscaped() + "</th>";
        }
        html += "</tr>";
        auto cell = [](const double value) {
            return "<td align=\"center\">" + (std::isnan(value)? QString(QChar(0x2014)) : QString::number(std::round(value))) + "</td>";
        };
        for(int giver = 0; giver < members.size(); giver++) {
            html += "<tr><td>" + members.at(giver).fullName().toHtmlEscaped() + "</td>";
            for(int recipient = 0; recipient < members.size(); recipient++) {
                html += cell((giver < contribution.ratings.size())? contribution.ratings.at(giver).at(recipient) : PeerReviewResults::NOVALUE);
            }
            html += "</tr>";
        }
        QString perceivedRow = "<tr><td><b>" + tr("Teammates' view") + "</b></td>";
        QString claimedRow = "<tr><td><b>" + tr("Own estimate") + "</b></td>";
        QString differenceRow = "<tr><td><b>" + tr("Own estimate minus teammates' view") + "</b></td>";
        for(const auto &member : members) {
            const auto *studentResult = results->result(member.ID);
            const double perceived = (studentResult != nullptr)? studentResult->perceived : PeerReviewResults::NOVALUE;
            const double claimed = (studentResult != nullptr)? studentResult->claimed : PeerReviewResults::NOVALUE;
            perceivedRow += cell(perceived);
            claimedRow += cell(claimed);
            differenceRow += cell(claimed - perceived);
        }
        html += perceivedRow + "</tr>" + claimedRow + "</tr>" + differenceRow + "</tr></table>";
    }
    contributionView->setHtml(html);
}

void PeerReviewResultsDialog::refreshCommentsTab()
{
    // the written answers that are shared with students (in the order of the answers, which is kept in each row as it is sorted)
    sharedComments.clear();
    for(const auto &answer : results->answers) {
        const auto &question = peerReview.questions.at(answer.questionNum);
        if((question.kind == PeerReviewQuestion::Kind::written) && (question.shareWith != PeerReviewQuestion::ShareWith::instructor) &&
            !answer.values.join("").trimmed().isEmpty()) {
            sharedComments << answer;
        }
    }

    const int selectedRow = commentsTable->currentRow();
    commentsTable->setSortingEnabled(false);
    commentsTable->clearContents();
    commentsTable->setRowCount(int(sharedComments.size()));
    for(int index = 0; index < sharedComments.size(); index++) {
        const auto &comment = sharedComments.at(index);
        const auto &question = peerReview.questions.at(comment.questionNum);
        const bool isDeleted = peerReview.commentIsHidden(comment);
        const bool isEdited = peerReview.release.editedComments.contains(PeerReview::commentKey(comment));

        // sorted at first by question, then by team, then by the writer
        auto *questionItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric,
                                                         QString::number(comment.questionNum + 1) + ". " + question.text);
        questionItem->setSortKey(QString::number(comment.questionNum).rightJustified(5, '0') + QString::number(comment.teamNum).rightJustified(5, '0') +
                                 studentName(comment.giverID));
        questionItem->setToolTip(question.text);
        questionItem->setData(Qt::UserRole + 1, index);
        auto *teamItem = new SortableTableWidgetItem(SortableTableWidgetItem::SortType::alphanumeric, peerReview.teams.at(comment.teamNum).name);
        teamItem->setSortKey(QString::number(comment.teamNum).rightJustified(5, '0') + studentName(comment.giverID));
        auto *fromItem = new QTableWidgetItem(studentName(comment.giverID) + (question.showsWriterNames()? (" (" + tr("name shown") + ")") : ""));
        auto *aboutItem = new QTableWidgetItem((comment.aboutID >= 0)? studentName(comment.aboutID) : tr("The team"));
        auto *commentItem = new QTableWidgetItem(peerReview.commentText(comment));
        auto *statusItem = new QTableWidgetItem(isDeleted? tr("Deleted") : (isEdited? tr("Edited") : ""));
        const QList<QTableWidgetItem *> items = {questionItem, teamItem, fromItem, aboutItem, commentItem, statusItem};
        for(int col = 0; col < items.size(); col++) {
            if(isDeleted) {
                items.at(col)->setForeground(QColor(Qt::gray));
            }
            commentsTable->setItem(index, col, items.at(col));
        }
    }
    commentsTable->setSortingEnabled(true);
    commentsTable->sortByColumn(0, Qt::AscendingOrder);
    updateSortIcons(commentsTable);
    commentsTable->resizeColumnsToContents();
    commentsTable->setColumnWidth(0, std::min(250, commentsTable->columnWidth(0)));
    commentsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    if((selectedRow >= 0) && (selectedRow < commentsTable->rowCount())) {
        commentsTable->selectRow(selectedRow);
    }
}

void PeerReviewResultsDialog::editComment(const int row)
{
    const auto *questionItem = commentsTable->item(row, 0);
    if(questionItem == nullptr) {
        return;
    }
    const int index = questionItem->data(Qt::UserRole + 1).toInt();
    if((index < 0) || (index >= sharedComments.size())) {
        return;
    }
    const auto &comment = sharedComments.at(index);
    const auto &question = peerReview.questions.at(comment.questionNum);
    const QString key = PeerReview::commentKey(comment);
    const QString originalText = comment.values.join(", ");
    const bool isDeleted = peerReview.commentIsHidden(comment);

    QDialog editDialog(this);
    editDialog.setWindowTitle(tr("Edit comment"));
    editDialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    editDialog.setStyleSheet(WHITEDIALOGSTYLE);
    auto *editLayout = new QVBoxLayout(&editDialog);
    auto *aboutLabel = new QLabel(QString::number(comment.questionNum + 1) + ". " + question.text + "\n" + tr("From") + " " + studentName(comment.giverID) +
                                  ", " + tr("about") + " " + ((comment.aboutID >= 0)? studentName(comment.aboutID) : tr("the team")), &editDialog);
    aboutLabel->setStyleSheet(LABEL10PTSTYLE);
    aboutLabel->setWordWrap(true);
    editLayout->addWidget(aboutLabel);
    if(peerReview.release.editedComments.contains(key)) {
        auto *originalLabel = new QLabel(tr("Original comment:") + "\n" + originalText, &editDialog);
        originalLabel->setStyleSheet(LABEL10PTSTYLE);
        originalLabel->setWordWrap(true);
        editLayout->addWidget(originalLabel);
    }
    auto *commentTextEdit = new QPlainTextEdit(&editDialog);
    commentTextEdit->setPlainText(peerReview.commentText(comment));
    editLayout->addWidget(FeedbackReleaseDialog::framed(commentTextEdit, 120, &editDialog));

    // delete (or restore) on the left, cancel and save on the right
    enum class Choice {cancel, save, deleteOrRestore};
    Choice choice = Choice::cancel;
    auto *deleteButton = new QPushButton(isDeleted? tr("Restore Comment") : tr("Delete Comment"), &editDialog);
    deleteButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    deleteButton->setToolTip(isDeleted? tr("Put this comment back into the feedback report") : tr("Leave this comment out of the feedback report"));
    connect(deleteButton, &QPushButton::clicked, &editDialog, [&choice, &editDialog](){choice = Choice::deleteOrRestore; editDialog.accept();});
    auto *cancelButton = new QPushButton(tr("Cancel"), &editDialog);
    cancelButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(cancelButton, &QPushButton::clicked, &editDialog, &QDialog::reject);
    auto *saveButton = new QPushButton(tr("Save"), &editDialog);
    saveButton->setStyleSheet(SMALLBUTTONSTYLE);
    connect(saveButton, &QPushButton::clicked, &editDialog, [&choice, &editDialog](){choice = Choice::save; editDialog.accept();});
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(saveButton);
    editLayout->addLayout(buttonLayout);
    editDialog.resize(LG_DLG_SIZE, editDialog.sizeHint().height());

    if((editDialog.exec() == QDialog::Rejected) || (choice == Choice::cancel)) {
        return;
    }
    if((choice == Choice::deleteOrRestore) && !isDeleted) {
        peerReview.release.hiddenComments.insert(key);
    }
    else {
        // saving, or restoring a deleted comment: either way, the text as it is in the box is kept
        if(choice == Choice::deleteOrRestore) {
            peerReview.release.hiddenComments.remove(key);
        }
        const QString newText = commentTextEdit->toPlainText().trimmed();
        if(newText.isEmpty() || (newText == originalText)) {
            peerReview.release.editedComments.remove(key);
        }
        else {
            peerReview.release.editedComments.insert(key, newText);
        }
    }
    peerReview.save();
    refreshCommentsTab();
}

void PeerReviewResultsDialog::exportSummarySpreadsheet()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("CSV File (*.csv);;Excel File (*.xlsx)"));
    if(fileName.isEmpty()) {
        return;
    }
    const auto format = (QFileInfo(fileName).suffix().compare("xlsx", Qt::CaseInsensitive) == 0)?
                            grueprGlobal::TabularFileFormat::xlsx : grueprGlobal::TabularFileFormat::csv;

    QStringList header = {tr("Team"), tr("First Name"), tr("Last Name"), tr("Email"), tr("Responded"), tr("Teammates rating them")};
    QList<int> numberedQuestions;
    for(int questionNum = 0; questionNum < peerReview.questions.size(); questionNum++) {
        const auto &question = peerReview.questions.at(questionNum);
        if(!question.isAboutTeammates() || !(question.isScored() || (question.kind == PeerReviewQuestion::Kind::numberEntry))) {
            continue;
        }
        numberedQuestions << questionNum;
        if(questionNum == results->contributionQuestionNum) {
            header << question.text + " [" + tr("teammates' view, 100 = equal share") + "]" << question.text + " [" + tr("own estimate, scaled") + "]" <<
                      question.text + " [" + tr("own answer") + "]";
        }
        else {
            header << question.text + " [" + tr("teammates' average") + "]" << question.text + " [" + tr("lowest") + "]" <<
                      question.text + " [" + tr("highest") + "]" << question.text + " [" + tr("self") + "]";
        }
    }
    header << tr("Overall rating from teammates (% of scale)") << tr("Overall self rating (% of scale)") << tr("Adjustment factor") << tr("Flags");

    auto number = [](const double value, const int decimals) {
        return std::isnan(value)? QString() : QString::number(value, 'f', decimals);
    };
    QList<QStringList> rows = {header};
    for(const auto &studentResult : results->students) {
        const auto *thisStudent = peerReview.student(studentResult.ID);
        if(thisStudent == nullptr) {
            continue;
        }
        QStringList rowValues = {peerReview.teams.at(studentResult.teamNum).name, thisStudent->firstname, thisStudent->lastname, thisStudent->email,
                                 (studentResult.numResponses > 0)? tr("Yes") : tr("No"), QString::number(studentResult.numRaters)};
        for(const auto questionNum : std::as_const(numberedQuestions)) {
            if(questionNum == results->contributionQuestionNum) {
                // the student's answer about themself, as chosen in the form (before the scaling that gives their own estimate)
                QString ownAnswer;
                for(const auto &answer : results->answers) {
                    if((answer.questionNum == questionNum) && (answer.giverID == studentResult.ID) && (answer.aboutID == studentResult.ID)) {
                        ownAnswer = answer.values.join(", ");
                    }
                }
                rowValues << number(studentResult.perceived, 0) << number(studentResult.claimed, 0) << ownAnswer;
            }
            else {
                const auto &summary = studentResult.scores.at(questionNum);
                rowValues << number(summary.average, 2) << number(summary.low, 2) << number(summary.high, 2) << number(summary.self, 2);
            }
        }
        rowValues << number(studentResult.peerOverall * 100, 1) << number(studentResult.selfOverall * 100, 1) << number(studentResult.factor, 3)
                  << studentResult.flags.join("; ");
        rows << rowValues;
    }

    if(!grueprGlobal::writeTabularFile(rows, format, fileName)) {
        grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
    }
}

void PeerReviewResultsDialog::remindStudents()
{
    const QList<const PeerReviewStudent *> notResponded = missingStudents();
    if(notResponded.isEmpty()) {
        ReminderDialog::infoMessage(this, tr("Every student has responded."));
        return;
    }

    // each student's link is to the form, or to their own team's quiz in Canvas
    QList<ReminderDialog::Recipient> recipients;
    for(const auto *student : notResponded) {
        const int teamNum = peerReview.teamOf(student->ID);
        QString link = peerReview.responderURL;
        if(peerReview.platform == PeerReview::Platform::canvasQuizzes) {
            link = ((teamNum >= 0) && (teamNum < peerReview.canvasQuizzes.size()))? peerReview.canvasQuizzes.at(teamNum).url : "";
        }
        recipients.append({student->ID, student->firstname, student->lastname, student->email,
                           (teamNum >= 0)? peerReview.teams.at(teamNum).name : "", link, student->LMSID});
    }

    // the Canvas course is known for a peer review run as Canvas quizzes; otherwise it's asked for if messages are sent in Canvas
    const long long courseID = (peerReview.platform == PeerReview::Platform::canvasQuizzes)? peerReview.canvasCourseID : -1;
    ReminderDialog reminderDialog(peerReview.title, tr("peer review"), recipients, courseID, nullptr, this);
    reminderDialog.exec();

    const QList<long long> messaged = reminderDialog.messagedIDs();
    if(!messaged.isEmpty()) {
        const QDateTime now = QDateTime::currentDateTime();
        for(const auto studentID : messaged) {
            peerReview.reminders.append({now, studentID});
        }
        peerReview.save();
        refreshResponsesTab();
    }
}

void PeerReviewResultsDialog::provideFeedback()
{
    FeedbackReleaseDialog feedbackDialog(peerReview, *results, this);
    feedbackDialog.exec();
    peerReview.save();
}
