#include "reminderDialog.h"
#include "delimitedTextFile.h"
#include "dialogs/feedbackReleaseDialog.h"
#include "LMS/canvashandler.h"
#include "LMS/googlehandler.h"
#include "widgets/styledComboBox.h"
#include <algorithm>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QToolTip>
#include <QVBoxLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// A dialog to remind the students who haven't responded to a survey or a peer review
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void ReminderDialog::infoMessage(QWidget *parent, const QString &message)
{
    QMessageBox messageBox(parent);
    messageBox.setWindowTitle("gruepr");
    messageBox.setStyleSheet(QString(LABEL10PTSTYLE) + SMALLBUTTONSTYLE);
    messageBox.setIconPixmap(QPixmap(":/icons_new/ok.png").scaled(MSGBOX_ICON_SIZE, MSGBOX_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    messageBox.setText(message);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.button(QMessageBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    messageBox.exec();
}

// an error message, followed by the error from Canvas or Google (if there is one)
static QString withErrorMessage(const QString &message, const QString &lastErrorMessage)
{
    return message + (lastErrorMessage.isEmpty()? "" : ("<br><br>" + QObject::tr("Error message:") + " <code>" + lastErrorMessage.toHtmlEscaped() + "</code>"));
}

ReminderDialog::ReminderDialog(const QString &title, const QString &kind, const QList<Recipient> &recipients, const long long canvasCourseID,
                               CanvasHandler *canvas, QWidget *parent)
    : QDialog(parent),
    title(title),
    kind(kind),
    recipients(recipients),
    canvasCourseID(canvasCourseID),
    canvas(canvas)
{
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle(tr("gruepr - Remind students") + " - " + title);
    setStyleSheet(QString(WHITEDIALOGSTYLE) + SCROLLBARSTYLE);
    setSizeGripEnabled(true);
    resize(SCREENWIDTH / 2, SCREENHEIGHT * 2 / 3);

    // the students, by team (if there are teams) and then by name
    hasTeams = std::any_of(this->recipients.cbegin(), this->recipients.cend(), [](const Recipient &recipient){return !recipient.team.isEmpty();});
    std::sort(this->recipients.begin(), this->recipients.end(), [](const Recipient &a, const Recipient &b){
        if(a.team != b.team) {
            return QString::localeAwareCompare(a.team, b.team) < 0;
        }
        return QString::localeAwareCompare(a.lastname + " " + a.firstname, b.lastname + " " + b.firstname) < 0;
    });

    const int numRecipients = int(this->recipients.size());
    summaryLabel = new QLabel(QString::number(numRecipients) + " " + ((numRecipients == 1)? tr("student hasn't") : tr("students haven't")) + " " +
                              tr("responded to") + " \"" + title + "\".", this);
    summaryLabel->setStyleSheet(LABEL12PTSTYLE);
    summaryLabel->setTextFormat(Qt::PlainText);
    summaryLabel->setWordWrap(true);
    noteLabel = new QLabel(this);
    noteLabel->setStyleSheet(LABEL10PTSTYLE);
    noteLabel->setTextFormat(Qt::PlainText);
    noteLabel->setWordWrap(true);
    noteLabel->hide();
    auto *explanationLabel = new QLabel(tr("The checked students get the reminder below, each with their own name and link."), this);
    explanationLabel->setStyleSheet(LABEL10PTSTYLE);
    explanationLabel->setWordWrap(true);

    studentsTable = new QTableWidget(this);
    studentsTable->setColumnCount(4);
    studentsTable->setHorizontalHeaderLabels({tr("Student"), tr("Team"), tr("Email"), tr("In Canvas")});
    studentsTable->setColumnHidden(teamColumn, !hasTeams);
    studentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    studentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    studentsTable->verticalHeader()->hide();
    studentsTable->horizontalHeader()->setStretchLastSection(true);
    studentsTable->setStyleSheet(QString(TABLESTYLE) + SCROLLBARSTYLE);

    // the message, which is personalized for each student
    auto *subjectLabel = new QLabel(tr("Subject:"), this);
    subjectLabel->setStyleSheet(LABEL10PTSTYLE);
    subjectLineEdit = new QLineEdit(tr("Reminder:") + " " + title, this);
    subjectLineEdit->setStyleSheet(LINEEDITSTYLE);
    auto *subjectLayout = new QHBoxLayout;
    subjectLayout->addWidget(subjectLabel);
    subjectLayout->addWidget(subjectLineEdit, 1);
    auto *messageLabel = new QLabel(tr("Message:"), this);
    messageLabel->setStyleSheet(LABEL10PTSTYLE);
    messageTextEdit = new QPlainTextEdit(this);
    // whatever is the same for every student (always the title, and the link or the team if everyone's is the same) is written in,
    // so the message reads as it will be sent; only what differs between students is left as a [tag] to be filled in for each one
    // (a link that's blank for everyone is left as the tag, so the message doesn't quietly end without one)
    const bool sameLink = !this->recipients.isEmpty() && !this->recipients.constFirst().link.isEmpty() &&
                          std::all_of(this->recipients.cbegin(), this->recipients.cend(),
                                      [this](const Recipient &recipient){return recipient.link == this->recipients.constFirst().link;});
    const bool sameTeam = hasTeams && std::all_of(this->recipients.cbegin(), this->recipients.cend(),
                                                  [this](const Recipient &recipient){return recipient.team == this->recipients.constFirst().team;});
    const QString linkText = sameLink? this->recipients.constFirst().link : "[link]";
    const QString teamText = sameTeam? this->recipients.constFirst().team : "[team]";
    messageTextEdit->setPlainText(tr("Hi [first name],") + "\n\n" + tr("This is a reminder to fill out our") + " " + kind + ", \"" + title + "\"" +
                                  (hasTeams? (", " + tr("for your team,") + " " + teamText) : QString()) + ". " + tr("You can find it here:") + "\n" +
                                  linkText + "\n\n" + tr("Thank you!"));
    auto *messageFrame = FeedbackReleaseDialog::framed(messageTextEdit, 170, this);
    QStringList tagsLeft = {"[first name]"};
    if(hasTeams && !sameTeam) {
        tagsLeft << "[team]";
    }
    if(!sameLink) {
        tagsLeft << "[link]";
    }
    QString tagsText = tagsLeft.constFirst();
    if(tagsLeft.size() == 2) {
        tagsText = tagsLeft.at(0) + " " + tr("and") + " " + tagsLeft.at(1);
    }
    else if(tagsLeft.size() == 3) {
        tagsText = tagsLeft.at(0) + ", " + tagsLeft.at(1) + ", " + tr("and") + " " + tagsLeft.at(2);
    }
    auto *placeholdersLabel = new QLabel(tagsText + " " + ((tagsLeft.size() == 1)? tr("is filled in for each student.") : tr("are filled in for each student.")), this);
    placeholdersLabel->setStyleSheet(LABEL10PTSTYLE);
    placeholdersLabel->setWordWrap(true);

    // the ways to send the reminder
    auto *closeButton = new QPushButton(tr("Close"), this);
    closeButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    auto *checkedLabel = new QLabel(tr("Checked students:"), this);
    checkedLabel->setStyleSheet(LABEL10PTSTYLE);
    auto *copyEmailsButton = new QPushButton(tr("Copy Emails"), this);
    copyEmailsButton->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    copyEmailsButton->setToolTip(tr("Copy the checked students' email addresses, to paste into an email"));
    connect(copyEmailsButton, &QPushButton::clicked, this, &ReminderDialog::copyEmails);
    auto *mailMergeButton = new QPushButton(tr("Save Mail Merge File..."), this);
    mailMergeButton->setStyleSheet(SMALLBUTTONSTYLE);
    mailMergeButton->setToolTip(tr("A spreadsheet with each checked student's email, link, and message, for a mail merge in Outlook, Word, or Gmail"));
    connect(mailMergeButton, &QPushButton::clicked, this, &ReminderDialog::saveMailMerge);
    auto *canvasButton = new QPushButton(tr("Send in Canvas..."), this);
    canvasButton->setStyleSheet(SMALLBUTTONSTYLE);
    canvasButton->setToolTip(tr("Send each checked student their message privately, to their Canvas Inbox"));
    connect(canvasButton, &QPushButton::clicked, this, &ReminderDialog::sendCanvasMessages);
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(checkedLabel);
    buttonLayout->addWidget(copyEmailsButton);
    buttonLayout->addWidget(mailMergeButton);
    buttonLayout->addWidget(canvasButton);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(summaryLabel);
    mainLayout->addWidget(noteLabel);
    mainLayout->addWidget(explanationLabel);
    mainLayout->addWidget(studentsTable, 1);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(subjectLayout);
    mainLayout->addWidget(messageLabel);
    mainLayout->addWidget(messageFrame);
    mainLayout->addWidget(placeholdersLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);

    refreshTable();
}

void ReminderDialog::setNote(const QString &note)
{
    noteLabel->setText(note);
    noteLabel->setVisible(!note.isEmpty());
}

void ReminderDialog::refreshTable()
{
    studentsTable->setRowCount(int(recipients.size()));
    for(int row = 0; row < recipients.size(); row++) {
        const auto &recipient = recipients.at(row);
        if(studentsTable->item(row, nameColumn) == nullptr) {
            auto *nameItem = new QTableWidgetItem(recipient.fullName());
            nameItem->setFlags(nameItem->flags() | Qt::ItemIsUserCheckable);
            nameItem->setCheckState(Qt::Checked);
            studentsTable->setItem(row, nameColumn, nameItem);
            studentsTable->setItem(row, teamColumn, new QTableWidgetItem(recipient.team));
            studentsTable->setItem(row, emailColumn, new QTableWidgetItem(recipient.email));
            studentsTable->setItem(row, canvasColumn, new QTableWidgetItem);
        }
        // whether the student is in the Canvas course is known only once the course is (until then, it's left blank)
        QString canvasStatus;
        if(sentRows.contains(row)) {
            canvasStatus = tr("Message sent");
        }
        else if(canvasCourseID != -1) {
            canvasStatus = (recipient.canvasID > 0)? tr("Yes") : tr("Not found");
        }
        studentsTable->item(row, canvasColumn)->setText(canvasStatus);
    }
    studentsTable->resizeColumnsToContents();
}

QList<int> ReminderDialog::checkedRows() const
{
    QList<int> rows;
    for(int row = 0; row < studentsTable->rowCount(); row++) {
        if((studentsTable->item(row, nameColumn) != nullptr) && (studentsTable->item(row, nameColumn)->checkState() == Qt::Checked)) {
            rows << row;
        }
    }
    return rows;
}

bool ReminderDialog::confirmMissingLinks(const QList<int> &rows)
{
    const QString text = subjectLineEdit->text() + messageTextEdit->toPlainText();
    if(!text.contains("[link]", Qt::CaseInsensitive)) {
        return true;
    }
    QStringList noLink;
    for(const auto row : rows) {
        if(recipients.at(row).link.isEmpty()) {
            noLink << recipients.at(row).fullName();
        }
    }
    return noLink.isEmpty() ||
           grueprGlobal::warningMessage(this, "gruepr", tr("There's no link to the") + " " + kind + " " + tr("for these students (for example, because "
                                                           "their team has no quiz), so their message won't have one:") + "\n\n" + noLink.join("\n"),
                                        tr("Continue"), tr("Cancel"));
}

QString ReminderDialog::personalized(const QString &text, const Recipient &recipient) const
{
    QString result = text;
    result.replace("[first name]", recipient.firstname, Qt::CaseInsensitive);
    result.replace("[team]", recipient.team, Qt::CaseInsensitive);
    result.replace("[link]", recipient.link, Qt::CaseInsensitive);
    result.replace("[title]", title, Qt::CaseInsensitive);
    return result;
}

void ReminderDialog::copyEmails()
{
    QStringList emails;
    for(const auto row : checkedRows()) {
        if(!recipients.at(row).email.isEmpty()) {
            emails << recipients.at(row).email;
        }
    }
    QGuiApplication::clipboard()->setText(emails.join("; "));
    QToolTip::showText(QCursor::pos(), emails.isEmpty()? tr("No emails to copy") : tr("%n email(s) copied", "", int(emails.size())),
                       this, QRect(), UI_DISPLAY_DELAYTIME);
}

void ReminderDialog::saveMailMerge()
{
    const QList<int> rows = checkedRows();
    if(rows.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("No students"), tr("No students are checked."));
        return;
    }
    if(!confirmMissingLinks(rows)) {
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("CSV File (*.csv);;Excel File (*.xlsx)"));
    if(fileName.isEmpty()) {
        return;
    }
    const auto format = (QFileInfo(fileName).suffix().compare("xlsx", Qt::CaseInsensitive) == 0)?
                            grueprGlobal::TabularFileFormat::xlsx : grueprGlobal::TabularFileFormat::csv;

    // the same columns as before, then the subject and message filled in for each student, so a mail merge can use them as they are
    QStringList header = {tr("Email"), tr("First Name"), tr("Last Name")};
    if(hasTeams) {
        header << tr("Team");
    }
    header << kind.left(1).toUpper() + kind.mid(1) << tr("Link") << tr("Subject") << tr("Message");
    QList<QStringList> fileRows = {header};
    const QString subject = subjectLineEdit->text().trimmed();
    const QString message = messageTextEdit->toPlainText().trimmed();
    for(const auto row : rows) {
        const auto &recipient = recipients.at(row);
        QStringList fileRow = {recipient.email, recipient.firstname, recipient.lastname};
        if(hasTeams) {
            fileRow << recipient.team;
        }
        fileRow << title << recipient.link << personalized(subject, recipient) << personalized(message, recipient);
        fileRows << fileRow;
    }
    if(!grueprGlobal::writeTabularFile(fileRows, format, fileName)) {
        grueprGlobal::errorMessage(this, tr("No Files Saved"), tr("No files were saved.\nThere was an issue writing the files."));
    }
}

CanvasHandler *ReminderDialog::connectToCanvas()
{
    if(canvas == nullptr) {
        auto *newCanvas = new CanvasHandler(this);
        if(!newCanvas->authenticate()) {
            newCanvas->deleteLater();
            return nullptr;
        }
        canvas = newCanvas;
    }
    canvas->lastErrorMessage.clear();
    return canvas;
}

bool ReminderDialog::chooseCanvasCourse()
{
    CanvasHandler *canvasConnection = connectToCanvas();
    if(canvasConnection == nullptr) {
        return false;
    }
    const long long courseID = chooseCanvasCourse(canvasConnection, tr("In which Canvas course should the messages be sent? "
                                                                       "The students are matched to its roster."), this);
    if(courseID == -1) {
        return false;
    }
    const QList<StudentRecord> roster = loadCanvasRoster(canvasConnection, courseID, this);
    if(roster.isEmpty()) {
        return false;
    }

    // each student is matched by their Canvas ID, then by email, then by name (if only one student on the roster has it);
    // students who would be matched to the same Canvas student are left unmatched, since there's no telling which is which
    QSet<long long> rosterIDs;
    QHash<QString, QList<long long>> rosterIDsOfEmail, rosterIDsOfName;
    for(const auto &student : roster) {
        rosterIDs.insert(student.LMSID);
        if(!student.email.trimmed().isEmpty()) {
            rosterIDsOfEmail[student.email.trimmed().toLower()] << student.LMSID;
        }
        rosterIDsOfName[(student.firstname + " " + student.lastname).simplified().toLower()] << student.LMSID;
    }
    QHash<long long, int> numMatched;
    for(auto &recipient : recipients) {
        if((recipient.canvasID > 0) && rosterIDs.contains(recipient.canvasID)) {
            numMatched[recipient.canvasID]++;
            continue;
        }
        const QList<long long> byEmail = rosterIDsOfEmail.value(recipient.email.trimmed().toLower());
        const QList<long long> byName = rosterIDsOfName.value(recipient.fullName().toLower());
        if(!recipient.email.trimmed().isEmpty() && (byEmail.size() == 1)) {
            recipient.canvasID = byEmail.constFirst();
        }
        else if(byName.size() == 1) {
            recipient.canvasID = byName.constFirst();
        }
        else {
            recipient.canvasID = -1;
            continue;
        }
        numMatched[recipient.canvasID]++;
    }
    for(auto &recipient : recipients) {
        if(numMatched.value(recipient.canvasID) > 1) {
            recipient.canvasID = -1;
        }
    }
    canvasCourseID = courseID;
    refreshTable();
    return true;
}

void ReminderDialog::sendCanvasMessages()
{
    const QList<int> rows = checkedRows();
    if(rows.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("No students"), tr("No students are checked."));
        return;
    }
    if(!grueprGlobal::internetIsGood()) {
        return;
    }
    if((canvasCourseID == -1) && !chooseCanvasCourse()) {
        return;
    }

    // only the students matched to a student in the Canvas course can be sent a message
    QList<int> rowsToMessage;
    QStringList notInCanvas;
    for(const auto row : rows) {
        if(recipients.at(row).canvasID > 0) {
            rowsToMessage << row;
        }
        else {
            notInCanvas << recipients.at(row).fullName();
        }
    }
    if(rowsToMessage.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("Not in Canvas"), tr("None of the checked students were matched to a student in the Canvas course, "
                                                                 "so no messages can be sent."));
        return;
    }
    if(!notInCanvas.isEmpty() && !grueprGlobal::warningMessage(this, "gruepr", tr("These students weren't matched to a student in the Canvas course, "
                                                                                  "so they won't get a message:") + "\n\n" + notInCanvas.join("\n"),
                                                               tr("Continue"), tr("Cancel"))) {
        return;
    }
    if(!confirmMissingLinks(rowsToMessage)) {
        return;
    }
    const int numToMessage = int(rowsToMessage.size());
    const int numSentBefore = int(std::count_if(rowsToMessage.cbegin(), rowsToMessage.cend(), [this](const int row){return sentRows.contains(row);}));
    QString confirmation = tr("Send a private Canvas message to %n student(s)? Each one gets their own message, and Canvas notifies them "
                              "according to their notification settings.", "", numToMessage);
    if(numSentBefore > 0) {
        confirmation += "\n\n" + tr("%n of these students were already sent a message here.", "", numSentBefore);
    }
    if(!grueprGlobal::warningMessage(this, "gruepr", confirmation, tr("Send Messages"), tr("Cancel"))) {
        return;
    }
    CanvasHandler *canvasConnection = connectToCanvas();
    if(canvasConnection == nullptr) {
        return;
    }

    // send each message, showing whose is being sent and a bar with the progress; the sending can be canceled between students
    auto *busyBox = canvasConnection->actionDialog(this);
    bool canceled = false;
    canvasConnection->actionDialogButtons->setStandardButtons(QDialogButtonBox::Cancel);
    canvasConnection->actionDialogButtons->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(canvasConnection->actionDialogButtons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, busyBox, [&canceled]{canceled = true;});
    const QString subject = subjectLineEdit->text().trimmed();
    const QString message = messageTextEdit->toPlainText().trimmed();
    QStringList problemStudents;
    int numDone = 0;
    for(const auto row : std::as_const(rowsToMessage)) {
        if(canceled) {
            break;
        }
        canvasConnection->showActionProgress(tr("Sending reminder") + " " + QString::number(numDone + 1) + " " + tr("of") + " " +
                                             QString::number(numToMessage) + "...", numDone, numToMessage);
        const auto &recipient = recipients.at(row);
        if(canvasConnection->sendMessage(canvasCourseID, recipient.canvasID, personalized(subject, recipient), personalized(message, recipient)) != -1) {
            sentRows.insert(row);
            messaged << recipient.ID;
            studentsTable->item(row, nameColumn)->setCheckState(Qt::Unchecked);
        }
        else {
            problemStudents << recipient.fullName() + (canvasConnection->lastErrorMessage.isEmpty()? "" :
                                                           (" (" + canvasConnection->lastErrorMessage.simplified() + ")"));
        }
        numDone++;
    }
    canvasConnection->actionComplete(busyBox);

    refreshTable();
    const int numRecipients = int(recipients.size());
    summaryLabel->setText(QString::number(numRecipients) + " " + ((numRecipients == 1)? tr("student hasn't") : tr("students haven't")) + " " +
                          tr("responded to") + " \"" + title + "\". " + tr("%n of them have been sent a reminder in Canvas.", "", int(sentRows.size())));
    if(!problemStudents.isEmpty()) {
        grueprGlobal::errorMessage(this, tr("Not all sent"), tr("These students could not be sent a message, for example because they're no "
                                                                "longer in the course:") + "\n" + problemStudents.join("\n"));
    }
}

