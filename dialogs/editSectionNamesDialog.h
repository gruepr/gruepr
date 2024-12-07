#ifndef EDITSECTIONNAMESDIALOG_H
#define EDITSECTIONNAMESDIALOG_H

#include "listTableDialog.h"

class editSectionNamesDialog : public listTableDialog
{
    Q_OBJECT

public:
    editSectionNamesDialog(const QStringList &incomingSectionNames = {}, QWidget *parent = nullptr);
    ~editSectionNamesDialog() override = default;
    editSectionNamesDialog(const editSectionNamesDialog&) = delete;
    editSectionNamesDialog operator= (const editSectionNamesDialog&) = delete;
    editSectionNamesDialog(editSectionNamesDialog&&) = delete;
    editSectionNamesDialog& operator= (editSectionNamesDialog&&) = delete;

    QStringList sectionNames;

private slots:
    void clearAllNames();

private:
    QStringList originalSectionNames;
    QList<QLineEdit *> sectionNameLineEdits;
};

#endif // EDITSECTIONNAMESDIALOG_H
