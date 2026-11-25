#include "feedbackReleaseDialog.h"
#include "gruepr_globals.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include <QBuffer>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPageSize>
#include <QPdfWriter>
#include <QProgressDialog>
#include <QPushButton>
#include <QSplitter>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to review each student's peer review feedback report, edit it, and save or send it
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// puts a text box inside a white frame with a border, so it clearly looks like a box to type in
// (the border is drawn by the frame because it isn't reliably drawn on a QPlainTextEdit itself)
QFrame *FeedbackReleaseDialog::framed(QPlainTextEdit *textEdit, const int height, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName("textBoxFrame");
    frame->setStyleSheet(TEXTBOXFRAMESTYLE);
    frame->setFixedHeight(height);
    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(2, 2, 2, 2);
    textEdit->setParent(frame);
    textEdit->setStyleSheet(TEXTBOXSTYLE);
    textEdit->setFrameShape(QFrame::NoFrame);
    frameLayout->addWidget(textEdit);
    return frame;
}

FeedbackReleaseDialog::FeedbackReleaseDialog(PeerReview &peerReview, const PeerReviewResults &results, QWidget *parent)
    : QDialog(parent),
    peerReview(peerReview),
    results(results)
{
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle(tr("gruepr - Provide feedback to students") + " - " + peerReview.title);
    setStyleSheet(QString(WHITEDIALOGSTYLE) + CHECKBOXSTYLE + SPINBOXSTYLE + SCROLLBARSTYLE);
    setSizeGripEnabled(true);
    resize(SCREENWIDTH * 3 / 4, SCREENHEIGHT * 3 / 4);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // left side: what goes into every report, and the list of students with what has been done with each one's report
    auto *leftWidget = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    auto *introLabel = new QLabel(tr("Introduction at the top of every report:"), leftWidget);
    introLabel->setStyleSheet(LABEL10PTSTYLE);
    introTextEdit = new QPlainTextEdit(leftWidget);
    introTextEdit->setPlainText(peerReview.release.introText);
    auto *introFrame = framed(introTextEdit, 110, leftWidget);
    connect(introTextEdit, &QPlainTextEdit::textChanged, this, [this](){
        this->peerReview.release.introText = introTextEdit->toPlainText();
        refreshPreview();
    });
    auto *resetIntroButton = new QPushButton(tr("Reset to Default Introduction"), leftWidget);
    resetIntroButton->setStyleSheet(QString(SMALLBUTTONSTYLEINVERTED).replace("12pt", "10pt"));
    connect(resetIntroButton, &QPushButton::clicked, this, [this](){introTextEdit->setPlainText(PEERREVIEWREPORTINTRO);});
    auto *minimumLayout = new QHBoxLayout;
    auto *minimumLabel = new QLabel(tr("Leave out unnamed ratings and comments when fewer than"), leftWidget);
    minimumLabel->setStyleSheet(LABEL10PTSTYLE);
    minimumLabel->setWordWrap(true);
    minimumSpinBox = new QSpinBox(leftWidget);
    minimumSpinBox->setRange(1, 10);
    minimumSpinBox->setValue(peerReview.release.minimumResponders);
    minimumSpinBox->setToolTip(tr("With just one teammate responding, a student could tell who said what"));
    auto *minimumLabel2 = new QLabel(tr("teammates responded"), leftWidget);
    minimumLabel2->setStyleSheet(LABEL10PTSTYLE);
    minimumLayout->addWidget(minimumLabel, 1);
    minimumLayout->addWidget(minimumSpinBox);
    minimumLayout->addWidget(minimumLabel2);
    connect(minimumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value){
        this->peerReview.release.minimumResponders = value;
        refreshStudentList();
        refreshCurrentStudent();
    });
    teamAverageCheckBox = new QCheckBox(tr("Include the team's average beside each rating"), leftWidget);
    teamAverageCheckBox->setChecked(peerReview.release.includeTeamAverage);
    connect(teamAverageCheckBox, &QCheckBox::toggled, this, [this](bool checked){this->peerReview.release.includeTeamAverage = checked; refreshPreview();});
    factorCheckBox = new QCheckBox(tr("Include the student's adjustment factor"), leftWidget);
    factorCheckBox->setChecked(peerReview.release.includeAdjustmentFactor);
    connect(factorCheckBox, &QCheckBox::toggled, this, [this](bool checked){this->peerReview.release.includeAdjustmentFactor = checked; refreshPreview();});
    auto *studentsLabel = new QLabel(tr("Students (uncheck a student to hold back their report):"), leftWidget);
    studentsLabel->setStyleSheet(LABEL10PTSTYLE);
    studentsTable = new QTableWidget(leftWidget);
    studentsTable->setColumnCount(3);
    studentsTable->setHorizontalHeaderLabels({tr("Team"), tr("Student"), tr("Report")});
    studentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    studentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    studentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    studentsTable->verticalHeader()->hide();
    studentsTable->horizontalHeader()->setHighlightSections(false);
    studentsTable->horizontalHeader()->setStretchLastSection(true);
    studentsTable->setStyleSheet(QString(TABLESTYLE) + SCROLLBARSTYLE);
    connect(studentsTable, &QTableWidget::itemSelectionChanged, this, &FeedbackReleaseDialog::refreshCurrentStudent);
    connect(studentsTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item){
        if(refreshing || (item->column() != 0)) {
            return;
        }
        const long long studentID = item->data(Qt::UserRole).toLongLong();
        if(item->checkState() == Qt::Checked) {
            this->peerReview.release.heldBack.remove(studentID);
        }
        else {
            this->peerReview.release.heldBack.insert(studentID);
        }
        QTimer::singleShot(0, this, &FeedbackReleaseDialog::refreshStudentList);     // after this signal is done with the item, since the refresh replaces it
    });
    leftLayout->addWidget(introLabel);
    leftLayout->addWidget(introFrame);
    leftLayout->addWidget(resetIntroButton, 0, Qt::AlignLeft);
    leftLayout->addLayout(minimumLayout);
    leftLayout->addWidget(teamAverageCheckBox);
    leftLayout->addWidget(factorCheckBox);
    leftLayout->addSpacing(10);
    leftLayout->addWidget(studentsLabel);
    leftLayout->addWidget(studentsTable, 1);

    // right side: the chosen student's report and their personal note (the comments are edited in the results window's Shared Comments tab)
    auto *rightWidget = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    auto *previewLabel = new QLabel(tr("This student's report, exactly as they will see it:"), rightWidget);
    previewLabel->setStyleSheet(LABEL10PTSTYLE);
    reportPreview = new QTextBrowser(rightWidget);
    reportPreview->setStyleSheet(QString("QTextBrowser{background-color: white; font-family: 'DM Sans'; font-size: 10pt; color: black; "
                                         "border: 1px solid lightGray;}") + SCROLLBARSTYLE);
    auto *noteLabel = new QLabel(tr("A personal note to this student (optional):"), rightWidget);
    noteLabel->setStyleSheet(LABEL10PTSTYLE);
    noteTextEdit = new QPlainTextEdit(rightWidget);
    auto *noteFrame = framed(noteTextEdit, 70, rightWidget);
    connect(noteTextEdit, &QPlainTextEdit::textChanged, this, [this](){
        if(refreshing) {
            return;
        }
        const long long studentID = currentStudentID();
        const QString note = noteTextEdit->toPlainText();
        if(note.trimmed().isEmpty()) {
            this->peerReview.release.personalNotes.remove(studentID);
        }
        else {
            this->peerReview.release.personalNotes.insert(studentID, note);
        }
        refreshPreview();
    });
    overrideCheckBox = new QCheckBox(tr("Include this student's unnamed ratings and comments even though few teammates responded"), rightWidget);
    connect(overrideCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        if(refreshing) {
            return;
        }
        if(checked) {
            this->peerReview.release.minimumOverridden.insert(currentStudentID());
        }
        else {
            this->peerReview.release.minimumOverridden.remove(currentStudentID());
        }
        refreshStudentList();
        refreshPreview();
    });
    rightLayout->addWidget(previewLabel);
    rightLayout->addWidget(reportPreview, 1);
    rightLayout->addWidget(noteLabel);
    rightLayout->addWidget(noteFrame);
    rightLayout->addWidget(overrideCheckBox);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    // the ways to save or send the reports
    auto *pdfButton = new QPushButton(tr("Save PDFs..."), this);
    pdfButton->setStyleSheet(SMALLBUTTONSTYLE);
    pdfButton->setToolTip(tr("A PDF file for each student, plus one PDF of all the reports for printing"));
    connect(pdfButton, &QPushButton::clicked, this, &FeedbackReleaseDialog::savePDFs);
    auto *mailMergeButton = new QPushButton(tr("Save Mail Merge File..."), this);
    mailMergeButton->setStyleSheet(SMALLBUTTONSTYLE);
    mailMergeButton->setToolTip(tr("A spreadsheet with each student's email and report, for a mail merge in Outlook, Word, or Gmail"));
    connect(mailMergeButton, &QPushButton::clicked, this, &FeedbackReleaseDialog::saveMailMerge);
    auto *googleDocsButton = new QPushButton(tr("Send as Google Docs..."), this);
    googleDocsButton->setStyleSheet(SMALLBUTTONSTYLE);
    googleDocsButton->setToolTip(tr("Create a Google Doc of each report in your Google Drive and share it with that student, view only; "
                                    "Google emails each student the link"));
    connect(googleDocsButton, &QPushButton::clicked, this, &FeedbackReleaseDialog::shareGoogleDocs);
    QPushButton *canvasCommentsButton = nullptr;
    if(peerReview.platform == PeerReview::Platform::canvasQuizzes) {
        canvasCommentsButton = new QPushButton(tr("Post as Canvas Comments..."), this);
        canvasCommentsButton->setStyleSheet(SMALLBUTTONSTYLE);
        canvasCommentsButton->setToolTip(tr("Post each report as a comment on the student's team quiz in Canvas, with a PDF of the report "
                                            "attached; Canvas notifies each student"));
        connect(canvasCommentsButton, &QPushButton::clicked, this, &FeedbackReleaseDialog::postCanvasComments);
    }
    auto *closeButton = new QPushButton(tr("Close"), this);
    closeButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *buttonLayout = new QHBoxLayout;
    auto *reportsLabel = new QLabel(tr("Checked students' reports:"), this);
    reportsLabel->setStyleSheet(LABEL10PTSTYLE);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(reportsLabel);
    buttonLayout->addWidget(pdfButton);
    buttonLayout->addWidget(mailMergeButton);
    buttonLayout->addWidget(googleDocsButton);
    if(canvasCommentsButton != nullptr) {
        buttonLayout->addWidget(canvasCommentsButton);
    }

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter, 1);
    mainLayout->addLayout(buttonLayout);

    refreshStudentList();
    if(studentsTable->rowCount() > 0) {
        studentsTable->selectRow(0);
    }
}