////////////////////////////////////////////
// Reminders for a survey made by the form builder
////////////////////////////////////////////

void ReminderDialog::remindAboutSurvey(QWidget *parent)
{
    // where the survey is
    QDialog sourceDialog(parent);
    sourceDialog.setWindowTitle(tr("Remind students about a survey"));
    sourceDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    sourceDialog.setStyleSheet(QString(WHITEDIALOGSTYLE) + RADIOBUTTONSTYLE);
    auto *layout = new QVBoxLayout(&sourceDialog);
    auto *label = new QLabel(tr("Where is the survey?"), &sourceDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *canvasRadioButton = new QRadioButton(tr("In Canvas (a survey or quiz in one of your courses)"), &sourceDialog);
    auto *googleRadioButton = new QRadioButton(tr("In Google Forms (a form made by gruepr's form builder)"), &sourceDialog);
    canvasRadioButton->setChecked(true);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &sourceDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Next"));
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &sourceDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &sourceDialog, &QDialog::reject);
    layout->addWidget(label);
    layout->addWidget(canvasRadioButton);
    layout->addWidget(googleRadioButton);
    layout->addWidget(buttonBox);
    if(sourceDialog.exec() == QDialog::Rejected) {
        return;
    }
    if(!grueprGlobal::internetIsGood()) {
        return;
    }

    if(canvasRadioButton->isChecked()) {
        remindAboutCanvasSurvey(parent);
    }
    else {
        remindAboutGoogleSurvey(parent);
    }
}

