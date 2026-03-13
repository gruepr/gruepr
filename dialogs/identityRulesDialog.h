#ifndef IDENTITYRULESDIALOG_H
#define IDENTITYRULESDIALOG_H

#include "criteria/criterion.h"
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>

class IdentityRulesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IdentityRulesDialog(QWidget *parent, QMap<QString, Criterion::IdentityRule> *identityRules, const QStringList &identityOptions,
                                 const QString &title = "Identity Rules", const QString &hint = "");

private slots:
    void addNewIdentityRule();

private:
    void populateTable();
    void saveRules();
    void addRow(const QString &identityKey, int value);
    QString identityKeyFromRow(int row) const;

    QTableWidget *rulesTable = nullptr;
    QScrollArea *scrollArea = nullptr;
    QVBoxLayout *mainLayout = nullptr;

    QMap<QString, Criterion::IdentityRule> *identityRules = nullptr;
    QStringList options;   // the individual identity values to choose from
};

#endif // IDENTITYRULESDIALOG_H