long long FeedbackReleaseDialog::currentStudentID() const
{
    const int row = studentsTable->currentRow();
    if((row < 0) || (studentsTable->item(row, 0) == nullptr)) {
        return -1;
    }
    return studentsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
}

void FeedbackReleaseDialog::refreshStudentList()
{
    refreshing = true;
    const int selectedRow = studentsTable->currentRow();
    studentsTable->setRowCount(int(results.students.size()));
    int row = 0;
    for(const auto &studentResult : results.students) {
        const auto *thisStudent = peerReview.student(studentResult.ID);
        auto *teamItem = new QTableWidgetItem(peerReview.teams.at(studentResult.teamNum).name);
        teamItem->setData(Qt::UserRole, studentResult.ID);
        teamItem->setFlags(teamItem->flags() | Qt::ItemIsUserCheckable);
        teamItem->setCheckState(peerReview.release.heldBack.contains(studentResult.ID)? Qt::Unchecked : Qt::Checked);
        studentsTable->setItem(row, 0, teamItem);
        studentsTable->setItem(row, 1, new QTableWidgetItem((thisStudent != nullptr)? thisStudent->fullName() : ""));

        // the report's status, then when it was last saved or sent each way (or just "Ready" if none of these)
        QStringList status;
        if(peerReview.release.heldBack.contains(studentResult.ID)) {
            status << tr("Held back");
        }
        else if(peerReview.feedbackIsWithheld(results, studentResult.ID)) {
            status << tr("Ratings and comments left out: %n teammate(s) responded", "", studentResult.numRaters);
        }
        QStringList methods;
        QHash<QString, QDateTime> latestTimes;
        for(const auto &entry : std::as_const(peerReview.release.log)) {
            if(entry.studentID == studentResult.ID) {
                if(!methods.contains(entry.method)) {
                    methods << entry.method;
                }
                latestTimes.insert(entry.method, entry.time);   // the log is in time order, so this ends up as the latest
            }
        }
        for(const auto &method : std::as_const(methods)) {
            status << doneText(method) + " " + latestTimes.value(method).toString(QLocale::system().dateFormat(QLocale::ShortFormat));
        }
        if(status.isEmpty()) {
            status << tr("Ready");
        }
        studentsTable->setItem(row, 2, new QTableWidgetItem(status.join("; ")));
        row++;
    }
    studentsTable->resizeColumnsToContents();
    if((selectedRow >= 0) && (selectedRow < studentsTable->rowCount())) {
        studentsTable->selectRow(selectedRow);
    }
    refreshing = false;
}