void ReminderDialog::remindAboutCanvasSurvey(QWidget *parent)
{
    auto *canvas = new CanvasHandler(parent);
    if(!canvas->authenticate()) {
        canvas->deleteLater();
        return;
    }

    // the course, then the survey
    QString courseName;
    const long long courseID = chooseCanvasCourse(canvas, tr("Which course is the survey in?"), parent, &courseName);
    if(courseID == -1) {
        canvas->deleteLater();
        return;
    }
    auto *busyBox = canvas->actionDialog(parent);
    const QList<CanvasHandler::CanvasQuiz> quizzes = canvas->getQuizList(courseID);
    canvas->actionComplete(busyBox);
    if(quizzes.isEmpty()) {
        grueprGlobal::errorMessage(parent, tr("No surveys"), withErrorMessage(tr("Canvas is responding with no surveys or quizzes in") + " " +
                                                                              courseName + ".", canvas->lastErrorMessage));
        canvas->deleteLater();
        return;
    }
    QStringList quizNames;
    for(const auto &quiz : quizzes) {
        quizNames << quiz.name;
    }
    const int quizNum = chooseFromList(tr("Choose survey"), tr("Which survey should students be reminded about?"), quizNames, 0, parent);
    if(quizNum == -1) {
        canvas->deleteLater();
        return;
    }
    const auto &quiz = quizzes.at(quizNum);

    // the course's students, and which of them have submitted the survey
    const QList<StudentRecord> roster = loadCanvasRoster(canvas, courseID, parent);
    if(roster.isEmpty()) {
        canvas->deleteLater();
        return;
    }
    busyBox = canvas->actionDialog(parent);
    canvas->showActionProgress(tr("Checking who has responded..."), 0, 0);
    bool ok = false;
    const QList<long long> submitterIDs = canvas->getQuizSubmitterIDs(courseID, quiz.ID, &ok);
    bool assigneesKnown = false;
    const CanvasHandler::QuizAssignees assignees = ok? canvas->getQuizAssignees(courseID, quiz.ID, &assigneesKnown) : CanvasHandler::QuizAssignees();
    canvas->actionComplete(busyBox);
    if(!ok) {
        grueprGlobal::errorMessage(parent, tr("Error"), withErrorMessage(tr("Canvas didn't say who has responded to the survey."), canvas->lastErrorMessage));
        canvas->deleteLater();
        return;
    }
    // only the students the survey is assigned to should respond (e.g., a quiz shown only to a section, or to one team)
    if(!assigneesKnown && !grueprGlobal::warningMessage(parent, "gruepr", tr("Canvas didn't say which students this survey is assigned to, "
                                                                             "so everyone in the course is listed."), tr("Continue"), tr("Cancel"))) {
        canvas->deleteLater();
        return;
    }
    auto isAssigned = [&assignees, assigneesKnown](const StudentRecord &student) {
        return !assigneesKnown || assignees.everyone || assignees.studentIDs.contains(student.LMSID);
    };

    const QString link = canvas->getBaseURL() + "/courses/" + QString::number(courseID) + "/quizzes/" + QString::number(quiz.ID);
    QList<Recipient> notResponded;
    for(int studentNum = 0; studentNum < roster.size(); studentNum++) {
        const auto &student = roster.at(studentNum);
        if(isAssigned(student) && !submitterIDs.contains(student.LMSID)) {
            notResponded.append({studentNum, student.firstname, student.lastname, student.email, "", link, student.LMSID});
        }
    }
    if(notResponded.isEmpty()) {
        infoMessage(parent, tr("Every student in") + " " + courseName + " " + tr("who was given") + " \"" + quiz.name + "\" " + tr("has responded to it."));
        canvas->deleteLater();
        return;
    }

    ReminderDialog reminderDialog(quiz.name, tr("survey"), notResponded, courseID, canvas, parent);
    reminderDialog.exec();
    canvas->deleteLater();
}

