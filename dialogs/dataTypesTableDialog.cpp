#include "dataTypesTableDialog.h"
#include "gruepr_globals.h"
#include "qboxlayout.h"
#include "qheaderview.h"
#include "qtablewidget.h"

dataTypesTableDialog::dataTypesTableDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("DataTypes Definition Table");
    setMinimumSize(LG_DLG_SIZE, LG_DLG_SIZE);
    QTableWidget *rulesTable = new QTableWidget(this);
    rulesTable->setColumnCount(4);
    rulesTable->setHorizontalHeaderLabels({"Data Type", "Description", "Format", "Example"});
    rulesTable->setRowCount(14); // Adjust row count based on the data provided

    QStringList dataTypes = {
        "Unused", "Timestamp", "First Name", "Last Name", "Email Address",
        "Gender", "Racial/ethnic identity", "Schedule", "Section", "Timezone",
        "Preferred Teammates", "Preferred Non-teammates", "Multiple Choice", "Notes"
    };

    QStringList descriptions = {
        "Category will be ignored", "Generic Date Field", "First name of student",
        "Last name of student", "Email address of student", "Student's self-identified gender (can be more than one)",
        "Student's self-identified racial, ethnic, or cultural identity",
        "Student's availability for team meetings throughout the week",
        "The Academic Section the student is enrolled in", "Timezone which the student is in",
        "A list of classmates with whom the student desires to work",
        "A list of classmates with whom the student does not want to work",
        "Instructor-defined questions/column-names with multiple-choice responses",
        "Additional notes added by the instructor that are not directly used by gruepr"
    };

    QStringList formats = {
        "N/A", "YYYY/MM/DD HH:MM:SS(AM/PM) TIMEZONE", "Text", "Text", "Text",
        "Categorical", "Free response", "Hourly times (semi-colon separated)",
        "Categorical", "Categorical", "List of names or student IDs",
        "List of names or student IDs", "Multiple choice with categorical or Likert scale answers",
        "Free response"
    };

    QStringList examples = {
        "N/A", "2018/02/07 3:48:34PM EST", "Ben", "Smith", "b.smith@school.edu",
        "Male, Female, Nonbinary, Unknown", "Asian, Hispanic, African, Caucasian",
        "8AM;9AM;10AM;", "Section 1, Section 2, Section 3", "EST, GMT",
        "Student A, Student B, Student C", "Student A, Student B, Student C",
        "What is your academic major?\nChemistry, Physics, Biology, English, Computer Science",
        "Free response"
    };

    for (int i = 0; i < dataTypes.size(); ++i) {
        rulesTable->setItem(i, 0, new QTableWidgetItem(dataTypes[i]));
        rulesTable->setItem(i, 1, new QTableWidgetItem(descriptions[i]));
        rulesTable->setItem(i, 2, new QTableWidgetItem(formats[i]));
        rulesTable->setItem(i, 3, new QTableWidgetItem(examples[i]));
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
                          "font-family: 'DM Sans'; font-size: 12pt;"
                          "padding: 5px;"
                          "border: 1px solid #ccc;"
                          "}");

    rulesTable->setAlternatingRowColors(true);
    rulesTable->setStyleSheet("QTableWidget {"
                              "alternate-background-color: #f0f0f0;"
                              "background-color: white;"
                              "font-family: 'DM Sans'; font-size: 10pt;"
                              "border: none;"
                              "}");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(rulesTable);
    setLayout(layout);
}