void FeedbackReleaseDialog::refreshCurrentStudent()
{
    if(refreshing) {
        return;
    }
    refreshing = true;
    const long long studentID = currentStudentID();
    const auto *studentResult = results.result(studentID);

    noteTextEdit->setPlainText(peerReview.release.personalNotes.value(studentID));
    const bool fewResponders = (studentResult != nullptr) && (studentResult->numRaters < peerReview.release.minimumResponders);
    overrideCheckBox->setVisible(fewResponders);
    overrideCheckBox->setChecked(peerReview.release.minimumOverridden.contains(studentID));

    refreshing = false;
    refreshPreview();
}

void FeedbackReleaseDialog::refreshPreview()
{
    const long long studentID = currentStudentID();
    if(studentID == -1) {
        reportPreview->clear();
        return;
    }
    reportPreview->setHtml(peerReview.reportHtml(results, studentID));
}

QString FeedbackReleaseDialog::doneText(const QString &method)
{
    if(method == methodName(Method::pdf)) {
        return tr("PDF saved");
    }
    if(method == methodName(Method::mailMerge)) {
        return tr("Mail merge file saved");
    }
    if(method == methodName(Method::googleDoc)) {
        return tr("Google Doc sent");
    }
    if(method == tr("Google Doc (not shared)")) {
        return tr("Google Doc created but not shared");
    }
    if(method == methodName(Method::canvasComment)) {
        return tr("Canvas comment posted");
    }
    return method;
}