void ReminderDialog::remindAboutGoogleSurvey(QWidget *parent)
{
    // the survey, and its responses
    auto *google = new GoogleHandler(parent);
    if(!google->authenticate()) {
        google->deleteLater();
        return;
    }
    const QStringList surveys = google->getSurveyList();
    if(surveys.isEmpty()) {
        grueprGlobal::errorMessage(parent, tr("No surveys"), tr("No surveys made by gruepr's form builder were found for this Google account."));
        google->deleteLater();
        return;
    }
    // (the list goes from the oldest survey to the newest, so the newest is chosen to start with)
    const int surveyNum = chooseFromList(tr("Choose survey"), tr("Which survey should students be reminded about?"), surveys,
                                         int(surveys.size()) - 1, parent);
    if(surveyNum == -1) {
        google->deleteLater();
        return;
    }
    const QString surveyName = surveys.at(surveyNum);
    const QString link = google->getSurveyResponderURL(surveyName);
    auto *busyBox = google->actionDialog(parent);
    const QString filePath = google->downloadSurveyResult(surveyName);
    google->actionComplete(busyBox);
    const QString googleErrorMessage = google->lastErrorMessage;
    google->deleteLater();
    if(filePath.isEmpty()) {
        grueprGlobal::errorMessage(parent, tr("Error"), withErrorMessage(tr("The survey's responses could not be downloaded."), googleErrorMessage));
        return;
    }

    // who has responded, by the answers to the form builder's email and name questions
    QSet<QString> respondentEmails, respondentNames;
    struct Respondent {QString email; QString name;};
    QList<Respondent> respondents;
    int emailColumn = -1, firstNameColumn = -1, lastNameColumn = -1;
    DelimitedTextFile responsesFile;
    if(responsesFile.openExistingFile(filePath)) {
        if(responsesFile.readHeader()) {
            const QStringList &headers = responsesFile.headerValues;
            auto findColumn = [&headers](const QString &question, const QString &partialText) {
                for(int column = 0; column < headers.size(); column++) {
                    if(headers.at(column).simplified().compare(question, Qt::CaseInsensitive) == 0) {
                        return column;
                    }
                }
                for(int column = 0; column < headers.size(); column++) {
                    if(headers.at(column).contains(partialText, Qt::CaseInsensitive)) {
                        return column;
                    }
                }
                return -1;
            };
            emailColumn = findColumn(EMAILQUESTION, "email");
            firstNameColumn = findColumn(FIRSTNAMEQUESTION, "first name");
            lastNameColumn = findColumn(LASTNAMEQUESTION, "last name");
            while(responsesFile.readDataRow()) {
                const QStringList &values = responsesFile.fieldValues;
                Respondent respondent;
                if((emailColumn != -1) && (emailColumn < values.size()) && !values.at(emailColumn).trimmed().isEmpty()) {
                    respondent.email = values.at(emailColumn).trimmed();
                    respondentEmails.insert(respondent.email.toLower());
                }
                if((firstNameColumn != -1) && (lastNameColumn != -1) && (firstNameColumn < values.size()) && (lastNameColumn < values.size())) {
                    respondent.name = (values.at(firstNameColumn) + " " + values.at(lastNameColumn)).simplified();
                    respondentNames.insert(respondent.name.toLower());
                }
                respondents << respondent;
            }
        }
        responsesFile.close(true);      // (the downloaded file was only needed for this)
    }
    else {
        QFile::remove(filePath);
    }
    if((emailColumn == -1) && ((firstNameColumn == -1) || (lastNameColumn == -1))) {
        grueprGlobal::errorMessage(parent, tr("Error"), tr("This survey doesn't ask for students' email addresses or names, "
                                                           "so gruepr can't tell who has responded."));
        return;
    }

    // who should respond: the students in a Canvas course, or those in a roster file
    QDialog rosterDialog(parent);
    rosterDialog.setWindowTitle(tr("Remind students about a survey"));
    rosterDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    rosterDialog.setStyleSheet(QString(WHITEDIALOGSTYLE) + RADIOBUTTONSTYLE);
    auto *layout = new QVBoxLayout(&rosterDialog);
    auto *label = new QLabel(tr("Who should respond to this survey?"), &rosterDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    auto *canvasRadioButton = new QRadioButton(tr("The students in a Canvas course"), &rosterDialog);
    auto *fileRadioButton = new QRadioButton(tr("The students in a roster file (.csv, .txt, or .xlsx)"), &rosterDialog);
    canvasRadioButton->setChecked(true);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &rosterDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Next"));
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &rosterDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &rosterDialog, &QDialog::reject);
    layout->addWidget(label);
    layout->addWidget(canvasRadioButton);
    layout->addWidget(fileRadioButton);
    layout->addWidget(buttonBox);
    if(rosterDialog.exec() == QDialog::Rejected) {
        return;
    }

    QList<Recipient> roster;
    long long courseID = -1;
    CanvasHandler *canvas = nullptr;
    if(canvasRadioButton->isChecked()) {
        canvas = new CanvasHandler(parent);
        if(!canvas->authenticate()) {
            canvas->deleteLater();
            return;
        }
        courseID = chooseCanvasCourse(canvas, tr("Which course's students should respond to the survey?"), parent);
        const QList<StudentRecord> students = (courseID == -1)? QList<StudentRecord>() : loadCanvasRoster(canvas, courseID, parent);
        if(students.isEmpty()) {
            canvas->deleteLater();
            return;
        }
        for(int studentNum = 0; studentNum < students.size(); studentNum++) {
            const auto &student = students.at(studentNum);
            roster.append({studentNum, student.firstname, student.lastname, student.email, "", link, student.LMSID});
        }
    }
    else {
        QStringList names, emails;
        const QSettings savedSettings;
        if(!grueprGlobal::loadRoster(parent, savedSettings.value("saveFileLocation").toString(), names, &emails)) {
            return;
        }
        for(int studentNum = 0; studentNum < names.size(); studentNum++) {
            const QStringList nameParts = names.at(studentNum).simplified().split(' ');
            roster.append({studentNum, nameParts.constFirst(), nameParts.mid(1).join(' '), (studentNum < emails.size())? emails.at(studentNum).trimmed() : "",
                           "", link, -1});
        }
    }

    // a student has responded if their email or their name is among the responses
    QList<Recipient> notResponded;
    for(const auto &student : std::as_const(roster)) {
        const bool respondedByEmail = !student.email.isEmpty() && respondentEmails.contains(student.email.toLower());
        const bool respondedByName = respondentNames.contains(student.fullName().toLower());
        if(!respondedByEmail && !respondedByName) {
            notResponded << student;
        }
    }
    if(notResponded.isEmpty()) {
        infoMessage(parent, tr("Every student has responded to") + " \"" + surveyName + "\".");
        if(canvas != nullptr) {
            canvas->deleteLater();
        }
        return;
    }

    // responses that match no one on the roster (e.g., a different email address, or a nickname) may be from students listed as not responded
    QSet<QString> rosterEmails, rosterNames;
    for(const auto &student : std::as_const(roster)) {
        if(!student.email.isEmpty()) {
            rosterEmails.insert(student.email.toLower());
        }
        rosterNames.insert(student.fullName().toLower());
    }
    QStringList unmatchedResponses;
    for(const auto &respondent : std::as_const(respondents)) {
        const bool matchedByEmail = !respondent.email.isEmpty() && rosterEmails.contains(respondent.email.toLower());
        const bool matchedByName = !respondent.name.isEmpty() && rosterNames.contains(respondent.name.toLower());
        if(!matchedByEmail && !matchedByName && (!respondent.name.isEmpty() || !respondent.email.isEmpty())) {
            unmatchedResponses << (respondent.name.isEmpty()? respondent.email :
                                   (respondent.name + (respondent.email.isEmpty()? "" : (" (" + respondent.email + ")"))));
        }
    }

    ReminderDialog reminderDialog(surveyName, tr("survey"), notResponded, courseID, canvas, parent);
    if(!unmatchedResponses.isEmpty()) {
        reminderDialog.setNote(tr("%n response(s) didn't match anyone on the roster, perhaps because of a different email address or name, "
                                  "so some of these students may have responded already:", "", int(unmatchedResponses.size())) + " " +
                               unmatchedResponses.join(", "));
    }
    reminderDialog.exec();
    if(canvas != nullptr) {
        canvas->deleteLater();
    }
}

