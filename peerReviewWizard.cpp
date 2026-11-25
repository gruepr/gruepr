#include "peerReviewWizard.h"
#include "gruepr_globals.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include "widgets/styledComboBox.h"
#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QToolTip>

PeerReviewWizard::PeerReviewWizard(const PeerReview *earlierPeerReview, QWidget *parent)
    : QWizard(parent),
    isNewRound(earlierPeerReview != nullptr)
{
    setWindowTitle(tr("gruepr - Get peer review"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumWidth(800);
    setMinimumHeight(600);
    setWindowFlag(Qt::CustomizeWindowHint, true);

    const QSettings savedSettings;
    restoreGeometry(savedSettings.value("peerReviewWizardWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("saveFileLocation", "").toString());

    setStyleSheet(QString() + COMBOBOXSTYLE + SPINBOXSTYLE + CHECKBOXSTYLE + RADIOBUTTONSTYLE + SCROLLBARSTYLE);

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    if(earlierPeerReview != nullptr) {
        // a new round: the same teams, questions, and settings, but a new form, and no responses or released feedback yet
        peerReview.title = nextRoundTitle(earlierPeerReview->title);
        peerReview.teamsSource = earlierPeerReview->teamsSource;
        peerReview.teams = earlierPeerReview->teams;
        peerReview.questions = earlierPeerReview->questions;
        peerReview.instructions = earlierPeerReview->instructions;
        peerReview.requireSignIn = earlierPeerReview->requireSignIn;
        peerReview.platform = earlierPeerReview->platform;
        peerReview.countTowardFinalGrade = earlierPeerReview->countTowardFinalGrade;
        peerReview.thresholds = earlierPeerReview->thresholds;
        peerReview.release.introText = earlierPeerReview->release.introText;
        peerReview.release.minimumResponders = earlierPeerReview->release.minimumResponders;
        peerReview.release.includeTeamAverage = earlierPeerReview->release.includeTeamAverage;
        peerReview.release.includeAdjustmentFactor = earlierPeerReview->release.includeAdjustmentFactor;
        earlierPeerReviewFile = earlierPeerReview->fileName;
        canvasTeamsCourseID = earlierPeerReview->canvasCourseID;
    }
    else {
        peerReview.questions = PeerReview::defaultQuestions();
    }

    setPage(Page::teams, new PeerReviewTeamsPage);
    setPage(Page::rated, new PeerReviewQuestionsPage(false));
    setPage(Page::written, new PeerReviewQuestionsPage(true));
    setPage(Page::previewcreate, new PeerReviewPreviewPage);
    startFromQuestions(peerReview.questions);

    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    setButtonLayout(buttonLayout);

    button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    setButtonText(QWizard::CancelButton, QString(LEFTDOUBLEARROW) + "  " + tr("Peer Reviews"));
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, QString(LEFTARROW) + "  " + tr("Previous Step"));
    setOption(QWizard::NoBackButtonOnStartPage);
    button(QWizard::NextButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    setButtonText(QWizard::NextButton, tr("Next Step") + "  " + RIGHTARROW);
}

PeerReviewWizard::~PeerReviewWizard()
{
    QSettings savedSettings;
    savedSettings.setValue("peerReviewWizardWindowGeometry", saveGeometry());
    savedSettings.setValue("saveFileLocation", saveFileLocation.canonicalFilePath());
}

void PeerReviewWizard::setQuestions(const QList<PeerReviewQuestion> &questions)
{
    QList<PeerReviewQuestion> ratedQuestions, writtenQuestions;
    for(const auto &question : questions) {
        if(question.kind == PeerReviewQuestion::Kind::written) {
            writtenQuestions << question;
        }
        else {
            ratedQuestions << question;
        }
    }
    qobject_cast<PeerReviewQuestionsPage *>(page(Page::rated))->setQuestions(ratedQuestions);
    qobject_cast<PeerReviewQuestionsPage *>(page(Page::written))->setQuestions(writtenQuestions);
}

QList<PeerReviewQuestion> PeerReviewWizard::currentQuestions() const
{
    return qobject_cast<PeerReviewQuestionsPage *>(page(Page::rated))->getQuestions() +
           qobject_cast<PeerReviewQuestionsPage *>(page(Page::written))->getQuestions();
}

QJsonArray PeerReviewWizard::questionsAsJson(const QList<PeerReviewQuestion> &questions)
{
    QJsonArray questionsArray;
    for(const auto &question : questions) {
        questionsArray.append(question.toJson());
    }
    return questionsArray;
}

void PeerReviewWizard::startFromQuestions(const QList<PeerReviewQuestion> &questions)
{
    setQuestions(questions);
    startingQuestions = questionsAsJson(currentQuestions());   // as the question pages hold them, to tell later if they've been changed
}

bool PeerReviewWizard::questionsWereEdited() const
{
    return questionsAsJson(currentQuestions()) != startingQuestions;
}

void PeerReviewWizard::loadTeams()
{
    qobject_cast<PeerReviewTeamsPage *>(page(Page::teams))->loadTeams();
}

CanvasHandler *PeerReviewWizard::connectToCanvas()
{
    if(canvas == nullptr) {
        auto *newCanvas = new CanvasHandler(this);
        if(!newCanvas->authenticate()) {
            newCanvas->deleteLater();
            return nullptr;
        }
        canvas = newCanvas;
    }
    canvas->lastErrorMessage.clear();       // so an error from an earlier use isn't shown again
    return canvas;
}

void PeerReviewWizard::dropCanvasConnection()
{
    if(canvas != nullptr) {
        canvas->deleteLater();
        canvas = nullptr;
    }
}

QStringList PeerReviewWizard::previewNames() const
{
    QStringList names;
    if(!peerReview.teams.isEmpty()) {
        for(int memberNum = 0; memberNum < peerReview.teams.constFirst().members.size(); memberNum++) {
            names << peerReview.memberLabel(0, memberNum);
        }
    }
    return names;
}

QString PeerReviewWizard::nextRoundTitle(const QString &earlierTitle)
{
    static const QRegularExpression roundNumber(R"(\((round|Round) (\d+)\)\s*$)");
    const QRegularExpressionMatch match = roundNumber.match(earlierTitle);
    if(match.hasMatch()) {
        QString title = earlierTitle;
        return title.replace(match.capturedStart(), match.capturedLength(),
                             "(" + match.captured(1) + " " + QString::number(match.captured(2).toInt() + 1) + ")");
    }
    return earlierTitle.trimmed() + " (" + tr("round 2") + ")";
}

void PeerReviewWizard::reject()
{
    if(!previewPageVisited || peerReviewHasBeenCreated) {
        QDialog::reject();
    }
    else {
        const bool okClose = grueprGlobal::warningMessage(this, "Are you sure?",
                                                          tr("You have not yet created this peer review.\n"
                                                             "Do you want to close?"),
                                                          tr("Close"), tr("Go back"));
        if(okClose) {
            QDialog::reject();
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PeerReviewTeamsPage::PeerReviewTeamsPage(QWidget *parent)
    : SurveyMakerPage(PeerReviewWizard::pageNames, PeerReviewWizard::Page::teams, tr("Peer Review Name and Teams"), 0, true, parent)
{
    auto *stretch = questionLayout->takeAt(0);   // will put this back at the end of the layout after adding everything

    // the name of the peer review
    auto *titleFrame = new QFrame(this);
    titleFrame->setStyleSheet(QString(BLUEFRAME) + SCROLLBARSTYLE);
    auto *titleLayout = new QVBoxLayout(titleFrame);
    auto *titleLabel = new QLabel(tr("Name of this peer review"), titleFrame);
    titleLabel->setStyleSheet(LABEL12PTSTYLE);
    titleLineEdit = new QLineEdit(titleFrame);
    titleLineEdit->setStyleSheet(LINEEDITSTYLE);
    titleLineEdit->setPlaceholderText(tr("e.g., Project 1 midpoint peer review"));
    auto *titleExplainer = new QLabel(tr("Students will see this name at the top of the form and of their feedback."), titleFrame);
    titleExplainer->setStyleSheet(LABEL10PTSTYLE);
    titleExplainer->setWordWrap(true);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleLineEdit);
    titleLayout->addWidget(titleExplainer);
    connect(titleLineEdit, &QLineEdit::textChanged, this, [this](const QString &newTitle){
        qobject_cast<PeerReviewWizard *>(wizard())->peerReview.title = newTitle.simplified();
    });
    questionLayout->addSpacing(10);
    questionLayout->addWidget(titleFrame);

    // where the teams come from
    auto *teamsFrame = new QFrame(this);
    teamsFrame->setStyleSheet(QString(BLUEFRAME) + SCROLLBARSTYLE);
    auto *teamsLayout = new QGridLayout(teamsFrame);
    auto *teamsLabel = new QLabel(tr("Teams"), teamsFrame);
    teamsLabel->setStyleSheet(LABEL12PTSTYLE);
    earlierReviewRadioButton = new QRadioButton(tr("The same teams as the earlier peer review"), teamsFrame);
    savedSessionRadioButton = new QRadioButton(tr("Teams from a saved gruepr session"), teamsFrame);
    savedSessionComboBox = new StyledComboBox(teamsFrame);
    savedSessionComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    savedSessionComboBox->installEventFilter(new MouseWheelBlocker(savedSessionComboBox));
    auto *teamSetLabel = new QLabel(tr("Set of teams:"), teamsFrame);
    teamSetLabel->setStyleSheet(LABEL10PTSTYLE);
    teamSetComboBox = new StyledComboBox(teamsFrame);
    teamSetComboBox->setFocusPolicy(Qt::StrongFocus);
    teamSetComboBox->installEventFilter(new MouseWheelBlocker(teamSetComboBox));
    spreadsheetRadioButton = new QRadioButton(tr("Teams from a spreadsheet"), teamsFrame);
    spreadsheetButton = new QPushButton(tr("Choose file..."), teamsFrame);
    spreadsheetButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    spreadsheetLabel = new QLabel(tr("The spreadsheet (.csv, .txt, or .xlsx) needs a column for each student's team and their name, "
                                     "and ideally their email address. A team spreadsheet saved by gruepr works as is."), teamsFrame);
    spreadsheetLabel->setStyleSheet(LABEL10PTSTYLE);
    spreadsheetLabel->setWordWrap(true);
    canvasRadioButton = new QRadioButton(tr("Teams from a Canvas group set"), teamsFrame);
    canvasButton = new QPushButton(tr("Choose group set..."), teamsFrame);
    canvasButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    canvasLabel = new QLabel(tr("Each group in the set that has students in it becomes a team."), teamsFrame);
    canvasLabel->setStyleSheet(LABEL10PTSTYLE);
    canvasLabel->setTextFormat(Qt::PlainText);     // (it gets the names of a course and group set)
    canvasLabel->setWordWrap(true);
    sourceButtons = new QButtonGroup(this);
    sourceButtons->addButton(earlierReviewRadioButton, Source::earlierReview);
    sourceButtons->addButton(savedSessionRadioButton, Source::savedSession);
    sourceButtons->addButton(spreadsheetRadioButton, Source::spreadsheet);
    sourceButtons->addButton(canvasRadioButton, Source::canvasGroupSet);
    int row = 0;
    teamsLayout->addWidget(teamsLabel, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->setRowMinimumHeight(row++, 10);
    teamsLayout->addWidget(earlierReviewRadioButton, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(savedSessionRadioButton, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(savedSessionComboBox, row++, 1, 1, -1, Qt::AlignVCenter);
    teamsLayout->addWidget(teamSetLabel, row++, 1, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(teamSetComboBox, row++, 1, 1, -1, Qt::AlignVCenter);
    teamsLayout->setRowMinimumHeight(row++, 10);
    teamsLayout->addWidget(spreadsheetRadioButton, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(spreadsheetButton, row++, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(spreadsheetLabel, row++, 1, 1, -1, Qt::AlignVCenter);
    teamsLayout->setRowMinimumHeight(row++, 10);
    teamsLayout->addWidget(canvasRadioButton, row++, 0, 1, -1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(canvasButton, row++, 1, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    teamsLayout->addWidget(canvasLabel, row++, 1, 1, -1, Qt::AlignVCenter);
    teamsLayout->setColumnMinimumWidth(0, 20);
    teamsLayout->setColumnStretch(1, 1);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(teamsFrame);

    // the questions to start from
    auto *questionsFrame = new QFrame(this);
    questionsFrame->setStyleSheet(QString(BLUEFRAME) + SCROLLBARSTYLE);
    auto *questionsFrameLayout = new QVBoxLayout(questionsFrame);
    auto *questionsLabel = new QLabel(tr("Questions"), questionsFrame);
    questionsLabel->setStyleSheet(LABEL12PTSTYLE);
    auto *questionsExplainer = new QLabel(tr("Start from these questions, then change them however you like on the next two pages."), questionsFrame);
    questionsExplainer->setStyleSheet(LABEL10PTSTYLE);
    questionsExplainer->setWordWrap(true);
    questionsComboBox = new StyledComboBox(questionsFrame);
    questionsComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    questionsComboBox->installEventFilter(new MouseWheelBlocker(questionsComboBox));
    questionsFrameLayout->addWidget(questionsLabel);
    questionsFrameLayout->addWidget(questionsExplainer);
    questionsFrameLayout->addWidget(questionsComboBox);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(questionsFrame);
    questionLayout->addItem(stretch);

    connect(sourceButtons, &QButtonGroup::idClicked, this, &PeerReviewTeamsPage::sourceChange);
    connect(savedSessionComboBox, &QComboBox::activated, this, &PeerReviewTeamsPage::savedSessionChange);
    connect(teamSetComboBox, &QComboBox::activated, this, &PeerReviewTeamsPage::teamSetChange);
    connect(spreadsheetButton, &QPushButton::clicked, this, &PeerReviewTeamsPage::loadSpreadsheet);
    connect(canvasButton, &QPushButton::clicked, this, &PeerReviewTeamsPage::loadCanvasGroupSet);
    connect(questionsComboBox, &QComboBox::activated, this, &PeerReviewTeamsPage::questionsChange);

    // the preview shows the teams, all in one label since there can be very many of them
    previewTitle = new QLabel(this);
    previewTitle->setStyleSheet(LABEL12PTSTYLE);
    previewTitle->setWordWrap(true);
    previewTitle->setContentsMargins(10, 10, 10, 10);
    teamsPreviewLabel = new QLabel(this);
    teamsPreviewLabel->setStyleSheet(LABEL10PTSTYLE);
    teamsPreviewLabel->setTextFormat(Qt::RichText);
    teamsPreviewLabel->setWordWrap(true);
    teamsPreviewLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    teamsPreviewLabel->setContentsMargins(10, 0, 10, 10);
    previewLayout->insertWidget(0, previewTitle);
    previewLayout->insertWidget(1, teamsPreviewLabel);
}

void PeerReviewTeamsPage::initializePage()
{
    if(initialized) {
        return;
    }
    initialized = true;

    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    titleLineEdit->setText(wiz->peerReview.title);

    earlierReviewRadioButton->setVisible(wiz->isNewRound);
    if(wiz->isNewRound) {
        earlierTeams = wiz->peerReview.teams;
        earlierTeamsSource = wiz->peerReview.teamsSource;
    }

    const QList<QPair<QString, QString>> sessions = PeerReview::savedSessions();
    for(const auto &session : sessions) {
        savedSessionComboBox->addItem(session.first, session.second);
    }
    savedSessionRadioButton->setEnabled(!sessions.isEmpty());
    if(sessions.isEmpty()) {
        savedSessionRadioButton->setToolTip(tr("There are no saved gruepr sessions on this computer yet."));
    }

    if(wiz->isNewRound) {
        earlierReviewRadioButton->setChecked(true);
    }
    else if(!sessions.isEmpty()) {
        savedSessionRadioButton->setChecked(true);
    }
    else {
        spreadsheetRadioButton->setChecked(true);
    }
    savedSessionComboBox->setEnabled(sourceButtons->checkedId() == Source::savedSession);
    teamSetComboBox->setEnabled(sourceButtons->checkedId() == Source::savedSession);
    spreadsheetButton->setEnabled(sourceButtons->checkedId() == Source::spreadsheet);
    canvasButton->setEnabled(sourceButtons->checkedId() == Source::canvasGroupSet);
    previewTitle->setText(tr("Loading teams..."));      // they are loaded in loadTeams(), once this page is showing

    // the questions to start from: built-in sets, a question set file, or the questions of any saved peer review
    questionsComboBox->addItem(tr("gruepr's default questions (5 rating questions)"), "default");
    questionsComboBox->addItem(tr("Contribution-based questions (a contribution question and 4 written questions)"), "contribution");
    questionsComboBox->addItem(tr("No questions"), "none");
    questionsComboBox->addItem(tr("Questions from a question set file..."), "file");
    const QList<PeerReview::StoredPeerReview> storedReviews = PeerReview::storedPeerReviews();
    for(const auto &storedReview : storedReviews) {
        questionsComboBox->addItem(tr("Questions from:") + " " + storedReview.name, "review");
        questionsComboBox->setItemData(questionsComboBox->count() - 1, storedReview.fileName, Qt::UserRole + 1);
        if(wiz->isNewRound && (storedReview.fileName == wiz->earlierPeerReviewFile)) {
            questionsComboBoxIndex = questionsComboBox->count() - 1;    // a new round starts with the earlier peer review's questions
        }
    }
    questionsComboBox->setCurrentIndex(questionsComboBoxIndex);
}

void PeerReviewTeamsPage::loadTeams()
{
    if(teamsLoaded) {
        return;
    }
    teamsLoaded = true;
    sourceChange();
}

void PeerReviewTeamsPage::questionsChange(int index)
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    const QString source = questionsComboBox->itemData(index).toString();
    QList<PeerReviewQuestion> startingQuestions;
    if(source == "default") {
        startingQuestions = PeerReview::defaultQuestions();
    }
    else if(source == "contribution") {
        startingQuestions = PeerReview::contributionBasedQuestions();
    }
    else if(source == "file") {
        const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), wiz->saveFileLocation.canonicalPath(),
                                                              tr(PeerReview::QUESTIONSETFILEFILTER) + ";;" + tr("All Files (*)"));
        if(fileName.isEmpty()) {
            questionsComboBox->setCurrentIndex(questionsComboBoxIndex);
            return;
        }
        if(!PeerReview::loadQuestionSet(fileName, startingQuestions)) {
            grueprGlobal::errorMessage(this, tr("File Error"), tr("This file cannot be read."));
            questionsComboBox->setCurrentIndex(questionsComboBoxIndex);
            return;
        }
        wiz->saveFileLocation.setFile(QFileInfo(fileName).canonicalPath());
    }
    else if(source == "review") {
        PeerReview earlierReview;
        if(!PeerReview::load(questionsComboBox->itemData(index, Qt::UserRole + 1).toString(), earlierReview)) {
            grueprGlobal::errorMessage(this, tr("File Error"), tr("This peer review cannot be read."));
            questionsComboBox->setCurrentIndex(questionsComboBoxIndex);
            return;
        }
        startingQuestions = earlierReview.questions;
    }

    if(wiz->questionsWereEdited()) {
        const bool replace = grueprGlobal::warningMessage(this, "Are you sure?", tr("This will replace the questions you have already changed\n"
                                                                                  "on the next two pages."),
                                                          tr("Replace"), tr("Cancel"));
        if(!replace) {
            questionsComboBox->setCurrentIndex(questionsComboBoxIndex);
            return;
        }
    }
    wiz->startFromQuestions(startingQuestions);
    questionsComboBoxIndex = index;
}

void PeerReviewTeamsPage::sourceChange()
{
    const int source = sourceButtons->checkedId();
    savedSessionComboBox->setEnabled(source == Source::savedSession);
    teamSetComboBox->setEnabled(source == Source::savedSession);
    spreadsheetButton->setEnabled(source == Source::spreadsheet);
    canvasButton->setEnabled(source == Source::canvasGroupSet);

    if(source == Source::earlierReview) {
        setTeams(earlierTeams, earlierTeamsSource);
    }
    else if(source == Source::savedSession) {
        savedSessionChange();
    }
    else if(source == Source::spreadsheet) {
        setTeams(spreadsheetTeams, spreadsheetSource);
    }
    else {
        setTeams(canvasTeams, canvasSource);
    }
}

void PeerReviewTeamsPage::savedSessionChange()
{
    // reading a large saved session takes a moment, so each one is read only once
    const QString fileName = savedSessionComboBox->currentData().toString();
    if(!savedSessionTeamSets.contains(fileName)) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
        previewTitle->setText(tr("Loading teams..."));
        previewTitle->repaint();
        savedSessionTeamSets.insert(fileName, PeerReview::teamSetsInSavedSession(fileName));
        QApplication::restoreOverrideCursor();
    }
    teamSetsInSession = savedSessionTeamSets.value(fileName);
    teamSetComboBox->clear();
    for(const auto &teamSet : std::as_const(teamSetsInSession)) {
        teamSetComboBox->addItem(teamSet.name + " (" + QString::number(teamSet.teams.size()) + " " + tr("teams") + ")");
    }
    if(teamSetsInSession.isEmpty()) {
        teamSetComboBox->addItem(tr("No teams have been formed in this session yet"));
        teamSetComboBox->setEnabled(false);
    }
    else {
        teamSetComboBox->setEnabled(true);
    }
    teamSetChange();
}

void PeerReviewTeamsPage::teamSetChange()
{
    const int teamSetNum = teamSetComboBox->currentIndex();
    if((teamSetNum < 0) || (teamSetNum >= teamSetsInSession.size())) {
        setTeams({}, "");
        return;
    }
    QString sessionName = savedSessionComboBox->currentText();
    sessionName = sessionName.left(sessionName.lastIndexOf(" [")).trimmed();     // remove the "[Last opened: ...]"
    setTeams(teamSetsInSession.at(teamSetNum).teams, tr("Saved session") + " \"" + sessionName + "\", " + teamSetsInSession.at(teamSetNum).name);
}

void PeerReviewTeamsPage::loadSpreadsheet()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    QList<QStringList> rows;
    QString fileName;
    if(!grueprGlobal::loadTeamsFromSpreadsheet(this, wiz->saveFileLocation.canonicalPath(), rows, fileName)) {
        return;
    }
    wiz->saveFileLocation.setFile(QFileInfo(fileName).canonicalFilePath());
    spreadsheetTeams = PeerReview::teamsFromSpreadsheetRows(rows);
    spreadsheetSource = tr("Spreadsheet") + " \"" + QFileInfo(fileName).fileName() + "\"";
    spreadsheetLabel->setText(tr("Loaded:") + " " + QFileInfo(fileName).fileName());
    spreadsheetRadioButton->setChecked(true);
    setTeams(spreadsheetTeams, spreadsheetSource);
}

void PeerReviewTeamsPage::loadCanvasGroupSet()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    CanvasHandler *canvas = wiz->connectToCanvas();
    if(canvas == nullptr) {
        return;
    }
    auto showError = [this, canvas](const QString &message, QWidget *parentWidget = nullptr) {
        QString errormsg = message;
        if(!canvas->lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + canvas->lastErrorMessage.toHtmlEscaped() + "</code>";
        }
        grueprGlobal::errorMessage((parentWidget != nullptr)? parentWidget : this, tr("Error"), errormsg);
    };

    auto *busyBox = canvas->actionDialog(this);
    const QList<CanvasHandler::CanvasCourse> canvasCourses = canvas->getCourses();
    canvas->actionComplete(busyBox);
    if(canvasCourses.isEmpty()) {
        showError(tr("Canvas is responding with no courses available."));
        wiz->dropCanvasConnection();     // (perhaps the URL or token is wrong, so the next try asks for them again)
        return;
    }

    // ask for the course and the group set (a course's group sets are loaded once it's chosen)
    QDialog chooseDialog(this);
    chooseDialog.setWindowTitle(tr("Choose Canvas group set"));
    chooseDialog.setWindowIcon(CanvasHandler::icon());
    chooseDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout(&chooseDialog);
    auto *courseLabel = new QLabel(tr("Course:"), &chooseDialog);
    courseLabel->setStyleSheet(LABEL10PTSTYLE);
    auto *coursesComboBox = new StyledComboBox(&chooseDialog);
    for(const auto &canvasCourse : canvasCourses) {
        coursesComboBox->addItem(canvasCourse.name, canvasCourse.ID);
    }
    coursesComboBox->setCurrentIndex(std::max(0, coursesComboBox->findData(wiz->canvasTeamsCourseID)));
    auto *groupSetLabel = new QLabel(tr("Group set:"), &chooseDialog);
    groupSetLabel->setStyleSheet(LABEL10PTSTYLE);
    auto *groupSetsComboBox = new StyledComboBox(&chooseDialog);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &chooseDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &chooseDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &chooseDialog, &QDialog::reject);
    vLayout->addWidget(courseLabel);
    vLayout->addWidget(coursesComboBox);
    vLayout->addSpacing(10);
    vLayout->addWidget(groupSetLabel);
    vLayout->addWidget(groupSetsComboBox);
    vLayout->addWidget(buttonBox);
    bool groupSetsLoadFailed = false;
    auto loadGroupSets = [canvas, coursesComboBox, groupSetsComboBox, buttonBox, &chooseDialog, &showError, &groupSetsLoadFailed]() {
        // nothing in the dialog can be changed while the group sets load
        coursesComboBox->setEnabled(false);
        groupSetsComboBox->setEnabled(false);
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        groupSetsComboBox->clear();
        groupSetsComboBox->addItem(tr("Loading..."));
        QApplication::setOverrideCursor(Qt::BusyCursor);
        bool ok = false;
        const QList<CanvasHandler::CanvasGroupSet> groupSets = canvas->getGroupSets(coursesComboBox->currentData().toLongLong(), &ok);
        QApplication::restoreOverrideCursor();
        groupSetsComboBox->clear();
        for(const auto &groupSet : groupSets) {
            groupSetsComboBox->addItem(groupSet.name, groupSet.ID);
        }
        if(groupSets.isEmpty()) {
            groupSetsComboBox->addItem(ok? tr("This course has no group sets") : tr("The group sets could not be loaded from Canvas"));
        }
        coursesComboBox->setEnabled(true);
        groupSetsComboBox->setEnabled(!groupSets.isEmpty());
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!groupSets.isEmpty());
        groupSetsLoadFailed = !ok;
        if(!ok) {
            showError(tr("The group sets could not be loaded from Canvas."), &chooseDialog);
        }
    };
    connect(coursesComboBox, &QComboBox::currentIndexChanged, &chooseDialog, loadGroupSets);
    loadGroupSets();
    if(chooseDialog.exec() == QDialog::Rejected) {
        if(groupSetsLoadFailed) {
            wiz->dropCanvasConnection();     // (perhaps the token can't be used for this, so the next try asks for it again)
        }
        return;
    }
    const long long courseID = coursesComboBox->currentData().toLongLong();
    const QString courseName = coursesComboBox->currentText();
    const long long groupSetID = groupSetsComboBox->currentData().toLongLong();
    const QString groupSetName = groupSetsComboBox->currentText();

    // get the course's students, then the groups, showing what's being loaded and a bar with the progress
    busyBox = canvas->actionDialog(this);
    const QList<StudentRecord> roster = canvas->getStudentRoster(courseID, [canvas](int stepsDone, int numSteps){
        canvas->showActionProgress(tr("Loading the course roster..."), stepsDone, numSteps);
    });
    if(roster.isEmpty()) {
        canvas->actionComplete(busyBox);
        showError(tr("Canvas is responding with no students in this course."));
        wiz->dropCanvasConnection();     // (perhaps the token can't be used for this, so the next try asks for it again)
        return;
    }
    bool ok = false;
    const QList<CanvasHandler::CanvasGroup> groups = canvas->getGroups(groupSetID, &ok, [canvas](int groupNum, int numGroups){
        canvas->showActionProgress(tr("Loading group") + " " + QString::number(groupNum + 1) + " " + tr("of") + " " + QString::number(numGroups) + "...",
                                   groupNum, numGroups);
    });
    canvas->actionComplete(busyBox);
    if(!ok) {
        showError(tr("The groups in this group set could not be loaded from Canvas."));
        wiz->dropCanvasConnection();     // (perhaps the token can't be used for this, so the next try asks for it again)
        return;
    }

    // each group with any of the course's students in it is a team (anyone else in a group, like a TA or the Test Student, is left out)
    QHash<long long, const StudentRecord *> studentWithID;
    for(const auto &student : roster) {
        studentWithID.insert(student.LMSID, &student);
    }
    QList<PeerReviewTeam> teams;
    QSet<long long> studentsInAGroup;
    int numEmptyGroups = 0;
    long long nextID = 0;
    for(const auto &group : groups) {
        PeerReviewTeam team;
        team.name = group.name;
        for(const long long memberID : group.memberIDs) {
            const StudentRecord *student = studentWithID.value(memberID, nullptr);
            if((student != nullptr) && !studentsInAGroup.contains(memberID)) {
                studentsInAGroup.insert(memberID);
                team.members.append({-1, student->LMSID, student->firstname, student->lastname, student->email, student->section});
            }
        }
        if(team.members.isEmpty()) {
            numEmptyGroups++;
            continue;
        }
        std::sort(team.members.begin(), team.members.end(), [](const PeerReviewStudent &a, const PeerReviewStudent &b){
            return QString::localeAwareCompare(a.lastname + " " + a.firstname, b.lastname + " " + b.firstname) < 0;
        });
        for(auto &member : team.members) {
            member.ID = nextID++;
        }
        teams << team;
    }
    if(teams.isEmpty()) {
        // (this message has no formatting, so it's shown as plain text, and the name doesn't need escaping)
        grueprGlobal::errorMessage(this, tr("No teams"), groups.isEmpty()?
                                                             (tr("The group set") + " \"" + groupSetName + "\" " + tr("has no groups yet.")) :
                                                             (tr("None of the groups in") + " \"" + groupSetName + "\" " +
                                                              tr("have any of the course's students in them.")));
        return;
    }
    const int numNotInAGroup = int(roster.size() - studentsInAGroup.size());

    canvasTeams = teams;
    canvasSource = tr("Canvas course") + " \"" + courseName + "\", " + tr("group set") + " \"" + groupSetName + "\"";
    QString loadedText = tr("Loaded:") + " \"" + groupSetName + "\" " + tr("from") + " " + courseName + ".";
    if(numEmptyGroups > 0) {
        loadedText += " " + tr("%n group(s) with no students skipped.", "", numEmptyGroups);
    }
    if(numNotInAGroup > 0) {
        loadedText += " " + tr("%n student(s) in the course aren't in any group of this set.", "", numNotInAGroup);
    }
    canvasLabel->setText(loadedText);
    wiz->canvasTeamsCourseID = courseID;
    wiz->peerReview.platform = PeerReview::Platform::canvasQuizzes;     // the teams are in Canvas, so their quizzes likely will be too (still changeable on the last page)
    canvasRadioButton->setChecked(true);
    setTeams(canvasTeams, canvasSource);
}

void PeerReviewTeamsPage::setTeams(const QList<PeerReviewTeam> &newTeams, const QString &source)
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    wiz->peerReview.teams = newTeams;
    wiz->peerReview.teamsSource = source;
    updatePreview();
    emit wiz->teamsChanged();
}

void PeerReviewTeamsPage::updatePreview()
{
    const auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    const auto &teams = wiz->peerReview.teams;

    if(teams.isEmpty()) {
        previewTitle->setText(tr("No teams chosen yet"));
        teamsPreviewLabel->clear();
        return;
    }

    previewTitle->setText(QString::number(teams.size()) + " " + tr("teams") + ", " + QString::number(wiz->peerReview.numStudents()) + " " +
                          tr("students") + "<br><span style=\"font-size: 10pt;\">" + wiz->peerReview.teamsSource.toHtmlEscaped() + "</span>");
    QString teamsText;
    int numWithoutEmail = 0;
    for(const auto &team : teams) {
        teamsText += "<p><b>" + team.name.toHtmlEscaped() + "</b>";
        if(team.members.size() == 1) {
            teamsText += " <i>(" + tr("only one member") + ")</i>";
        }
        for(const auto &member : team.members) {
            teamsText += "<br>&nbsp;&nbsp;" + member.fullName().toHtmlEscaped();
            if(member.email.isEmpty()) {
                numWithoutEmail++;
            }
            else {
                teamsText += " <span style=\"color: gray;\">" + member.email.toHtmlEscaped() + "</span>";
            }
        }
        teamsText += "</p>";
    }
    if(numWithoutEmail > 0) {
        teamsText += "<p>" + tr("%n student(s) have no email address. They can still fill out the form, but their feedback "
                                "can't be shared with them as a Google Doc.", "", numWithoutEmail) + "</p>";
    }
    teamsPreviewLabel->setText(teamsText);
}

bool PeerReviewTeamsPage::validatePage()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    if(wiz->peerReview.teams.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("No teams"), tr("Please choose the teams for this peer review."));
        return false;
    }

    if(wiz->peerReview.title.isEmpty()) {
        const QString defaultTitle = tr("Peer review") + " " + QDate::currentDate().toString(QLocale::system().dateFormat(QLocale::ShortFormat));
        titleLineEdit->setText(defaultTitle);
    }

    const bool oneMemberTeam = std::any_of(wiz->peerReview.teams.cbegin(), wiz->peerReview.teams.cend(),
                                           [](const PeerReviewTeam &team){return team.members.size() < 2;});
    if(oneMemberTeam) {
        const bool continueOrNah = grueprGlobal::warningMessage(this, "Are you sure?",
                                                                tr("At least one team has only one member, so no one can give them feedback.\n"),
                                                                tr("Continue"), tr("Go back"));
        if(!continueOrNah) {
            return false;
        }
    }

    // teammates with the same name are told apart in the form by a number, in the order the team lists them
    const QStringList sameNames = wiz->peerReview.teamsWithSameNameMembers();
    if(!sameNames.isEmpty()) {
        const bool continueOrNah = grueprGlobal::warningMessage(this, "Are you sure?",
                                                                tr("These teams have two or more members with the same name, so the form numbers them:") +
                                                                    "\n\n" + sameNames.join("\n") + "\n\n" +
                                                                    tr("Please tell these students which number is theirs."),
                                                                tr("Continue"), tr("Go back"));
        if(!continueOrNah) {
            return false;
        }
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PeerReviewQuestionsPage::PeerReviewQuestionsPage(const bool writtenQuestions, QWidget *parent)
    : SurveyMakerPage(PeerReviewWizard::pageNames, writtenQuestions? PeerReviewWizard::Page::written : PeerReviewWizard::Page::rated,
                      writtenQuestions? tr("Written Questions") : tr("Rating and Choice Questions"), 0, true, parent),
    written(writtenQuestions)
{
    auto *stretch = questionLayout->takeAt(0);   // will put this back at the end of the layout after adding everything
    auto *tipFrame = new QFrame(this);
    tipFrame->setStyleSheet("background-color: " TROPICALHEX "; color: " DEEPWATERHEX ";");
    auto *tipIcon = new QLabel;
    tipIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    auto *tipLabel = new QLabel(written? tr("Comments about a teammate can be shared with them, without names, after you have read them.") :
                                         tr("Unsure of what to ask? Take a look at some example questions!"));
    tipLabel->setStyleSheet(LABEL10PTSTYLE);
    tipLabel->setWordWrap(true);
    auto *tipLayout = new QHBoxLayout(tipFrame);
    tipLayout->addWidget(tipIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);
    tipLayout->addWidget(tipLabel, 1, Qt::AlignVCenter);
    auto *sampleQuestionsButton = new QPushButton(tr("View Examples"));
    sampleQuestionsButton->setStyleSheet(EXAMPLEBUTTONSTYLE);
    sampleQuestionsDialog = new PeerReviewSampleQuestionsDialog(written, this);
    connect(sampleQuestionsButton, &QPushButton::clicked, sampleQuestionsDialog, &QDialog::show);
    connect(sampleQuestionsDialog, &PeerReviewSampleQuestionsDialog::addQuestionRequested, this, &PeerReviewQuestionsPage::addSampleQuestion);
    tipLayout->addWidget(sampleQuestionsButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(tipFrame);
    // questions are built on demand in createQuestion(), which inserts each one here rather than
    // appending, since the "create another question" button and the stretch come after them
    firstQuestionItemIndex = questionLayout->count();

    auto *addQuestionButtonFrame = new QFrame(this);
    addQuestionButtonFrame->setStyleSheet(BLUEFRAME);
    auto *addQuestionButtonLayout = new QHBoxLayout(addQuestionButtonFrame);
    addQuestionButton = new QPushButton;
    addQuestionButton->setStyleSheet(ADDBUTTONSTYLE);
    addQuestionButton->setText(tr("Create another question"));
    addQuestionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    connect(addQuestionButton, &QPushButton::clicked, this, &PeerReviewQuestionsPage::addQuestion);
    addQuestionButtonLayout->addWidget(addQuestionButton, 0, Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(addQuestionButtonFrame);
    questionLayout->addItem(stretch);
}

// build one question and its preview; questions are always created in order, lowest first
void PeerReviewQuestionsPage::createQuestion(const int questionNum)
{
    //add the question
    spacers << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionLayout->insertSpacerItem(firstQuestionItemIndex + (2 * questionNum), spacers.last());
    QFrame *questionWidget = nullptr;
    QWidget *questionPreviewWidget = nullptr;
    if(written) {
        writtenQuestions << new PeerReviewWrittenQuestion(questionNum + 1);
        questionWidget = writtenQuestions.last();
        questionPreviewWidget = writtenQuestions.last()->previewWidget;
    }
    else {
        ratedQuestions << new PeerReviewRatedQuestion(questionNum + 1);
        questionWidget = ratedQuestions.last();
        questionPreviewWidget = ratedQuestions.last()->previewWidget;
    }
    questionLayout->insertWidget(firstQuestionItemIndex + (2 * questionNum) + 1, questionWidget);
    questionWidget->hide();

    //add the preview
    questionPreviews << new QWidget;
    questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
    questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
    questionPreviewLayouts << new QVBoxLayout;
    questionPreviews.last()->setLayout(questionPreviewLayouts.last());
    const QString fillInQuestion = "[" + tr("Question") + " " + QString::number(questionNum + 1) + "]";
    questionPreviewTopLabels << new QLabel(fillInQuestion);
    questionPreviewTopLabels.last()->setStyleSheet(LABEL12PTSTYLE);
    questionPreviewTopLabels.last()->setWordWrap(true);
    questionPreviewLayouts.last()->addWidget(questionPreviewTopLabels.last());
    questionPreviewLayouts.last()->addWidget(questionPreviewWidget);
    previewSeparators << new QFrame;
    previewSeparators.last()->setStyleSheet("border-color: " DEEPWATERHEX);
    previewSeparators.last()->setLineWidth(1);
    previewSeparators.last()->setMidLineWidth(1);
    previewSeparators.last()->setFrameShape(QFrame::HLine);
    previewSeparators.last()->setFrameShadow(QFrame::Plain);
    questionPreviewLayouts.last()->addWidget(previewSeparators.last());
    previewLayout->insertWidget(questionNum, questionPreviews.last());
    questionPreviews.last()->hide();

    //connect question to delete action and to updating the preview
    const int i = questionNum;
    auto updateTopLabel = [this, i, fillInQuestion](const QString &newText) {
        questionPreviewTopLabels[i]->setText(newText.simplified().isEmpty()? fillInQuestion : newText.simplified());
    };
    if(written) {
        connect(writtenQuestions.last(), &PeerReviewWrittenQuestion::deleteRequested, this, [this, i]{deleteAQuestion(i);});
        connect(writtenQuestions.last(), &PeerReviewWrittenQuestion::questionChanged, this, updateTopLabel);
    }
    else {
        connect(ratedQuestions.last(), &PeerReviewRatedQuestion::deleteRequested, this, [this, i]{deleteAQuestion(i);});
        connect(ratedQuestions.last(), &PeerReviewRatedQuestion::questionChanged, this, updateTopLabel);
    }
}

QFrame *PeerReviewQuestionsPage::questionWidgetAt(const int questionNum) const
{
    if(written) {
        return writtenQuestions.at(questionNum);
    }
    return ratedQuestions.at(questionNum);
}

PeerReviewQuestion PeerReviewQuestionsPage::questionAt(const int questionNum) const
{
    return written? writtenQuestions.at(questionNum)->getPeerReviewQuestion() : ratedQuestions.at(questionNum)->getPeerReviewQuestion();
}

void PeerReviewQuestionsPage::setQuestionAt(const int questionNum, const PeerReviewQuestion &question)
{
    if(written) {
        writtenQuestions[questionNum]->setPeerReviewQuestion(question);
    }
    else {
        ratedQuestions[questionNum]->setPeerReviewQuestion(question);
    }
}

void PeerReviewQuestionsPage::clearQuestionAt(const int questionNum)
{
    if(written) {
        writtenQuestions[questionNum]->clear();
    }
    else {
        ratedQuestions[questionNum]->clear();
    }
}

void PeerReviewQuestionsPage::initializePage()
{
    updatePreviewNames();
}

void PeerReviewQuestionsPage::updatePreviewNames()
{
    const auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    if(wiz == nullptr) {
        return;
    }
    const QStringList names = wiz->previewNames();
    for(auto *question : std::as_const(ratedQuestions)) {
        question->setPreviewNames(names);
    }
    for(auto *question : std::as_const(writtenQuestions)) {
        question->setPreviewNames(names);
    }
}

bool PeerReviewQuestionsPage::validatePage()
{
    int questionsWOText = 0, questionsWOResponses = 0;
    for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
        const PeerReviewQuestion question = questionAt(questionNum);
        if(question.text.isEmpty()) {
            questionsWOText++;
        }
        if(((question.kind == PeerReviewQuestion::Kind::likert) || (question.kind == PeerReviewQuestion::Kind::choice)) && question.options.isEmpty()) {
            questionsWOResponses++;
        }
    }

    if(questionsWOText > 0 || questionsWOResponses > 0) {
        const QString missingInfo = ((questionsWOText > 0)? tr("text") : "") +
                                    ((questionsWOText > 0 && questionsWOResponses > 0)? tr(" or ") : "") +
                                    ((questionsWOResponses > 0)? tr("response options") : "");
        const bool continueOrNah = grueprGlobal::warningMessage(this, "Are you sure?",
                                                                tr("You have one or more questions missing\nthe question ") + missingInfo + ".\n" +
                                                                tr("These questions will be left out of the peer review."),
                                                                tr("Continue"), tr("Go back"));
        if(!continueOrNah) {
            return false;
        }
    }

    return true;
}

void PeerReviewQuestionsPage::setQuestions(const QList<PeerReviewQuestion> &newQuestions)
{
    while(numQuestions > 0) {
        deleteAQuestion(0);
    }
    for(const auto &newQuestion : newQuestions) {
        if(numQuestions == MAXQUESTIONS) {
            break;
        }
        addQuestion();
        setQuestionAt(numQuestions - 1, newQuestion);
    }
}

QList<PeerReviewQuestion> PeerReviewQuestionsPage::getQuestions() const
{
    // questions without text, or without the response options they need, are left out
    QList<PeerReviewQuestion> pageQuestions;
    for(int questionNum = 0; questionNum < numQuestions; questionNum++) {
        const PeerReviewQuestion question = questionAt(questionNum);
        if(question.text.isEmpty() ||
            (((question.kind == PeerReviewQuestion::Kind::likert) || (question.kind == PeerReviewQuestion::Kind::choice)) && question.options.isEmpty())) {
            continue;
        }
        pageQuestions << question;
    }
    return pageQuestions;
}

void PeerReviewQuestionsPage::addQuestion()
{
    const int numBuilt = written? int(writtenQuestions.size()) : int(ratedQuestions.size());
    for(int questionNum = numBuilt; questionNum <= numQuestions; questionNum++) {
        createQuestion(questionNum);
    }

    spacers[numQuestions]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionWidgetAt(numQuestions)->show();
    questionPreviews[numQuestions]->show();
    const auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    if(wiz != nullptr) {
        if(written) {
            writtenQuestions[numQuestions]->setPreviewNames(wiz->previewNames());
        }
        else {
            ratedQuestions[numQuestions]->setPreviewNames(wiz->previewNames());
        }
    }

    numQuestions++;

    if(numQuestions == MAXQUESTIONS) {
        addQuestionButton->setEnabled(false);
        addQuestionButton->setToolTip(tr("Maximum number of questions reached."));
    }
}

void PeerReviewQuestionsPage::addSampleQuestion(const PeerReviewQuestion &question)
{
    // fill in the last question if it's still blank, otherwise add a new one
    if((numQuestions == 0) || !questionAt(numQuestions - 1).text.isEmpty()) {
        if(numQuestions == MAXQUESTIONS) {
            grueprGlobal::errorMessage(sampleQuestionsDialog, tr("Maximum reached"), tr("Maximum number of questions reached."));
            return;
        }
        addQuestion();
    }
    setQuestionAt(numQuestions - 1, question);
}

void PeerReviewQuestionsPage::deleteAQuestion(int questionNum)
{
    //bump the data from every subsequent question up one
    for(int i = questionNum; i < (numQuestions - 1); i++) {
        setQuestionAt(i, questionAt(i + 1));
    }

    //clear the last question currently displayed, then hide it
    clearQuestionAt(numQuestions - 1);

    spacers[numQuestions - 1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionWidgetAt(numQuestions - 1)->hide();
    questionPreviews[numQuestions - 1]->hide();

    numQuestions--;

    if(numQuestions < MAXQUESTIONS) {
        addQuestionButton->setEnabled(true);
        addQuestionButton->setToolTip("");
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////


PeerReviewPreviewPage::PeerReviewPreviewPage(QWidget *parent)
    : SurveyMakerPage(PeerReviewWizard::pageNames, PeerReviewWizard::Page::previewcreate, tr("Peer Review Preview"), 0, false, parent)
{
    // what's in this peer review
    auto *summaryFrame = new QFrame(this);
    summaryFrame->setStyleSheet(QString(BLUEFRAME) + SCROLLBARSTYLE);
    auto *summaryLayout = new QVBoxLayout(summaryFrame);
    summaryLabel = new QLabel(summaryFrame);
    summaryLabel->setStyleSheet(LABEL12PTSTYLE);
    summaryLabel->setTextFormat(Qt::RichText);
    summaryLabel->setWordWrap(true);
    summaryLayout->addWidget(summaryLabel);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(summaryFrame);

    // a preview of the form, as seen by the first team
    auto *formFrame = new QFrame(this);
    formFrame->setStyleSheet(QString(BLUEFRAME) + SCROLLBARSTYLE);
    auto *formLayout = new QVBoxLayout(formFrame);
    formPreview = new QWidget(formFrame);
    formPreview->setStyleSheet(QString() + "QWidget{border-style:none;}" + SCROLLBARSTYLE);
    formPreview->setAttribute(Qt::WA_TransparentForMouseEvents);
    formPreview->setFocusPolicy(Qt::NoFocus);
    formPreviewLayout = new QVBoxLayout(formPreview);
    formPreviewLayout->setSpacing(6);
    formLayout->addWidget(formPreview);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(formFrame);

    // creating the form
    auto *createFrame = new QFrame;
    createFrame->setStyleSheet(QString("background-color: " DEEPWATERHEX "; color: white; font-family: 'DM Sans'; font-size: 12pt;"));
    auto *createLayout = new QGridLayout(createFrame);
    auto *createTitle = new QLabel("<span style=\"color: white; font-family:'DM Sans'; font-size:14pt;\">" + tr("Create the Peer Review:") + "</span>");
    instructionsLabel = new QLabel(tr("Instructions shown at the top of the form:"));
    instructionsLabel->setStyleSheet(QString(LABEL10PTSTYLE).replace(DEEPWATERHEX, "white"));
    instructionsTextEdit = new QPlainTextEdit;
    instructionsTextEdit->setStyleSheet(PLAINTEXTEDITSTYLE);
    instructionsTextEdit->setFixedHeight(120);
    requireSignInCheckBox = new QCheckBox(tr("Require students to sign in with Google, so gruepr can check that each student chose their own name "
                                             "(students without a Google account then can't respond)"));
    requireSignInCheckBox->setStyleSheet(QString(CHECKBOXSTYLE).replace("font-size: 10pt;}", "font-size: 10pt; color: white;}"));
    countGradeCheckBox = new QCheckBox(tr("Count completing it toward the final grade (each team's quiz is worth 1 point, graded complete/incomplete)"));
    countGradeCheckBox->setStyleSheet(QString(CHECKBOXSTYLE).replace("font-size: 10pt;}", "font-size: 10pt; color: white;}"));
    auto *destination = new QGroupBox("");
    destination->setStyleSheet("border-style:none;");
    destinationGoogle = new QRadioButton(tr("Google Form"));
    destinationCanvas = new QRadioButton(tr("Canvas quizzes (one per team)"));
    destinationCanvas->setToolTip(tr("A Classic Canvas quiz for each team, which only that team's students can see"));
    destinationGoogle->setChecked(true);
    connect(destinationGoogle, &QRadioButton::toggled, this, &PeerReviewPreviewPage::showDestinationSettings);
    auto *vbox = new QVBoxLayout;
    vbox->addWidget(destinationGoogle);
    vbox->addWidget(destinationCanvas);
    destination->setLayout(vbox);
    auto *saveQuestionSetButton = new QPushButton(tr("Save Question Set..."));
    saveQuestionSetButton->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("14pt", "12pt"));
    saveQuestionSetButton->setToolTip(tr("Save these questions to a file, to start another peer review from them"));
    connect(saveQuestionSetButton, &QPushButton::clicked, this, &PeerReviewPreviewPage::saveQuestionSet);
    createButton = new QPushButton(tr("Create Peer Review"));
    createButton->setStyleSheet(NEXTBUTTONSTYLE);
    connect(createButton, &QPushButton::clicked, this, &PeerReviewPreviewPage::createPeerReview);
    int row = 0;
    createLayout->addWidget(createTitle, row++, 0, 1, -1, Qt::AlignLeft);
    createLayout->addWidget(instructionsLabel, row++, 0, 1, -1);
    createLayout->addWidget(instructionsTextEdit, row++, 0, 1, -1);
    createLayout->addWidget(destination, row++, 0, 1, -1);
    createLayout->addWidget(requireSignInCheckBox, row++, 0, 1, -1);
    createLayout->addWidget(countGradeCheckBox, row++, 0, 1, -1);
    createLayout->addWidget(saveQuestionSetButton, row, 0, 1, 1);
    createLayout->addWidget(createButton, row++, 1, 1, 1);
    createLayout->setColumnStretch(1, 1);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(createFrame);

    questionLayout->addStretch(1);
}

void PeerReviewPreviewPage::initializePage()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    wiz->previewPageVisited = true;
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    auto &peerReview = wiz->peerReview;
    peerReview.questions = wiz->currentQuestions();

    instructionsTextEdit->setPlainText(peerReview.instructions);
    requireSignInCheckBox->setChecked(peerReview.requireSignIn);
    countGradeCheckBox->setChecked(peerReview.countTowardFinalGrade);
    destinationCanvas->setChecked(peerReview.platform == PeerReview::Platform::canvasQuizzes);
    destinationGoogle->setChecked(peerReview.platform == PeerReview::Platform::googleForm);
    showDestinationSettings();

    int numShared = 0;
    for(const auto &question : std::as_const(peerReview.questions)) {
        if(question.shareWith != PeerReviewQuestion::ShareWith::instructor) {
            numShared++;
        }
    }
    summaryLabel->setText("<b>" + peerReview.title.toHtmlEscaped() + "</b><br>" +
                          QString::number(peerReview.teams.size()) + " " + tr("teams") + ", " + QString::number(peerReview.numStudents()) + " " +
                          tr("students") + " (" + peerReview.teamsSource.toHtmlEscaped() + ")<br>" +
                          QString::number(peerReview.questions.size()) + " " + tr("questions") + ", " + tr("of which") + " " +
                          QString::number(numShared) + " " + tr("can be shared with students after you review their answers"));

    // the preview: the name question, then the first team's page
    formPreview->setUpdatesEnabled(false);
    grueprGlobal::clearLayout(formPreviewLayout);
    auto *formTitle = new QLabel(peerReview.title.toHtmlEscaped());
    formTitle->setStyleSheet(LABEL14PTSTYLE);
    formPreviewLayout->addWidget(formTitle);
    auto *nameLabel = new QLabel(PEERREVIEWNAMEQUESTION);
    nameLabel->setStyleSheet(LABEL12PTSTYLE);
    formPreviewLayout->addWidget(nameLabel);
    auto *nameComboBox = new StyledComboBox;
    for(const auto &nameOption : peerReview.nameOptions()) {
        nameComboBox->addItem(nameOption.first);
    }
    formPreviewLayout->addWidget(nameComboBox, 0, Qt::AlignLeft);
    if(!peerReview.teams.isEmpty()) {
        auto *teamPageLabel = new QLabel("<br><b>" + tr("Then, for example, the page for") + " " + peerReview.teams.constFirst().name.toHtmlEscaped() + ":</b>");
        teamPageLabel->setStyleSheet(LABEL12PTSTYLE);
        formPreviewLayout->addWidget(teamPageLabel);
    }
    const QStringList names = wiz->previewNames();
    for(const auto &question : std::as_const(peerReview.questions)) {
        auto *questionLabel = new QLabel(question.text.toHtmlEscaped());
        questionLabel->setStyleSheet(LABEL12PTSTYLE);
        questionLabel->setWordWrap(true);
        formPreviewLayout->addSpacing(10);
        formPreviewLayout->addWidget(questionLabel);
        auto *questionPreview = new QWidget;
        auto *questionPreviewLayout = new QVBoxLayout(questionPreview);
        questionPreviewLayout->setContentsMargins(20, 0, 0, 0);
        PeerReviewPreview::fillPreview(questionPreviewLayout, question, names);
        formPreviewLayout->addWidget(questionPreview);
    }
    formPreview->setUpdatesEnabled(true);
    formPreview->update();

    createButton->setEnabled(!peerReview.questions.isEmpty() && !peerReview.teams.isEmpty());
}

void PeerReviewPreviewPage::cleanupPage()
{
    // going back to previous page
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    wiz->button(QWizard::NextButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: " DEEPWATERHEX "; "));
    wiz->peerReview.instructions = instructionsTextEdit->toPlainText().trimmed();
    wiz->peerReview.requireSignIn = requireSignInCheckBox->isChecked();
    wiz->peerReview.countTowardFinalGrade = countGradeCheckBox->isChecked();
    wiz->peerReview.platform = destinationCanvas->isChecked()? PeerReview::Platform::canvasQuizzes : PeerReview::Platform::googleForm;
}

void PeerReviewPreviewPage::showDestinationSettings()
{
    const bool toCanvas = destinationCanvas->isChecked();
    instructionsLabel->setText(toCanvas? tr("Instructions shown at the top of each team's quiz:") : tr("Instructions shown at the top of the form:"));
    // the default instructions for a Google Form tell students to choose their name, which they don't do in Canvas (unedited, they're swapped)
    const QString instructions = instructionsTextEdit->toPlainText().trimmed();
    if(toCanvas && (instructions == QString(PEERREVIEWINSTRUCTIONS).trimmed())) {
        instructionsTextEdit->setPlainText(PEERREVIEWCANVASINSTRUCTIONS);
    }
    else if(!toCanvas && (instructions == QString(PEERREVIEWCANVASINSTRUCTIONS).trimmed())) {
        instructionsTextEdit->setPlainText(PEERREVIEWINSTRUCTIONS);
    }
    requireSignInCheckBox->setVisible(!toCanvas);       // (Canvas already knows who each student is)
    countGradeCheckBox->setVisible(toCanvas);
}

void PeerReviewPreviewPage::saveQuestionSet()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), wiz->saveFileLocation.canonicalPath(),
                                                          tr(PeerReview::QUESTIONSETFILEFILTER) + ";;" + tr("All Files (*)"));
    if(fileName.isEmpty()) {
        return;
    }
    if(!PeerReview::saveQuestionSet(fileName, wiz->peerReview.questions)) {
        grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("This question set was not saved.\nThere was an issue writing the file to disk."));
        return;
    }
    wiz->saveFileLocation.setFile(QFileInfo(fileName).canonicalFilePath());
}

void PeerReviewPreviewPage::createPeerReview()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    auto &peerReview = wiz->peerReview;
    peerReview.instructions = instructionsTextEdit->toPlainText().trimmed();
    peerReview.requireSignIn = requireSignInCheckBox->isChecked();
    peerReview.countTowardFinalGrade = countGradeCheckBox->isChecked();

    if(!grueprGlobal::internetIsGood()) {
        return;
    }

    if(destinationCanvas->isChecked()) {
        createCanvasQuizzes();
    }
    else {
        createGoogleForm();
    }
}

QStringList PeerReviewPreviewPage::matchToCanvasRoster(const QList<StudentRecord> &roster)
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    QSet<long long> rosterIDs;
    QHash<QString, QList<long long>> rosterIDsOfEmail, rosterIDsOfName;
    for(const auto &student : roster) {
        rosterIDs.insert(student.LMSID);
        if(!student.email.trimmed().isEmpty()) {
            rosterIDsOfEmail[student.email.trimmed().toLower()] << student.LMSID;
        }
        rosterIDsOfName[(student.firstname + " " + student.lastname).simplified().toLower()] << student.LMSID;
    }

    // each member's match (-1 if none), then how many members each Canvas student was matched to
    QHash<long long, int> numMatched;
    for(auto &team : wiz->peerReview.teams) {
        for(auto &member : team.members) {
            if((member.LMSID > 0) && rosterIDs.contains(member.LMSID)) {
                numMatched[member.LMSID]++;
                continue;
            }
            const QList<long long> byEmail = rosterIDsOfEmail.value(member.email.trimmed().toLower());
            const QList<long long> byName = rosterIDsOfName.value(member.fullName().toLower());
            if(!member.email.trimmed().isEmpty() && (byEmail.size() == 1)) {
                member.LMSID = byEmail.constFirst();
            }
            else if(byName.size() == 1) {
                member.LMSID = byName.constFirst();
            }
            else {
                member.LMSID = -1;
                continue;
            }
            numMatched[member.LMSID]++;
        }
    }

    // a Canvas student matched to more than one member (e.g., two students of the same name, and only one of them in the course)
    // can't tell which one they are, so neither is matched
    QStringList unmatched;
    for(auto &team : wiz->peerReview.teams) {
        for(auto &member : team.members) {
            if(member.LMSID == -1) {
                unmatched << member.fullName() + " (" + team.name + ")";
            }
            else if(numMatched.value(member.LMSID) > 1) {
                unmatched << member.fullName() + " (" + team.name + ", " + tr("more than one student matches the same student in Canvas") + ")";
                member.LMSID = -1;
            }
        }
    }
    return unmatched;
}

void PeerReviewPreviewPage::createCanvasQuizzes()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    auto &peerReview = wiz->peerReview;

    //connect to Canvas (the wizard keeps the connection, so if it was made while getting the teams, there's no signing in again)
    CanvasHandler *canvas = wiz->connectToCanvas();
    if(canvas == nullptr) {
        return;
    }

    //ask the user in which course to create the quizzes
    auto *busyBox = canvas->actionDialog(this);
    const QList<CanvasHandler::CanvasCourse> canvasCourses = canvas->getCourses();
    canvas->actionComplete(busyBox);
    if(canvasCourses.isEmpty()) {
        QString errormsg = tr("Canvas is responding with no courses available.");
        if(!canvas->lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + canvas->lastErrorMessage + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        wiz->dropCanvasConnection();     // (perhaps the URL or token is wrong, so the next try asks for them again)
        return;
    }
    QDialog canvasCoursesDialog(this);
    canvasCoursesDialog.setWindowTitle(tr("Choose Canvas course"));
    canvasCoursesDialog.setWindowIcon(CanvasHandler::icon());
    canvasCoursesDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *vLayout = new QVBoxLayout(&canvasCoursesDialog);
    auto *label = new QLabel(tr("In which course should the team quizzes be created?"), &canvasCoursesDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *coursesComboBox = new StyledComboBox(&canvasCoursesDialog);
    for(const auto &canvasCourse : canvasCourses) {
        coursesComboBox->addItem(canvasCourse.name, canvasCourse.ID);
    }
    coursesComboBox->setCurrentIndex(std::max(0, coursesComboBox->findData(wiz->canvasTeamsCourseID)));    // the course the teams came from, if any
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &canvasCoursesDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &canvasCoursesDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &canvasCoursesDialog, &QDialog::reject);
    vLayout->addWidget(label);
    vLayout->addWidget(coursesComboBox);
    vLayout->addWidget(buttonBox);
    if(canvasCoursesDialog.exec() == QDialog::Rejected) {
        return;
    }
    const long long courseID = coursesComboBox->currentData().toLongLong();
    const QString courseName = coursesComboBox->currentText();

    //match the students to the course's roster, so each team's quiz can be shown to just its students
    busyBox = canvas->actionDialog(this);
    const QList<StudentRecord> roster = canvas->getStudentRoster(courseID, [canvas](int stepsDone, int numSteps){
        canvas->showActionProgress(tr("Loading the course roster..."), stepsDone, numSteps);
    });
    canvas->actionComplete(busyBox);
    if(roster.isEmpty()) {
        QString errormsg = tr("Canvas is responding with no students in this course.");
        if(!canvas->lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + canvas->lastErrorMessage + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        wiz->dropCanvasConnection();     // (perhaps the token can't be used for this, so the next try asks for it again)
        return;
    }
    const QStringList unmatched = matchToCanvasRoster(roster);
    if(!unmatched.isEmpty()) {
        const QString message = tr("These students weren't found in the roster of") + " " + courseName + tr(", so they won't be able to take "
                                   "their team's quiz:") + "\n\n" + unmatched.join("\n");
        if(!grueprGlobal::warningMessage(this, "gruepr", message, tr("Create Anyway"), tr("Cancel"))) {
            return;
        }
    }

    //create the quizzes, showing which team's quiz is being made and a bar with the progress (each request to Canvas is a step)
    busyBox = canvas->actionDialog(this);
    const int numTeams = int(peerReview.teams.size());
    const bool fail = !canvas->createPeerReviewQuizzes(courseID, &peerReview, [canvas, numTeams](int teamNum, int stepsDone, int numSteps){
        canvas->showActionProgress(tr("Creating the quiz for team") + " " + QString::number(teamNum + 1) + " " + tr("of") + " " + QString::number(numTeams) + "...",
                                   stepsDone, numSteps);
    });
    const QString lastErrorMessage = canvas->lastErrorMessage;
    canvas->hideActionProgress();

    const QPixmap resultIcon(fail? ":/icons_new/error.png" : ":/icons_new/ok.png");
    const QSize iconSize = canvas->actionDialogIcon->size();
    canvas->actionDialogIcon->setPixmap(resultIcon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    canvas->actionDialogLabel->setText(fail? (tr("Error. Peer review not created.") +
                                              (lastErrorMessage.isEmpty() ? "" : ("<br><br><code>" + lastErrorMessage + "</code>"))) :
                                             tr("Peer review created"));
    QEventLoop loop;
    busyBox->adjustSize();
    QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
    loop.exec();
    canvas->actionComplete(busyBox);

    if(fail) {
        QString errormsg = tr("Error. Peer review not created. Any quizzes that had already been made in Canvas were deleted.");
        if(!lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        wiz->dropCanvasConnection();     // (perhaps the token can't be used for this, so the next try asks for it again)
        return;
    }

    peerReview.canvasCourseName = courseName;
    peerReview.createdTime = QDateTime::currentDateTime();
    if(!peerReview.save()) {
        grueprGlobal::errorMessage(this, tr("Error"), tr("The quizzes were created, but gruepr could not save this peer review on your computer, "
                                                         "so it can't load their results.\nThere was an issue writing the file to disk."));
    }
    wiz->peerReviewHasBeenCreated = true;

    const int numQuizzes = int(std::count_if(peerReview.canvasQuizzes.cbegin(), peerReview.canvasQuizzes.cend(),
                                             [](const PeerReview::CanvasTeamQuiz &canvasQuiz){return canvasQuiz.quizID != -1;}));
    auto *successDialog = new QMessageBox(this);
    successDialog->setStyleSheet(QString(LABEL10PTSTYLE) + SMALLBUTTONSTYLE);
    successDialog->setTextFormat(Qt::RichText);
    successDialog->setTextInteractionFlags(Qt::TextBrowserInteraction);
    const QString textheight = QString::number(successDialog->fontMetrics().boundingRect('G').height() * 2);
    QString successText = tr("Success!") + " " + QString::number(numQuizzes) + " " + tr("team quizzes were created in") + " <a href='" +
                          peerReview.canvasCourseURL + "/quizzes'>" + courseName.toHtmlEscaped() + "</a> <img height=\"" + textheight +
                          "\" width=\"" + textheight + R"(" src=":/icons_new/external-link.png">.<br><br>)";
    if(numQuizzes < numTeams) {
        successText += QString::number(numTeams - numQuizzes) + " " + tr("team(s) got no quiz, since none of their students are in the course.") + "<br><br>";
    }
    successText += tr("Each student sees only their own team's quiz, in Canvas under Quizzes (and in their To Do list). "
                      "Completing it earns 1 point, graded complete/incomplete") +
                   (peerReview.countTowardFinalGrade? QString(".") : (", " + tr("which doesn't count toward the final grade."))) + " " +
                   tr("The quizzes are in the course's assignment group") + " \"" + CanvasHandler::PEERREVIEWASSIGNMENTGROUP.toHtmlEscaped() + "\", " +
                   tr("which holds every peer review's quizzes in the course.") + "<br><br>" +
                   tr("Please don't change the quizzes' questions, since gruepr matches the answers to the questions it created. "
                      "You can still change their titles, instructions, and dates.<br><br>"
                      "To see who has responded, and to provide feedback to students, choose this peer review from the list "
                      "and click \"Load Results\".");
    successDialog->setText(successText);
    successDialog->setStandardButtons(QMessageBox::Ok);
    successDialog->exec();
    successDialog->deleteLater();

    wiz->accept();
}

void PeerReviewPreviewPage::createGoogleForm()
{
    auto *wiz = qobject_cast<PeerReviewWizard *>(wizard());
    auto &peerReview = wiz->peerReview;
    peerReview.platform = PeerReview::Platform::googleForm;

    //create googleHandler and authenticate
    auto *google = new GoogleHandler(this);
    if(!google->authenticate()) {
        google->deleteLater();
        return;
    }

    //create the form
    auto *busyBox = google->actionDialog(this);
    const auto form = google->createPeerReviewForm(&peerReview);
    const bool fail = form.name.isEmpty();
    const QString lastErrorMessage = google->lastErrorMessage;

    const QPixmap resultIcon(fail? ":/icons_new/error.png" : ":/icons_new/ok.png");
    const QSize iconSize = google->actionDialogIcon->size();
    google->actionDialogIcon->setPixmap(resultIcon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    const QString resultText = fail ? (tr("Error. Peer review not created.") +
                                       (lastErrorMessage.isEmpty() ? "" : ("<br><br><code>" + lastErrorMessage + "</code>")))
                                    : tr("Peer review created");
    google->actionDialogLabel->setText(resultText);
    QEventLoop loop;
    busyBox->adjustSize();
    QTimer::singleShot(UI_DISPLAY_DELAYTIME, &loop, &QEventLoop::quit);
    loop.exec();
    google->actionComplete(busyBox);
    google->deleteLater();

    if(fail) {
        QString errormsg = tr("Error. Peer review not created.");
        if(!lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + lastErrorMessage + "</code>";
        }
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        return;
    }

    peerReview.createdTime = QDateTime::currentDateTime();
    if(!peerReview.save()) {
        grueprGlobal::errorMessage(this, tr("Error"), tr("The form was created, but gruepr could not save this peer review on your computer, "
                                                         "so it can't load its results.\nThere was an issue writing the file to disk."));
    }
    wiz->peerReviewHasBeenCreated = true;

    auto *successDialog = new QMessageBox(this);
    successDialog->setStyleSheet(QString(LABEL10PTSTYLE) + SMALLBUTTONSTYLE);
    successDialog->setTextFormat(Qt::RichText);
    successDialog->setTextInteractionFlags(Qt::TextBrowserInteraction);
    const QString textheight = QString::number(successDialog->fontMetrics().boundingRect('G').height() * 2);
    QString successText;
    if(form.published) {
        successText = tr("Success! The peer review form was created in your") + " <a href='https://drive.google.com'>Google Drive</a> "
                      "<img height=\"" + textheight + "\" width=\"" + textheight + R"(" src=":/icons_new/external-link.png">.<br><br>)";
    }
    else {
        successText = tr("The peer review form was created, but it could not be published, so students cannot respond to it yet.<br><br>"
                         "Please open it in your") + " <a href='https://drive.google.com'>Google Drive</a> "
                      "<img height=\"" + textheight + "\" width=\"" + textheight + R"(" src=":/icons_new/external-link.png">)" +
                      tr(", find this form, and click <strong>Publish</strong> before sending the link below to your students.<br><br>");
    }
    successText += tr("Students should fill out the peer review by going to the following URL:<br><strong>") + form.responderURL.toEncoded() +
                   tr("</strong><br>Each student chooses their name, and then answers about their own team.<br><br>"
                      "Please don't change the questions in the form, since gruepr matches the answers to the questions it created. "
                      "You can still change the form's title and instructions.<br><br>"
                      "To see who has responded, and to provide feedback to students, choose this peer review from the list "
                      "and click \"Load Results\".<br><br>You can copy the URL to your clipboard with the button below.");
    if(!form.warning.isEmpty()) {
        successText += "<br><br><b>" + tr("Note:") + "</b> " + form.warning;
    }
    successDialog->setText(successText);
    successDialog->setStandardButtons(QMessageBox::Ok);
    auto *copyButton = successDialog->addButton(tr("Copy URL to clipboard"), QMessageBox::ResetRole);
    copyButton->setStyleSheet(copyButton->styleSheet() + QString(BIGTOOLTIPSTYLE).replace("background-color: white;", "background-color: green;"));
    copyButton->disconnect();     // disconnect the button from all slots so that it doesn't close the dialog when clicked
    connect(copyButton, &QPushButton::clicked, successDialog, [&form, &copyButton](){
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(form.responderURL.toEncoded());
        QToolTip::showText(copyButton->mapToGlobal(QPoint(0, 0)),
                           tr("URL copied"), copyButton, QRect(),
                           UI_DISPLAY_DELAYTIME);
    });
    successDialog->exec();
    successDialog->deleteLater();

    wiz->accept();
}