QList<long long> FeedbackReleaseDialog::studentsToDeliver() const
{
    QList<long long> studentIDs;
    for(const auto &studentResult : results.students) {
        if(!peerReview.release.heldBack.contains(studentResult.ID)) {
            studentIDs << studentResult.ID;
        }
    }
    return studentIDs;
}

QString FeedbackReleaseDialog::methodName(const Method method)
{
    switch(method) {
    case Method::pdf:
        return tr("PDF");
    case Method::mailMerge:
        return tr("Mail merge");
    case Method::googleDoc:
        return tr("Google Doc");
    case Method::canvasComment:
        return tr("Canvas comment");
    }
    return {};
}

bool FeedbackReleaseDialog::confirmDelivery(const QList<long long> &studentIDs, const Method method)
{
    if(studentIDs.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("No students"), tr("No students are checked."));
        return false;
    }
    const QString name = methodName(method);
    int numWithheld = 0, numDoneBefore = 0;
    for(const auto studentID : studentIDs) {
        if(peerReview.feedbackIsWithheld(results, studentID)) {
            numWithheld++;
        }
        if(std::any_of(peerReview.release.log.cbegin(), peerReview.release.log.cend(), [studentID, &name](const PeerReviewRelease::LogEntry &entry)
                       {return (entry.studentID == studentID) && (entry.method == name);})) {
            numDoneBefore++;
        }
    }

    const int numStudents = int(studentIDs.size());
    QString message, doneBeforeMessage, buttonText;
    switch(method) {
    case Method::pdf:
        message = tr("Save PDF files of the reports of %n student(s)?", "", numStudents);
        doneBeforeMessage = tr("%n of these reports were already saved as PDFs.", "", numDoneBefore);
        buttonText = tr("Save PDFs");
        break;
    case Method::mailMerge:
        message = tr("Save a mail merge file with the reports of %n student(s)?", "", numStudents);
        doneBeforeMessage = tr("%n of these reports were already saved in a mail merge file.", "", numDoneBefore);
        buttonText = tr("Save File");
        break;
    case Method::googleDoc:
        message = tr("Send %n student(s) their reports as Google Docs? Google emails each student a link to their report right away.", "", numStudents);
        doneBeforeMessage = tr("%n of these students were already sent a Google Doc of their report, and will get another email.", "", numDoneBefore);
        buttonText = tr("Continue...");
        break;
    case Method::canvasComment:
        message = tr("Post the reports of %n student(s) as comments on their team quiz in Canvas, each with a PDF of the report attached? "
                     "Canvas notifies each student according to their notification settings.", "", numStudents);
        doneBeforeMessage = tr("%n of these students already have their report as a comment. It will be replaced by the current report "
                               "in a new comment, and they'll be notified again.", "", numDoneBefore);
        buttonText = tr("Post Comments");
        break;
    }
    if(numWithheld > 0) {
        message += "\n\n" + tr("%n of these reports leave out the unnamed ratings and comments, since too few teammates responded.", "", numWithheld);
    }
    if(numDoneBefore > 0) {
        message += "\n\n" + doneBeforeMessage;
    }
    return grueprGlobal::warningMessage(this, "gruepr", message, buttonText, tr("Cancel"));
}