long long ReminderDialog::chooseCanvasCourse(CanvasHandler *canvas, const QString &prompt, QWidget *parent, QString *courseName)
{
    auto *busyBox = canvas->actionDialog(parent);
    const QList<CanvasHandler::CanvasCourse> courses = canvas->getCourses();
    canvas->actionComplete(busyBox);
    if(courses.isEmpty()) {
        grueprGlobal::errorMessage(parent, tr("Error"), withErrorMessage(tr("Canvas is responding with no courses available."), canvas->lastErrorMessage));
        return -1;
    }
    QStringList courseNames;
    for(const auto &course : courses) {
        courseNames << course.name;
    }
    const int courseNum = chooseFromList(tr("Choose Canvas course"), prompt, courseNames, 0, parent);
    if(courseNum == -1) {
        return -1;
    }
    if(courseName != nullptr) {
        *courseName = courses.at(courseNum).name;
    }
    return courses.at(courseNum).ID;
}

int ReminderDialog::chooseFromList(const QString &windowTitle, const QString &prompt, const QStringList &items, const int selected, QWidget *parent)
{
    QDialog chooseDialog(parent);
    chooseDialog.setWindowTitle(windowTitle);
    chooseDialog.setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    auto *layout = new QVBoxLayout(&chooseDialog);
    auto *label = new QLabel(prompt, &chooseDialog);
    label->setStyleSheet(LABEL10PTSTYLE);
    label->setWordWrap(true);
    auto *comboBox = new StyledComboBox(&chooseDialog);
    comboBox->addItems(items);
    comboBox->setCurrentIndex(std::clamp(selected, 0, int(items.size()) - 1));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &chooseDialog);
    buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(SMALLBUTTONSTYLE);
    buttonBox->button(QDialogButtonBox::Cancel)->setStyleSheet(SMALLBUTTONSTYLEINVERTED);
    connect(buttonBox, &QDialogButtonBox::accepted, &chooseDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &chooseDialog, &QDialog::reject);
    layout->addWidget(label);
    layout->addWidget(comboBox);
    layout->addWidget(buttonBox);
    if(chooseDialog.exec() == QDialog::Rejected) {
        return -1;
    }
    return comboBox->currentIndex();
}

QList<StudentRecord> ReminderDialog::loadCanvasRoster(CanvasHandler *canvas, const long long courseID, QWidget *parent)
{
    auto *busyBox = canvas->actionDialog(parent);
    const QList<StudentRecord> roster = canvas->getStudentRoster(courseID, [canvas](int stepsDone, int numSteps){
        canvas->showActionProgress(tr("Loading the course roster..."), stepsDone, numSteps);
    });
    canvas->actionComplete(busyBox);
    if(roster.isEmpty()) {
        grueprGlobal::errorMessage(parent, tr("Error"), withErrorMessage(tr("Canvas is responding with no students in this course."), canvas->lastErrorMessage));
    }
    return roster;
}
