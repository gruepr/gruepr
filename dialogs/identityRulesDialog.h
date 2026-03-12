#ifndef IDENTITYRULESDIALOG_H
#define IDENTITYRULESDIALOG_H

#include "dataOptions.h"
#include "qscrollarea.h"
#include "qtablewidget.h"
#include "teamingOptions.h"
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

class IdentityRulesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode {gender, urm};

    explicit IdentityRulesDialog(QWidget *parent, Mode mode, TeamingOptions *teamingOptions, const DataOptions *dataOptions);

private slots:
    void addNewIdentityRule();

private:
    void populateTable();
    void saveRules();
    void addRow(const QString &identityText, const QString &operation = "!=", int value = 0);

    // Returns the list of identity category strings for the combo box
    QStringList identityOptions() const;

    QTableWidget *rulesTable = nullptr;
    QScrollArea *scrollArea = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    TeamingOptions *const teamingOptions;
    const DataOptions *const dataOptions;
    const Mode mode;
};

#endif // IDENTITYRULESDIALOG_H
