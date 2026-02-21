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
    explicit IdentityRulesDialog(QWidget *parent = nullptr, const Gender identity = Gender::woman,
                                 TeamingOptions *teamingOptions = nullptr, const DataOptions *dataOptions = nullptr);
    QHBoxLayout* createIdentityOperatorRule(Gender identity, QString operatorString, int noOfIdentity);

private slots:
    void addNewIdentityRule();
    void removeIdentityRule(const QString &operatorString, int noOfIdentity);

private:
    void updateDialog();
    void populateTable();
    QTableWidget *rulesTable = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *scrollContentWidget = nullptr;
    QVBoxLayout *rulesLayout = nullptr;

    Gender genderIdentity;
    QMap<Gender, TeamingOptions::identityRule> identityRules;
    QVBoxLayout *mainLayout = nullptr;
    TeamingOptions *teamingOptions = nullptr;
    const DataOptions *dataOptions = nullptr;
};

#endif // IDENTITYRULESDIALOG_H