void FeedbackReleaseDialog::logDelivery(const long long studentID, const QString &method, const QString &detail)
{
    peerReview.release.log.append({QDateTime::currentDateTime(), studentID, method, detail});
}

void FeedbackReleaseDialog::printReport(const QString &html, QPdfWriter &writer)
{
    writer.setPageSize(QPageSize(QPageSize::Letter));
    writer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);
    QTextDocument document;
    document.setDefaultFont(QFont("DM Sans", 11));
    document.setHtml(html);
    document.print(&writer);
}

void FeedbackReleaseDialog::savePDFs()
{
    const QList<long long> studentIDs = studentsToDeliver();
    if(!confirmDelivery(studentIDs, Method::pdf)) {
        return;
    }
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Choose a folder for the PDF files"));
    if(folder.isEmpty()) {
        return;
    }

    QProgressDialog progress(tr("Saving PDF files..."), tr("Cancel"), 0, int(studentIDs.size()) + 1, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setStyleSheet(QString(LABEL10PTSTYLE) + PROGRESSBARSTYLE);

    // prints html to a pdf file; a file already there (e.g., from an earlier save) is removed first,
    // so that the check afterwards shows whether this file was actually written
    auto printToPDF = [](const QString &html, const QString &fileName) {
        if(QFileInfo::exists(fileName) && !QFile::remove(fileName)) {
            return false;   // e.g., the earlier file is open in a viewer that locks it
        }
        {
            QPdfWriter writer(fileName);
            printReport(html, writer);
        }   // the file is finished when the writer goes out of scope
        const QFileInfo writtenFile(fileName);
        return writtenFile.exists() && (writtenFile.size() > 0);
    };

    QStringList problemStudents;
    QString allReports = "<html><body>";
    int numDone = 0;
    for(const auto studentID : studentIDs) {
        if(progress.wasCanceled()) {
            break;
        }
        const QString html = peerReview.reportHtml(results, studentID);
        const QString fileName = QDir(folder).filePath(peerReview.reportFileName(studentID) + ".pdf");
        if(printToPDF(html, fileName)) {
            logDelivery(studentID, methodName(Method::pdf), fileName);
        }
        else {
            problemStudents << peerReview.student(studentID)->fullName();
        }
        QString body = html;
        body.remove("<html><body>").remove("</body></html>");
        allReports += ((numDone > 0)? "<div style=\"page-break-before: always;\">" : "<div>") + body + "</div>";
        progress.setValue(++numDone);
    }
    allReports += "</body></html>";
    if(numDone > 0) {
        printToPDF(allReports, QDir(folder).filePath(QObject::tr("All feedback reports") + ".pdf"));
    }
    progress.setValue(int(studentIDs.size()) + 1);

    peerReview.save();
    refreshStudentList();
    if(!problemStudents.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("Not all files saved"), tr("These reports could not be saved:") + "\n" + problemStudents.join("\n"));
    }
}

