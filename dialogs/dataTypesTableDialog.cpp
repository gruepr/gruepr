#include "dataTypesTableDialog.h"
#include "gruepr_globals.h"
#include <QBoxLayout>
#include <QHeaderView>
#include <QTableWidget>

dataTypesTableDialog::dataTypesTableDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("DataTypes Definition Table");
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    setMaximumSize(SCREENWIDTH * 5 / 6, SCREENHEIGHT * 5 / 6);
    setAttribute(Qt::WA_DeleteOnClose);
    auto *rulesTable = new QTableWidget(this);
    rulesTable->setColumnCount(5);
    rulesTable->setHorizontalHeaderLabels({"Data Type", "Description", "Grouping Rules", "Format", "Example"});
    rulesTable->setRowCount(15);

    QStringList dataTypes = {
        "Unused", "Timestamp", "First Name", "Last Name", "Email Address",
        "Gender", "Racial/ethnic identity", "Schedule", "Section", "Timezone", "Grade",
        "Preferred Teammates", "Preferred Non-teammates", "Multiple Choice", "Notes"
    };

    QStringList descriptions = {
        "Category will be ignored", "Date Field", "Student's first name",
        "Student's last name", "Student's email address", "Student's self-identified gender (can be more than one)",
        "Student's self-identified racial, ethnic, or cultural identity",
        "Student's availability for team meetings throughout the week",
        "Student's enrolled course section", "Student's timezone", "Student's numerical grade",
        "A list of classmates with whom the student desires to work",
        "A list of classmates with whom the student does not want to work",
        "Student's response(s) to a instructor-defined multiple-choice question",
        "Additional notes or question responses that will not be used by gruepr"
    };

    QStringList groupingRules = {
        "--",
        "--",
        "--",
        "--",
        "--",
        "Prevent the creation of teams with a single teammate of a particular gender identity.",
        "Prevent the creation of teams with a single teammate of a particular racial/ethnic identity.",
        "Specify a minimum number of weekly times where all teammates are available to meet.",
        "Generate separate team sets for each academic section.",
        "Group students by similar time zones for easier coordination.",
        "Balance groups so average grades fall within a specified range.",
        "Students listed here will be grouped together as preferred teammates.",
        "Students listed here will be kept apart as non-preferred teammates.",
        "Create teams that are either diverse or similar in the question responses; ensure that all teams have at least one teammate with a particular response.",
        "--"
    };


    QStringList formats = {
        "N/A", "YYYY/MM/DD HH:MM:SS", "Text", "Text", "Text",
        "Categorical", "Free response", "Hourly times (semi-colon separated)",
        "Categorical", "Categorical", "Numeric", "List of names (comma separated)",
        "List of names (comma separated)", "Multiple choice with categorical or Likert scale answers",
        "Free response"
    };

    QStringList examples = {
        "N/A", "2018/02/07 3:48:34PM EST", "Toni", "Tester", "t.tester@school.edu",
        "Male, Nonbinary", "Asian",
        "8AM;9AM;10AM;", "Mon/Wed Section", "EST", "93.5",
        "\"Preston Polasek, Shelia Swafford\"", "Bryan Blackwelder",
        "\"Chemistry, Physics\"",
        "Free response"
    };

    for (int i = 0; i < dataTypes.size(); ++i) {
        rulesTable->setItem(i, 0, new QTableWidgetItem(dataTypes[i]));
        rulesTable->setItem(i, 1, new QTableWidgetItem(descriptions[i]));
        rulesTable->setItem(i, 2, new QTableWidgetItem(groupingRules[i]));
        rulesTable->setItem(i, 3, new QTableWidgetItem(formats[i]));
        rulesTable->setItem(i, 4, new QTableWidgetItem(examples[i]));
    }

    rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rulesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Enable word wrap for all cells
    rulesTable->setWordWrap(true);

    // Ensure rows resize vertically to fit content
    rulesTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    rulesTable->verticalHeader()->setVisible(false); // Hide row numbers
    // Styling
    QHeaderView *header = rulesTable->horizontalHeader();
    header->setStyleSheet("QHeaderView::section {"
                          "background-color: " OPENWATERHEX";"  // Replace BUBBLYHEX with a color
                          "color: white;"
                          "font-family: 'DM Sans'; font-size: 10pt;"
                          "padding: 5px;"
                          "border: 1px solid #ccc;"
                          "}");

    rulesTable->setAlternatingRowColors(true);
    rulesTable->setStyleSheet(QString("QTableWidget {"
                              "alternate-background-color: #f0f0f0;"
                              "background-color: white;"
                              "font-family: 'DM Sans'; font-size: 10pt;"
                              "border: none;"
                              "}") + SCROLLBARSTYLE);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(rulesTable);
    setLayout(layout);
}
