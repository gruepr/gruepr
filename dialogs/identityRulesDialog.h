#ifndef IDENTITYRULESDIALOG_H
#define IDENTITYRULESDIALOG_H

#include "dataOptions.h"
#include "qscrollarea.h"
#include "qtablewidget.h"
#include "teamingOptions.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QMap>
#include <QList>
#include <QString>
#include <QDebug>

class IdentityRulesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IdentityRulesDialog(QWidget *parent = nullptr,const QString &identity = "", TeamingOptions *teamingOptions = nullptr, DataOptions *dataOptions = nullptr);
    QMap<QString, QMap<QString, QList<int>>> getIdentityRules() const; //also in teamingOptions
    QHBoxLayout* createIdentityOperatorRule(QString identity, QString operatorString, int noOfIdentity);

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

    QString identity;
    QMap<QString, QMap<QString, QList<int>>> identityRules;
    QVBoxLayout *mainLayout = nullptr;
    TeamingOptions *teamingOptions = nullptr;
    DataOptions *dataOptions = nullptr;
};

#endif // IDENTITYRULESDIALOG_H