void FeedbackReleaseDialog::saveMailMerge()
{
    const QList<long long> studentIDs = studentsToDeliver();
    if(!confirmDelivery(studentIDs, Method::mailMerge)) {
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("CSV File (*.csv);;Excel File (*.xlsx)"));
    if(fileName.isEmpty()) {
        return;
    }
    const auto format = (QFileInfo(fileName).suffix().compare("xlsx", Qt::CaseInsensitive) == 0)?
                            grueprGlobal::TabularFileFormat::xlsx : grueprGlobal::TabularFileFormat::csv;

    QList<QStringList> rows = {{tr("Email"), tr("First Name"), tr("Last Name"), tr("Team"), tr("Peer Review"), tr("Feedback")}};
    for(const auto studentID : studentIDs) {
        const auto *thisStudent = peerReview.student(studentID);
        const int teamNum = peerReview.teamOf(studentID);
        rows.append({thisStudent->email, thisStudent->firstname, thisStudent->lastname, (teamNum >= 0)? peerReview.teams.at(teamNum).name : "",
                     peerReview.title, peerReview.reportPlainText(results, studentID)});
    }
    if(!grueprGlobal::writeTabularFile(rows, format, fileName)) {
        grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
        return;
    }

    for(const auto studentID : studentIDs) {
        logDelivery(studentID, methodName(Method::mailMerge), fileName);
    }
    peerReview.save();
    refreshStudentList();
}

void FeedbackReleaseDialog::shareGoogleDocs()
{
    const QList<long long> allStudentIDs = studentsToDeliver();
    QList<long long> studentIDs;
    QStringList withoutEmail;
    for(const auto studentID : allStudentIDs) {
        const auto *thisStudent = peerReview.student(studentID);
        if(thisStudent->email.contains('@')) {
            studentIDs << studentID;
        }
        else {
            withoutEmail << thisStudent->fullName();
        }
    }
    if(!withoutEmail.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("No email address"), tr("These students have no email address, so their reports can't be shared "
                                                                    "as Google Docs. Please send them another way:") + "\n" + withoutEmail.join("\n"));
    }
    if(!confirmDelivery(studentIDs, Method::googleDoc)) {
        return;
    }

    // what Google's email to each student will say
    QDialog messageDialog(this);
    messageDialog.setWindowTitle(tr("Send as Google Docs"));
    messageDialog.setStyleSheet(WHITEDIALOGSTYLE);
    auto *messageLayout = new QVBoxLayout(&messageDialog);
    auto *messageExplanation = new QLabel(tr("gruepr will create a folder in your Google Drive with a view-only Google Doc of each report, "
                                             "and share each Doc with that student's email address. Google then emails each student a link "
                                             "to their Doc, with this message:"), &messageDialog);
    messageExplanation->setStyleSheet(LABEL10PTSTYLE);
    messageExplanation->setWordWrap(true);
    auto *messageTextEdit = new QPlainTextEdit(&messageDialog);
    messageTextEdit->setPlainText(tr("Here is the feedback from your teammates in our peer review, \"%1\".").arg(peerReview.title));
    auto *messageFrame = framed(messageTextEdit, 90, &messageDialog);
    auto *messageNote = new QLabel(tr("Some schools don't allow sharing with addresses outside the school, and students without a Google "
                                      "account may need to sign in to Google to open their Doc."), &messageDialog);
    messageNote->setStyleSheet(LABEL10PTSTYLE);
    messageNote->setWordWrap(true);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &messageDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Share and Email Students"));
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &messageDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &messageDialog, &QDialog::reject);
    messageLayout->addWidget(messageExplanation);
    messageLayout->addWidget(messageFrame);
    messageLayout->addWidget(messageNote);
    messageLayout->addWidget(buttonBox);
    messageDialog.resize(LG_DLG_SIZE, SM_DLG_SIZE);
    if(messageDialog.exec() == QDialog::Rejected) {
        return;
    }
    const QString message = messageTextEdit->toPlainText().trimmed();

    if(!grueprGlobal::internetIsGood()) {
        return;
    }
    auto *google = new GoogleHandler(this);
    if(!google->authenticate()) {
        google->deleteLater();
        return;
    }

    const QString folderName = peerReview.title + " - " + tr("feedback reports");
    const QString folderID = google->createDriveFolder(folderName);
    if(folderID.isEmpty()) {
        QString errormsg = tr("The folder for the reports could not be created in your Google Drive.");
        if(!google->lastErrorMessage.isEmpty()) {
            errormsg += "<br><br>" + tr("Error message:") + " <code>" + google->lastErrorMessage.toHtmlEscaped() + "</code>";
        }
        google->deleteLater();
        grueprGlobal::errorMessage(this, tr("Error"), errormsg);
        return;
    }

    QProgressDialog progress(tr("Sharing Google Docs..."), tr("Cancel"), 0, int(studentIDs.size()), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setStyleSheet(QString(LABEL10PTSTYLE) + PROGRESSBARSTYLE);
    QStringList problemStudents;
    int numDone = 0;
    for(const auto studentID : std::as_const(studentIDs)) {
        if(progress.wasCanceled()) {
            break;
        }
        const auto *thisStudent = peerReview.student(studentID);
        progress.setLabelText(tr("Sharing with") + " " + thisStudent->fullName() + "...");
        QString docURL;
        if(google->shareFeedbackDoc(folderID, peerReview.title + " - " + thisStudent->fullName(), peerReview.reportHtml(results, studentID),
                                    thisStudent->email, message, docURL)) {
            logDelivery(studentID, methodName(Method::googleDoc), docURL);
        }
        else {
            problemStudents << thisStudent->fullName() + (google->lastErrorMessage.isEmpty()? "" : (" (" + google->lastErrorMessage + ")"));
            if(!docURL.isEmpty()) {
                logDelivery(studentID, tr("Google Doc (not shared)"), docURL);
            }
        }
        progress.setValue(++numDone);
    }
    google->deleteLater();

    peerReview.save();
    refreshStudentList();
    if(!problemStudents.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("Not all shared"), tr("These reports could not be shared:") + "\n" + problemStudents.join("\n"));
    }
}

void FeedbackReleaseDialog::postCanvasComments()
{
    // a student's report can be posted only if they're matched to a Canvas student and their team has a quiz
    const QList<long long> allStudentIDs = studentsToDeliver();
    QList<long long> studentIDs;
    QStringList cantPost;
    for(const auto studentID : allStudentIDs) {
        const auto *thisStudent = peerReview.student(studentID);
        const int teamNum = peerReview.teamOf(studentID);
        const bool teamHasQuiz = (teamNum >= 0) && (teamNum < peerReview.canvasQuizzes.size()) && (peerReview.canvasQuizzes.at(teamNum).assignmentID != -1);
        if((thisStudent != nullptr) && (thisStudent->LMSID > 0) && teamHasQuiz) {
            studentIDs << studentID;
        }
        else if(thisStudent != nullptr) {
            cantPost << thisStudent->fullName();
        }
    }
    if(!cantPost.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("Not in Canvas"), tr("These students weren't matched to a student in Canvas, or their team has no quiz, "
                                                                 "so their reports can't be posted as Canvas comments. Please send them "
                                                                 "another way:") + "\n" + cantPost.join("\n"));
    }
    if(!confirmDelivery(studentIDs, Method::canvasComment)) {
        return;
    }

    if(!grueprGlobal::internetIsGood()) {
        return;
    }
    auto *canvas = new CanvasHandler(this);
    if(!canvas->authenticate()) {
        canvas->deleteLater();
        return;
    }

    // post each report, showing whose is being posted and a bar with the progress; the posting can be canceled between students
    auto *busyBox = canvas->actionDialog(this);
    bool canceled = false;
    canvas->actionDialogButtons->setStandardButtons(QDialogButtonBox::Cancel);
    canvas->actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(canvas->actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, busyBox, [&canceled]{canceled = true;});
    const QString method = methodName(Method::canvasComment);
    const int numStudents = int(studentIDs.size());
    QStringList problemStudents, earlierNotRemoved, notShown;
    int numDone = 0;
    for(const auto studentID : std::as_const(studentIDs)) {
        if(canceled) {
            break;
        }
        const auto *thisStudent = peerReview.student(studentID);
        canvas->showActionProgress(tr("Posting comment") + " " + QString::number(numDone + 1) + " " + tr("of") + " " + QString::number(numStudents) +
                                   "...", numDone, numStudents);
        const long long assignmentID = peerReview.canvasQuizzes.at(peerReview.teamOf(studentID)).assignmentID;

        // the report as a PDF, made in memory
        QBuffer pdfBuffer;
        pdfBuffer.open(QIODevice::WriteOnly);
        {
            QPdfWriter writer(&pdfBuffer);
            printReport(peerReview.reportHtml(results, studentID), writer);
        }   // the PDF is finished when the writer goes out of scope
        pdfBuffer.close();

        // the comment this student was last given, if any, which is deleted once the new one is posted
        long long earlierCommentID = -1;
        for(auto entry = peerReview.release.log.crbegin(); entry != peerReview.release.log.crend(); ++entry) {
            if((entry->studentID == studentID) && (entry->method == method)) {
                earlierCommentID = entry->detail.toLongLong();
                break;
            }
        }

        const long long commentID = canvas->postSubmissionComment(peerReview.canvasCourseID, assignmentID, thisStudent->LMSID,
                                                                  peerReview.reportPlainText(results, studentID),
                                                                  peerReview.reportFileName(studentID) + ".pdf", pdfBuffer.data(), "application/pdf");
        if(commentID != -1) {
            // (an ID of 0 means it was posted, but Canvas didn't show which comment it is, so it can't be replaced if the report is posted again)
            logDelivery(studentID, method, (commentID > 0)? QString::number(commentID) : QString());
            if(commentID == 0) {
                notShown << thisStudent->fullName();
            }
            if((earlierCommentID > 0) &&
               !canvas->deleteSubmissionComment(peerReview.canvasCourseID, assignmentID, thisStudent->LMSID, earlierCommentID)) {
                earlierNotRemoved << thisStudent->fullName();
            }
        }
        else {
            problemStudents << thisStudent->fullName() + (canvas->lastErrorMessage.isEmpty()? "" : (" (" + canvas->lastErrorMessage.simplified() + ")"));
        }
        numDone++;
    }
    canvas->actionComplete(busyBox);
    canvas->deleteLater();

    peerReview.save();
    refreshStudentList();
    QStringList problems;
    if(!problemStudents.isEmpty()) {
        problems << tr("These reports could not be posted, for example because the student is no longer in the course:") + "\n" +
                    problemStudents.join("\n");
    }
    if(!earlierNotRemoved.isEmpty()) {
        problems << tr("These students' new reports were posted, but their earlier report comments could not be removed, so they'll see both. "
                       "You can delete the earlier comments in SpeedGrader:") + "\n" + earlierNotRemoved.join("\n");
    }
    if(!notShown.isEmpty()) {
        problems << tr("These reports were posted, but Canvas didn't show the new comments, so if these reports are posted again, "
                       "these comments won't be replaced:") + "\n" + notShown.join("\n");
    }
    if(!problems.isEmpty()) {
        grueprGlobal::errorMessage(this, problemStudents.isEmpty()? tr("Posted, with problems") : tr("Not all posted"), problems.join("\n\n"));
    }
}
